#include <unordered_set>
#include "Keywords.hh"


const std::unordered_set<std::string> functions = {
	"__build_class__", "__import__", "_displayhook", "abs", "all", "any", "ascii", 
	"asym", "bin", "breakpoint", "callable", "canonicalise", "cdb2python", 
	"cdb2python_string", "chr", "collect_components", "collect_factors", 
	"collect_terms", "combine", "compile", "compile_package__", "complete", 
	"create_empty_scope", "create_scope", "create_scope_from_global", "decompose", 
	"decompose_product", "delattr", "diff", "dir", "display", "distribute", 
	"divmod", "drop_weight", "einsteinify", "eliminate_kronecker", 
	"eliminate_metric", "eliminate_vielbein", "epsilon_to_delta", "eval", 
	"evaluate", "exec", "expand", "expand_delta", "expand_diracbar", 
	"expand_dummies", "expand_power", "explicit_indices", "factor", "factor_in", 
	"factor_out", "fierz", "flatten_sum", "format", "getattr", "globals", 
	"hasattr", "hash", "hex", "id", "indexsort", "init_ipython", "input", 
	"integrate", "integrate_by_parts", "isinstance", "issubclass", "iter", 
	"join_gamma", "keep_terms", "keep_weight", "kernel", "latex", "len", "lhs", 
	"locals", "lower_free_indices", "lr_tensor", "map_sympy", "max", "meld", "min", 
	"next", "oct", "open", "ord", "order", "post_process", "pow", "print", 
	"product_rule", "properties", "raise_free_indices", "reduce_delta", 
	"remember_display_hook", "rename_dummies", "replace_match", "repr", 
	"rewrite_indices", "rhs", "round", "save_history", "setattr", "simplify", 
	"slot_asym", "slot_sym", "sort_product", "sort_spinors", "sort_sum", "sorted", 
	"split_gamma", "split_index", "sqrt", "substitute", "sum", "sym", "symbols", 
	"tab_dimension", "take_match", "terms", "tree", "trigsimp", "untrace", 
	"unwrap", "unzoom", "user_config_dir", "user_data_dir", "vars", "vary", 
	"young_project", "young_project_product", "young_project_tensor", "zoom", 
};

const std::unordered_set<std::string> classes = {
	"Accent", "AntiCommuting", "AntiSymmetric", "ArithmeticError", 
	"AssertionError", "AttributeError", "BaseException", "BlockingIOError", 
	"BrokenPipeError", "BufferError", "BytesWarning", "ChildProcessError", 
	"Commuting", "CommutingAsProduct", "CommutingAsSum", "CommutingBehaviour", 
	"ConnectionAbortedError", "ConnectionError", "ConnectionRefusedError", 
	"ConnectionResetError", "Console", "Coordinate", "DAntiSymmetric", "Depends", 
	"DependsBase", "DeprecationWarning", "Derivative", "Determinant", "Diagonal", 
	"DifferentialForm", "DifferentialFormBase", "DiracBar", "Distributable", 
	"EOFError", "EnvironmentError", "EpsilonTensor", "Ex", "ExNode", "Exception", 
	"ExteriorDerivative", "FileExistsError", "FileNotFoundError", "FilledTableau", 
	"FloatingPointError", "FutureWarning", "GammaMatrix", "GeneratorExit", 
	"IOError", "ImaginaryI", "ImplicitIndex", "ImportError", "ImportWarning", 
	"IndentationError", "IndexError", "IndexInherit", "Indices", "Integer", 
	"InterruptedError", "InverseMetric", "InverseVielbein", "IsADirectoryError", 
	"Kernel", "KeyError", "KeyboardInterrupt", "KroneckerDelta", "LaTeXForm", 
	"LookupError", "Matrix", "MemoryError", "MetaPathFinder", "Metric", 
	"ModuleNotFoundError", "ModuleSpec", "NameError", "NonCommuting", 
	"NotADirectoryError", "NotImplementedError", "NumericalFlat", "OSError", 
	"OverflowError", "PackageCompiler", "PartialDerivative", "PathFinder", 
	"PendingDeprecationWarning", "PermissionError", "ProcessLookupError", 
	"ProgressMonitor", "Property", "RecursionError", "ReferenceError", 
	"ResourceWarning", "RiemannTensor", "RuntimeError", "RuntimeWarning", 
	"SatisfiesBianchi", "SelfAntiCommuting", "SelfCommuting", 
	"SelfCommutingBehaviour", "SelfNonCommuting", "Server", "SortOrder", 
	"SourceFileLoader", "Spinor", "StopAsyncIteration", "StopIteration", 
	"Stopwatch", "Symbol", "Symmetric", "SympyBridge", "SyntaxError", 
	"SyntaxWarning", "SystemError", "SystemExit", "TabError", "Tableau", 
	"TableauBase", "TableauInherit", "TableauObserver", "TableauSymmetry", 
	"TimeoutError", "Total", "Trace", "Traceless", "TypeError", 
	"UnboundLocalError", "UnicodeDecodeError", "UnicodeEncodeError", 
	"UnicodeError", "UnicodeTranslateError", "UnicodeWarning", "UserWarning", 
	"ValueError", "Vielbein", "Warning", "Weight", "WeightBase", "WeightInherit", 
	"WeylTensor", "WindowsError", "ZeroDivisionError", "__loader__", "bool", 
	"bytearray", "bytes", "classmethod", "complex", "cos", "dict", "enumerate", 
	"filter", "float", "frozenset", "int", "labelled_property", "list", 
	"list_property", "map", "match_t", "memoryview", "object", "parent_rel_t", 
	"property", "range", "result_t", "reversed", "sMatrix", "scalar_backend_t", 
	"set", "sin", "slice", "staticmethod", "str", "super", "tan", "tuple", "type", 
	"unicode", "warn_t", "zip", 
};

const std::unordered_set<std::string> keywords = {
	"False", "None", "True", "and", "as", "assert", "async", "await", "break", 
	"class", "continue", "def", "del", "elif", "else", "except", "finally", "for", 
	"from", "global", "if", "import", "in", "is", "lambda", "nonlocal", "not", 
	"or", "pass", "raise", "return", "try", "while", "with", "yield", 
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
