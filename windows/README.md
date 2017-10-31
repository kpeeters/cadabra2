== Directions to build on windows using Microsoft Visual Studio 2017 Community ===
* If you don't have MSVC2017 community, it's free to download and install
  * Note: also used msvc2013 to build pcre and gmp so ymmv
* Clone, build install from https://github.com/bluelips/pcre-win-build
  * You want release, x86, build libpcrecpp
* Install sqlite3
  * git clone https://github.com/alex85k/sqlite3-cmake
  * cd sqlite3-cmake && mkdir build && cd build && cmake ..
  * Built sqlite3.sln in msvc2017
* Downloaded boost 1.61 and installed to c:\create\boost_1_61_0\
  * Downloaded windows prebuild binaries, the 32-bit version seems to be necessary. For my version of studio 2017, this was boost_1_61_0-msvc-14.0-32.exe
  * If boost doesn't line up from cmake, try -DBoost_DEBUG=ON as cmake param
* Build libgmp
  * Used https://github.com/bluelips/gmp, built with studio from SMP\libgmp.sln
  * You may need to install yasm for studio http://www.megastormsystems.com/news/yasm-integration-with-visual-studio-2010-2012-and-2013 (the official site's yasm is old and actually failed for vs2013 for me)
* Next, the cmake (btw you need cmake ;) sequence from this dir:
  * mkdir build && cd build
  * cmake -DCMAKE_SUPPRESS_REGENERATION:BOOL=1 -DUSE_PYTHON_3=NO -DSQLITE3_INCLUDE_DIR_SEARCH="c:/create/sqlite3-cmake/src" -DSQLITE3_LIBRARIES_SEARCH="c:/create/sqlite3-cmake/build/Release" -DBOOST_ROOT="c:/create/boost_1_61_0" -DBOOST_LIBRARYDIR="c:/create/boost_1_61_0" -DPCRE_LIBRARY="C:\create\pcre-win-build\build-VS2013\Release\libpcrecpp.lib" -DPCRE_INCLUDE_DIR="C:\create\pcre-win-build\include" -DGTK3_BUNDLE_INCLUDE_DIR_SEARCH="C:\gtk-build\gtk\Win32\include" -DGTK3_BUNDLE_LIBRARIES_SEARCH="C:\gtk-build\gtk\Win32\lib" -DGTKMM3_BUNDLE_INCLUDE_DIR_SEARCH="C:\create\gtkmm-win32\gtkmm\Win32\include" -DGTKMM3_BUNDLE_LIBRARIES_SEARCH="C:\create\gtkmm-win32\gtkmm\Win32\lib" ..
    * If you have cmake problems, turn on lots of debugging that gets piped to "attempt.txt" like so:
    * cmake -DCMAKE_SUPPRESS_REGENERATION:BOOL=1 -DUSE_PYTHON_3=NO -DSQLITE3_INCLUDE_DIR_SEARCH="c:/create/sqlite3-cmake/src" -DSQLITE3_LIBRARIES_SEARCH="c:/create/sqlite3-cmake/build/Release" -DBOOST_ROOT="c:/create/boost_1_61_0" -DBOOST_LIBRARYDIR="c:/create/boost_1_61_0" -DPCRE_LIBRARY="C:\create\pcre-win-build\build-VS2013\Release\libpcrecpp.lib" -DPCRE_INCLUDE_DIR="C:\create\pcre-win-build\include" -DGTK3_BUNDLE_INCLUDE_DIR_SEARCH="C:\gtk-build\gtk\Win32\include" -DGTK3_BUNDLE_LIBRARIES_SEARCH="C:\gtk-build\gtk\Win32\lib" -DGTKMM3_BUNDLE_INCLUDE_DIR_SEARCH="C:\create\gtkmm-win32\gtkmm\Win32\include" -DGTKMM3_BUNDLE_LIBRARIES_SEARCH="C:\create\gtkmm-win32\gtkmm\Win32\lib" --debug_output --system_information -Wdev --trace .. > attempt.txt 2>&1 || tail attempt.txt
* Now build relevant projects from the cadabra2.sln

== CURRENT STATUS ==
* Basic running cadabra-gtk client, not well tested

== TODO ==
* Make gmp a sub-project of cadabra2
* Boost python cmake setup seems flawed on some platforms: https://travis-ci.org/kpeeters/cadabra2/jobs/272162045
  * Didn't reproduce with a fresh ubuntu vm so will have to recreate that environment

== notes ==
* Building gmp dropped libs and includes into c:\msvc\ which is gross and should be changed when gmp is turned into a subproject
* Found gmp through cmake by modifying the command to include the path hint directly
* Using rm -rf * to clean cmake from build dir is a bad idea, as I trashed everything once when accidently in root project dir, so use instead
  * cd .. && rm -rf build && mkdir build && cd build
* Need 32-bit python, I used 2.7. If you have 64-bit python you won't be able to link cadabra2 and you will get cryptic linker errors regarding boost python. No idea if python3 works but still holding out for a grand unification with python4 anyway haha
* Debuging gtk problems might be helped by setting G_SPAWN_WIN32_DEBUG=1 env var 
* Definite problems mixing release and debug as std::string crosses the dll boundary and has different sizes, try releasewithdbginfo
* Currently able to run command line by doing the following: 
  * Made a new bin directory and copied relevant *.dll, *.exe and .py files 
  * Rename (or copy) cadabra2.dll to cadabra2.pyd 
  * Copy cadabra2 file (it's a no extension python file) 
  * Make sure to copy over boost.dll, gmpxx.dll, libpcrecpp.dll 
    * If there are problems with the import, try this from a python command line in the same dir 
	```
	import os
	import sys
	sys.path.append(os.getcwd())
	import cadabra2
	```
    * If that still fails, use depends.exe on cadabra2.dll to figure out what is missing 
  * execute "python cadabra2" from this directory to start interactive mode

== gtk frontend ==
* The frontend is optional
* Building gtk on windows is a long process
* Built https://github.com/bluelips/gtk-win32 using its associated directions
  * After restarts if it doesn't work, use -SkipDownload to avoid errors when repeating the build.ps1
* Building https://github.com/bluelips/gtkmm-win32 for msvc2017 using its associated directions
  * To try it out to see if it worked (switched to cygwin)
    * find -name "*.dll" -print | grep Debug | xargs -i cp '{}' ../untracked/winbin/
    * find -name "*.exe" -print | grep Debug | grep Win32 | xargs -i cp '{}' ../untracked/winbin/
    * cd /cygdrive/c/gtk-build/build/Win32 && find -name "*.dll" -print | grep Debug | xargs -i cp '{}' /cygdrive/c/create/gtkmm-win32/untracked/winbin/
    * cd /cygdrive/c/gtk-build/gtk/Win32/bin && find -name "*.dll" -print | xargs -i cp '{}' /cygdrive/c/create/gtkmm-win32/untracked/winbin/


=== gtk todo ===
* package into nice release for others to use binaries
* fix gtk3 mm demo project
* fix the leak associated with each cell, seems to be a linker problem associated with __cdecl calling convention