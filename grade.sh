#!/bin/sh

verbose=false

if [ "x$1" = "x-x" ]
then
	verbose=true
	out=/dev/stdout
	err=/dev/stderr
else
	out=/dev/null
	err=/dev/null
fi


runtest() {
	perl -e "print '$1: '"
	rm -f kern/init.o kern/kernel kern/bochs.img
	if $verbose
	then
		perl -e "print 'gmake $2... '"
	fi
	if ! gmake $2 kern/bochs.img fs/fs.img >$out
	then
		echo gmake failed 
		exit 1
	fi
	(
		ulimit -t 20
		(echo c; echo die; echo quit) |
			bochs-nogui 'parport1: enabled=1, file="bochs.out"'
	) >$out 2>$err
	if [ ! -s bochs.out ]
	then
		echo 'no bochs.out'
	else
		shift
		shift
		continuetest "$@"
	fi
}

quicktest() {
	perl -e "print '$1: '"
	shift
	continuetest "$@"
}

continuetest() {
	okay=yes

	not=false
	for i
	do
		if [ "x$i" = "x!" ]
		then
			not=true
		elif $not
		then
			if egrep "^$i\$" bochs.out >/dev/null
			then
				echo "got unexpected line '$i'"
				if $verbose
				then
					exit 1
				fi
				okay=no
			fi
			not=false
		else
			if ! egrep "^$i\$" bochs.out >/dev/null
			then
				echo "missing '$i'"
				if $verbose
				then
					exit 1
				fi
				okay=no
			fi
			not=false
		fi
	done
	if [ "$okay" = "yes" ]
	then
		score=`echo 5+$score | bc`
		echo OK
	else
		echo WRONG
	fi
}

runtest1() {
	if [ $1 = -tag ]
	then
		shift
		tag=$1
		prog=$2
		shift
		shift
	else
		tag=$1
		prog=$1
		shift
	fi
	runtest "$tag" "DEFS=-DTEST=binary_user_${prog}_start DEFS+=-DTESTSIZE=binary_user_${prog}_size" "$@"
}


score=0

runtest1 -tag 'fs i/o' testfsipc \
	'FS can do I/O' \
	! 'idle loop can do I/O' \

quicktest 'read_block' \
	'superblock is good' \

quicktest 'write_block' \
	'write_block is good' \

quicktest 'read_bitmap' \
	'read_bitmap is good' \

quicktest 'alloc_block' \
	'alloc_block is good' \

quicktest 'file_open' \
	'file_open is good' \

quicktest 'file_get_block' \
	'file_get_block is good' \

quicktest 'file_truncate' \
	'file_truncate is good' \

quicktest 'file_flush' \
	'file_flush is good' \

quicktest 'file rewrite' \
	'file rewrite is good' \

quicktest 'serv_*' \
	'serve_open is good' \
	'serve_map is good' \
	'serve_close is good' \
	'stale fileid is good' \

echo PART A SCORE: $score/55

echo PART B grading script is not available yet.
echo we will announce when it is ready.
exit 0


