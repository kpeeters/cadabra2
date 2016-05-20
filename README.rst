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

Prerequisites
-------------

Linux (Debian/Ubuntu/Mint)
~~~~~~~~~~~~~~~~~~~~~~~~~~

On Debian/Ubuntu you can install all that is needed with::

    sudo apt-get install cmake python3-dev g++ libpcre3 libpcre3-dev libgmp3-dev 
    sudo apt-get install libgtkmm-3.0-dev libboost-all-dev libgmp-dev
    sudo apt-get install python-sympy libsqlite3-dev uuid-dev
    sudo apt-get install texlive texlive-latex-extra python3-matplotlib python3-mpmath dvipng

The configuration script will warn you if dependencies are missing. 
To build the documentation locally, you need::

    sudo apt-get install doxygen libjs-mathjax  

This is the development platform and issues are typically first fixed
here. You can use either g++ or the clang++ compiler.


Linux (Fedora/CentOS/Scientific Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On Fedora/CentOS/Scientific Linux you can install the dependencies with::

    sudo yum install cmake gcc-c++ python-devel pcre-devel gmp-devel
    sudo yum install libuuid-devel sqlite-devel
    sudo yum install gtkmm30-devel boost-devel 
    sudo yum install texlive python-matplotlib

This platform receives less testing so please get in touch if you run
into any issues. You can use either g++ or the clang++ compiler.



Linux (Fedora/CentOS/Scientific Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On Scientific Linux you can install the dependencies with::

    sudo yum install cmake gcc-c++ python-devel pcre-devel gmp-devel
    sudo yum install libuuid-devel sqlite-devel
    sudo yum install gtkmm30-devel boost-devel

You also need to install jsconcpp-devel, which is not in the standard
repositories. Find an appropriate package for your distribution at
http://www.rpmfind.net/ and install that (either version 0.6.0 or 0.10.5
will do).

At present there is a problem running the notebook interface on
Scientific Linux.


OS X
~~~~

In order to build on OS X you need a number of packages from Homebrew
(see http://brew.sh).  Install these packages with::

    brew install cmake boost boost-python pcre gmp python3 
    brew install pkgconfig ossp-uuid 
    brew install gtkmm3 adwaita-icon-theme

If this prompts you to install XCode, go ahead and let it do
that. Cadabra builds with the standard Apple compiler.

You also need a TeX installation such as MacTeX,
http://tug.org/mactex/ .  *Any* TeX will do, as long as 'latex' and
'dvipng' are available, and the 'breqn' package is installed. Make
sure to *install TeX* before attempting to build Cadabra, otherwise
the Cadabra style files will not be installed in the appropriate
place. Make sure 'latex' works from the terminal in which you will
build Cadabra.

With the above packages, you will build the Gtk interface for Cadabra.
I am still planning a native OS X interface, but because building the
Gtk interface is so easy and the result looks relatively decent, this
may take a while (definitely until after 2.0 has been released).

Feedback from OS X users is *very* welcome because this is not my main
development platform.


Installation instructions
-------------------------

Once you have the required prerequisites installed, you can build 
Cadabra, on all supported platforms, using the standard::

    mkdir build
    cd build
    cmake ..
    make

This will build all binaries relevant for your platform. You will get 
warned when dependencies are missing. Use::

    sudo make install

to install the software. The notebook interface is started with::

    cadabra-gtk

while the command-line version is called::

    cadabra2



Tutorials and other help
------------------------

Please consult http://cadabra.science/ for tutorial-style notebooks
and all other documentation.



