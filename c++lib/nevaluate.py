#!/usr/bin/python3
#
# Test program to determine the speed of sympy/numpy numerical
# evaluation of functions, to compare with our C++ implementation (see
# nevaluate.cc for details; we about 1.5 faster for this example,
# provided we force numpy to do the computations over the complex
# numbers, like we do in Cadabra).

from sympy import *
from sympy.abc import x,y
import numpy
import time

expr = cos(x)*sin(y)
f = lambdify([x,y], expr, "numpy")
xl = numpy.linspace(0, 3.14, 1000, dtype=complex)
yl = numpy.linspace(0, 3.14, 1000, dtype=complex)
X, Y = numpy.meshgrid(xl, yl)
#F = numpy.vectorize(f)

total = 0
num = 100
for i in range(num):
    start = time.time()
    q = f(X, Y)
    end = time.time()
    total += (end - start)

print("cos(x)*sin(y) with numpy over a 1000x1000 grid took", total/num, "seconds on average")

