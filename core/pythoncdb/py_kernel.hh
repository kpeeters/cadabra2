#pragma once

#include <pybind11/pybind11.h>

#include "../Kernel.hh"

namespace cadabra
	{

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
	Kernel *create_scope();
	Kernel *create_scope_from_global();
	Kernel *create_empty_scope();

	/// \ingroup pythoncore
	///
	/// Inject properties directly into the Kernel, even if the kernel is not yet
	/// on the Python stack (needed when we create a new local scope: in this case we
	/// create the kernel and pass it back to be turned into local __cdbkernel__ by
	/// Python, but we want to populate the kernel with defaults before we hand it
	/// back).
	void inject_defaults(Kernel*);

	/// \ingroup pythoncore
	///
	/// Get a pointer to the currently visible kernel.
	Kernel *get_kernel_from_scope();


	void init_kernel(pybind11::module& m);

	}