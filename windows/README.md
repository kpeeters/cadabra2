== Directions to build on windows using Microsoft Visual Studio 2017 Community ===
* If you don't have MSVC2017 community, it's free to download and install
* Install gnuwin32 pcre library (went to C:\Program Files (x86)\GnuWin32\ for me)
* Downloaded boost 1.61 and installed to c:\create\boost_1_61_0\
** Downloaded windows prebuild binaries, the 32-bit version seems to be necessary. For my version of studio 2017, this was boost_1_61_0-msvc-14.0-32.exe
** If boost doesn't line up from cmake, try -DBoost_DEBUG=ON as cmake param
* To get gmp, used https://github.com/bluelips/gmp, built with studio from the SMP directory
* Finally the cmake (btw you need cmake ;) sequence from this dir:
** mkdir build && cd build
** cmake -DUSE_PYTHON_3=NO -DBOOST_ROOT="c:/create/boost_1_61_0" -DBOOST_LIBRARYDIR="c:/create/boost_1_61_0" -DBoost_USE_STATIC_LIBS=OFF -DPCRE_LIBRARY="C:\Program Files (x86)\GnuWin32\lib" -DPCRE_INCLUDE_DIR="C:\Program Files (x86)\GnuWin32\include" --debug_output --system_information -Wdev --trace .. > attempt.txt 2>&1

== CURRENT STATUS ==
* gmp and gmpxx built
* trying building cadabra with 2017 as 2013 maybe had insufficient c++11?

== TODO ==
* Make gmp a sub-project of cadabra2
* port sys/time.h calls
* lots of compile errors

== notes ==
* Building gmp dropped libs and includes into c:\msvc\
* Found gmp through cmake by modifying the command to include the path hint directly
