#!/bin/bash

###
# Following code until next comment block is pretty standard and shouldn't change between projects,
# unless you would like to make some improvements of course.
###

expecteddir="${TESTCASES:-.}/expected"

BOLDRED='\033[1;31m'
BOLDBLUE='\033[1;34m'
NC='\033[0m'			# No color

echo "$(basename "$(pwd)")"
echo

function evaluate {
	DIFF=`diff "$1" "$expecteddir/$1" 2>&1`
	if [ "$DIFF" = "" ]; then
		echo -e "${BOLDBLUE}TRUE${NC}"
	else
		echo -e "${BOLDRED}FALSE${NC}"
	fi
}

###
# Modify rest of the script for specific needs of a project
###

echo CASE1
evaluate "result1-1"

alltrue="${BOLDBLUE}TRUE${NC}"
for i in {2..20}; do
	result="$(evaluate "result1-$i")"
	if [[ "$result" =~ FALSE ]]; then
		alltrue="${BOLDRED}FALSE${NC}"
	fi
done

echo -e "$alltrue"
echo

echo CASE2
for i in {1..3}; do
	evaluate "result2-$i"
done
echo

echo CASE3
for i in {1..7}; do
	evaluate "result3-$i"
done
echo
