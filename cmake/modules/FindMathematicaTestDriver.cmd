@echo off
rem FindMathematica test driver script for Windows

setlocal enabledelayedexpansion

rem echo !CMDCMDLINE!
rem echo !PATH!

set "TEST_NAME=%~1"
set "TEST_CONFIGURATION=%~2"
set "TEST_INPUT_OPTION=%~3"
if "!TEST_INPUT_OPTION!" == "input" (
	set "TEST_INPUT=%~4"
	set "TEST_EXECUTABLE=%~5"
	shift
	shift
	shift
	shift
	shift
	shift
) else if "!TEST_INPUT_OPTION!" == "inputfile" (
	set "TEST_INPUT_FILE=%~4"
	set "TEST_EXECUTABLE=%~5"
	shift
	shift
	shift
	shift
	shift
	shift
) else (
	set "TEST_EXECUTABLE=%~4"
	shift
	shift
	shift
	shift
	shift
)

if "!TEST_INPUT_OPTION!" == "input" (
	echo !TEST_INPUT! | "!TEST_EXECUTABLE!" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
) else if "!TEST_INPUT_OPTION!" == "inputfile" (
	"!TEST_EXECUTABLE!" < "!TEST_INPUT_FILE!" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
) else (
	"!TEST_EXECUTABLE!" %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
)
