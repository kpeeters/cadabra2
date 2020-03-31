# Conda build script to build a Cadabra Jupyter kernel.

rm -Rf build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${PREFIX} \
      -DCMAKE_BUILD_TYPE=Release ${SRC_DIR} \
		-DENABLE_MATHEMATICA=OFF \
      -DENABLE_JUPYTER=ON \
      -DENABLE_FRONTEND=OFF \
      -DCMAKE_INCLUDE_PATH=${HOME}/miniconda3/include \
      -DCMAKE_LIBRARY_PATH=${HOME}/miniconda3/lib \
      ..

cmake --build .
cmake --build . --target install
