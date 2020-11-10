#pragma once

#include <string>
#include <tuple>
#include <sstream>
#include <regex>
#include <memory>
#include <pybind11/pybind11.h>
#include "py_ex.hh"
#include "py_kernel.hh"
#include "py_tableau.hh"
#include "properties/LaTeXForm.hh"

namespace cadabra {
	class BoundPropertyBase
		: public std::enable_shared_from_this<BoundPropertyBase>
	{
	public:
		BoundPropertyBase();
		BoundPropertyBase(const property* prop, Ex_ptr for_obj);
		virtual ~BoundPropertyBase();

		/// Human-readable form in text, i.e. no special formatting.
		std::string str_() const;
		/// Human-readable form using LaTeX markup.
		std::string latex_() const;
		/// Python-parseable form. FIXME: not correct right now.
		std::string repr_() const;

		static Kernel& get_kernel();
		static Properties& get_props();
		Ex& get_ex() const;
		Ex::iterator get_it() const;

		// We keep a pointer to the C++ property, so it is possible to
		// query properties using the Python interface. However, this C++
		// object is owned by the C++ kernel and does not get destroyed
		// when the Python object goes out of scope.
		const property* prop;

		// We also keep a shared pointer to the expression for which we
		// have defined this property, so that we can print sensible
		// information.
		Ex_ptr for_obj;
	};


	template <typename PropT, typename... ParentTs>
	class BoundProperty
		: virtual public ParentTs...
	{
		public:
			using cpp_type = PropT;
			using py_type = pybind11::class_<BoundProperty, std::shared_ptr<BoundProperty>, ParentTs...>;

			// Default construct with null pointers
			BoundProperty();
			// Construct new property from expression and argument list
			BoundProperty(Ex_ptr ex, Ex_ptr param);
			// Construct from existing cpp property object
			BoundProperty(const PropT* prop, Ex_ptr for_obj);

			/// Human-readable form in text, i.e. no special formatting.
			std::string str_() const;
			/// Human-readable form using LaTeX markup.
			std::string latex_() const;
			/// Python-parseable form. FIXME: not correct right now.
			std::string repr_() const;
			/// Attach this property to a different symbol.
			void attach(Ex_ptr ex) const;
			
			// Get existing cpp property by querying kernel
			static std::shared_ptr<BoundProperty> get_from_kernel(Ex::iterator ex, const std::string& label, bool ignore_parent_rel);

			// Return type is not the same as BoundPropertyBase, but this is ok
			// by the standard as cpp_type* is convertible to property*
			const cpp_type* get_prop() const;
		};

	template <typename PropT> std::string get_name();

	template <typename BoundPropT> typename BoundPropT::py_type def_abstract_prop(pybind11::module& m);
	template <typename BoundPropT> typename BoundPropT::py_type def_prop(pybind11::module& m);

	pybind11::list list_properties();

	void init_properties(pybind11::module& m);

	}
