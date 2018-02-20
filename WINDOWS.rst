Windows build instructions
==========================

- Install Visual Studio from [http://....]
- Install cmake 3.10.2 from [http://cmake.org/]
- Install Anaconda (a 64 bit version!) from [...].
- Open the VC development prompt.
- Clone the vcpkg repository::
	 
	 git clone https://github.com/Microsoft/vcpkg

- Run the bootstrap script::

	 cd vcpkg
	 bootstrap-vcpkg.bat

  The latter will spit out a CMAKE toolchain path, you need that in a minute.

- vcpkg install --triplet=x64-windows glibmm mpir boost-system boost-regex boost-filesystem 
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
  -DCMAKE_VERBOSE_OUTPUT=ON
      -G "Visual Studio 15 2017 Win64" ..

[Does
		
cmake --build .

  

		

	 

  

