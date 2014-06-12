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
	rm -f kern/init.o
	if $verbose
	then
		perl -e "print 'gmake $2... '"
	fi
	if ! gmake $2 kern/bochs.img >$out
	then
		echo gmake failed 
		exit 1
	fi
	(
		ulimit -t 10
		(echo c; echo quit) |
			bochs-nogui 'parport1: enabled=1, file="bochs.out"'
	) >$out 2>$err
	if [ ! -s bochs.out ]
	then
		echo 'no bochs.out'
	else
		shift
		shift
		okay=yes

		for i
		do
			if ! egrep "^$i\$" bochs.out >/dev/null
			then
				echo "missing '$i'"
				if $verbose
				then
					exit 1
				fi
				okay=no
			fi
		done
		if [ "$okay" = "yes" ]
		then
			score=`echo 5+$score | bc`
			echo OK
		else
			echo WRONG
		fi
	fi
}

runtest1() {
	tag=$1
	shift
	runtest $tag "DEFS=-DTEST=binary_user_${tag}_start DEFS+=-DTESTSIZE=binary_user_${tag}_size" "$@"
}

score=0

runtest1 hello \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'hello, world' \
	'i am environment 00001001' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001'

# the [00001001] tags should have [] in them, but that's 
# a regular expression reserved character, and i'll be damned
# if i can figure out how many \ i need to add to get through 
# however many times the shell interprets this string.  sigh.

runtest pingpong2 'DEFS=-DTEST_PINGPONG2' \
	'1802 got 0 from 1001' \
	'1001 got 1 from 1802' \
	'1802 got 8 from 1001' \
	'1001 got 9 from 1802' \
	'1802 got 10 from 1001' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' \
	'.00001802. exiting gracefully' \
	'.00001802. free env 00001802'

echo PART A SCORE: $score/10

score=0

runtest1 buggyhello \
	'.00001001. PFM_KILL va 00000001 ip f01.....' \
	'.00001001. free env 00001001'

runtest1 evilhello \
	'.00001001. PFM_KILL va ef800000 ip f01.....' \
	'.00001001. free env 00001001'

runtest1 fault \
	'.00001001. user fault va 00000000 ip 0080008b' \
	'.00001001. free env 00001001'

runtest1 faultdie \
	'i faulted at va deadbeef, err 6' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' 

runtest1 faultalloc \
	'fault deadbeef' \
	'this string was faulted in at deadbeef' \
	'fault cafebffe' \
	'fault cafec000' \
	'this string was faulted in at cafebffe' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001'

runtest1 faultallocbad \
	'.00001001. PFM_KILL va deadbeef ip f01.....' \
	'.00001001. free env 00001001' 

runtest1 faultbadhandler \
	'.00001001. PFM_KILL va eebfcffc ip f01.....' \
	'.00001001. free env 00001001'

runtest1 faultbadstack \
	'.00001001. PFM_KILL va ef800000 ip f01.....' \
	'.00001001. free env 00001001'

runtest1 faultgoodstack \
	'i faulted at va deadbeef, err 6, stack eebfd...' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' 

runtest1 faultevilhandler \
	'.00001001. PFM_KILL va eebfcffc ip f01.....' \
	'.00001001. free env 00001001'

runtest1 faultevilstack \
	'.00001001. PFM_KILL va ef800000 ip f01.....' \
	'.00001001. free env 00001001'

echo PART B SCORE: $score/55

score=0

runtest1 pingpong1 \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'.00001001. new env 00001802' \
	'send 0 from 1001 to 1802' \
	'1802 got 0 from 1001' \
	'1001 got 1 from 1802' \
	'1802 got 8 from 1001' \
	'1001 got 9 from 1802' \
	'1802 got 10 from 1001' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' \
	'.00001802. exiting gracefully' \
	'.00001802. free env 00001802' \

runtest1 pingpong \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'.00001001. new env 00001802' \
	'send 0 from 1001 to 1802' \
	'1802 got 0 from 1001' \
	'1001 got 1 from 1802' \
	'1802 got 8 from 1001' \
	'1001 got 9 from 1802' \
	'1802 got 10 from 1001' \
	'.00001001. exiting gracefully' \
	'.00001001. free env 00001001' \
	'.00001802. exiting gracefully' \
	'.00001802. free env 00001802' \

runtest1 primes \
	'.00000000. new env 00000800' \
	'.00000000. new env 00001001' \
	'.00001001. new env 00001802' \
	'2 .00001802. new env 00002003' \
	'3 .00002003. new env 00002804' \
	'5 .00002804. new env 00003005' \
	'7 .00003005. new env 00003806' \
	'11 .00003806. new env 00004007' 

echo PART C SCORE: $score/15






