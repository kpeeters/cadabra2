Building the Cadabra Jupyter kernel
===================================

The Cadabra build scripts can now build a Jupyter kernel, so that you
can use the Jupyter notebook to write Cadabra code (using all of the
Cadabra notation, i.e. without having to resort to the much more ugly
Python interface). At the moment this is only supported by compiling
against a Conda python, simply because that enables us to build on the
'xeus' library more easily.


Building using Conda
--------------------

If you intend to use the Cadabra Jupyter kernel with Jupyter
distributed through Conda, then you will need to build the Cadabra
kernel against Conda libraries, not system-provided ones
(mix-and-match will also not work). If you do not have Conda yet, get
it from

  https://docs.conda.io/en/latest/miniconda.html

(install a Python3.x version).  

When building against Conda, Cadabra will build only the Python module
and the cadabra-jupyter-kernel binary. It is not possible to build
many of the other parts of Cadabra using Conda, for various reasons:
Conda's glibmm is not built with c++11 enabled, there is no gtkmm
library, and probably others. For a discussion on this, see

  https://groups.google.com/a/continuum.io/d/msg/anaconda/oHtExJU9oiM/oMZLGpn1CAAJ

and if you don't think this is a problem, see e.g.

  https://unix.stackexchange.com/questions/414904/anaconda-qt-vs-system-qt

Anyway, on to building. First activate your miniconda distribution::

    source ~/miniconda3/etc/profile.d/conda.sh
	 export PATH="${HOME}/miniconda3/bin:$PATH"

All dependencies for Cadabra's Jupyter kernel can then be installed from
Conda directly, with::

    conda config --add channels conda-forge
    conda install xeus -c QuantStack -c conda-forge
    conda install cmake pkg-config glibmm zeromq cppzmq xtl cryptopp sqlite util-linux
    conda install nlohmann_json -c conda-forge/label/gcc7
    conda install sympy jupyter
	 
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

You should see that it has configured using the Conda Python; look for
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

This will install the kernel in::

    ${HOME}/miniconda3/bin/

and the JSON configuration files in::

    ${HOME}/miniconda3/share/jupyter/kernels/cadabra/

If you now start Jupyter, you should be able to choose a Cadabra
kernel::

    ${HOME}/miniconda3/bin/jupyter notebook

There is a sample `schwarzschild.ipynb` in the `examples` directory.	
