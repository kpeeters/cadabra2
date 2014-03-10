#include <boost/python.hpp>

/// @brief Mockup types.
struct X {};
struct Y {};
struct Z {
		Z();
};

/// @brief Auxiliary function that will create X and inject an Y object
///        as 'y' into the caller's frame.
X* make_x_and_inject_y()
{
  // Boost.Python objects may throw, so use a smart pointer that can
  // release ownership to manage memory.
  std::auto_ptr<X> x(new X());

  // Borrow a reference from the locals dictionary to create a handle.
  // If PyEval_GetLocals() returns NULL, then Boost.Python will throw.
  namespace python = boost::python;
  python::object locals(python::borrowed(PyEval_GetLocals()));

  // Inject an instance of Y into the frame's locals as variable 'y'.
  // Boost.Python will handle the conversion of C++ Y to Python Y.
  locals["y"] = Y();

  // Transfer ownership of X to Boost.Python.
  return x.release();
}

Z::Z()
	{
	namespace python = boost::python;
	python::object locals(python::borrowed(PyEval_GetLocals()));
	locals["y"] = Y();
	locals["fik"]=42;
	}

void fun()
	{
	namespace python = boost::python;
	python::object locals(python::borrowed(PyEval_GetLocals()));
	locals["y"] = Y();
	locals["fik"]=42;
	locals["_fik"]=43;
	}


BOOST_PYTHON_MODULE(example)
{
  namespace python = boost::python;

  // Expose X, explicitly suppressing Boost.Python from creating a
  // default constructor, and instead exposing a custom constructor.
  python::class_<X>("X", python::no_init)
    .def("__init__", python::make_constructor(&make_x_and_inject_y))
    ;
  python::class_<Y>("Y", python::init<>());

  python::class_<Z>("Z", python::init<>());

  python::def("fun", &fun);
}
