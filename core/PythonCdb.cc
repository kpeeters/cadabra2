
#include "PythonCdb.hh"
#include "Parser.hh"
#include "Exceptions.hh"
#include "DisplayTeX.hh"
#include "Cleanup.hh"
#include "PreClean.hh"
#include "PythonException.hh"

#include <boost/python/implicit.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/python.hpp>
#include <boost/python.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/def.hpp>
#include <boost/python/module.hpp>

#include <sstream>
#include <memory>

// Properties.

#include "properties/Accent.hh"
#include "properties/AntiCommuting.hh"
#include "properties/AntiSymmetric.hh"
#include "properties/Commuting.hh"
#include "properties/Coordinate.hh"
#include "properties/Depends.hh"
#include "properties/Derivative.hh"
#include "properties/DiracBar.hh"
#include "properties/GammaMatrix.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/DAntiSymmetric.hh"
#include "properties/Diagonal.hh"
#include "properties/Distributable.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/Indices.hh"
#include "properties/IndexInherit.hh"
#include "properties/Integer.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/Metric.hh"
#include "properties/NonCommuting.hh"
#include "properties/PartialDerivative.hh"
#include "properties/RiemannTensor.hh"
#include "properties/SatisfiesBianchi.hh"
#include "properties/SelfAntiCommuting.hh"
#include "properties/SelfCommuting.hh"
#include "properties/SortOrder.hh"
#include "properties/Spinor.hh"
#include "properties/Symmetric.hh"
#include "properties/TableauSymmetry.hh"
#include "properties/Traceless.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"
#include "properties/WeylTensor.hh"

// Algorithms.

#include "algorithms/canonicalise.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/distribute.hh"
#include "algorithms/eliminate_kronecker.hh"
#include "algorithms/indexsort.hh"
#include "algorithms/join_gamma.hh"
#include "algorithms/product_rule.hh"
#include "algorithms/rename_dummies.hh"
#include "algorithms/split_index.hh"
#include "algorithms/substitute.hh"
#include "algorithms/unwrap.hh"
#include "algorithms/young_project_tensor.hh"

// Helper algorithms, not for users.

#include "algorithms/reduce_sub.hh"


// TODO: 
//
// - Make a list of useful things to pass to functions which are not Ex objects. 
//   Then abstract a new def_algo from there.
//
//        keep_terms:  list of integers
//        

bool output_ipython=false;

// Expression constructor/destructor members.

Ex::Ex(const Ex& other)
	: state_(Algorithm::result_t::l_no_action)
	{
   //	std::cout << "Ex copy constructor" << std::endl;
	}

Ex::Ex(int val) 
	: state_(Algorithm::result_t::l_no_action)
	{
	tree.set_head(str_node("\\expression"));
	exptree::iterator it = tree.append_child(tree.begin(), str_node("1"));
	multiply(it->multiplier, val);
	}

Ex::~Ex()
	{
	}

// Output routines in display, input, latex and html formats (the latter two
// for use with IPython).

std::string Ex::str_() const
	{
	std::ostringstream str;

//	if(state()==Algorithm::result_t::l_no_action)
//		str << "(unchanged)" << std::endl;
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
	: state_(Algorithm::result_t::l_no_action)
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
	cleanup_dispatch_deep(*get_kernel_from_scope(), tree);
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

Algorithm::result_t Ex::state() const
	{
	return state_;
	}

void Ex::update_state(Algorithm::result_t newstate)
	{
	switch(newstate) {
		case Algorithm::result_t::l_error:
			state_=newstate;
			break;
		case Algorithm::result_t::l_applied:
			if(state_!=Algorithm::result_t::l_error)
				state_=newstate;
			break;
		default:
			break;
		}
	}

void Ex::reset_state() 
	{
	state_=Algorithm::result_t::l_no_action;
	}

// Functions to construct an Ex object and then create an additional
// reference '_' on the global python stack that points to this object
// as well.

std::shared_ptr<Ex> make_Ex_from_string(const std::string& str) 
	{
	auto ptr = std::make_shared<Ex>(str);

	// The local variable stack is not writeable so we cannot insert '_'
	// as a local variable. Instead, we push it onto the global stack.

	boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
	globals["_"]=ptr;
	return ptr;
	}

std::shared_ptr<Ex> make_Ex_from_int(int num)
	{
	auto ptr = std::make_shared<Ex>(num);
	boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
	globals["_"]=ptr;
	return ptr;
	}


// Initialise mathematics typesetting for IPython.

std::string init_ipython()
	{
	boost::python::object obj = boost::python::exec("from IPython.display import Math");
	return "Cadabra typeset output for IPython notebook initialised.";
	}

// Generate a Python list of all properties declared in the current scope. These will
// (FIXME: should be) displayed in input form, i.e. they can be fed back into Python.
// FIXME: most of this has nothing to do with Python, factor back into core.

boost::python::list list_properties()
	{
//	std::cout << "listing properties" << std::endl;
	Kernel *kernel=get_kernel_from_scope();
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
		res += str.str();

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

// Debug function to display an expression in tree form.

std::string print_tree(Ex *ex)
	{
	std::ostringstream str;
	ex->tree.print_entire_tree(str);
	return str.str();
	}

// Setup logic to pass C++ exceptions down to Python properly.
 
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

// Return the kernel (with symbol __cdbkernel__) in local scope if
// any, or the one in global scope if none is available in local
// scope.

Kernel *get_kernel_from_scope()
	{
	// Lookup the kernel in the local/global scope. 
	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	Kernel *local_kernel=0;
	try {
		boost::python::object obj = locals["__cdbkernel__"];
		local_kernel = boost::python::extract<Kernel *>(obj);
		}
	catch(boost::python::error_already_set& err) {
		std::string err2 = parse_python_exception();
		local_kernel=0;
		}
	if(local_kernel!=0)  {
		return local_kernel;
		}

	// If there is no kernel in local scope, find one in global scope.

 	boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
	Kernel *global_kernel=0;
	try {
		boost::python::object obj = globals["__cdbkernel__"];
		global_kernel = boost::python::extract<Kernel *>(obj);
		}
	catch(boost::python::error_already_set& err) {
		std::string err2 = parse_python_exception();
		global_kernel=0;
		}
	
	if(global_kernel!=0) {
		return global_kernel;
		}

	// If there is no kernel in global scope either, construct one.

	global_kernel = new Kernel();

	// Store this as a Python object, making sure (using boost::ref) that the
	// kernel Python refers to by __cdbkernel__ is the same object as the one
	// we will return to our caller.
	globals["__cdbkernel__"]=boost::ref(global_kernel);
	inject_defaults(global_kernel);

	return global_kernel;
	}

// The following handle setup of local scope for Cadabra properties.

Kernel *create_scope()
	{
	Kernel *k=create_empty_scope();
	inject_defaults(k);
	return k;
	}

Kernel *create_scope_from_global()
	{
	Kernel *k=create_empty_scope();
	// FIXME: copy global properties
	return k;
	}

Kernel *create_empty_scope()
	{
	Kernel *k = new Kernel();
	return k;
	}

// Inject properties directly into the Kernel, even if the kernel is not yet
// on the Python stack (needed when we create a new local scope: in this case we
// create the kernel and pass it back to be turned into local __cdbkernel__ by
// Python, but we want to populate the kernel with defaults before we hand it
// back).

void inject_defaults(Kernel *k)
	{
	// Create and inject properties; these then get owned by the kernel.
	inject_property(k, new Distributable(),      std::make_shared<Ex>("\\prod{#}"), 0);
	inject_property(k, new IndexInherit(),       std::make_shared<Ex>("\\prod{#}"), 0);
	inject_property(k, new CommutingAsProduct(), std::make_shared<Ex>("\\prod{#}"), 0);
	inject_property(k, new CommutingAsSum(),     std::make_shared<Ex>("\\sum{#}"), 0);
	}

void inject_property(Kernel *kernel, property *prop, std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param)
	{
	exptree::iterator it=ex->tree.begin();
	assert(*(it->name)=="\\expression");
	it=ex->tree.begin(it);

	if(param) {
		keyval_t keyvals;
		prop->parse_to_keyvals(param->tree, keyvals);
		prop->parse(kernel->properties, keyvals);
		}
	prop->validate(kernel->properties, exptree(it));
	kernel->properties.master_insert(exptree(it), prop);
	}

// Property constructor and display members for Python purposes.

template<class Prop>
Property<Prop>::Property(std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param) 
	{
	Kernel *kernel=get_kernel_from_scope();
	prop = new Prop(); // we keep a pointer, but the kernel owns it.
	inject_property(kernel, prop, ex, param);
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

// Templates to dispatch function calls in Python to algorithms in
// C++.  The number attached to each name indicates the number of Ex
// arguments that the algorithm takes. All algorithms take the 'deep'
// and 'repeat' boolean arguments, plus an arbitrary list of
// additional boolean arguments indicated by 'args' (see the
// declaration of 'join_gamma' below for an example of how to declare
// those additional arguments).

template<class F, typename... Args>
Ex *dispatch_1_ex(Ex *ex, bool deep, bool repeat, Args... args)
	{
	F algo(*get_kernel_from_scope(), ex->tree, args...);

	exptree::iterator it=ex->tree.begin().begin();

	ex->reset_state();
	ex->update_state(algo.apply_generic(it, deep, repeat));
	
	return ex;
	}

template<class F>
Ex *dispatch_1_string(const std::string& ex, bool deep, bool repeat)
	{
	Ex *exobj = new Ex(ex);
	return dispatch_1_ex<F>(exobj, deep, repeat);
	}

template<class F>
Ex *dispatch_2_ex_ex(Ex *ex, Ex *args, bool deep, bool repeat)
	{
	F algo(*get_kernel_from_scope(), ex->tree, args->tree);

	exptree::iterator it=ex->tree.begin().begin();

	ex->reset_state();
	ex->update_state(algo.apply_generic(it, deep, repeat));
	
	return ex;
	}

template<class F>
Ex *dispatch_2_ex_string(Ex *ex, const std::string& args, bool deep, bool repeat)
	{
	Ex *argsobj = new Ex(args);
	return dispatch_2_ex_ex<F>(ex, argsobj, deep, repeat);
	}

template<class F>
Ex *dispatch_2_string_string(const std::string& ex, const std::string& args, bool deep, bool repeat)
	{
	Ex *exobj   = new Ex(ex);
	Ex *argsobj = new Ex(args);
	return dispatch_2_ex_ex<F>(exobj, argsobj, deep, repeat);
	}

// Templated function which declares various forms of the algorithm entry points in one shot.
// First the ones with no argument, just a deep flag.

template<class F>
void def_algo_1(const std::string& name) 
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_1_ex<F>,        (arg("ex"),arg("deep")=true,arg("repeat")=false), 
		 return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_1_string<F>, (arg("ex"),arg("deep")=true,arg("repeat")=false), 
		 return_value_policy<manage_new_object>() );
	}

// Then the ones which take an additional Ex argument (e.g. substitute).

template<class F>
void def_algo_2(const std::string& name) 
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_2_ex_ex<F>,     (arg("ex"),arg("args"),arg("deep")=true,arg("repeat")=false), 
		 return_internal_reference<1>() );
	
	// The algorithm returns a pointer to the 'ex' argument, which for the 'ex_string' version of the
	// algorithm is something that was already present on the python side. Hence return_internal_reference,
	// not manage_new_object.

	def(name.c_str(),  &dispatch_2_ex_string<F>, (arg("ex"),arg("args"),arg("deep")=true,arg("repeat")=false), 
		 return_internal_reference<1>() );

	// The following does lead to a new object being created from the string, and this new object needs
	// to be managed.

	def(name.c_str(),  &dispatch_2_string_string<F>, (arg("ex"),arg("args"),arg("deep")=true,arg("repeat")=false), 
		 return_value_policy<manage_new_object>() );
	}

// Declare a property. These take one Ex to which they will be attached, and
// one optional additional Ex which is a list of parameters. The latter are thus always
// Cadabra expressions, and cannot easily contain Python constructions (for the time
// being this follows most closely the setup we had in Cadabra v1; if the need arises
// we can make this more complicated later). 

template<class P>
void def_prop()
	{
	using namespace boost::python;
	P *p = new P;

	class_<Property<P>, bases<BaseProperty> > pr(p->name().c_str(), init<std::shared_ptr<Ex>, 
																optional<std::shared_ptr<Ex> > >());

	delete p;

	pr.def("__str__", &Property<P>::str_).def("__repr__", &Property<P>::repr_);
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
	pyParseException.def("__str__", &ParseException::what);
	ParseExceptionType=pyParseException.ptr();

	class_<ArgumentException> pyArgumentException("ArgumentException", init<std::string>());
	pyArgumentException.def("__str__", &ArgumentException::py_what);
	ArgumentExceptionType=pyArgumentException.ptr();

	// Declare the Kernel object for Python so we can store it in the local Python context.
	class_<Kernel> pyKernel("Kernel", init<>());

	// Declare the Ex object to store expressions and manipulate on the Python side.
	class_<Ex, std::shared_ptr<Ex> > pyEx("Ex", boost::python::no_init);
	pyEx.def("__init__", boost::python::make_constructor(&make_Ex_from_string));
	pyEx.def("__init__", boost::python::make_constructor(&make_Ex_from_int));
	pyEx.def("__str__",  &Ex::str_)
		.def("_repr_latex_", &Ex::_repr_latex_)
		.def("_repr_html_", &Ex::_repr_html_)
		.def("__repr__", &Ex::repr_)
		.def("__eq__",   &Ex::operator==)
		.def("__eq__",   &Ex::__eq__int)
		.def("state",    &Ex::state);
	

	enum_<Algorithm::result_t>("result_t")
		.value("changed", Algorithm::result_t::l_applied)
		.value("unchanged", Algorithm::result_t::l_no_action)
		.value("error", Algorithm::result_t::l_error)
		.export_values()
		;

	// Inspection algorithms and other global functions which do not fit into the C++
   // framework anymore.

	def("tree", &print_tree);
	def("init_ipython", &init_ipython);
	def("properties", &list_properties);

	def("create_scope", &create_scope, 
		 return_value_policy<manage_new_object>() );
	def("create_scope_from_global", &create_scope_from_global, 
		 return_value_policy<manage_new_object>());
	def("create_empty_scope", &create_empty_scope, 
		 return_value_policy<manage_new_object>());

	// You cannot use implicitly_convertible to convert a string parameter to an Ex object
	// automatically: think about how that would work in C++. You would need to be able to
	// pass a 'std::string' to a function that expects an 'Ex *'. That will never work.

	// Algorithms with only the Ex as argument.
	def_algo_1<canonicalise>("canonicalise");
	def_algo_1<collect_terms>("collect_terms");
	def_algo_1<distribute>("distribute");
	def_algo_1<eliminate_kronecker>("eliminate_kronecker");
	def_algo_1<indexsort>("indexsort");
	def_algo_1<product_rule>("product_rule");
	def_algo_1<rename_dummies>("rename_dummies");
	def_algo_1<reduce_sub>("reduce_sub");
	def_algo_1<sort_product>("sort_product");
	def_algo_1<unwrap>("unwrap");

	def("young_project_tensor", &dispatch_1_ex<young_project_tensor, bool>, 
		 (arg("ex"),arg("deep")=true,arg("repeat")=false,
		  arg("modulo_monoterm")=false),
		 return_internal_reference<1>() );

	def("join_gamma",  &dispatch_1_ex<join_gamma, bool, bool>, 
		 (arg("ex"),arg("deep")=true,arg("repeat")=false,
		  arg("expand")=true,arg("use_gendelta")=false),
		 return_internal_reference<1>() );

	// Algorithms which take a second Ex as argument.
	def_algo_2<substitute>("substitute");
	def_algo_2<split_index>("split_index");
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

	def_prop<Accent>();
	def_prop<AntiCommuting>();
	def_prop<AntiSymmetric>();
	def_prop<Coordinate>();
	def_prop<Commuting>();
	def_prop<CommutingAsProduct>();
	def_prop<CommutingAsSum>();
	def_prop<DAntiSymmetric>();
	def_prop<Depends>();
	def_prop<Derivative>();
	def_prop<Diagonal>();
	def_prop<Distributable>();
	def_prop<DiracBar>();
	def_prop<GammaMatrix>();
	def_prop<ImplicitIndex>();	
	def_prop<IndexInherit>();
	def_prop<Indices>();	
	def_prop<Integer>();
	def_prop<KroneckerDelta>();
	def_prop<Metric>();
	def_prop<NonCommuting>();
	def_prop<PartialDerivative>();
	def_prop<RiemannTensor>();
	def_prop<SatisfiesBianchi>();
	def_prop<SelfAntiCommuting>();
	def_prop<SelfCommuting>();
	def_prop<SortOrder>();
	def_prop<Spinor>();
	def_prop<Symmetric>();
	def_prop<TableauSymmetry>();
	def_prop<Traceless>();
	def_prop<Weight>();
	def_prop<WeightInherit>();
	def_prop<WeylTensor>();

	register_exception_translator<ParseException>(&translate_ParseException);
	register_exception_translator<ArgumentException>(&translate_ArgumentException);

	// How can we give Python access to information stored in properties?
	}
