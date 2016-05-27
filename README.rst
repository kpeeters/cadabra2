Cadabra
=======

|Build status|

.. |Build status| image:: https://secure.travis-ci.org/kpeeters/cadabra2.svg?branch=master
   :target: http://travis-ci.org/kpeeters/cadabra2

*A field-theory motivated approach to computer algebra.*

Kasper Peeters

**end user documentation**: http://cadabra.science/

**source code documentation**: http://kpeeters.github.io/cadabra2

This repository holds the 2.x series of the Cadabra computer
algebra system. It is slowly getting ready for public consumption, but 
expect some rough edges.

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

Cadabra builds on Linux and OS X, select your distribution below:

- `Linux (Debian/Ubuntu/Mint)`_
- `Linux (Fedora/CentOS/Scientific Linux)`_
- `Linux (Arch/Manjaro)`_
- `Mac OS X`_


Linux (Debian/Ubuntu/Mint)
~~~~~~~~~~~~~~~~~~~~~~~~~~

On Debian/Ubuntu you can install all that is needed with::

    sudo apt-get install cmake python3-dev g++ libpcre3 libpcre3-dev libgmp3-dev 
    sudo apt-get install libgtkmm-3.0-dev libboost-all-dev libgmp-dev
    sudo apt-get install python-sympy libsqlite3-dev uuid-dev
    sudo apt-get install texlive texlive-latex-extra python3-matplotlib python3-mpmath dvipng

This is the development platform and issues are typically first fixed
here. You can use either g++ or the clang++ compiler. Building is then
done with the standard::

    mkdir build
    cd build
    cmake ..
    make
    sudo make install

This will produce the command line app::

    cadabra2

and the Gtk notebook interface::

    cadabra-gtk

You can also find the latter in the 'Education' menu.


Linux (Fedora/CentOS/Scientific Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On Fedora/CentOS/Scientific Linux you can install the dependencies with::

    sudo yum install cmake gcc-c++ python-devel pcre-devel gmp-devel
    sudo yum install libuuid-devel sqlite-devel
    sudo yum install gtkmm30-devel boost-devel 
    sudo yum install texlive python3-matplotlib

This platform receives less testing so please get in touch if you run
into any issues. You can use either g++ or the clang++
compiler. Building is then done with the standard::

    mkdir build
    cd build
    cmake ..
    make
    sudo make install

This will produce the command line app::

    cadabra2

and the Gtk notebook interface::

    cadabra-gtk

You can also find the latter in the 'Education' menu.


Linux (Arch/Manjaro)
~~~~~~~~~~~~~~~~~~~~

The development package for Arch Linux is `cadabra2-git
<https://aur.archlinux.org/packages/cadabra2-git/>`.  Building and
installing (including dependencies) can be accomplished with::

    yaourt -Sy cadabra2-git

Alternatively use ``makepkg``::

    curl -L -O https://aur.archlinux.org/cgit/aur.git/snapshot/cadabra2-git.tar.gz
    tar -xvf cadabra2-git.tar.gz
    cd cadabra2-git
    makepkg -sri

Please consult the `Arch Wiki
<https://wiki.archlinux.org/index.php/Arch_User_Repository#Installing_packages>`
for more information regarding installing packages from the AUR.


Mac OS X
~~~~~~~~

Cadabra builds with the standard Apple compiler, but in order to
build on OS X you need a number of packages from Homebrew (see
http://brew.sh).  Install these packages with::

    brew install cmake boost pcre gmp python3 
    brew uninstall boost-python
    brew install boost-python --with-python3
    brew install pkgconfig ossp-uuid 
    brew install gtkmm3 adwaita-icon-theme

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

This will produce the command line app::

    cadabra2

and the Gtk notebook interface::

    cadabra-gtk

I am still planning a native OS X interface, but because building the
Gtk interface is so easy and the result looks relatively decent, this
may take a while (definitely until after 2.0 has been released).

Feedback from OS X users is *very* welcome because this is not my main
development platform.



Tutorials and other help
------------------------

Please consult http://cadabra.science/ for tutorial-style notebooks
and all other documentation, and http://kpeeters.github.io/cadabra2
for doxygen documentation of the current master branch. The latter can
also be generated locally; you will need (on Debian and derivatives)::

    sudo apt-get install doxygen libjs-mathjax  

For any questions, please contact mailto:info@cadabra.science .






