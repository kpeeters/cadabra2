""" keywords scraped from cadabra.science/man.html """

properties = [
    "Accent",
    "AntiCommuting",
    "AntiSymmetric",
    "Commuting",
    "CommutingAsProduct",
    "CommutingAsSum",
    "Coordinate",
    "DAntiSymmetric",
    "Depends",
    "Derivative",
    "Determinant",
    "Diagonal",
    "DiracBar",
    "EpsilonTensor",
    "FilledTableau",
    "GammaMatrix",
    "ImplicitIndex",
    "IndexInherit",
    "Indices",
    "Integer",
    "InverseMetric",
    "KroneckerDelta",
    "LaTeXForm",
    "Metric",
    "NonCommuting",
    "PartialDerivative",
    "RiemannTensor",
    "SatisfiesBianchi",
    "SelfAntiCommuting",
    "SelfCommuting",
    "SelfNonCommuting",
    "SortOrder",
    "Spinor",
    "Symbol",
    "Symmetric",
    "Tableau",
    "TableauSymmetry",
    "WeightInherit",
]


import re


class CodeCompleter:
    def __init__(self, kernel):
        self._kernel = kernel
        self.code = ""

    def get_last_word(self):
        return re.compile(r"[\W\s]").split(self.code)[-1]

    def cursor_on_property(self, last):
        search_query = re.compile(".*::{}$".format(last), re.MULTILINE)
        return re.search(search_query, self.code)

    def match(self, last, options):
        opts = list(filter(lambda i: re.match("^{}.*".format(last), i), options))
        # return options and length of the last matched word
        return opts, len(last)

    def match_member(self, last, klass):
        """ matches last word in the namespace of the currently selected class """
        instance = self._kernel._sandbox_context._sandbox[klass]
        return self.match(last, dir(instance))

    def get_class(self):
        """ get class currently behind cursor  """
        return re.findall(r"(\w+)\.\w*$", self.code)

    def __call__(self, code, cursor_pos, namespace):
        # only choose up until current cursor position
        self.code = code[:cursor_pos]

        last = self.get_last_word()
        matched_klass = self.get_class()

        if self.cursor_on_property(last):
            # cursor on cadabra property
            if last:
                return self.match(last, properties)
            else:
                return properties, 0

        elif matched_klass and matched_klass[-1] in namespace:
            # cursor on member function
            return self.match_member(last, matched_klass[-1])
        else:
            return self.match(last, namespace)
