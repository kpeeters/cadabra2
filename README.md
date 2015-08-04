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

To get started with the source code documentation, navigate to the
\ref modules overview.

## Installation instructions

Run

    mkdir build
    cd build
    cmake ..
    make

to build all binaries relevant for your platform. You will get 
warned when dependencies are missing. Use

    make install

to install the software.

