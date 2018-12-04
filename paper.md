---
title: 'Cadabra2: A field-theory motivated approach to computer algebra'
tags:
  - C++
  - Python
  - field theory
  - tensor field theory
authors:
  - name: Kasper Peeters
    orcid: 0000-0002-3077-8193
    affiliation: "1"
affiliations:
 - name: Durham University
   index: 1
date: 4 December 2018
bibliography: paper.bib
---

# Summary

Tensor field theory is an area of mathematics required in a wide range
of theoretical physics problems, from general relativity to
high-energy particle physics and condensed matter theory. Symbolic
computations in this field tend to be difficult to do with mainstream
computer algebra systems, because the required algorithmic
functionality is often simply not avaiable, but also because the
standard notation tends to hide a lot of implicit mathematical
structure which cannot easily be hidden.  ``Cadabra2`` is an open
source computer algebra system specifically written for the solution
of tensor field theory problems. It enables manipulation of
Lagrangians, computation of equations of motion, analysis of
symmetries and so on in an interactive notebook interface, using an
input format which closely resembles standard mathematical notation,
combined with a familiar Python language to manipulate expressions.

The core of ``Cadabra2`` consists of a set of algorithms for tensor
field theory written in C++, which are in part based on functionality
of an earlier version of the software [@cadabrahep, @Peeters:2006kp].
These algorithms take care of specific tensor aspects of computer
algebra, such as dummy indices, implicit coordinate dependence,
implicit index lines and commutativity properties. All standard scalar
algebra is handed off to a scalar backend, currently either Sympy
[@sympy] or Mathematica [@mathematica]. The core is accessible from
Python, using a wrapper built using pybind11 [@pybind11]. At the
highest level there is a custom pre-processor which enables input in a
mixture of LaTeX for mathematical expressions and Python for
expression manipulation. The user interface consists of a command-line
client, as well as a graphical cell-based notebook built using gtkmm,
with TeX-driven maths typesetting.  The software builds and runs on
Linux, macOS and Windows.

``Cadabra2`` has been used to derive results in a variety of recent
papers, in areas such as supergravity [@geissbuhler2011double,
@butter2017component], cosmology [@malik2009cosmological],
applications of the string/gauge theory correspondence
[@christensen2014boundary,@buchel2008universal,koile2015hadron], and
general relativity [@durkee2010generalization]. The software is
supported by an on-line Q\&A forum and an active user base.


# Acknowledgements



# References
