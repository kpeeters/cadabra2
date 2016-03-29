Cadabra
=======

|Build status|

.. |Build status| image:: https://secure.travis-ci.org/kpeeters/cadabra2.svg?branch=master
   :target: http://travis-ci.org/kpeeters/cadabra2

A field-theory motivated approach to computer algebra.

Kasper Peeters

http://cadabra.science/

This repository holds the 2.x series of the Cadabra computer
algebra system. It is not yet ready for public consumption.

Cadabra was designed specifically for the solution of problems
encountered in field theory. It has extensive functionality for tensor
computer algebra, tensor polynomial simplification including
multi-term symmetries, fermions and anti-commuting variables, Clifford
algebras and Fierz transformations, implicit coordinate dependence,
multiple index types and many more. The input format is a subset of
TeX. Both a command-line and a graphical interface are available.

Prerequisites
-------------

The configuration script will warn you if dependencies are missing. On
Debian/Ubuntu you can install all that is needed with
::
    sudo apt-get install cmake python-dev g++ libpcre3 libpcre3-dev libgmp3-dev 
    sudo apt-get install libgtkmm-3.0-dev libjsoncpp-dev libboost-all-dev libgmp-dev
    sudo apt-get install texlive python-matplotlib python-mpmath
    sudo apt-get install doxygen libjs-mathjax  
    sudo apt-get install libsqlite3-dev uuid-dev

Installation instructions
-------------------------

Once you have the required prerequisites installed, you can build 
Cadabra using the standard
::
    mkdir build
    cd build
    cmake ..
    make

This will build all binaries relevant for your platform. You will get 
warned when dependencies are missing. Use
::
    make install

to install the software. The notebook interface is started with
::
    cadabra-gtk

while the command-line version is called
::
    cadabra2

Tutorials and other help
------------------------

Please consult http://cadabra.science/ for tutorial-style notebooks
and all other documentation.



