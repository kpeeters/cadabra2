
"""
   Schwarzschild spacetime in Schwarzschild coordinates t,r,\theta,\phi.
   Uses greek space-time indices.
"""

import cadabra2
from cadabra2 import *
__cdbkernel__ = cadabra2.__cdbkernel__

Coordinate(Ex(r't,r,\theta,\phi'),Ex(r''))
Indices(Ex(r'\mu,\nu,\rho,\sigma,\kappa,\lambda,\chi,\gamma'), Ex(r'name=spacetime, values={t,r,\theta,\phi}, position=fixed'))
Metric(Ex(r'g_{\mu\nu}'), Ex(r''))
InverseMetric(Ex(r'g^{\mu\nu}'), Ex(r''))

def metric():
    return Ex(r'{ g_{t t} = -(1-2 M/r), g_{r r} = 1/(1-2 M/r), g_{\theta\theta} = r**2, g_{\phi\phi}=r**2 \sin(\theta)**2 }')

