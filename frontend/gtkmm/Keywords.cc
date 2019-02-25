#include <unordered_set>
#include "Keywords.hh"


const std::unordered_set<std::string> functions = {
	"__build_class__", "__import__", "abs", "all", "any", "ascii", "bin", 
	"callable", "chr", "compile", "delattr", "dir", "divmod", "eval", "exec", 
	"format", "getattr", "globals", "hasattr", "hash", "hex", "id", "input", 
	"isinstance", "issubclass", "iter", "len", "locals", "max", "min", "next", 
	"oct", "open", "ord", "pow", "print", "repr", "round", "setattr", "sorted", 
	"sum", "vars", "asym", "canonicalise", "collect_components", "collect_factors", 
	"collect_terms", "combine", "compile_package__", "complete", 
	"create_empty_scope", "create_scope", "create_scope_from_global", "decompose", 
	"decompose_product", "distribute", "drop_weight", "einsteinify", 
	"eliminate_kronecker", "eliminate_metric", "epsilon_to_delta", "evaluate", 
	"expand", "expand_delta", "expand_diracbar", "expand_power", 
	"explicit_indices", "factor_in", "factor_out", "fierz", "flatten_sum", 
	"indexsort", "init_ipython", "integrate_by_parts", "join_gamma", "keep_terms", 
	"keep_weight", "kernel", "lhs", "lower_free_indices", "lr_tensor", "map_sympy", 
	"order", "product_rule", "properties", "raise_free_indices", "reduce_delta", 
	"rename_dummies", "replace_match", "rewrite_indices", "rhs", "simplify", 
	"sort_product", "sort_spinors", "sort_sum", "split_gamma", "split_index", 
	"substitute", "sym", "tab_dimension", "take_match", "terms", "tree", "untrace", 
	"unwrap", "unzoom", "vary", "young_project", "young_project_product", 
	"young_project_tensor", "zoom", 
};

const std::unordered_set<std::string> classes = {
	"ArithmeticError", "AssertionError", "AttributeError", "BaseException", 
	"BlockingIOError", "BrokenPipeError", "BufferError", "BytesWarning", 
	"ChildProcessError", "ConnectionAbortedError", "ConnectionError", 
	"ConnectionRefusedError", "ConnectionResetError", "DeprecationWarning", 
	"EOFError", "EnvironmentError", "Exception", "FileExistsError", 
	"FileNotFoundError", "FloatingPointError", "FutureWarning", "GeneratorExit", 
	"IOError", "ImportError", "ImportWarning", "IndentationError", "IndexError", 
	"InterruptedError", "IsADirectoryError", "KeyError", "KeyboardInterrupt", 
	"LookupError", "MemoryError", "ModuleNotFoundError", "NameError", 
	"NotADirectoryError", "NotImplementedError", "OSError", "OverflowError", 
	"PendingDeprecationWarning", "PermissionError", "ProcessLookupError", 
	"RecursionError", "ReferenceError", "ResourceWarning", "RuntimeError", 
	"RuntimeWarning", "StopAsyncIteration", "StopIteration", "SyntaxError", 
	"SyntaxWarning", "SystemError", "SystemExit", "TabError", "TimeoutError", 
	"TypeError", "UnboundLocalError", "UnicodeDecodeError", "UnicodeEncodeError", 
	"UnicodeError", "UnicodeTranslateError", "UnicodeWarning", "UserWarning", 
	"ValueError", "Warning", "WindowsError", "ZeroDivisionError", "__loader__", 
	"bool", "bytearray", "bytes", "classmethod", "complex", "dict", "enumerate", 
	"filter", "float", "frozenset", "int", "list", "map", "memoryview", "object", 
	"property", "range", "reversed", "set", "slice", "staticmethod", "str", 
	"super", "tuple", "type", "zip", "Accent", "AntiCommuting", "AntiSymmetric", 
	"Commuting", "CommutingAsProduct", "CommutingAsSum", "Coordinate", 
	"DAntiSymmetric", "Depends", "Derivative", "Determinant", "Diagonal", 
	"DifferentialForm", "DiracBar", "Distributable", "EpsilonTensor", "Ex", 
	"ExNode", "ExteriorDerivative", "FilledTableau", "GammaMatrix", "ImaginaryI", 
	"ImplicitIndex", "IndexInherit", "Indices", "Integer", "InverseMetric", 
	"InverseVielbein", "Kernel", "KroneckerDelta", "LaTeXForm", "Matrix", "Metric", 
	"NonCommuting", "NumericalFlat", "PartialDerivative", "ProgressMonitor", 
	"Property", "RiemannTensor", "SatisfiesBianchi", "SelfAntiCommuting", 
	"SelfCommuting", "SelfNonCommuting", "SortOrder", "Spinor", "Stopwatch", 
	"Symbol", "Symmetric", "Tableau", "TableauSymmetry", "Total", "Trace", 
	"Traceless", "Vielbein", "Weight", "WeightInherit", "WeylTensor", 
	"parent_rel_t", "result_t", "scalar_backend_t", 
};

const std::unordered_set<std::string> keywords = {
	"False", "None", "True", "and", "as", "assert", "break", "class", "continue", 
	"def", "del", "elif", "else", "except", "finally", "for", "from", "global", 
	"if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise", 
	"return", "try", "while", "with", "yield", 
};

const char* get_keyword_group(const std::string& name)
{
	if (functions.find(name) != functions.end())
		return "function";
	if (classes.find(name) != classes.end())
		return "class";
	if (keywords.find(name) != keywords.end())
		return "keyword";
	return nullptr;
}
