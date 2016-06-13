
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

