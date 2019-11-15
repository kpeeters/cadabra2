# Conda build script to build a Cadabra Jupyter kernel.

mkdir build
cd build
cmake -G $CMAKE_GENERATOR \
      -DCMAKE_INSTALL_PREFIX=$PREFIX \
      -DCMAKE_BUILD_TYPE=Release $SRC_DIR \
      -DENABLE_JUPYTER=ON \
      -DENABLE_FRONTEND=OFF \
      -DCMAKE_INCLUDE_PATH=${HOME}/miniconda3/include \
      -DCMAKE_LIBRARY_PATH=${HOME}/miniconda3/lib \
      -DCMAKE_INSTALL_PREFIX=${HOME}/miniconda3

cmake --build .
cmake --build . --target install
