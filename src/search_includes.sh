#!/bin/bash

filename="$1"
indent="$2"

while read -r w1 w2
do
    a="$w1"
    b="$w2"
    c="#include"
    r="\""
    
    if [ "$a" == "$c" ]
    then
	if [ "${b:0:1}" == "$r" ]
	then
	    echo "$indent: $filename: $b"
	    sleep 0.1
	    ./search_includes.sh ${b:1:-1} "$indent¤"
	fi
    fi
done < "$filename"
