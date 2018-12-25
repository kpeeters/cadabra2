Building the Cadabra Jupyter kernel
===================================

The Cadabra build scripts can now build a Jupyter kernel



Using pip (recommended)
-----------------------

Using pip::

    git clone https://github.com/QuantStack/xtl.git
    cd xtl
  cmake
  sudo make install


  
  

Using Conda
-----------

If you intend to use the Cadabra Jupyter kernel with Jupyter
distributed through Conda, then you will need to build the Cadabra
kernel against Conda libraries, not system-provided ones
(mix-and-match will also not work). So you need a whole bunch of
package from Conda which may already exist on your system.

If you build the Jupyter kernel using Conda, Cadabra will build only
the Python module and the cadabra-jupyter-kernel binary. It is not
possible to build many of the other parts of Cadabra using Conda, for
various reasons: Conda's glibmm is not built with c++11 enabled, there
is no gtkmm library, and probably others. For a discussion on this, see::

  https://groups.google.com/a/continuum.io/d/msg/anaconda/oHtExJU9oiM/oMZLGpn1CAAJ

and if you don't think this is a problem, see e.g.::

  https://unix.stackexchange.com/questions/414904/anaconda-qt-vs-system-qt

Anyway, on to building. Most dependencies for Cadabra's Jupyter kernel
can be installed from Conda directly, with::

    conda config --add channels conda-forge
    conda install cmake pkg-config zeromq cppzmq xtl cryptopp sqlite util-linux
	 conda install nlohmann_json -c conda-forge/label/gcc7

You will need to build the `xeus` library yourself, as it is outdated
on Conda. For that, first ensure that your PATH picks up the Python
installed by Conda, that is, do something like::

    export PATH=${HOME}/miniconda3/bin:${PATH}
    export LD_LIBRARY_PATH=${HOME}/miniconda3/lib:${LD_LIBRARY_PATH}

If you forget these two, then you will use the system CMake, which is
not necessarily compatible with what the Conda software needs, and you
will also link things by default to the system libraries, even if
libraries of the same name exist in the Conda installation. Beware.

Then get the sources and do::

    cd build
    cmake ..
    sudo make install
	 
Now it is time to do the Cadabra build. Configure with options which
ensure that CMake picks up the Conda libraries first, and make it
install the Cadabra things in a place which does not interfere with
any 'normal' build you may have sitting around::

    cd build
    cmake -DENABLE_JUPYTER=ON -DENABLE_FRONTEND=OFF \
                              -DCMAKE_INCLUDE_PATH=${HOME}/miniconda3/include \
                              -DCMAKE_LIBRARY_PATH=${HOME}/miniconda3/lib \
										-DCMAKE_INSTALL_PREFIX=${HOME}/miniconda3
										..

you should see that it has configured using the Conda Python; look for
the `Configuring Python` block, which should be something like::

    -------------------------------------------
      Configuring Python
    -------------------------------------------
    -- Building for use with Python 3 (good!)
    -- Found PythonInterp: /home/kasper/miniconda3/bin/python3.7 (found version "3.7.1") 
    -- Found PythonLibs: /home/kasper/miniconda3/lib/libpython3.7m.so
    -- pybind11 v2.3.dev0
    -- Found python /home/kasper/miniconda3/lib/libpython3.7m.so

Note the reference to the Conda installation path. Further down you
should then also see a block for the Jupyter kernel::

	 -------------------------------------------
      Configuring Jupyter kernel build
    -------------------------------------------
 
If that's all ok, you can build with the standard::

    make
    sudo make install

