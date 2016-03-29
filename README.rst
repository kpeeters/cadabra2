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

Installation instructions
-------------------------

Run
    
    mkdir build
    cd build
    cmake ..
    make

to build all binaries relevant for your platform. You will get 
warned when dependencies are missing. Use

    make install

to install the software.

