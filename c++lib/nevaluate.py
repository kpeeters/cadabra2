#!/usr/bin/python3
#
# Test program to determine the speed of sympy/numpy numerical
# evaluation of functions, to compare with our C++ implementation
# (see nevaluate.cc for details; we about an order of magnitude
# faster for this example).

from sympy import *
from sympy.abc import x,y
import numpy
import time

expr = cos(x)*sin(y)
f = lambdify([x,y], expr, "numpy")
xl = numpy.linspace(0, 3.14, 1000)
yl = numpy.linspace(0, 3.14, 1000)
X, Y = numpy.meshgrid(xl, yl)
F = numpy.vectorize(f)

start = time.time()
q = F(X, Y)
end = time.time()

print("cos(x)*sin(y) over a 1000x1000 grid took", end-start, "seconds")
