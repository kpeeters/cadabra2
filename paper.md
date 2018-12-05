---
title: 'Cadabra2: computer algebra for field theory revisited'
tags:
  - C++
  - Python
  - field theory
  - tensor algebra
  - physics
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

Field theory is an area of mathematics required in a wide range of
theoretical physics problems, from general relativity to high-energy
particle physics and condensed matter theory. Symbolic computations in
this field tend to be difficult to do with mainstream computer algebra
systems, because the required algorithmic functionality is often
simply not available, but also because the standard notation tends to
hide a lot of implicit mathematical structure which cannot easily be
represented.  ``Cadabra2`` is an open source computer algebra system
specifically written for the solution of tensor field theory
problems. It enables manipulation of Lagrangians, computation of
equations of motion, analysis of symmetries and so on in an
interactive notebook interface, using an input format which closely
resembles standard mathematical notation, combined with a familiar
Python environment to manipulate expressions.

The core of ``Cadabra2`` consists of a set of algorithms for tensor
field theory written in C++, which are in part based on functionality
of an earlier version of the software [@cadabra_hep; @cadabra_cpc].
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

``Cadabra2`` has been used to derive or verify results in a variety of
recent papers, in areas such as supergravity [@geissbuhler; @butter],
cosmology [@malik], applications of the string/gauge theory
correspondence [@christensen; @buchel; @koile], and general relativity
[@durkee], to name a few. The software is supported by an on-line Q\&A
forum, a collection of tutorials and on-line manual pages, and has an
active user base. The source code for ``Cadabra2`` has been archived
to Zenodo with the DOI [@cadabra_zenodo].

# Acknowledgements

Special thanks to José M. Martín-García, James Allen and Dominic Price
for various contributions to the code, and the Software Sustainability
Institute and the Institute of Advanced Study at Durham University for
support.

# References

