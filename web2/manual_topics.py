# Topic groups for the reference manual index (man.html), used by
# scan.py. Each algorithm and property is listed under exactly one
# topic; anything with a manual page that does not appear here ends
# up under 'Other', and scan.py prints a warning about it.

ALGORITHM_TOPICS = [
    ("Expanding", [
        "distribute", "expand", "expand_power", "expand_delta",
        "expand_dummies", "product_rule", "unwrap", "explicit_indices",
    ]),
    ("Simplifying and canonical form", [
        "canonicalise", "collect_terms", "collect_factors", "meld",
        "sort_product", "sort_sum", "factor_in", "factor_out",
        "simplify", "reduce_delta", "eliminate_kronecker",
    ]),
    ("Substitution and selecting subexpressions", [
        "substitute", "complete", "zoom", "take_match", "replace_match",
        "keep_weight", "drop_weight", "vary", "integrate_by_parts",
    ]),
    ("Indices and metrics", [
        "einsteinify", "eliminate_metric", "eliminate_vielbein",
        "rewrite_indices", "lower_free_indices", "raise_free_indices",
        "rename_dummies", "split_index", "epsilon_to_delta",
    ]),
    ("Symmetrisation and Young tableaux", [
        "asym", "slot_asym", "young_project_tensor",
        "young_project_product", "decompose", "decompose_product",
        "lr_tensor",
    ]),
    ("Spinors and gamma matrices", [
        "fierz", "join_gamma", "split_gamma", "sort_spinors",
        "expand_diracbar", "combine", "untrace",
    ]),
    ("Components, numerics and SymPy", [
        "evaluate", "nevaluate", "nval", "ndsolve", "map_sympy",
    ]),
]

PROPERTY_TOPICS = [
    ("Symbols and indices", [
        "Indices", "Symbol", "Coordinate", "Integer", "LaTeXForm",
        "Accent", "ImplicitIndex", "IndexInherit", "Depends", "Weight",
        "WeightInherit",
    ]),
    ("Symmetries", [
        "Symmetric", "AntiSymmetric", "DAntiSymmetric", "TableauSymmetry",
        "Tableau", "FilledTableau", "SatisfiesBianchi", "Diagonal",
    ]),
    ("Commutation and ordering", [
        "Commuting", "AntiCommuting", "NonCommuting", "SelfCommuting",
        "SelfAntiCommuting", "SelfNonCommuting", "CommutingAsProduct",
        "CommutingAsSum", "SortOrder",
    ]),
    ("Metrics and special tensors", [
        "Metric", "InverseMetric", "KroneckerDelta", "EpsilonTensor",
        "RiemannTensor", "Determinant", "Vielbein", "InverseVielbein",
        "Trace",
    ]),
    ("Derivatives and operators", [
        "Derivative", "PartialDerivative", "Distributable",
    ]),
    ("Spinors", [
        "Spinor", "DiracBar", "GammaMatrix",
    ]),
]
