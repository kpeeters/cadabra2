
#include "PythonCdb.hh"
#include "Parser.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "DisplayTeX.hh"
#include "Cleanup.hh"
#include "PreClean.hh"
#include "PythonException.hh"

#include <boost/python/implicit.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/python.hpp>
#include <boost/python.hpp>
#include <boost/mpl/vector.hpp>
#include <sstream>
#include <memory>

#include "properties/Accent.hh"
#include "properties/AntiCommuting.hh"
#include "properties/Commuting.hh"
#include "properties/Derivative.hh"
#include "properties/GammaMatrix.hh"
#include "properties/NonCommuting.hh"
#include "properties/AntiSymmetric.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/Distributable.hh"
#include "properties/Indices.hh"
#include "properties/IndexInherit.hh"
#include "properties/SelfAntiCommuting.hh"
#include "properties/SortOrder.hh"
#include "properties/Spinor.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"

#include "algorithms/collect_terms.hh"
#include "algorithms/distribute.hh"
#include "algorithms/reduce_sub.hh"
#include "algorithms/rename_dummies.hh"
#include "algorithms/substitute.hh"

Kernel *get_kernel_from_scope(bool for_write=false) ;

// TODO: 
//
// - When we stick objects into the locals or globals, does Python become the owner and manage them?
//   where do the kernel copy constructor calls come from?
//
// - We do not use can_apply yet?
//
// - Make a list of useful things to pass to functions which are not Ex objects. 
//   Then abstract a new def_algo from there.
//
//        keep_terms:  list of integers
//        

class X {
	public:
		X() {};
		X(const X& other) {};
		std::string repr() { return "hi"; };
};


bool output_ipython=false;

Ex::Ex(const Ex& other)
	{
//	std::cout << "Ex copy constructor" << std::endl;
	}

Ex::Ex(int val) 
	{
	tree.set_head(str_node("\\expression"));
	exptree::iterator it = tree.append_child(tree.begin(), str_node("1"));
	multiply(it->multiplier, val);
	}

Ex::~Ex()
	{
//	std::cerr << "~Ex";
//	if(ex.begin()!=ex.end()) {
//		std::cerr << " " << *(tree.begin()->name);
//		}
//	std::cerr << std::endl;
	}

// Output routines for Ex objects.

std::string Ex::str_() const
	{
	std::ostringstream str;

	DisplayTeX dt(get_kernel_from_scope()->properties, tree);
	dt.output(str);

	return str.str();
	}

std::string Ex::repr_() const
	{
	exptree::iterator it = tree.begin();
	std::ostringstream str;
	tree.print_entire_tree(str);
	return str.str();
	}


std::string Ex::_repr_latex_() const
	{
	std::ostringstream str;

	DisplayTeX dt(get_kernel_from_scope()->properties, tree);
	dt.output(str);

	return "$"+str.str()+"$";
	}

std::string Ex::_repr_html_() const
	{
	return "<strong>hi</strong>";
	}

Ex::Ex(std::string ex_) 
//	: ex(ex_)
	{
	Parser parser;
	std::stringstream str(ex_);

//	std::cerr << ex_ << std::endl;

	try {
		str >> parser;
		tree=parser.tree;
		}
	catch(std::exception& except) {
		throw ParseException("Cannot parse");
		}

	// First pull in any expressions referred to with @(...) notation, because the
	// full expression may not have consistent indices otherwise.
	pull_in();

	// Basic cleanup of rationals and subtractions, followed by
   // cleanup of nested sums and products.
	pre_clean(*get_kernel_from_scope(), tree, tree.begin());
	exptree::iterator top = tree.begin();
	cleanup_nests_below(tree, top);
	}

std::shared_ptr<Ex> Ex::fetch_from_python(const std::string nm)
	{
	try {
		boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
		boost::python::object obj = locals[nm];

		if(obj.is_none()) // We never actually get here, an exception will have been thrown.
			std::cout << "object unknown" << std::endl;
		else {
			// We can include this Python object into the expression only if it is an Ex object.
			boost::python::extract<std::shared_ptr<Ex> > extract_Ex(obj);
			if(extract_Ex.check()) {
				std::shared_ptr<Ex> ex = extract_Ex();
				return ex;
				}
			// getting python objects included is tricky because of memory management of the generated Ex.
			//			boost::python::extract<int> extract_int(obj);
			//			if(extract_int.check()) {
			//				int x = extract_int();
			//				std::ostringstream str;
			//				str << "Ex('" << x << "')" << std::ends;
			//				std::cout << "recursive for " << str.str() << std::endl;
			//				return fetch_from_python(str.str());
			//				}
			std::cout << nm << " is not of type cadabra.Ex" << std::endl;
			return 0;
			}
		}
	catch(boost::python::error_already_set const &) {
		// In order to prevent the error from propagating, we have to read
		// it out. And in any case, we want to give some feedback to the user.
		std::string err = parse_python_exception();
		if(err.substr(0,29)=="<type 'exceptions.TypeError'>")
			std::cout << nm << " is not of type cadabra.Ex." << std::endl;
		else 
			std::cout << nm << " is not defined." << std::endl;
      //		std::cout << parse_python_exception() << std::endl;
		}

	return 0;
	}

// Replace any objects of the form '@(...)' in the expression tree by the
// python expression '...' if it exists. Rename dummies to avoid clashes.

void Ex::pull_in()
	{
	ratrewrite rr(*get_kernel_from_scope(), tree);
	
	exptree::iterator it=tree.begin();
	while(it!=tree.end()) {
		if(*it->name=="@") {
			if(tree.number_of_children(it)==1) {
				std::string pobj = *(tree.begin(it)->name);
				std::shared_ptr<Ex> ex = fetch_from_python(pobj);
				if(ex) {
					// The top node is an \expression, so we need the first child of that.
					// FIMXE: assert consistency.
					exptree::iterator expression_it = ex->tree.begin();
					exptree::iterator topnode_it    = ex->tree.begin(expression_it);

					multiplier_t mult=*(it->multiplier);
					it=tree.replace(it, topnode_it);
					multiply(it->multiplier, mult);
					rr.rename_replacement_dummies(it, false);
					}
				}
			}
		++it;
		}

	return;
	}


bool Ex::operator==(const Ex& other) const
	{
	return tree_equal(&(get_kernel_from_scope()->properties), tree, other.tree);
	}

bool Ex::__eq__int(int other) const
	{
	Ex ex(other);
	return (*this)==ex;
	}

// Functions to construct an Ex object and then create an additional
// reference '_' on the local python stack that points to this object
// as well.

std::shared_ptr<Ex> make_Ex_from_string(const std::string& str) 
	{
	auto ptr = std::make_shared<Ex>(str);
	// FIXME: very weird things happen if we store a ref on the locals
	// stack.  It looks like a new frame is being created, or something
	// like that.  Creating on the globals stack is not right, but the
	// best we can do right now.

//	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	boost::python::object locals(boost::python::borrowed(PyEval_GetGlobals()));
	locals["_"]=ptr;
	return ptr;
	}

std::shared_ptr<Ex> make_Ex_from_int(int num)
	{
	auto ptr = std::make_shared<Ex>(num);
//	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	boost::python::object locals(boost::python::borrowed(PyEval_GetGlobals()));
	locals["_"]=ptr;
	return ptr;
	}

// Templates to dispatch function calls in Python to algorithms in C++.

template<class F>
Ex *dispatch_1(Ex *ex, bool repeat)
	{
	F algo(*get_kernel_from_scope(), ex->tree);

	exptree::iterator it=ex->tree.begin().begin();
//	std::cout << "applying at:";
//	ex->tree.print_recursive_treeform(std::cout, it);
	if(repeat) {
		if(algo.apply_recursive(it)==false)
			std::cout << "no change" << std::endl;
		}
	else {
		if(algo.apply_once(it)==false)
			std::cout << "no change" << std::endl;
		}

	return ex;
	}

template<class F>
Ex *dispatch_1_string(const std::string& ex, bool repeat)
	{
	Ex *exobj = new Ex(ex);
	return dispatch_1<F>(exobj, repeat);
	}

template<class F>
Ex *dispatch_2(Ex *ex, Ex *args, bool repeat)
	{
	F algo(*get_kernel_from_scope(), ex->tree, args->tree);

	exptree::iterator it=ex->tree.begin().begin();
	if(repeat) {
		if(algo.apply_recursive(it)==false)
			std::cout << "no change" << std::endl;
		}
	else {
		if(algo.apply_once(it)==false)
			std::cout << "no change" << std::endl;
		}
	return ex;
	}

template<class F>
Ex *dispatch_2_string(Ex *ex, const std::string& args, bool repeat)
	{
	Ex *argsobj = new Ex(args);
	return dispatch_2<F>(ex, argsobj, repeat);
	}

std::string print_status()
	{
//	Kernel *kernel = get_kernel_from_scope();
	return "hi";
	}

std::string init_ipython()
	{
	boost::python::object obj = boost::python::exec("from IPython.display import Math");
	return "Cadabra typeset output for IPython notebook initialised.";
	}

boost::python::list list_properties()
	{
	Kernel *kernel=get_kernel_from_scope(false);
	Properties& props=kernel->properties;

	boost::python::list ret;
	std::string res;
	bool multi=false;
	for(auto it=props.pats.begin(); it!=props.pats.end(); ++it) {
		// print the property name if we are at the end or if the next entry is for
		// a different property.
		decltype(it) nxt=it;
		++nxt;
		if(res=="" && (nxt!=props.pats.end() && it->first==nxt->first)) {
			res+="{";
			multi=true;
			}


		DisplayTeX dt(get_kernel_from_scope()->properties, it->second->obj);
		std::ostringstream str;
		dt.output(str);
		res += str.str(); //*((*it).second->obj.begin()->name);

		if(nxt==props.pats.end() || it->first!=nxt->first) {
			if(multi)
				res+="}";
			multi=false;
			res += "::";
			res +=(*it).first->name();
			ret.append(res);
			res="";
			}
		else {
			res+=", ";
			}
		}

	return ret;
	}

std::string print_tree(Ex *ex)
	{
	std::ostringstream str;
	ex->tree.print_entire_tree(str);
	return str.str();
	}
 
PyObject *ParseExceptionType = NULL;
PyObject *ArgumentExceptionType = NULL;

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}

void translate_ArgumentException(const ArgumentException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ArgumentExceptionType, pythonExceptionInstance.ptr());
	}

// Return the kernel in local scope if any, or the one in global scope if none is available
// in local scope. However, if a request for a writeable kernel is made, and no local is
// present, it will always be created (and filled with the content of the global one).

Kernel *get_kernel_from_scope(bool for_write) 
	{
//	std::cerr << "get_kernel_from_scope " << (for_write?"for writing":"for reading") << std::endl;

	// update locals
//	boost::python::object __cdbkernel__ = boost::python::eval("cadabra_kernel");

	// Lookup the properties object in the local scope. 
	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
	Kernel *local_kernel=0;
	Kernel *global_kernel=0;
	try {
		boost::python::object obj = locals["cadabra_kernel"];
		local_kernel = boost::python::extract<Kernel *>(obj);
//		std::cerr << "local kernel = " << local_kernel << std::endl;
		}
	catch(boost::python::error_already_set& err) {
		std::string err2 = parse_python_exception();
		local_kernel=0;
		}
	try {
		boost::python::object obj = globals["cadabra_kernel"];
		global_kernel = boost::python::extract<Kernel *>(obj);
//		std::cerr << "global kernel = " << global_kernel << std::endl;
		}
	catch(boost::python::error_already_set& err) {
		std::string err2 = parse_python_exception();
		global_kernel=0;
		}
	
	if(for_write) {
		// Need a local kernel because we want to write into it. Since 
		// the C++ only handles one kernel, we copy the content of the
		// global kernel into the current local one.
		if(local_kernel==0) {
			local_kernel = new Kernel();
//			std::cerr << "creating new local kernel " << local_kernel << std::endl;
			if(global_kernel) {
//				std::cerr << "copying global properties into kernel" << std::endl;
				local_kernel->properties = global_kernel->properties; 
				}
			locals["cadabra_kernel"]=boost::ref(local_kernel);
			}
		return local_kernel;
		}
	else {
		if(local_kernel) {
			return local_kernel;
			}
		else if(global_kernel) {
			return global_kernel;
			}
		else {
			// At the start of a program there is no global kernel yet,
			// so it has to be created.
			// std::cerr << "creating new global kernel" << std::endl;
			global_kernel = new Kernel();
			globals["cadabra_kernel"]=boost::ref(global_kernel);
			return global_kernel;
			}
		}
	}

template<class Prop>
Property<Prop>::Property(std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param) 
	{
	exptree::iterator it=ex->tree.begin();
	assert(*(it->name)=="\\expression");
	it=ex->tree.begin(it);

	Kernel *kernel=get_kernel_from_scope(true);

	prop=new Prop();
	if(param) {
		keyval_t keyvals;
		prop->parse_to_keyvals(param->tree, keyvals);
		prop->parse(keyvals);
		}
	kernel->properties.master_insert(exptree(it), prop);
	}

template<class Prop>
std::string Property<Prop>::str_() const
	{
	return prop->name();
	}

template<class Prop>
std::string Property<Prop>::repr_() const
	{
	return "Property::repr: "+prop->name();
	}

// Templated function which declares various forms of the algorithm entry points in one shot.
// First the ones with no argument, just a repeat flag.

template<class F>
void def_algo_1(const std::string& name) 
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_1<F>,        (arg("ex"),arg("repeat")=true), 
		 return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_1_string<F>, (arg("ex"),arg("repeat")=true), 
		 return_value_policy<manage_new_object>() );
	}

template<class F>
void def_algo_2(const std::string& name) 
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_2<F>,        (arg("ex"),arg("args"),arg("repeat")=true), 
		 return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_2_string<F>, (arg("ex"),arg("args"),arg("repeat")=true), 
		 return_value_policy<manage_new_object>() );
	}

// Declare a property. These take one Ex to which they will be attached, and
// one optional additional Ex which is a list of parameters. The latter are thus always
// Cadabra expressions, and cannot easily contain Python constructions (for the time
// being this follows most closely the setup we had in Cadabra v1; if the need arises
// we can make this more complicated later). 

template<class P>
void def_prop(const std::string& name)
	{
	using namespace boost::python;

	class_<Property<P>, bases<BaseProperty> > pr(name.c_str(), init<std::shared_ptr<Ex>, optional<std::shared_ptr<Ex> > >());
	pr.def("__str__", &Property<P>::str_).def("__repr__", &Property<P>::repr_);
	}

// Experimental new way to define algorithms; intended to do something with algorithms which require
// more arguments.

template<class F, class p1>
void def_algo_new(const std::string& name)
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_2<F, p1>,        (arg("ex"),arg("args"),arg("repeat")=true), return_internal_reference<1>() );
//	def(name.c_str(),  &dispatch_2_string<F, p1>, (arg("ex"),arg("args"),arg("repeat")=true), return_value_policy<manage_new_object>() );
	
//	boost::python::list& ns
	}


// Entry point for registration of the Cadabra Python module. 
// This registers the main Ex class which wraps Cadabra expressions, as well
// as the various algorithms that can act on these and the properties that can
// be attached to Cadabra patterns.
// http://stackoverflow.com/questions/6050996/boost-python-overloaded-functions-with-default-arguments-problem

BOOST_PYTHON_MODULE(cadabra2)
	{
	using namespace boost::python;

	class_<ParseException> pyParseException("ParseException", init<std::string>());
	ParseExceptionType=pyParseException.ptr();

	// Declare the Kernel object for Python so we can store it in the local Python context.
	class_<Kernel> pyKernel("Kernel", init<>());

	// Test class
	class_<X> pyX("X", init<>());
	pyX.def("__repr__", &X::repr);

	// Declare the Ex object to store expressions and manipulate on the Python side.
	class_<Ex, std::shared_ptr<Ex> > pyEx("Ex", boost::python::no_init);
	pyEx.def("__init__", boost::python::make_constructor(&make_Ex_from_string));
	pyEx.def("__init__", boost::python::make_constructor(&make_Ex_from_int));
	pyEx.def("__str__",  &Ex::str_)
		.def("_repr_latex_", &Ex::_repr_latex_)
		.def("_repr_html_", &Ex::_repr_html_)
		.def("__repr__", &Ex::repr_)
		.def("__eq__",   &Ex::operator==)
		.def("__eq__",   &Ex::__eq__int);


	// Inspection algorithms and other global functions which do not fit into the C++
   // framework anymore.

	def("tree", &print_tree);
	def("cdb", &print_status);
	def("init_ipython", &init_ipython);
	def("properties", &list_properties);

	// You cannot use implicitly_convertible to convert a string parameter to an Ex object
	// automatically: think about how that would work in C++. You would need to be able to
	// pass a 'std::string' to a function that expects an 'Ex *'. That will never work.

	// Algorithms with only the Ex as argument.
	def_algo_1<collect_terms>("collect_terms");
	def_algo_1<distribute>("distribute");
	def_algo_1<rename_dummies>("rename_dummies");
	def_algo_1<reduce_sub>("reduce_sub");
	def_algo_1<sort_product>("sort_product");

	// Algorithms which take a second Ex as argument.
	def_algo_2<substitute>("substitute");
   //	def_algo_new<keep_terms, boost::python::list>("keep_terms");


	// Properties are declared as objects on the Python side as well. They all take two
	// Ex objects as constructor parameters: the first one is the object(s) to which the
	// property is attached, the second one is the argument list (represented as an Ex).
	// 
	// It might have been more logical to make property declarations through a function,
	// so that we have Indices(ex=Ex('{a,b,c,}'), name='vector') being a function that takes
   // one expression and a number of arguments. From the Python point of view this would be
   // added to a map in the cadabra2.Kernel object. However, keeping control over an object 
	// also has the advantage that we can refer to it again if necessary from the Python side,
	// and keeps C++ and Python more in sync.

	class_<BaseProperty>("Property", no_init);

	def_prop<Accent>("Accent");
	def_prop<AntiCommuting>("AntiCommuting");
	def_prop<AntiSymmetric>("AntiSymmetric");
	def_prop<Commuting>("Commuting");
	def_prop<CommutingAsProduct>("CommutingAsProduct");
	def_prop<CommutingAsSum>("CommutingAsSum");
	def_prop<Derivative>("Derivative");
	def_prop<Distributable>("Distributable");
	def_prop<GammaMatrix>("GammaMatrix");
	def_prop<IndexInherit>("IndexInherit");
	def_prop<Indices>("Indices");
	def_prop<NonCommuting>("NonCommuting");
	def_prop<SelfAntiCommuting>("SelfAntiCommuting");
	def_prop<SortOrder>("SortOrder");
	def_prop<Spinor>("Spinor");
	def_prop<Weight>("Weight");
	def_prop<WeightInherit>("WeightInherit");

	register_exception_translator<ParseException>(&translate_ParseException);
	register_exception_translator<ArgumentException>(&translate_ArgumentException);

	// How can we give Python access to information stored in properties?
	}
