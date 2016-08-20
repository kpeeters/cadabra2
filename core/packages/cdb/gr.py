
# General relativity package for Cadabra.

import cadabra2 as cdb

def RiemannFromChristoffel(R, c):
    """
    @param R: Riemann tensor.
    @param c: Christoffel symbol.
    """

    # - (determine index structure on R).
    # - determine name of head node on R.
    # - determine name of head node on c.
    # - construct rule

    ex = cdb.Ex(r'@(R)^{\rho}_{\sigma\mu\nu} = \partial_{\mu}{@(c)^{\rho}_{\nu\sigma}} -\partial_{\nu}{@(c)^{\rho}_{\mu\sigma}} + @(c)^{\rho}_{\mu\lambda} @(c)^{\lambda}_{\nu\sigma} - @(c)^{\rho}_{\nu\lambda} @(c)^{\lambda}_{\mu\sigma}')

    return ex

def ChristoffelFromMetric(c, g):
    """
    @param c: Christoffel symbol.
    @param g: metric tensor.
    """

    ex = cdb.Ex(r'@(c)^{\lambda}_{\mu\nu} = 1/2 g^{\lambda\kappa} ( \partial_{\mu}{ g_{\kappa\nu} } + \partial_{\nu}{ g_{\kappa\mu} } - \partial_{\kappa}{ g_{\mu\nu} } )')

    return ex

