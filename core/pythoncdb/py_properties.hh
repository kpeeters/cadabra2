#pragma once

#include <string>
#include <sstream>
#include <regex>
#include <memory>
#include <pybind11/pybind11.h>
#include "py_ex.hh"
#include "py_kernel.hh"
#include "properties/LaTeXForm.hh"

namespace cadabra
	{
	/// \ingroup pythoncore
	///
	/// Helper class to ensure that all Python property objects derive from the
	/// same base class.
	class BaseProperty
		{
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
	class Property : public std::enable_shared_from_this<Property<T>>, public BaseProperty
		{
		public:
			Property(std::shared_ptr<cadabra::Ex> obj, std::shared_ptr<cadabra::Ex> params = 0);

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


	template<class Prop>
	Property<Prop>::Property(std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param)
		{
		for_obj = ex;
		Kernel *kernel = get_kernel_from_scope();
		prop = new Prop(); // we keep a pointer, but the kernel owns it.
						   //	std::cerr << "Declaring property " << prop->name() << " in kernel " << kernel << std::endl;
		kernel->inject_property(prop, ex, param);
		}

	template<class Prop>
	std::string Property<Prop>::str_() const
		{
		std::ostringstream str;
		str << "Attached property ";
		prop->latex(str); // FIXME: this should call 'str' on the property, which does not exist yet
		str << " to " + Ex_as_str(for_obj) + ".";
		return str.str();
		}

	template<class Prop>
	std::string Property<Prop>::latex_() const
		{
		std::ostringstream str;

		//	HERE: this text should go away, property should just print itself in a python form,
		//   the decorating text should be printed in a separate place.

		str << "\\text{Attached property ";
		prop->latex(str);
		std::string bare = Ex_as_latex(for_obj);
		str << " to~}" + bare + ".";
		return str.str();
		}

	template<>
	std::string Property<LaTeXForm>::latex_() const;

	template<class Prop>
	std::string Property<Prop>::repr_() const
		{
		// FIXME: this needs work, it does not output things which can be fed back into python.
		return "Property::repr: " + prop->name();
		}

	template<class P>
	void def_prop(pybind11::module& m, const char* docstring = "")
		{
		using namespace pybind11;

		class_<Property<P>, std::shared_ptr<Property<P>>, BaseProperty>(m, std::make_shared<P>()->name().c_str())
			.def(
				init<std::shared_ptr<Ex>, std::shared_ptr<Ex>>(),
				arg("ex"),
				arg("param"),
				docstring
			)
			.def("__str__", &Property<P>::str_)
			.def("__repr__", &Property<P>::repr_)
			.def("_latex_", &Property<P>::latex_);
		}


	pybind11::list list_properties();


	void init_properties(pybind11::module& m);

	}
