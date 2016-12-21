Cadabra
=======

|Build status|

.. |Build status| image:: https://secure.travis-ci.org/kpeeters/cadabra2.svg?branch=master
   :target: http://travis-ci.org/kpeeters/cadabra2

*A field-theory motivated approach to computer algebra.*

Kasper Peeters <info@cadabra.science>

- End-user documentation at http://cadabra.science/
- Source code documentation at http://kpeeters.github.io/cadabra2

This repository holds the 2.x series of the Cadabra computer algebra
system. It is slowly getting ready for public consumption, but expect
some rough edges.

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

Cadabra builds on Linux and Mac OS X, and might soon build on Windows
too. Select your system from the list bel

- `Linux (Debian/Ubuntu/Mint)`_
- `Linux (Fedora 24)`_
- `Linux (Fedora <=23/CentOS/Scientific Linux)`_
- `Linux (OpenSUSE)`_
- `Linux (Arch/Manjaro)`_
- `Mac OS X`_
- `Windows`_

Binaries for these platforms may (or may not) be provided from the
download page at http://cadabra.science/download.html, but they are
not always very up-to-date.


Linux (Debian/Ubuntu/Mint)
~~~~~~~~~~~~~~~~~~~~~~~~~~

On Debian/Ubuntu you can install all that is needed with::

    sudo apt-get install cmake python3-dev g++ libpcre3 libpcre3-dev libgmp3-dev 
    sudo apt-get install libgtkmm-3.0-dev libboost-all-dev libgmp-dev libsqlite3-dev uuid-dev 
    sudo apt-get install texlive texlive-latex-extra dvipng
    sudo apt-get install python3-matplotlib python3-mpmath python3-pip python3-setuptools
    sudo pip3 install sympy

This is the development platform and issues are typically first fixed
here. You can use either g++ or the clang++ compiler. Building is then
done with the standard::

    mkdir build
    cd build
    cmake ..
    make
    sudo make install

This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. You can also find the latter in
the 'Education' menu.

Linux (Fedora 24)
~~~~~~~~~~~~~~~~~

Fedora 24 is the first Fedora to have Python 3 by default; for older
Fedora versions see below. Install the dependencies with::

    sudo dnf install python3-devel cmake gcc-c++ 
    sudo dnf install pcre-devel gmp-devel libuuid-devel sqlite-devel
    sudo dnf install gtkmm30-devel boost-devel boost-python3-devel
    sudo dnf install texlive python3-matplotlib
    sudo dnf install python3-pip
    sudo pip3 install sympy

This platform receives less testing so please get in touch if you run
into any issues. You can use either g++ or the clang++
compiler. Building is then done with the standard::

    mkdir build
    cd build
    cmake ..
    make
    sudo make install

This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. You can also find the latter in
the 'Education' menu.


Linux (Fedora ≤23/CentOS/Scientific Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On Fedora/CentOS/Scientific Linux you can install the dependencies with::

    sudo yum install epel-release
    sudo yum install python-devel cmake gcc-c++ 
    sudo yum install pcre-devel gmp-devel libuuid-devel sqlite-devel
    sudo yum install gtkmm30-devel boost-devel 
    sudo yum install texlive python-matplotlib

There is no Python 3 by default on this platform, so the instructions
here will build Cadabra for use with Python 2. You also need to
install sympy by hand::

    sudo yum install python-pip
    sudo pip install sympy

This platform receives less testing so please get in touch if you run
into any issues. You can use either g++ or the clang++
compiler. Building is then done with the standard::

    mkdir build
    cd build
    cmake .. -DUSE_PYTHON_3=OFF
    make
    sudo make install

This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. You can also find the latter in
the 'Education' menu.

Linux (OpenSUSE)
~~~~~~~~~~~~~~~~

For OpenSUSE (tested on 'Leap', probably also fine with minor changes
for 'Tumbleweed') you first need to add the `devel:libraries:c_c++`
repository. To do this, start YaST, go to Software/Software
Repositories/Add/Add by URL.  Use the URL

    http://download.opensuse.org/repositories/devel:/libraries:/c_c++/openSUSE_Leap_42.1

After that, dependencies can be installed with::

    sudo zypper install cmake python3-devel gcc-c++
    sudo zypper install pcre-devel gmp-devel libuuid-devel sqlite-devel
    sudo zypper install gtkmm3-devel 
    sudo zypper install texlive python3-matplotlib
    sudo zypper install python3-pip
    sudo zypper install boost_1_61-devel libboost_python3-1_61_0
    sudo pip3 install sympy

This platform receives less testing so please get in touch if you run
into any issues. Building is then done with the standard::

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


Mac OS X
~~~~~~~~

Cadabra builds with the standard Apple compiler, but in order to
build on OS X you need a number of packages from Homebrew (see
http://brew.sh).  Install these packages with::

    brew install cmake boost pcre gmp python3 
    brew uninstall boost-python
    brew install boost-python --with-python3
    brew install pkgconfig 
    brew install gtkmm3 adwaita-icon-theme
    sudo pip3 install sympy

The uninstall of boost-python in the 2nd line is to ensure that you
have a version with python3 support. If the lines above prompt you to
install XCode, go ahead and let it do that.

You also need a TeX installation such as MacTeX,
http://tug.org/mactex/ .  *Any* TeX will do, as long as 'latex' and
'dvipng' are available. Make sure to *install TeX* before attempting
to build Cadabra, otherwise the Cadabra style files will not be
installed in the appropriate place. Make sure 'latex' works from the
terminal in which you will build Cadabra.

Building is then
done with the standard::

    mkdir build
    cd build
    cmake ..
    make
    sudo make install

This will produce the command line app ``cadabra2`` and the Gtk
notebook interface ``cadabra2-gtk``. 

I am still planning a native OS X interface, but because building the
Gtk interface is so easy and the result looks relatively decent, this
may take a while (definitely until after 2.0 has been released).

Feedback from OS X users is *very* welcome because this is not my main
development platform.


Windows
-------

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
    pacman -S mingw-w64-x86_64-adwaita-icon-theme

Then close the MSYS2 shell and open the MINGW64 shell. Run::
  
    cd cadabra2/build
    cmake -G "MinGW Makefiles" -DUSE_PYTHON_3=NO -DCMAKE_INSTALL_PREFIX=/home/[user] ..
    mingw32-make

Replace '[user]' with your user name.
If the cmake fails with a complaint about 'sh.exe', just run it again.
The above builds for python2, let me know if you know how to make it
pick up python3 on Windows.

This fails to install the shared libraries, but they do get
built. Copy them all in ~/bin, and also copy a whole slew of other
things into there. In addition you need 

    cp /mingw64/bin/gspawn-win* ~/bin
    export PYTHONPATH=/mingw64/lib/python2.7:/home/kasper/bin

This fails to start the server with 'The application has requested the
Runtime to terminate it in an unusual way'.



Tutorials and other help
------------------------

Please consult http://cadabra.science/ for tutorial-style notebooks
and all other documentation, and http://kpeeters.github.io/cadabra2
for doxygen documentation of the current master branch. The latter can
also be generated locally; you will need (on Debian and derivatives)::

    sudo apt-get install doxygen libjs-mathjax  

For any questions, please contact info@cadabra.science .



Special thanks
--------------

Special thanks to José M. Martín-García (for the xPerm
canonicalisation code), James Allen (for writing much of the factoring
code) and the Software Sustainability Institute. Thanks to the many
people who have sent me bug reports (keep 'm coming), and thanks to
all of you who cited the Cadabra papers.
