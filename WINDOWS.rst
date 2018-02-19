
- Install Visual Studio
- Install cmake 3.10.2 from http://cmake.org/
- Open the VC development prompt.
- Clone the vcpkg repository:

    git clone https://github.com/Microsoft/vcpkg

Run the bootstrap script,

cd vcpkg
bootstrap-vcpkg.bat
vcpkg.exe integrate install

The latter will spit out a CMAKE toolchain path, you need that in a minute.

- vcpkg install glibmm mpir
  [instructions on which packages to install]

  
cmake
mpir


Then configure as:

  cd cadabra2/build
  cmake
  -DCMAKE_TOOLCHAIN_FILE=C:/Users/kasper/Development/git.others/vcpkg/scripts/buildsystems/vcpkg.cmake
  -DVCPKG_TARGET_TRIPLET=x86-windows -DENABLE_FRONTEND=OFF
  -DCMAKE_VERBOSE_OUTPUT=ON
      -G "Visual Studio 15 2017 Win64" ..

cmake --build .

  

		

	 

  

