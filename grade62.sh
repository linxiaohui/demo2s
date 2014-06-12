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
timeout=60
runtest() {
	perl -e "print '$1: '"
	rm -f kern/init.o kern/kernel kern/bochs.img fs/fs.img
	rm -f user/console.o user/init
	gmake user/console-closed.o >$out 2>$err
	cp user/console-closed.o user/console.o
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
		ulimit -t $timeout
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

# 20 points - run-icode
pts=20
runtest1 -tag 'updated file system switch' icode \
	'icode: read /motd' \
	'This is /motd, the message of the day.' \
	'icode: spawn /init' \
	'init: running' \
	'init: data seems okay' \
	'icode: exiting' \
	'init: bss seems okay' \
	"init: args: 'init' 'initarg1' 'initarg2'" \
	'init: running sh' \

pts=10
runtest1 -tag 'PTE_LIBRARY' testptelibrary \
	'fork handles PTE_LIBRARY right' \
	'spawn handles PTE_LIBRARY right' \

# 10 points - run-testfdsharing
pts=10
runtest1 -tag 'fd sharing' testfdsharing \
	'read in parent succeeded' \
	'read in child succeeded' 

# 10 points - run-testpipe
pts=10
runtest1 -tag 'pipe' testpipe \
	'pipe read closed properly' \
	'pipe write closed properly' \

# 10 points - run-testpiperace
pts=10
runtest1 -tag 'pipe race' testpiperace \
	! 'child detected race' \
	! 'RACE: pipe appears closed' \
	"race didn't happen" \

# 10 points - run-testpiperace2
pts=10
timeout=180
echo 'The pipe race 2 test has 3 minutes to complete.  Be patient.'
runtest1 -tag 'pipe race 2' testpiperace2 \
	! 'RACE: pipe appears closed' \
	! 'child detected race' \
	"race didn't happen" \

# 10 points - run-primespipe
pts=10
timeout=180
echo 'The primes test has 3 minutes to complete.  Be patient.'
runtest1 -tag 'primes' primespipe \
	! 1 2 3 ! 4 5 ! 6 7 ! 8 ! 9 \
	! 10 11 ! 12 13 ! 14 ! 15 ! 16 17 ! 18 19 \
	! 20 ! 21 ! 22 23 ! 24 ! 25 ! 26 ! 27 ! 28 29 \
	! 30 31 ! 32 ! 33 ! 34 ! 35 ! 36 37 ! 38 ! 39 \
	541 1009 1097

# 20 points - run-testshell
pts=20
timeout=240
echo 'The shell test has 4 minutes to complete.  Be patient.'
runtest1 -tag 'shell' testshell \
	'shell ran correctly' \

echo SCORE: $score/100


