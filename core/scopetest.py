
from scopetest import *

def f():
    a=42
    cfun()
    print a
    print locals()['a']

f()

def g():
    cfun()
    print a

g()


# This produces 42 and 'global name 'a' is undefined, showing that
# the change
