#!/bin/bash
# FindMathematica test driver script for UNIX systems

#logger -- $# "$@"
#logger -- LD_LIBRARY_PATH=$LD_LIBRARY_PATH
#logger -- DYLD_FRAMEWORK_PATH=$DYLD_FRAMEWORK_PATH
#logger -- DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH

export TEST_NAME=$1
export TEST_CONFIGURATION=$2
export TEST_INPUT_OPTION=$3

if [ "$TEST_INPUT_OPTION" = "input" ]
then
	export TEST_INPUT=$4
	export TEST_EXECUTABLE=$5
elif [ "$TEST_INPUT_OPTION" = "inputfile" ]
then
	export TEST_INPUT_FILE=$4
	export TEST_EXECUTABLE=$5
else
	export TEST_EXECUTABLE=$4
fi

if [ "$OSTYPE" = "cygwin" ]
then
	# make sure that executable has the right format under Cygwin
	export TEST_EXECUTABLE="`/usr/bin/cygpath --unix \"$TEST_EXECUTABLE\"`"
fi

if [ "$TEST_INPUT_OPTION" = "input" ]
then
	echo "$TEST_INPUT" | exec "$TEST_EXECUTABLE" "${@:6}"
elif [ "$TEST_INPUT_OPTION" = "inputfile" ]
then
	exec < "$TEST_INPUT_FILE" "$TEST_EXECUTABLE" "${@:6}"
else
	exec "$TEST_EXECUTABLE" "${@:5}"
fi
