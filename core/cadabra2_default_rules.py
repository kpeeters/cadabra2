def pre_default_rules(ex):
    relax
    
def post_default_rules(ex):
    prodsort(ex)
    eliminate_kronecker(ex)
    canonicalise(ex)
    collect_terms(ex)
