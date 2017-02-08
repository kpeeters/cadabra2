import cadabra2
from cadabra2 import *
__cdbkernel__ = cadabra2.__cdbkernel__

Indices(Ex(r'{i,j,k,l,m,n,q,r}'), Ex(r'space'))
Integer(Ex(r'{i,j,k,l,m,n,q,r}'), Ex(r'1..4'))
Indices(Ex(r'{a,b,c,d,e,f,g}'),   Ex(r'group'))
Indices(Ex(r'{a,b,c,d,e,f,g}'),   Ex(r'1..3'))
EpsilonTensor(Ex(r'\epsilon^{a b c}'),Ex(''))
EpsilonTensor(Ex(r'\epsilon_{i j k l}'),Ex(''))
KroneckerDelta(Ex(r'\delta_{i j}'), Ex(r''))

def thooft_symbols():
    TableauSymmetry(Ex(r'\eta^{a}_{i j}'), Ex(r'shape={1,1},indices={1,2}'))
    rl = Ex(r'{ \eta^{a}_{i j} \eta^{a}_{k l} -> \delta_{i k} \delta_{j l} - \delta_{i l} \delta_{k j} + \epsilon_{i j k l}, \epsilon^{a b c} \eta^{b}_{i k} \eta^{c}_{j l} -> -\delta_{i j} \eta^{a}_{k l} - \delta_{k l} \eta^{a}_{i j} + \delta_{i l} \eta^{a}_{k j} + \delta_{k j} \eta^{a}_{i l}, \eta^{a}_{i k} \eta^{b}_{k j} -> -\delta^{a b} \delta_{i j} + \epsilon^{a b c} \eta^{c}_{i j}, \epsilon_{i j l k} \eta^{a}_{l m} -> - \delta_{i m} \eta^{a}_{j k} - \delta_{j m} \eta^{a}_{k i} - \delta_{k m} \eta^{a}_{i j}, \epsilon_{i j k l} \eta^{a}_{k l} -> - 2 \eta^{a}_{i j} }')
    return rl
