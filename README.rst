Cadabra
=======

.. image:: http://joss.theoj.org/papers/10.21105/joss.01118/status.svg
   :target: https://doi.org/10.21105/joss.01118

.. image:: https://zenodo.org/badge/DOI/10.5281/zenodo.2500762.svg
   :target: https://doi.org/10.5281/zenodo.2500762
				
*A field-theory motivated approach to computer algebra.*

Kasper Peeters <info@cadabra.science>

- End-user documentation at http://cadabra.science/
- Source code documentation at http://kpeeters.github.io/cadabra2

This repository holds the 2.x series of the Cadabra computer algebra
system. It supersedes the 1.x series, which can still be found at
http://github.com/kpeeters/cadabra.

Cadabra was designed specifically for the solution of problems
encountered in quantum and classical field theory. It has extensive
functionality for tensor computer algebra, tensor polynomial
simplification including multi-term symmetries, fermions and
anti-commuting variables, Clifford algebras and Fierz transformations,
implicit coordinate dependence, multiple index types and many
more. The input format is a subset of TeX. Both a command-line and a
graphical interface are available.

Installation
-------------

Cadabra builds on Linux, macOS, OpenBSD and Windows. Select your
system from the list below for detailed instructions.

- `Linux (Debian/Ubuntu/Mint)`_
- `Linux (Fedora 24 and later)`_
- `Linux (CentOS/Scientific Linux)`_
- `Linux (openSUSE)`_
- `Linux (Arch/Manjaro)`_
- `Linux (Solus)`_
- `OpenBSD`_
- `macOS`_
- `Windows`_

Binaries for these platforms may (or may not) be provided from the
download page at http://cadabra.science/download.html, but they are
not always very up-to-date.

See `Building Cadabra as C++ library`_ for instructions on how to
build the entire Cadabra functionality as a library which you can use
in a C++ program.

See `Building a Jupyter kernel`_  for instructions on how to build a
Jupyter kernel for Cadabra sessions.


Linux (Debian/Ubuntu/Mint)
~~~~~~~~~~~~~~~~~~~~~~~~~~

On Debian/Ubuntu you can install all that is needed with::

    sudo apt install git cmake python3-dev g++ libpcre3 libpcre3-dev libgmp3-dev \
          libgtkmm-3.0-dev libboost-all-dev libgmp-dev libsqlite3-dev uuid-dev  \
          texlive texlive-latex-extra dvipng \
          python3-matplotlib python3-mpmath python3-sympy python3-gmpy2

(on Ubuntu 14.04 you need to replace `cmake` with `cmake3` and also
install g++-4.9; get in touch if you don't know how to do this). On
older systems you may want to install `sympy` using `sudo pip3 install
sympy`, but that is discouraged in general.
	 
This is the development platform and issues are typically first fixed
here. You can use either g++ or the clang++ compiler to build. You need to
clone the cadabra2 git repository (if you download the .zip file you
will not have all data necessary to build). So first do::

    git clone https://github.com/kpeeters/cadabra2

Building is then done with the standard::

    cd cadabra2
    mkdir build
    cd build
    cmake ..
    make
    sudo make install

This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. You can also find the latter in
the 'Education' menu.

Linux (Fedora 24 and later)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Fedora 24 is the first Fedora to have Python 3; you can build Cadabra
using Python 2 but you are strongly encouraged to upgrade. The Fedora
platform receives less testing so please get in touch if you run into
any issues. You can use either g++ or the clang++ compiler.

Install the dependencies with::

    sudo dnf install git python3-devel cmake gcc-c++ \
         pcre-devel gmp-devel libuuid-devel sqlite-devel \
         gtkmm30-devel boost-devel \
         texlive python3-matplotlib \
         python3-pip
    sudo pip3 install sympy

You need to clone the cadabra2 git repository (if you download the
.zip file you will not have all data necessary to build). So first do::

    git clone https://github.com/kpeeters/cadabra2

Building is then done with the standard::

    cd cadabra2
    mkdir build
    cd build
    cmake ..
    make
    sudo make install

This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. You can also find the latter
when searching for the 'Cadabra' app from the 'Activities' menu.

Linux (CentOS/Scientific Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On CentOS/Scientific Linux you need to activate The Software
Collections (SCL) and Extra Packages for Enterprise Linux (EPEL) to
get access to a modern C++ compiler, Python3 and all required build
tools. On CentOS first do::

    sudo yum install centos-release-scl epel-release

On Scientific Linux the equivalent is::

    sudo yum install yum-conf-softwarecollections epel-release
	 
Now install all build dependencies with::
  
    sudo yum install devtoolset-7 rh-python36 cmake3 \
	          gmp-devel libuuid-devel sqlite-devel \
             gtkmm30-devel boost-devel git \
	          texlive python-matplotlib

You need to enable the Python3 and C++ compiler which you just
installed with::

    scl enable rh-python36 bash					
    scl enable devtoolset-7 bash

(note: do *not* use sudo here!).
	 
You also need to install sympy by hand::

    sudo pip3 install sympy

Now need to clone the cadabra2 git repository (if you download the
.zip file you will not have all data necessary to build)::

    git clone https://github.com/kpeeters/cadabra2

Building is then done with the standard::

    cd cadabra2
    mkdir build
    cd build
    cmake3 .. 
    make
    sudo make install

This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. You can also find the latter in
the 'Education' menu.


Linux (openSUSE)
~~~~~~~~~~~~~~~~

For openSUSE (tested on 'Leap 15.0', probably also fine with minor
changes for 'Tumbleweed') you first need to install the dependencies
with::

    sudo zypper install git cmake python3-devel gcc-c++ \
                  pcre-devel gmp-devel libuuid-devel sqlite-devel \
                  gtkmm3-devel  \
                  texlive python3-matplotlib \
                  python3-sympy \
                  libboost_system1_66_0-devel libboost_filesystem1_66_0-devel \
                  libboost_date_time1_66_0-devel libboost_program_options1_66_0-devel 

This platform receives less testing so please get in touch if you run
into any issues. You need to clone the cadabra2 git repository (if you
download the .zip file you will not have all data necessary to
build). So first do::

    git clone https://github.com/kpeeters/cadabra2

Building is then done with the standard::

    cd cadabra2
    mkdir build
    cd build
    cmake .. 
    make
    sudo make install

This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. 


Linux (Arch/Manjaro)
~~~~~~~~~~~~~~~~~~~~

The development package for Arch Linux is cadabra2-git
https://aur.archlinux.org/packages/cadabra2-git/.  Building and
installing (including dependencies) can be accomplished with::

    yaourt -Sy cadabra2-git

Alternatively use ``makepkg``::

    curl -L -O https://aur.archlinux.org/cgit/aur.git/snapshot/cadabra2-git.tar.gz
    tar -xvf cadabra2-git.tar.gz
    cd cadabra2-git
    makepkg -sri

Please consult the Arch Wiki
https://wiki.archlinux.org/index.php/Arch_User_Repository#Installing_packages
for more information regarding installing packages from the AUR.


Linux (Solus)
~~~~~~~~~~~~~

Support for Solux Linux is experimental. To build from source on Solus
Linux, first install the dependencies by doing::

    sudo eopkg install -c system.devel
    sudo eopkg install libboost-devel gmp-devel libgtkmm-3-devel 
    sudo eopkg install sqlite3-devel texlive python3-devel
    sudo eopkg install git cmake make g++

Then configure and build with::

    cd cadabra2
    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make
    sudo make install

This installs below ``/usr`` (instead of ``/usr/local`` on other
platforms) because I could not figure out how to make it pick up
libraries there.

Any feedback on these instructions is welcome.


OpenBSD
~~~~~~~

Install the dependencies with::

  pkg_add git cmake boost python-3.6.2 gtk3mm texlive_texmf-full py3-sympy

We will build using the default clang-4.0.0 compiler; building with
the alternative g++-4.9.4 leads to trouble when linking against the
libraries added with pkg_add.

Configure and build with::

  cd cadabra2
  mkdir build
  cd build
  cmake -DENABLE_MATHEMATICA=OFF ..
  make
  su
  make install

The command-line version is now available as ``cadabra2`` and the
notebook interface as ``cadabra2-gtk``.

Any feedback on this platform is welcome as this is not our
development platform and testing is done only occasionally.


	 
macOS
~~~~~

Cadabra builds with the standard Apple compiler, but in order to build
on macOS you need a number of packages from Homebrew (see
http://brew.sh). Quite a few Homebrew installations have broken
permissions; best to first do::

    sudo chown -R ${USER}:admin /usr/local/

to clean that up. Then install the required dependencies with::

    brew install cmake boost pcre gmp python3 
    brew install pkgconfig 
    brew install gtkmm3 adwaita-icon-theme
    pip3 install sympy gmpy2

If the lines above prompt you to install XCode, go ahead and let it do
that.

You also need a TeX installation such as MacTeX,
http://tug.org/mactex/ .  *Any* TeX will do, as long as 'latex' and
'dvipng' are available. Make sure to *install TeX* before attempting
to build Cadabra, otherwise the Cadabra style files will not be
installed in the appropriate place. Make sure 'latex' works from the
terminal in which you will build Cadabra.

You can build against an Anaconda Python installation (in case you
prefer Anaconda over the Homebrew Python); cmake will automaticaly
pick this up if available.

You need to clone the cadabra2 git repository (if you download the
.zip file you will not have all data necessary to build). So do::

    git clone https://github.com/kpeeters/cadabra2

After that you can build with the standard::

    cd cadabra2
    mkdir build
    cd build
    cmake -DENABLE_MATHEMATICA=OFF ..
    make
    sudo make install

(*note* the `-DENABLE_MATHEMATICA=OFF` in the `cmake` line above; the
Mathematica scalar backend does not yet work on macOS).
  
This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. 

Feedback from macOS users is *very* welcome because this is not the main
development platform.


Windows
~~~~~~~

On Windows the main constraint on the build process is that we want to
link to Anaconda's Python, which has been built with Visual
Studio. The recommended way to build Cadabra is thus to build against
libraries which are all built using Visual Studio as well (if you are
happy to not use Anaconda, you can also build with the excellent MSYS2
system from http://www.msys2.org/; see below). It is practically
impossible to build all dependencies yourself without going crazy, but
fortunately that is not necessary because of the VCPKG library at
https://github.com/Microsoft/vcpkg. This contains all dependencies
(boost, gtkmm, sqlite and various others) in ready-to-use form.


Building with vcpkg (recommended)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you do not already have it, first install Visual Studio Community
Edition from https://www.visualstudio.com/downloads/ and install
Anaconda (a 64 bit version!) from https://www.anaconda.com/download/.
You also need a TeX distribution, for instance MiKTeX from
http://miktex.org and of course git from
e.g. https://gitforwindows.org/. You need all four before you can
start building Cadabra.

The instructions below are for building using the Visual Studio 'x64
Native Tools Command Prompt' (not the GUI). First, clone the vcpkg
repository::

    git clone https://github.com/Microsoft/vcpkg

Run the bootstrap script to set things up::

    cd vcpkg
    bootstrap-vcpkg.bat

Install all the dependencies with (this is a *very* slow process, be
warned, it can easily take several hours, but at least it's automatic)::
  
    vcpkg install mpir:x64-windows glibmm:x64-windows   (go have a coffee)
    vcpkg install sqlite3:x64-windows boost:x64-windows (go for dinner)
    vcpkg install gtkmm:x64-windows                     (run overnight)
    vcpkg integrate install

The last line will spit out a CMAKE toolchain path; write it down, you need that shortly.
Now clone the cadabra repository and configure as::

    cd ..
    git clone https://github.com/kpeeters/cadabra2
    cd cadabra2
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=[the path obtained in the last step]
          -DCMAKE_BUILD_TYPE=RelWithDebInfo
    		 -DVCPKG_TARGET_TRIPLET=x64-windows
    		 -DCMAKE_INSTALL_PREFIX=C:\Cadabra
          -G "Visual Studio 15 2017 Win64" ..

the latter all on one line, in which you replace the
``CMAKE_TOOLCHAIN_PATH`` with the path produced by the ``vcpkg integrate
install`` step. Do _not_ forget the ``..`` at the very end!
You can ignore warnings (but not errors) about Boost. Finally build with::
		
    cmake --build . --config RelWithDebInfo --target install

This will build and then install in ``C:\Cadabra``. The self-tests can be run by
doing::

    ctest

(still fails tensor_monomials, bianchi_identities, paper and young
when in Release build).

Finally, the command-line version of Cadabra can now be started with::

    python C:\Cadabra\bin\cadabra2

and you can start the notebook interface with::

  C:\Cadabra\bin\cadabra2-gtk

It should be possible to simply copy the C:\Cadabra folder to a
different machine and run it there (that is essentially what the
binary installer does).


Building with MSYS2 (not recommended)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Warning: the instructions below are just for guidance, we have not
tried this for quite a while.**

If you are happy with a Cadabra which cannot access an Anaconda Python
distribution, it is possible to build using MSYS2. First, install
MSYS2 from http://www.msys2.org. Once you have a working MSYS2
shell, do the following to install various packages (all from an MSYS2
shell!)::

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


Building a Jupyter kernel
-------------------------

The Cadabra build scripts are now able to build a Jupyter kernel for
Cadabra, so that you can use the Cadabra notation inside a Jupyter
notebook session. For full instructions, see
`building a Jupyter kernel <https://github.com/kpeeters/cadabra2/blob/master/JUPYTER.rst>`_. This is
*experimental* at the moment; all feedback is welcome.


Tutorials and other help
------------------------

Please consult http://cadabra.science/ for tutorial-style notebooks
and all other documentation, and http://kpeeters.github.io/cadabra2
for doxygen documentation of the current master branch. The latter can
also be generated locally; you will need (on Debian and derivatives)::

    sudo apt-get install doxygen libjs-mathjax  

For any questions, please contact info@cadabra.science .


Building Cadabra as C++ library
-------------------------------

EXPERIMENTAL: If you want to use the functionality of Cadabra inside
your own C++ programs, you can build Cadabra as a shared library. To
do this::

    cd c++lib
    mkdir build
    cmake ..
    make
    sudo make install

There is a sample program `simple.cc
<https://github.com/kpeeters/cadabra2/blob/master/c%2B%2Blib/simple.cc>`_
in the `c++lib` directory which shows how to use the Cadabra library.


Special thanks
--------------

Special thanks to José M. Martín-García (for the xPerm
canonicalisation code), James Allen (for writing much of the factoring
code), Dominic Price (for the conversion to pybind and the Windows
port), the Software Sustainability Institute and the Institute of
Advanced Study. Thanks to the many people who have sent me bug reports
(keep 'm coming), and thanks to all of you who uses cadabra, sent
feedback or cited the Cadabra papers.
