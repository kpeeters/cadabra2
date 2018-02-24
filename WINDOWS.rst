Windows build instructions
==========================

On Windows the main constraint on the build process is that we want to
link to Anaconda's Python, which has been built with Visual
Studio. The recommended way to build Cadabra is thus to build against
libraries which are all built using Visual Studio. It is practically
impossible to build all dependencies yourself, but fortunately that is
not necessary because of the VCPKG library collection. This contains
all dependencies (boost, gtkmm, sqlite and various others) in
ready-to-use form.


Building with vcpkg
-------------------

- Install Visual Studio from [http://....]
- Install Anaconda (a 64 bit version!) from [...].
- Open the Visual Studio 'x64 Native Tools Command Prompt'
- Clone the vcpkg repository::
	 
	 git clone https://github.com/Microsoft/vcpkg

- Run the bootstrap script::

	 cd vcpkg
	 bootstrap-vcpkg.bat

  The latter will spit out a CMAKE toolchain path, you need that in a minute.

- vcpkg install --triplet=x64-windows glibmm mpir boost-system
boost-regex boost-filesystem boost-timer boost-uuid
  [instructions on which packages to install]

  The '--triplet' is important, otherwise you may end up with 32 bits
  versions of all software.

vcpkg.exe integrate install
  
cmake
mpir


Then configure as:

  cd cadabra2/build
  cmake
  -DCMAKE_TOOLCHAIN_FILE=C:/Users/kasper/Development/git.others/vcpkg/scripts/buildsystems/vcpkg.cmake
  -DVCPKG_TARGET_TRIPLET=x64-windows -DENABLE_FRONTEND=OFF
  -DCMAKE_VERBOSE_OUTPUT=ON -G "Visual Studio 15 2017 Win64" ..

[Does
		
cmake --build .

  

		

	 

  

Building with MSYS2
-------------------

Warning: building with MSYS2 does not work at the moment. Even if it
can be made to work again, it will use the MSYS2 Python, not any
Anaconda installation. 

- Install MSYS2 from [....]
Building on Windows does not work yet completely, but here is
something to get things at least roughly in the right
direction. First, install MSYS2 from http://msys2.github.io. Once you
have a working MSYS2 shell, do the following to install various
packages (all from an MSYS2 shell!)::

    pacman -S mingw-w64-x86_64-gcc
    pacman -S mingw-w64-x86_64-gtkmm3
    pacman -S mingw-w64-x86_64-boost
    pacman -S gmp gmp-devel pcre-devel
    pacman -S mingw-w64-x86_64-cmake
	 pacman -S mingw-w64-x86_64-sqlite3
    pacman -S mingw-w64-x86_64-python3  
    pacman -S mingw-w64-x86_64-adwaita-icon-theme

Then close the MSYS2 shell and open the MINGW64 shell. Run::
  
    cd cadabra2/build
    cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=/home/[user] ..
    mingw32-make

Replace '[user]' with your user name.
If the cmake fails with a complaint about 'sh.exe', just run it again.
The above builds for python2, let me know if you know how to make it
pick up python3 on Windows.

This fails to install the shared libraries, but they do get
built. Copy them all in ~/bin, and also copy a whole slew of other
things into there. In addition you need::

    cp /mingw64/bin/gspawn-win* ~/bin
    export PYTHONPATH=/mingw64/lib/python2.7:/home/[user]/bin

This fails to start the server with 'The application has requested the
Runtime to terminate it in an unusual way'.

