
"""
   General relativity package for Cadabra.
   NOTE: this is a proof of concept, not something that is finished in any sense.
"""

import cadabra2
from cadabra2 import *
__cdbkernel__ = cadabra2.__cdbkernel__

def riemann_from_christoffel(R=Ex(r'R'), c=Ex(r'\Gamma')):
    PartialDerivative(Ex(r'\partial{#}'), Ex(r''))
    ex = Ex(r'@(R)^{\rho}_{\sigma\mu\nu} = \partial_{\mu}{@(c)^{\rho}_{\nu\sigma}} -\partial_{\nu}{@(c)^{\rho}_{\mu\sigma}} + @(c)^{\rho}_{\mu\lambda} @(c)^{\lambda}_{\nu\sigma} - @(c)^{\rho}_{\nu\lambda} @(c)^{\lambda}_{\mu\sigma}')

    return ex

def christoffel_from_metric(c=Ex(r'\Gamma'), g=Ex(r'g')):
    PartialDerivative(Ex(r'\partial{#}'), Ex(r''))
    ex = Ex(r'@(c)^{\lambda}_{\mu\nu} = 1/2 g^{\lambda\kappa} ( \partial_{\mu}{ g_{\kappa\nu} } + \partial_{\nu}{ g_{\kappa\mu} } - \partial_{\kappa}{ g_{\mu\nu} } )')

    return ex

def expand_covariant_derivative():
    ex = Ex(r'\nabla_{\mu}{A?^{\nu}} = \partial_{\mu}{A?^{\nu}} + \Gamma^{\nu}_{\mu\rho} A?^{\rho}')

    return ex

def riemann_to_ricci(ex):
    """ Convert contractions of Riemann tensors to Ricci tensors or scalars. """

    rl1 = Ex(r'R^{a?}_{b? a? c?}     =  R_{b? c?}, R^{a?}_{b? c? a?}     = -R_{b? c?}')
    rl2 = Ex(r'R_{a?}_{b?}^{a?}_{c?} =  R_{b? c?}, R_{a?}_{b? c?}^{a?}   = -R_{b? c?}')
    rl3 = Ex(r'R_{b?}^{a?}_{c? a?}   =  R_{b? c?}, R_{b?}^{a?}_{a? c?}   = -R_{b? c?}')
    rl4 = Ex(r'R_{b?}_{a?}^{c? a?}   =  R_{b?}^{c?}')    
    rl5 = Ex(r'R^{a?}_{a?} = R, R_{a?}^{a?} = R')

    substitute(ex, rl1+rl2+rl3+rl4+rl5, repeat=True)

    return ex
