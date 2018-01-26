/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#pragma once

#include <pybind11/pybind11.h>
#include <stdexcept>
#include "Storage.hh"
#include "Kernel.hh"
#include "Algorithm.hh"



/// \ingroup pythoncore
///
/// Comparison operator for Ex objects in Python. Since comparison operators
/// need a Properties object, we cannot have such operator== things in C++,
/// but we can in Python since we can get the kernel in the current scope.

bool __eq__Ex_Ex(std::shared_ptr<cadabra::Ex>, std::shared_ptr<cadabra::Ex>);

/// \ingroup pythoncore
///
/// Comparison operator for Ex objects in Python. See __eq__Ex_Ex for more.

bool __eq__Ex_int(std::shared_ptr<cadabra::Ex>, int);

/// Fetch an Ex object from the Python side using its Python identifier.

std::shared_ptr<cadabra::Ex> fetch_from_python(const std::string& nm);

/// \ingroup pythoncore
///
/// Generate the Python str() and repr() representation of the Ex object.

std::string Ex_str_(std::shared_ptr<cadabra::Ex>);
std::string Ex_repr_(std::shared_ptr<cadabra::Ex>);

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

std::string Ex_latex_(std::shared_ptr<cadabra::Ex>);

/// \ingroup scalar
///
/// Outputs a Cadabra 'Ex' as a Sympy expression. This first converts the
/// Cadabra expression to a string, and then reads that back in by calling
/// sympy.parsing.sympy_parser.parse_expr. Is mapped to a '_sympy_()' 
/// function on each Ex object. 
/// When you feed an Ex object to a Sympy function, the Ex gets converted
/// to a Sympy object in 'sympy.sympify' because the latter attempts to 
/// call __sympy__ on every object that you feed it.

pybind11::object Ex_to_Sympy(std::shared_ptr<cadabra::Ex>);

/// Similar to Ex_to_Sympy, but only producing a string which can be parsed
/// by Sympy, instead of a full-fledged Sympy expression.

std::string Ex_to_Sympy_string(std::shared_ptr<cadabra::Ex>);


/// \ingroup pythoncore
///
/// Add two expressions, adding a top-level \sum node if required.

cadabra::Ex operator+(std::shared_ptr<cadabra::Ex> ex1, std::shared_ptr<cadabra::Ex> ex2);

/// \ingroup pythoncore
///
/// Subtract two expressions, adding a top-level \sum node if required.

cadabra::Ex operator-(std::shared_ptr<cadabra::Ex> ex1, std::shared_ptr<cadabra::Ex> ex2);

/// \ingroup pythoncore
///
/// Helper class to ensure that all Python property objects derive from the
/// same base class.

class BaseProperty : public std::enable_shared_from_this<BaseProperty> {
};

/// \ingroup pythoncore
///
/// Property is a templated wrapper around a C++ property object. It
/// provides it with _latex, __str__ and __repr__ methods. In order to
/// have a quick way to figure out in Python whether an object is a
/// property, we derive it from BaseProperty, which is an empty
/// placeholder (in Python BaseProperty is called Property).
///
/// Properties can have arguments. These are not parsed by Python, but
/// rather by the C++ side. There is a number of reasons for doing
/// things this way. The most important of them is that this makes it
/// a lot easier for Cadabra to provide useful feedback on parameters
/// which are not valid. So all properties get initialised with two
/// Cadabra Ex objects: the 1st is the pattern to which the property
/// should be attached, the 2nd is the argument of the property,
/// interpreted as a Cadabra expression.
///
/// Cadabra properties cannot be proper Python properties, because we
/// need to give the latter names in order to prevent them from going
/// out of scope. So Cadabra keeps a list of 'anonymous property objects 
/// in the current scope'. 
///
/// The question is now what we do when Python keeps a pointer to these
/// objects, and let that pointer escape local scope (e.g. by returning
/// the Python property object). How do we keep it in scope?

template<class T>
class Property : public std::enable_shared_from_this<Property<T>>, public BaseProperty {
	public:
		Property(std::shared_ptr<cadabra::Ex> obj, std::shared_ptr<cadabra::Ex> params=0);

		/// Human-readable form in text, i.e. no special formatting.
		std::string str_() const;

		/// Human-readable form using LaTeX markup.
		std::string latex_() const;

		/// Python-parseable form. FIXME: not correct right now.
		std::string repr_() const;


	private:
		// We keep a pointer to the C++ property, so it is possible to 
		// query properties using the Python interface. However, this C++
		// object is owned by the C++ kernel and does not get destroyed
		// when the Python object goes out of scope.

		// When the Python object survives the local scope, results are
		// undefined. 
		T *prop;

		// We also keep a shared pointer to the expression for which we
		// have defined this property, so that we can print sensible 
		// information.
		std::shared_ptr<cadabra::Ex> for_obj;
};


/// \ingroup pythoncore
///
/// Setup of kernels in current scope, callable from Python.
/// When the decision was made to graft Cadabra onto Python, a choice had
/// to be made about how Python variable scope would influence the
/// visibility of Cadabra properties. It clearly makes sense to be able to
/// declare properties which only hold inside a particular
/// function. However Cadabra expressions and properties do not directly
/// correspond to Python objects. Rather, declaring a property is more
/// like a function call into the Cadabra module, which leaves its imprint
/// on the state of the C++ part but does not change anything on the
/// Python side, as you typically do not assign the created property to a
/// Python symbol. Therefore, properties do not naturally inherit Python's
/// scoping rules.\footnote{This is different from e.g.~SymPy, in which
///   mathematical objects are always in one-to-one correspondence with a
///   Python object.} A more fundamental problem is that properties can be
/// attached to patterns, and those patterns can involve more than just
/// the symbols which one passes into a function.
/// 
/// In order to not burden the user, properties are therefore by default
/// global variables, stored in a single global Cadabra object
/// \verb|__cdbkernel__| which is initialised at import of the Cadabra module. 
/// If you add new properties inside a function scope, these will go
/// into this already existing \emph{global} property list by default.
/// If you want to create a local scope for your computations, create a
/// new \verb|__cdbkernel__| as in
/// \begin{verbatim}
/// def fun():
///    __cdbkernel__ = cadabra.create_scope();
///    [your code here]
/// \end{verbatim}
/// Now computations will not see the global properties at all. 
/// If you want to import the global properties, use instead
/// \begin{verbatim}
/// def fun():
///    __cdbkernel__ = cadabra.create_scope_from_global()
///    [your code here]
/// \end{verbatim}
/// It is crucial that the
/// \verb|__cdbkernel__| symbol is referenced from within Python and visible to the bytecompiler, because 
/// it is not possible to create new variables on the local stack at runtime.
/// Internally, the second version above fetches, at runtime, the
/// \verb|__cdbkernel__| from the globals stack, copies all properties in there
/// into a new kernel, and returns the latter. 
/// 
/// Both versions above do populate the newly created kernel with
/// Cadabra's default properties. If you want a completely clean slate
/// (for e.g.~testing purposes, or because you really do not want default
/// rules for sums and products), use
/// \begin{verbatim}
/// def fun():
///    __cdbkernel__ = cadabra.create_empty_scope()
///    [your code here]
/// \end{verbatim}
/// Note that in all these cases, changes to properties remain local and
/// do not leak into the global property list.
/// 
/// All Cadabra algorithms, when called from Python, will first look for a
/// kernel on the locals stack (i.e.~what \verb|locals()| produces). If
/// there is no kernel available locally, they will then revert to using
/// the global kernel. 

cadabra::Kernel *create_scope();
cadabra::Kernel *create_scope_from_global();
cadabra::Kernel *create_empty_scope();

/// \ingroup pythoncore
///
/// Inject properties directly into the Kernel, even if the kernel is not yet
/// on the Python stack (needed when we create a new local scope: in this case we
/// create the kernel and pass it back to be turned into local __cdbkernel__ by
/// Python, but we want to populate the kernel with defaults before we hand it
/// back).

void    inject_defaults(cadabra::Kernel *);

/// \ingroup pythoncore
///
/// Get a pointer to the currently visible kernel.
cadabra::Kernel *get_kernel_from_scope();

/// \ingroup pythoncore
///
/// Run the post-process Python function (if defined) on the given expression.

void call_post_process(cadabra::Kernel&, std::shared_ptr<cadabra::Ex> ex);
