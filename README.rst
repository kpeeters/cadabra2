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

    sudo apt-get install cmake python-dev g++ libpcre3 libpcre3-dev libgmp3-dev uuid-dev
    sudo apt-get install libgtkmm-3.0-dev libjsoncpp-dev libboost-all-dev libgmp-dev
    sudo apt-get install python-sympy libsqlite3-dev texlive python-matplotlib python-mpmath
    sudo apt-get install doxygen libjs-mathjax  
    sudo apt-get install libsqlite3-dev uuid-dev

The configuration script will warn you if dependencies are missing. 
In order to actually be able to run the Cadabra notebook frontend, you
then also need::

    sudo apt-get install texlive texlive-latex-extra python-matplotlib python-mpmath dvipng

but most likely you have a TeX installation already. 


Linux (Fedora/CentOS/Scientific Linux)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On Fedora/CentOS/Scientific Linux you can install the dependencies with::

    sudo yum install cmake gcc-c++ python-devel pcre-devel gmp-devel
    sudo yum install libuuid-devel sqlite-devel
    sudo yum install gtkmm30-devel boost-devel




OS X
~~~~

Support for OS X is experimental right now, and while there is a
beginning of a native notebook interface, this does **not** work
yet. You can build the Gtk notebook interface, but this is suboptimal.
The command line version is fully functional.

In order to build on OS X you need a number of packages from Homebrew
(see http://brew.sh).  Install these packages with::

    brew install cmake boost boost-python pcre gmp python 
    brew install pkgconfig ossp-uuid gtkmm3 gnome-icon-theme

If this prompts you to install XCode, go ahead and let it do that.

In order to run the Cadabra notebook interface succesfully, you also
need a TeX installation such as MacTeX, http://tug.org/mactex/ .
*Any* TeX will do, as long as 'latex' and 'dvipng' are available, and
the 'breqn' package is installed. 



Installation instructions
-------------------------

Once you have the required prerequisites installed, you can build 
Cadabra using the standard::

    mkdir build
    cd build
    cmake ..
    make

This will build all binaries relevant for your platform. You will get 
warned when dependencies are missing. Use::

    make install

to install the software. The notebook interface is started with::

    cadabra-gtk

while the command-line version is called::

    cadabra2

Tutorials and other help
------------------------

Please consult http://cadabra.science/ for tutorial-style notebooks
and all other documentation.



