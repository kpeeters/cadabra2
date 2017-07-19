== Directions to build on windows using Microsoft Visual Studio 2017 Community ===
* If you don't have MSVC2017 community, it's free to download and install
** Note: also used msvc2013 to build pcre and gmp, ymmv
* Clone, build install from https://github.com/bluelips/pcre-win-build
** You want release, x86, build libpcrecpp 
* Downloaded boost 1.61 and installed to c:\create\boost_1_61_0\
** Downloaded windows prebuild binaries, the 32-bit version seems to be necessary. For my version of studio 2017, this was boost_1_61_0-msvc-14.0-32.exe
** If boost doesn't line up from cmake, try -DBoost_DEBUG=ON as cmake param
* Build libgmp
** Used https://github.com/bluelips/gmp, built with studio from SMP\libgmp.sln
** You may need to install yasm for studio http://www.megastormsystems.com/news/yasm-integration-with-visual-studio-2010-2012-and-2013 (the official site's yasm is old and actually failed for vs2013 for me)
* Finally the cmake (btw you need cmake ;) sequence from this dir:
** mkdir build && cd build
** cmake -DUSE_PYTHON_3=NO -DBOOST_ROOT="c:/create/boost_1_61_0" -DBOOST_LIBRARYDIR="c:/create/boost_1_61_0" -DBoost_USE_STATIC_LIBS=ON -DPCRE_LIBRARY="C:\create\pcre-win-build\build-VS2013\Release\libpcrecpp.lib" -DPCRE_INCLUDE_DIR="C:\create\pcre-win-build\include" ..
*** If you have cmake problems, turn on lots of debugging that gets piped to "attempt.txt" like so:
*** cmake -DUSE_PYTHON_3=NO -DBOOST_ROOT="c:/create/boost_1_61_0" -DBOOST_LIBRARYDIR="c:/create/boost_1_61_0" -DBoost_USE_STATIC_LIBS=ON -DPCRE_LIBRARY="C:\create\pcre-win-build\build-VS2013\Release\libpcrecpp.lib" -DPCRE_INCLUDE_DIR="C:\create\pcre-win-build\include" --debug_output --system_information -Wdev --trace .. > attempt.txt 2>&1 || tail attempt.txt
 
== CURRENT STATUS ==
* The base cadabra2 project seems to be building but other projects are not yet ported

== TODO ==
* Make gmp a sub-project of cadabra2

== notes ==
* Building gmp dropped libs and includes into c:\msvc\ which is gross and should be changed when gmp is turned into a subproject
* Found gmp through cmake by modifying the command to include the path hint directly
* Using rm -rf * to clean cmake from build dir is a bad idea, as I trashed everything once when accidently in root project dir, so use instead
** cd .. && rm -rf build && mkdir build && cd build
* Need 32-bit python, I used 2.7. If you have 64-bit python you won't be able to link cadabra2 and you will get cryptic linker errors regarding boost python. No idea if python3 works but still holding out for a grand unification with python4 anyway haha