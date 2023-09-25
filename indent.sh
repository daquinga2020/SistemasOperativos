#!/usr/bin/bash

usage(){
	echo "usage: indent *.[ch]" 1>&2;
	exit 1
}

case $# in
0)
	usage
esac

for i in "$@"; do
	if ! echo $i|grep '.*\.[hc]' > /dev/null 2>&1; then
		echo "$i" is not a C file 1>&2;
		usage
	fi
	if ! test -f $i > /dev/null 2>&1; then
		echo "$i does not exist or is not a file" 1>&2;
		usage
	fi
	if ! file "$i" |grep 'C source' > /dev/null 2>&1; then
		echo "$i does not contain C code" 1>&2;
		usage
	fi
	indent -hnl -nsc -bl -ts8 -i8 -ut -bad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4  -cli0 -d0 -di1 -nfc1 -ip0 -l80 -lp -npcs -nprs -psl -sai -saf -saw -ncs -sob -nfca -cp33 -ss -il1 "$i"
	rm "$i"~
done
