# This is a pure-python initialisation script to set the 
# path to sympy, setup printing of cadabra expressions and
# attach basic cadabra properties. 

import sys
sys.path.insert(0,'/home/kasper/git/cadabra2/src/build') 
from cadabra2 import *
sys.path.insert(0,'/home/kasper/Development/git.others/sympy') 
import sympy

# Do not
#import signal
#signal.signal(signal.SIGINT, signal.SIG_DFL)

def _displayhook(arg):
    global remember_display_hook
    if isinstance(arg, Ex):
        print(str(arg))
    elif isinstance(arg, Property):
        print(str(arg))
    else:
        remember_display_hook(arg)

remember_display_hook = sys.displayhook
sys.displayhook = _displayhook

Distributable(Ex("\prod{#}"))
#IndexInherit(Ex("\prod{#}"))
#CommutingAsProduct(Ex("\prod{#}"))

# \prod{#}::DependsInherit.
# \prod{#}::WeightInherit(label=all, type=Multiplicative).
# \prod{#}::NumericalFlat.
# 

#CommutingAsSum(Ex("\sum{#}"))

# \sum{#}::DependsInherit.
# \sum{#}::IndexInherit.
# \sum{#}::WeightInherit(label=all, type=Additive).
# 
# \pow{#}::DependsInherit.
# 
# \indexbracket{#}::Distributable.
# \indexbracket{#}::IndexInherit.
# \commutator{#}::IndexInherit.
# \commutator{#}::Derivative.
# \anticommutator{#}::IndexInherit.
# \anticommutator{#}::Derivative.
