#pragma once

#include <memory>

#include "../Storage.hh"
#include "../ExNode.hh"

namespace cadabra {
	using Ex_ptr = std::shared_ptr<Ex>;

	/// \ingroup pythoncore
	///
	/// Comparison operator for Ex objects in Python. Since comparison operators
	/// need a Properties object, we cannot have such operator== things in C++,
	/// but we can in Python since we can get the kernel in the current scope.
	bool Ex_compare(Ex_ptr, Ex_ptr);
	bool Ex_compare(Ex_ptr, int);

	/// \ingroup pythoncore
	///
	/// Add two expressions, adding a top-level '\sum' node if required.

	Ex_ptr Ex_add(const Ex_ptr ex1, const ExNode ex2);
	Ex_ptr Ex_add(const Ex_ptr ex1, const Ex_ptr ex2);
	Ex_ptr Ex_add(const Ex_ptr ex1, const Ex_ptr ex2, Ex::iterator top2);

	/// \ingroup pythoncore
	///
	/// Multiply two expressions, adding a top-level '\prod' node if required.
	Ex_ptr Ex_mul(const Ex_ptr ex1, const Ex_ptr ex2);
	Ex_ptr Ex_mul(const Ex_ptr ex1, const Ex_ptr ex2, Ex::iterator top2);

	/// \ingroup pythoncore
	///
	/// Subtract two expressions, adding a top-level '\sum' node if required.
	Ex_ptr Ex_sub(const Ex_ptr ex1, const ExNode ex2);
	Ex_ptr Ex_sub(const Ex_ptr ex1, const Ex_ptr ex2);
	Ex_ptr Ex_sub(const Ex_ptr ex1, const Ex_ptr ex2, Ex::iterator top2);

	/// \ingroup pythoncore
	///
	/// Fetch an Ex object from the Python side using its Python identifier.
	Ex_ptr fetch_from_python(const std::string& nm);
	Ex_ptr fetch_from_python(const std::string& nm, pybind11::object scope);

	/// \ingroup pythoncore
	///
	/// Generate the Python str() and repr() representation of the Ex object.
	std::string Ex_as_str(Ex_ptr);
	std::string Ex_as_repr(Ex_ptr);

	/// \ingroup pythoncore
	///
	/// The Python 'print' function always calls the 'str' member on
	/// objects to be printed. This one is required to produce output
	/// which looks readable but is also still valid input. In order to
	/// produce proper LaTeX output, this is therefore not the right
	/// function to use, because Cadabra only reads a restricted subset
	/// of LaTeX (for instance, we output spacing commands like '\,' but
	/// do not accept it on input).
	/// So we have a separate _latex_() member on each object, which
	///internally uses DisplayTeX to do the actual printing.
	std::string Ex_as_latex(Ex_ptr);

	/// \ingroup scalar
	///
	/// Outputs a Cadabra 'Ex' as a Sympy expression. This first converts the
	/// Cadabra expression to a string, and then reads that back in by calling
	/// sympy.parsing.sympy_parser.parse_expr. Is mapped to a '_sympy_()'
	/// function on each Ex object.
	/// When you feed an Ex object to a Sympy function, the Ex gets converted
	/// to a Sympy object in 'sympy.sympify' because the latter attempts to
	/// call __sympy__ on every object that you feed it.
	pybind11::object Ex_as_sympy(Ex_ptr);

	/// Similar to Ex_to_Sympy, but only producing a string which can be parsed
	/// by Sympy, instead of a full-fledged Sympy expression.
	std::string Ex_as_sympy_string(Ex_ptr);

	std::string Ex_as_input(Ex_ptr ex);

	std::string Ex_as_MMA(Ex_ptr ex, bool use_unicode);

	std::string Ex_as_tree(Ex *ex);

	cadabra::Ex lhs(Ex_ptr ex);
	cadabra::Ex rhs(Ex_ptr ex);

	Ex Ex_getslice(Ex_ptr ex, pybind11::slice slice);
	Ex Ex_getitem(Ex &ex, int index);
	void Ex_setitem(Ex_ptr ex, int index, Ex val);
	void Ex_setitem_iterator(Ex_ptr ex, ExNode en, Ex_ptr val);
	size_t Ex_len(Ex_ptr ex);
	std::string Ex_head(Ex_ptr ex);
	pybind11::object Ex_get_mult(Ex_ptr ex);

	// Split a 'sum' expression into its individual terms.
	// FIXME: now deprecated because we have operator[]?
	pybind11::list terms(Ex_ptr ex);

	Ex_ptr Ex_from_string(const std::string& in, bool make_ref = true, Kernel * kernel = nullptr);
	Ex_ptr Ex_from_int(int num, bool make_ref = true);

	Ex_ptr map_sympy_wrapper(Ex_ptr ex, std::string head, pybind11::args args);
#ifdef MATHEMATICA_FOUND
	Ex_ptr map_mma_wrapper(Ex_ptr ex, std::string head);
#endif

	void call_post_process(Kernel& kernel, Ex_ptr ex);

	void init_ex(pybind11::module& m);
	}
