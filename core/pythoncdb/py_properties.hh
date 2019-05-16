#pragma once

#include <string>
#include <sstream>
#include <regex>
#include <memory>
#include <pybind11/pybind11.h>
#include "py_ex.hh"
#include "py_kernel.hh"
#include "properties/LaTeXForm.hh"

namespace cadabra {
	/// \ingroup pythoncore
	///
	/// Helper class to ensure that all Python property objects derive from the
	/// same base class.
	class BoundPropertyBase {
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
	template <typename PropT, typename... ParentTs>
	class BoundProperty
		: public std::enable_shared_from_this<BoundProperty<PropT>>
		, public BoundPropertyBase
		, public BoundProperty<ParentTs>... {
		public:
			BoundProperty();
			BoundProperty(Ex_ptr obj, Ex_ptr params = 0);

			/// Human-readable form in text, i.e. no special formatting.
			std::string str_() const;

			/// Human-readable form using LaTeX markup.
			std::string latex_() const;

			/// Python-parseable form. FIXME: not correct right now.
			std::string repr_() const;

			static BoundProperty get_from_it(Ex::iterator ex, bool ignore_parent_rel);
			static BoundProperty get_from_ex(Ex_ptr ex, bool ignore_parent_rel);
			static BoundProperty get_from_exnode(ExNode exnode, bool ignore_parent_rel);

			// We keep a pointer to the C++ property, so it is possible to
			// query properties using the Python interface. However, this C++
			// object is owned by the C++ kernel and does not get destroyed
			// when the Python object goes out of scope.

			// When the Python object survives the local scope, results are
			// undefined.
			const PropT *prop;

			// We also keep a shared pointer to the expression for which we
			// have defined this property, so that we can print sensible
			// information.
			Ex_ptr for_obj;
		};

	template <typename PropT, typename... ParentTs>
	using PyProperty = pybind11::class_<
		BoundProperty<PropT>, // C++ type
		std::shared_ptr<BoundProperty<PropT>>, // Holder type
		BoundPropertyBase, BoundProperty<ParentTs>... // Parent classes
	>;

	template <typename PropT, typename... ParentTs>
	PyProperty<PropT, ParentTs...> def_prop(pybind11::module& m, const char* docstring = "");

	pybind11::list list_properties();

	void init_properties(pybind11::module& m);

	}
