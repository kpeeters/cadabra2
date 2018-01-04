Cadabra                                         {#mainpage}
=======

\author  Kasper Peeters
\version 2.x series
\see     http://cadabra.science


This repository holds the 2.x series of the Cadabra computer
algebra system. 

Cadabra was designed specifically for the solution of problems
encountered in field theory. It has extensive functionality for tensor
computer algebra, tensor polynomial simplification including
multi-term symmetries, fermions and anti-commuting variables, Clifford
algebras and Fierz transformations, implicit coordinate dependence,
multiple index types and many more. The input format is a subset of
TeX. Both a command-line and a graphical interface are available.

The Cadabra system is built around the C++ expression storage class
cadabra::Ex.  This stores mathematical expressions in the form of a
symbol tree. Properties, derived from cadabra::property, can be
attached to symbols inside the tree.  Algorithms, derived from
cadabra::Algorithm, act on the cadabra::Ex objects, transforming their
content by making use of the property information.

The C++ objects and algorithms can be accessed via Python (via the
cadabra2 Python module built in PyCdb.cc), or they can be used
directly through the use of Cadabra-as-a-library.

To get started with the source code documentation, navigate to the
\ref modules overview.
