Cadabra documentation                                  {#mainpage}
=====================

\author  Kasper Peeters
\version 2.x
\see     http://cadabra.phi-sci.com


This repository holds the 2.x series of the Cadabra computer
algebra system. It is not yet ready for public consumption.

Cadabra was designed specifically for the solution of problems
encountered in field theory. It has extensive functionality for tensor
computer algebra, tensor polynomial simplification including
multi-term symmetries, fermions and anti-commuting variables, Clifford
algebras and Fierz transformations, implicit coordinate dependence,
multiple index types and many more. The input format is a subset of
TeX. Both a command-line and a graphical interface are available.


## Installation instructions

Run

    mkdir build
    cd build
    cmake ..
    make

to build the code, and 

    make install

to install it. 


## Documentation overview

The source is split into the following modules:

  \ref core

  \ref clientserver 

  \ref frontend
