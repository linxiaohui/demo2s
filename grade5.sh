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


pts=5

runtest() {
	perl -e "print '$1: '"
	rm -f kern/init.o kern/kernel kern/bochs.img
	if $verbose
	then
		perl -e "print 'make $2... '"
	fi
	if ! make $2 kern/bochs.img fs/fs.img >$out 2>$err
	then
		echo make failed 
		exit 1
	fi
	(
		ulimit -t 20
		(echo c; echo quit) |
			bochs -q 'parport1: enabled=1, file="bochs.out"'
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
		score=`echo $pts+$score | bc`
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

rm -f fs/fs.img
make fs/fs.img >$out 2>$err

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

rm -f fs/fs.img
make fs/fs.img >$out 2>$err

score=0
pts=10
runtest1 -tag 'motd display' writemotd \
	'OLD MOTD' \
	'This is /motd, the message of the day.' \
	'NEW MOTD' \
	'This is the NEW message of the day!' \

runtest1 -tag 'motd change' writemotd \
	'OLD MOTD' \
	'This is the NEW message of the day!' \
	'NEW MOTD' \
	! 'This is /motd, the message of the day.' \

rm -f fs/fs.img
make fs/fs.img >$out 2>$err

pts=25
runtest1 -tag 'spawn via icode' icode \
	'icode: read /motd' \
	'This is /motd, the message of the day.' \
	'icode: spawn /init' \
	'/init: running' \
	'/init: data seems okay' \
	'icode: exiting' \
	'/init: bss seems okay' \
	"/init: args: 'init' 'initarg1' 'initarg2'" \
	'/init: exiting' \

echo PART B SCORE: $score/45

exit 0


