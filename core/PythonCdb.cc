


#include <memory>

#include "Config.hh"
#include "PythonCdb.hh"
#include "SympyCdb.hh"

#include "Parser.hh"
#include "Bridge.hh"
#include "Exceptions.hh"
#include "DisplayMMA.hh"
#include "DisplayTeX.hh"
#include "DisplaySympy.hh"
#include "DisplayTerminal.hh"
#include "Cleanup.hh"
#include "PreClean.hh"
#include "PythonException.hh"
#include "ProgressMonitor.hh"

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <boost/algorithm/string/replace.hpp>
#include <memory>
#include <sstream>

// Properties.

#include "properties/Accent.hh"
#include "properties/AntiCommuting.hh"
#include "properties/AntiSymmetric.hh"
#include "properties/Commuting.hh"
#include "properties/Coordinate.hh"
#include "properties/Depends.hh"
#include "properties/DependsInherit.hh"
#include "properties/Derivative.hh"
#include "properties/DifferentialForm.hh"
#include "properties/DiracBar.hh"
#include "properties/GammaMatrix.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/DAntiSymmetric.hh"
#include "properties/Diagonal.hh"
#include "properties/Distributable.hh"
#include "properties/EpsilonTensor.hh"
#include "properties/ExteriorDerivative.hh"
#include "properties/FilledTableau.hh"
#include "properties/ImaginaryI.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/Indices.hh"
#include "properties/IndexInherit.hh"
#include "properties/Integer.hh"
#include "properties/InverseMetric.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/LaTeXForm.hh"
#include "properties/Matrix.hh"
#include "properties/Metric.hh"
#include "properties/NonCommuting.hh"
#include "properties/NumericalFlat.hh"
#include "properties/PartialDerivative.hh"
#include "properties/RiemannTensor.hh"
#include "properties/SatisfiesBianchi.hh"
#include "properties/SelfAntiCommuting.hh"
#include "properties/SelfCommuting.hh"
#include "properties/SelfNonCommuting.hh"
#include "properties/SortOrder.hh"
#include "properties/Spinor.hh"
#include "properties/Symbol.hh"
#include "properties/Symmetric.hh"
#include "properties/Tableau.hh"
#include "properties/TableauSymmetry.hh"
#include "properties/Traceless.hh"
#include "properties/Vielbein.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"
#include "properties/WeylTensor.hh"

// Algorithms.

#include "algorithms/canonicalise.hh"
#include "algorithms/collect_components.hh"
#include "algorithms/collect_factors.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/combine.hh"
#include "algorithms/complete.hh"
#include "algorithms/decompose_product.hh"
#include "algorithms/distribute.hh"
#include "algorithms/drop_weight.hh"
#include "algorithms/einsteinify.hh"
#include "algorithms/eliminate_kronecker.hh"
#include "algorithms/eliminate_metric.hh"
#include "algorithms/epsilon_to_delta.hh"
#include "algorithms/evaluate.hh"
#include "algorithms/expand.hh"
#include "algorithms/expand_delta.hh"
#include "algorithms/expand_diracbar.hh"
#include "algorithms/expand_power.hh"
#include "algorithms/factor_in.hh"
#include "algorithms/factor_out.hh"
#include "algorithms/fierz.hh"
#include "algorithms/flatten_sum.hh"
#include "algorithms/indexsort.hh"
#include "algorithms/integrate_by_parts.hh"
#include "algorithms/join_gamma.hh"
#include "algorithms/keep_terms.hh"
#include "algorithms/lr_tensor.hh"
#ifdef MATHEMATICA_FOUND
   #include "algorithms/map_mma.hh"
#endif
#include "algorithms/map_sympy.hh"
#include "algorithms/order.hh"
#include "algorithms/product_rule.hh"
#include "algorithms/reduce_delta.hh"
#include "algorithms/rename_dummies.hh"
#include "algorithms/simplify.hh"
#include "algorithms/sort_product.hh"
#include "algorithms/sort_spinors.hh"
#include "algorithms/sort_sum.hh"
#include "algorithms/split_gamma.hh"
#include "algorithms/split_index.hh"
#include "algorithms/substitute.hh"
#include "algorithms/sym.hh"
#include "algorithms/tab_dimension.hh"
#include "algorithms/take_match.hh"
#include "algorithms/replace_match.hh"
#include "algorithms/rewrite_indices.hh"
#include "algorithms/unwrap.hh"
#include "algorithms/vary.hh"
#include "algorithms/young_project.hh"
#include "algorithms/young_project_product.hh"
#include "algorithms/young_project_tensor.hh"


using namespace cadabra;

// Wrap the 'totals' member of ProgressMonitor to return a Python list.

pybind11::list ProgressMonitor_totals_helper(ProgressMonitor& self)
	{
	pybind11::list list;
	auto totals = self.totals();
	for(auto& total: totals)
		list.append(total);
	return list;
	}


// Split a 'sum' expression into its individual terms. FIXME: now deprecated because we have operator[]?

pybind11::list terms(std::shared_ptr<Ex> ex) 
	{
	Ex::iterator it=ex->begin();

	if(*it->name!="\\sum")
		throw ArgumentException("terms() expected a sum expression.");

	pybind11::list ret;

	auto sib=ex->begin(it);
	while(sib!=ex->end(it)) {
		ret.append(Ex(sib));
		++sib;
		}
	
	return ret;
	}

Ex lhs(std::shared_ptr<Ex> ex)
	{
	auto it=ex->begin();
	if(it==ex->end()) 
		throw ArgumentException("Empty expression passed to 'lhs'.");

	if(*it->name!="\\equals")
		throw ArgumentException("Cannot take 'lhs' of expression which is not an equation.");

	return Ex(ex->begin(ex->begin()));
	}

Ex rhs(std::shared_ptr<Ex> ex)
	{
	auto it=ex->begin();
	if(it==ex->end()) 
		throw ArgumentException("Empty expression passed to 'rhs'.");

	if(*it->name!="\\equals")
		throw ArgumentException("Cannot take 'rhs' of expression which is not an equation.");

	auto sib=ex->begin(ex->begin());
	++sib;
	return Ex(sib);
	}

Ex Ex_getslice(std::shared_ptr<Ex> ex, pybind11::slice slice)
	{
	Ex result;

	pybind11::size_t start, stop, step, length;
	slice.compute(ex->size(), &start, &stop, &step, &length);
	if (length == 0)
		return result;
	
	// Set head
	auto it = result.set_head(*ex->begin());
	
	// Iterate over fully-closed range.
	for (; start != stop; start += step) {
		Ex::iterator toadd(ex->begin());
		std::advance(toadd, start);
		result.append_child(it, toadd);
		}
	Ex::iterator toadd(ex->begin());
	std::advance(toadd, start);
	result.append_child(it, toadd);
	return result;
	}

Ex Ex_getitem(Ex &ex, int index)
	{
	Ex::iterator it=ex.begin();

	size_t num=ex.number_of_children(it);
	if(index>=0 && (size_t)index<num)
		return Ex(ex.child(it, index));
	else {
//		if(num==0 && index==0) {
//			std::cerr << "returning " << ex << std::endl;
//			return Ex(ex);
//			}
//		else
			throw ArgumentException("index "+std::to_string(index)+" out of range, must be smaller than "+std::to_string(num));
		}
	}


/// ExNode is a combination of an Ex::iterator and an interface which
/// we can use to manipulate the data pointed to by this iterator.
/// In this way, we can use
///
///   for it in ex:
///      ...
///
/// loops and still use 'it' to do things like insertion etc.
/// which requires knowing the Ex::iterator.
///
/// Iterators are much safer than in C++, because they carry the
/// tree modification interface themselves, and can thus compute
/// their next value for any destructive operation.

class ExNode {
   public:
      ExNode(Ex&);
      
      Ex&          ex;
      Ex::iterator it;

      ExNode& iter();
      ExNode& next();
      
      std::string get_name() const;
      void        set_name(std::string);

      /// Take a child argument out of the node and
      /// add as child of current.
//      ExNode      unwrap(ExNode child);

		/// Replace the subtree at the current node with the given
		/// expression. Updates the iterator so that it points to the
		/// replacement subtree.
		void        replace(Ex& rep);
		
      /// Get a new iterator which always stays
      /// below the current one.
      ExNode      getitem_string(std::string tag);

		/// Get a new iterator which only iterates over all first-level
		/// indices.
		ExNode      indices();
      
		/// Get a new iterator which only iterates over all first-level
		/// arguments (non-indices).
		ExNode      args();
      
      std::string tag;
		bool        indices_only, args_only;

      void update(bool first);
      Ex::iterator         nxtit;
		Ex::sibling_iterator sibnxtit;
      Ex::iterator         topit, stopit;
};

ExNode ExNode::getitem_string(std::string tag)
   {
   ExNode ret(ex);
   ret.tag=tag;
   ret.ex=ex;
   ret.topit=it;
   ret.stopit=it;
   ret.stopit.skip_children();
   ++ret.stopit;
   ret.update(true);
   return ret;
   }

ExNode ExNode::indices()
	{
	ExNode ret(ex);
	ret.topit=it;
	ret.indices_only=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::args()
	{
	ExNode ret(ex);
	ret.topit=it;
	ret.args_only=true;
	ret.update(true);
	return ret;
	}

void ExNode::replace(Ex& rep)
	{
	it=ex.replace(it, rep.begin());
	}

std::string ExNode::get_name() const
   {
   return *it->name;
   }

void ExNode::set_name(std::string nm)
   {
   it->name = name_set.insert(nm).first;
   }

ExNode::ExNode(Ex& ex_)
   : ex(ex_), indices_only(false), args_only(false)
   {
   }

ExNode& ExNode::iter()
   {
   return *this;
   }

void ExNode::update(bool first)
   {
	if(indices_only || args_only) {
		if(first) sibnxtit=ex.begin(topit);
		else      ++sibnxtit;

		while(sibnxtit!=ex.end(topit)) {
			if(indices_only) 
				if(sibnxtit->fl.parent_rel==str_node::p_sub || sibnxtit->fl.parent_rel==str_node::p_super) 
					return;
			if(args_only)
				if(sibnxtit->fl.parent_rel==str_node::p_none)
					return;
			++sibnxtit;
			}
		}
	else {
		if(first) nxtit=topit;
		else      ++nxtit;

		while(nxtit!=stopit) {
			if(*nxtit->name==tag)
				return;
			++nxtit;
			}
		}
   }

ExNode& ExNode::next()
   {
   if(indices_only || args_only) {
		if(sibnxtit==ex.end(topit))
			throw pybind11::stop_iteration();			
		it=sibnxtit;
		}
	else {
		if(nxtit==stopit)
			throw pybind11::stop_iteration();
		it=nxtit;		
		}

   update(false);
   return *this;
   }

ExNode Ex_getitem_string(Ex &ex, std::string tag)
	{
	ExNode ret(ex);
	ret.tag=tag;
	ret.ex=ex;
	ret.topit=ex.begin();
	ret.stopit=ex.end();
	ret.update(true);
	return ret;
	}

void Ex_setitem(Ex &ex, int index, Ex val)
	{
	Ex::iterator it=ex.begin();

	size_t num=ex.number_of_children(it);
	if(index>=0 && (size_t)index<num) 
		ex.replace(ex.child(it, index), val.begin());
	else 
		throw ArgumentException("index "+std::to_string(index)+" out of range, must be smaller than "+std::to_string(num));
	}

size_t Ex_len(Ex& ex)
	{
	Ex::iterator it=ex.begin();

	return ex.number_of_children(it);
	}

std::string Ex_head(Ex& ex)
   {
   if(ex.begin()==ex.end())
	   throw ArgumentException("Expression is empty, no head.");
   return *ex.begin()->name;
   }

pybind11::object Ex_mult(Ex& ex)
   {
   if(ex.begin()==ex.end())
	   throw ArgumentException("Expression is empty, no head.");
	pybind11::object mpq = pybind11::module::import("gmpy2").attr("mpq");
	auto m = *ex.begin()->multiplier;
//	return mpq(2,3);
	
	pybind11::object mult = mpq(m.get_num().get_si(), m.get_den().get_si());
	return mult;
   }

bool output_ipython=false;
bool post_process_enabled=true;

// Output routines in display, input, latex and html formats (the latter two
// for use with IPython).

std::string Ex_str_(std::shared_ptr<Ex> ex) 
	{
 	std::ostringstream str;
// 
// //	if(state()==Algorithm::result_t::l_no_action)
// //		str << "(unchanged)" << std::endl;
// 	DisplayTeX dt(get_kernel_from_scope()->properties, ex);

	DisplayTerminal dt(*get_kernel_from_scope(), *ex, true);
	dt.output(str);

	return str.str();
	}

std::string Ex_to_input(std::shared_ptr<Ex> ex)
	{
 	std::ostringstream str;
// 
// //	if(state()==Algorithm::result_t::l_no_action)
// //		str << "(unchanged)" << std::endl;
// 	DisplayTeX dt(get_kernel_from_scope()->properties, ex);

	DisplayTerminal dt(*get_kernel_from_scope(), *ex, false);
	dt.output(str);

	return str.str();
	}

std::string Ex_latex_(std::shared_ptr<Ex> ex) 
	{
	if(!ex) return "";
 	std::ostringstream str;
	DisplayTeX dt(*get_kernel_from_scope(), *ex);
	dt.output(str);
	return str.str();
	}

std::string Ex_repr_(std::shared_ptr<Ex> ex) 
	{
	if(!ex) return "";
	if(ex->begin()==ex->end()) return "";

	Ex::iterator it = ex->begin();
	std::ostringstream str;
	ex->print_python(str, ex->begin());
	return str.str();
	}

std::string Ex_to_Sympy_string(std::shared_ptr<Ex> ex)
	{
	// Check to see if the expression is a scalar without dummy indices.
//	Algorithm::index_map_t ind_free, ind_dummy;
//	Algorithm::classify_indices(ex.begin(), ind_free, ind_dummy);
//	if(ind_dummy.size()>0) 
//		throw NonScalarException("Expression contains dummy indices.");
//	if(ind_free.size()>0) 
//		throw NonScalarException("Expression contains free indices.");

	if(!ex) return "";
	std::ostringstream str;
	DisplaySympy dt(*get_kernel_from_scope(), *ex);
	dt.output(str);
	return str.str();
	}

pybind11::object Ex_to_Sympy(std::shared_ptr<Ex> ex)
	{
	// Generate a string which can be parsed by Sympy.
	std::string txt = Ex_to_Sympy_string(ex);
	
	// Call sympify on a sympy-parseable  textual representation.
	pybind11::module sympy_parser = pybind11::module::import("sympy.parsing.sympy_parser");
	auto parse  = sympy_parser.attr("parse_expr");
	pybind11::object ret=parse(txt);
	return ret;
	}

std::string Ex_to_MMA(std::shared_ptr<Ex> ex, bool use_unicode)
	{
	// Check to see if the expression is a scalar without dummy indices.
//	Algorithm::index_map_t ind_free, ind_dummy;
//	Algorithm::classify_indices(ex.begin(), ind_free, ind_dummy);
//	if(ind_dummy.size()>0) 
//		throw NonScalarException("Expression contains dummy indices.");
//	if(ind_free.size()>0) 
//		throw NonScalarException("Expression contains free indices.");

	std::ostringstream str;
	DisplayMMA dt(*get_kernel_from_scope(), *ex, use_unicode);
	dt.output(str);

	return str.str();
	}


pybind11::object get_locals()
        {
        return pybind11::reinterpret_borrow<pybind11::object>(PyEval_GetLocals());
        }

pybind11::object get_globals()
        {
        return pybind11::reinterpret_borrow<pybind11::object>(PyEval_GetGlobals());
        }


// Fetch objects from the Python side using their Python identifier.

std::shared_ptr<Ex> fetch_from_python(const std::string& nm)
	{
	try {
		pybind11::object locals = get_locals();
		pybind11::object obj = locals[nm.c_str()];

		if(obj.is_none()) // We never actually get here, an exception will have been thrown.
			std::cout << "object unknown" << std::endl;
		else {
			// We can include this Python object into the expression only if it is an Ex object.
			try {
				return obj.cast<std::shared_ptr<Ex>>();
				}
			catch(const pybind11::cast_error& e) {
				std::cout << nm << " is not of type cadabra.Ex" << std::endl;
				}
			}
		}
	catch(pybind11::error_already_set const &) {
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



bool __eq__Ex_Ex(std::shared_ptr<Ex> one, std::shared_ptr<Ex> other) 
	{
	return tree_equal(&(get_kernel_from_scope()->properties), *one, *other);
	}

bool __eq__Ex_int(std::shared_ptr<Ex> one, int other) 
	{
	auto ex=std::make_shared<Ex>(other);
	return __eq__Ex_Ex(one, ex);
	}


// Functions to construct an Ex object and then create an additional
// reference '_' on the global python stack that points to this object
// as well. We disable automatic constructor generation in the declaration
// of Ex on the Python side, so all creation of Ex objects in Python goes
// through these two functions.

std::shared_ptr<Ex> make_Ex_from_string(const std::string& ex_, bool make_ref, Kernel *kernel)
	{
	auto ptr = std::make_shared<Ex>();
	// Parse the string expression.
	Parser parser(ptr);
	std::stringstream str(ex_);

	try {
		str >> parser;
		}
	catch(std::exception& except) {
		throw ParseException("Cannot parse");
		}
	parser.finalise();

	// First pull in any expressions referred to with @(...) notation, because the
	// full expression may not have consistent indices otherwise.
	pull_in(ptr, kernel);
	//	std::cerr << "pulled in" << std::endl;
   
	// Basic cleanup of rationals and subtractions, followed by
   // cleanup of nested sums and products.
	pre_clean_dispatch_deep(*kernel, *ptr);
	cleanup_dispatch_deep(*kernel, *ptr);
	check_index_consistency(*kernel, *ptr, (*ptr).begin());
	call_post_process(*kernel, ptr);
	//	std::cerr << "cleaned up" << std::endl;

	return ptr;
	}

std::shared_ptr<Ex> construct_Ex_from_string(const std::string& ex_) 
	{
	Kernel *k=get_kernel_from_scope();
	return make_Ex_from_string(ex_, true, k);
	}

std::shared_ptr<Ex> construct_Ex_from_string_2(const std::string& ex_, bool add_ref) 
	{
	Kernel *k=get_kernel_from_scope();
	return make_Ex_from_string(ex_, add_ref, k);
	}

std::shared_ptr<Ex> make_Ex_from_int(int num, bool make_ref=true)
	{
	auto ptr = std::make_shared<Ex>(num);
	return ptr;
	}

std::shared_ptr<Ex> construct_Ex_from_int(int num)
	{
	return make_Ex_from_int(num, true);
	}

std::shared_ptr<Ex> construct_Ex_from_int_2(int num, bool add_ref)
	{
	return make_Ex_from_int(num, add_ref);
	}


Ex operator+(const Ex& ex1, const Ex& ex2)
	{
	if(ex1.size()==0) return ex2;
	if(ex2.size()==0) return ex1;

	bool comma1 = (*ex1.begin()->name=="\\comma");
	bool comma2 = (*ex2.begin()->name=="\\comma");

	if(comma1 || comma2) {
		if(comma1) {
			Ex ret(ex1);
			auto loc = ret.append_child(ret.begin(), ex2.begin());
			if(comma2)
				ret.flatten_and_erase(loc);
			return ret;
			}
		else {
			Ex ret(ex2);
			auto loc = ret.prepend_child(ret.begin(), ex1.begin());
			if(comma1)
				ret.flatten_and_erase(loc);
			return ret;
			}
		}
	else {
		Ex ret(ex1);
		if(*ret.begin()->name!="\\sum") 
			ret.wrap(ret.begin(), str_node("\\sum"));
		ret.append_child(ret.begin(), ex2.begin());
		
		auto it=ret.begin();
		cleanup_dispatch(*get_kernel_from_scope(), ret, it);
		return ret;
		}
	}

Ex operator-(const Ex& ex1, const Ex& ex2)
	{
	if(ex1.size()==0) {
		if(ex2.size()!=0) {
			Ex ret(ex2);
			multiply(ex2.begin()->multiplier, -1);
			auto it=ret.begin();
			cleanup_dispatch(*get_kernel_from_scope(), ret, it);
			return ret;
			}
		else return ex2;
		}
	if(ex2.size()==0) return ex1;

	Ex ret(ex1);
	if(*ret.begin()->name!="\\sum") 
		ret.wrap(ret.begin(), str_node("\\sum"));
	multiply( ret.append_child(ret.begin(), ex2.begin())->multiplier, -1 );

	auto it=ret.begin();
	cleanup_dispatch(*get_kernel_from_scope(), ret, it);
	
	return ret;
	}

// Initialise mathematics typesetting for IPython.

std::string init_ipython()
	{
	pybind11::exec("from IPython.display import Math");
	return "Cadabra typeset output for IPython notebook initialised.";
	}

// Generate a Python list of all properties declared in the current scope. These will
// (FIXME: should be) displayed in input form, i.e. they can be fed back into Python.
// FIXME: most of this has nothing to do with Python, factor back into core.

pybind11::list list_properties()
	{
//	std::cout << "listing properties" << std::endl;
	Kernel *kernel=get_kernel_from_scope();
	Properties& props=kernel->properties;

	pybind11::list ret;
	std::string res;
	bool multi=false;
	for(auto it=props.pats.begin(); it!=props.pats.end(); ++it) {
		if(it->first->hidden()) continue;
		
		// print the property name if we are at the end or if the next entry is for
		// a different property.
		decltype(it) nxt=it;
		++nxt;
		if(res=="" && (nxt!=props.pats.end() && it->first==nxt->first)) {
			res+="{";
			multi=true;
			}


		//std::cerr << Ex(it->second->obj) << std::endl;
//		DisplayTeX dt(*get_kernel_from_scope(), it->second->obj);
		std::ostringstream str;
		// std::cerr << "displaying" << std::endl;
//		dt.output(str);

		str << it->second->obj;
		
		// std::cerr << "displayed " << str.str() << std::endl;
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

//pybind11::list indices() 
//	{
//	Kernel *kernel=get_kernel_from_scope();
//	}

// Debug function to display an expression in tree form.

std::string print_tree(Ex *ex)
	{
	std::ostringstream str;
	ex->print_entire_tree(str);
	return str.str();
	}


// Return the kernel (with symbol __cdbkernel__) in local scope if
// any, or the one in global scope if none is available in local
// scope.

Kernel *get_kernel_from_scope()
	{
	Kernel *kernel=nullptr;
	
	// Try and find the kernel in the local scope
	try {
		// Get from the python localcs dict
		pybind11::object locals = get_locals();
		kernel = locals["__cdbkernel__"].cast<Kernel*>();
		}
	catch (const pybind11::error_already_set& err) {
		// __cdbkernel__ not found in locals dict
		kernel = nullptr;
		}
	if (kernel)  {
		// Return if found
		return kernel;
		}
	
	// No kernel in local scope, find one in global scope.
	try {
		// Get from the python globals dict
		pybind11::object globals = get_globals();
		kernel = globals["__cdbkernel__"].cast<Kernel*>();
		}
	catch(pybind11::error_already_set& err) {
		// __cdbkernel__ not found in globals dict
		kernel = nullptr;
		}
	if(kernel) {
		//Return if found
		return kernel;
		}
	
	// No kernel in local or global scope, construct a new global one
	kernel = new Kernel();
	inject_defaults(kernel);
	pybind11::object globals = get_globals();
	globals["__cdbkernel__"] = kernel;
	return kernel;
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

void inject_defaults(Kernel *k)
	{
	// Create and inject properties; these then get owned by the kernel.
	post_process_enabled=false;

	k->inject_property(new Distributable(),      make_Ex_from_string("\\prod{#}",false, k), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\prod{#}",false, k), 0);
	k->inject_property(new CommutingAsProduct(), make_Ex_from_string("\\prod{#}",false, k), 0);
	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\prod{#}",false, k), 0);
	k->inject_property(new NumericalFlat(),      make_Ex_from_string("\\prod{#}",false, k), 0);
	auto wi2=new WeightInherit();
	wi2->combination_type = WeightInherit::multiplicative;
	auto wa2=make_Ex_from_string("label=all, type=multiplicative", false, k);
	k->inject_property(wi2,                      make_Ex_from_string("\\prod{#}",false, k), wa2);

	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\frac{#}",false, k), 0);
	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\frac{#}",false, k), 0);
	
	k->inject_property(new Distributable(),      make_Ex_from_string("\\wedge{#}",false, k), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\wedge{#}",false, k), 0);
//	k->inject_property(new CommutingAsProduct(), make_Ex_from_string("\\prod{#}",false), 0);
	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\wedge{#}",false, k), 0);
	k->inject_property(new NumericalFlat(),      make_Ex_from_string("\\wedge{#}",false, k), 0);
	auto wi4=new WeightInherit();
	wi4->combination_type = WeightInherit::multiplicative;
	auto wa4=make_Ex_from_string("label=all, type=multiplicative", false, k);
	k->inject_property(wi4,                      make_Ex_from_string("\\wedge{#}",false, k), wa4);

	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\sum{#}",false, k), 0);
	k->inject_property(new CommutingAsSum(),     make_Ex_from_string("\\sum{#}",false, k), 0);
	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\sum{#}",false, k), 0);
	auto wi=new WeightInherit();
	auto wa=make_Ex_from_string("label=all, type=additive", false, k);
	k->inject_property(wi,                       make_Ex_from_string("\\sum{#}", false, k), wa);

	auto d = new Derivative();
	d->hidden(true);
	k->inject_property(d,                        make_Ex_from_string("\\cdbDerivative{#}",false, k), 0);
	
	k->inject_property(new Derivative(),         make_Ex_from_string("\\commutator{#}",false, k), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\commutator{#}",false, k), 0);

	k->inject_property(new Derivative(),         make_Ex_from_string("\\anticommutator{#}",false, k), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\anticommutator{#}",false, k), 0);

	k->inject_property(new Distributable(),      make_Ex_from_string("\\indexbracket{#}",false, k), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\indexbracket{#}",false, k), 0);

	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\pow{#}",false, k), 0);
	auto wi3=new WeightInherit();
	auto wa3=make_Ex_from_string("label=all, type=power", false, k);
	k->inject_property(wi3,                      make_Ex_from_string("\\pow{#}",false, k), wa3);

	k->inject_property(new NumericalFlat(),      make_Ex_from_string("\\int{#}",false, k), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\int{#}",false, k), 0);

	// Accents, necessary for proper display.
	k->inject_property(new Accent(),             make_Ex_from_string("\\hat{#}",false, k), 0);
	k->inject_property(new Accent(),             make_Ex_from_string("\\bar{#}",false, k), 0);
	k->inject_property(new Accent(),             make_Ex_from_string("\\overline{#}",false, k), 0);
	k->inject_property(new Accent(),             make_Ex_from_string("\\tilde{#}",false, k), 0);

	post_process_enabled=true;
//	k->inject_property(new Integral(),           make_Ex_from_string("\\int{#}",false), 0);
	}


// Property constructor and display members for Python purposes.

template<class Prop>
Property<Prop>::Property(std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param) 
	{
	for_obj = ex;
	Kernel *kernel=get_kernel_from_scope();
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
	str << " to "+Ex_str_(for_obj)+".";
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
	std::string bare=Ex_latex_(for_obj);
	str << " to~}"+bare+".";
	return str.str();
	}

template<>
std::string Property<LaTeXForm>::latex_() const
	{
	std::ostringstream str;
	str << "\\text{Attached property ";
	prop->latex(str);
	std::string bare=Ex_str_(for_obj);
	replace_all(bare, "\\", "$\\backslash{}$}");
	str << " to {\\tt "+bare+"}.";
	return str.str();
	}

template<class Prop>
std::string Property<Prop>::repr_() const
	{
	// FIXME: this needs work, it does not output things which can be fed back into python.
	return "Property::repr: "+prop->name();
	}

// Templates to dispatch function calls in Python to algorithms in
// C++.  All algorithms take the 'deep' and 'repeat' boolean
// arguments, the 'depth' integer argument, plus an arbitrary list of
// additional arguments (of any type) indicated by 'args' (see the
// declaration of 'join_gamma' below for an example of how to declare
// those additional arguments).

ProgressMonitor *pm=0;

template<class F>
std::shared_ptr<Ex> dispatch_base(std::shared_ptr<Ex> ex, F& algo, bool deep, bool repeat, unsigned int depth, bool pre_order)
	{
	Ex::iterator it=ex->begin();
	if(ex->is_valid(it)) { // This may be called on an empty expression; just safeguard against that.
		if(pm==0) {
			try {
				pybind11::object globals=get_globals();
				pybind11::object obj = globals["server"];
				pm = obj.cast<ProgressMonitor *>(); 
				}
			catch(pybind11::error_already_set& err) {
				std::cerr << "Cannot find ProgressMonitor derived 'server' object." << std::endl;
				}
			}

		algo.set_progress_monitor(pm);
		if(!pre_order)
			ex->update_state(algo.apply_generic(it, deep, repeat, depth));
		else
			ex->update_state(algo.apply_pre_order(repeat));			
		call_post_process(*get_kernel_from_scope(), ex);
		}
	return ex;
	}

std::shared_ptr<Ex> map_sympy_wrapper(std::shared_ptr<Ex> ex, std::string head, pybind11::args args)
	{
	std::vector<std::string> av;
	for(auto& arg: args)
		av.push_back(arg.cast<std::string>());
	map_sympy algo(*get_kernel_from_scope(), *ex, head, av);
	return dispatch_base(ex, algo, true, false, 0, true);
	}

#ifdef MATHEMATICA_FOUND
std::shared_ptr<Ex> map_mma_wrapper(std::shared_ptr<Ex> ex, std::string head)
	{
	map_mma algo(*get_kernel_from_scope(), *ex, head);
	return dispatch_base(ex, algo, true, false, 0, true);
	}
#endif

void call_post_process(Kernel& kernel, std::shared_ptr<Ex> ex) 
	{
	// Find the 'post_process' function, and if found, turn off
	// post-processing, then call the function on the current Ex.
	if(post_process_enabled) {
		if(ex->number_of_children(ex->begin())==0)
			return;

		post_process_enabled=false;

		pybind11::object post_process;
		
		try {
			// First try the locals.
			pybind11::object locals = get_locals();
			post_process = locals["post_process"];
			// std::cerr << "local post_process" << std::endl;
			}
		catch(const pybind11::error_already_set& exc) {
			// In order to prevent the error from propagating, we have to read it out. 			
			std::string err = parse_python_exception();
			try {
				pybind11::object globals = get_globals();
				post_process = globals["post_process"];
				// std::cerr << "global post_process" << std::endl;				
				}
			catch(const pybind11::error_already_set& exc) {
				// In order to prevent the error from propagating, we have to read it out. 
				std::string err = parse_python_exception();
				post_process_enabled=true;
				return;
				}
			}
		// std::cerr << "calling post-process" << std::endl;
		post_process(std::ref(kernel), ex);
		post_process_enabled=true;
		}
	}

template<class F>
std::shared_ptr<Ex> dispatch_ex(std::shared_ptr<Ex> ex, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), *ex);
	return dispatch_base(ex, algo, deep, repeat, depth, false);
	}

template<class F, typename Arg1>
std::shared_ptr<Ex> dispatch_ex(std::shared_ptr<Ex> ex, Arg1 arg, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), *ex, arg);
	return dispatch_base(ex, algo, deep, repeat, depth, false);
	}

template<class F, typename Arg1>
std::shared_ptr<Ex> dispatch_ex_preorder(std::shared_ptr<Ex> ex, Arg1 arg, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), *ex, arg);
	return dispatch_base(ex, algo, deep, repeat, depth, true);
	}

template<class F, typename Arg1, typename Arg2>
std::shared_ptr<Ex> dispatch_ex(std::shared_ptr<Ex> ex, Arg1 arg1, Arg2 arg2, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), *ex, arg1, arg2);
	return dispatch_base(ex, algo, deep, repeat, depth, false);
	}

template<class F, typename Arg1, typename Arg2, typename Arg3>
std::shared_ptr<Ex> dispatch_ex(std::shared_ptr<Ex> ex, Arg1 arg1, Arg2 arg2, Arg3 arg3, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), *ex, arg1, arg2, arg3);
	return dispatch_base(ex, algo, deep, repeat, depth, false);
	}


// template<class F, typename Arg1>
// Ex* dispatch_2_ex_string_new(Ex& ex, const std::string& args, Arg1 arg1, bool deep, bool repeat, unsigned int depth)
// 	{
// 	auto argsobj = make_Ex_from_string(args, false);
// 	return dispatch_2_ex_ex_new<F, Arg1>(ex, *argsobj, arg1, deep, repeat, depth);
// 	}
// 
// template<class F>
// Ex* dispatch_2_ex_ex(Ex& ex, Ex& args, bool deep, bool repeat, unsigned int depth)
// 	{
// 	F algo(*get_kernel_from_scope(), ex, args);
// 
// 	Ex::iterator it=ex.begin().begin();
// 
// 	ex.reset_state();
// 	ex.update_state(algo.apply_generic(it, deep, repeat, depth));
// 	
// 	return &ex;
// 	}

// template<class F>
// Ex* dispatch_2_ex_string(Ex& ex, const std::string& args, bool deep, bool repeat, unsigned int depth)
// 	{
// 	auto argsobj = make_Ex_from_string(args, false);
// 	return dispatch_2_ex_ex<F>(ex, *argsobj, deep, repeat, depth);
// 	}
// 
// template<class F>
// Ex* dispatch_2_string_string(const std::string& ex, const std::string& args, bool deep, bool repeat, unsigned int depth)
// 	{
// 	auto exobj   = make_Ex_from_string(ex);
// 	auto argsobj = make_Ex_from_string(args, false);
// 	return dispatch_2_ex_ex<F>(*exobj, *argsobj, deep, repeat, depth);
// 	}

// Templated function which declares various forms of the algorithm entry points in one shot.
// First the ones with no argument, just a deep flag.

template<class F>
void def_algo_1(const std::string& name, pybind11::module& m) 
	{
	using namespace pybind11;

	m.def(name.c_str(),
		 &dispatch_ex<F>,
		 arg("ex"),
		 arg("deep")=true,
		 arg("repeat")=false,
		 arg("depth")=0,
		 return_value_policy::reference_internal
		 );
	}


// Declare a property. These take one Ex to which they will be attached, and
// one optional additional Ex which is a list of parameters. The latter are thus always
// Cadabra expressions, and cannot easily contain Python constructions (for the time
// being this follows most closely the setup we had in Cadabra v1; if the need arises
// we can make this more complicated later). 

template<class P>
void def_prop(pybind11::module& m)
	{
	using namespace pybind11;

	class_<Property<P>, std::shared_ptr<Property<P>>, BaseProperty>(m, std::make_shared<P>()->name().c_str())
		.def(
			init<std::shared_ptr<Ex>, std::shared_ptr<Ex>>(),
			arg("ex"),
			arg("param")
			  )
		.def("__str__", &Property<P>::str_)
		.def("__repr__", &Property<P>::repr_)
		.def("_latex_", &Property<P>::latex_);
	}

// // All this class does is provide a trampoline class
// // for pybind to properly handle the constructor
// // of ProgressMonitor
// 
// class PyProgressMonitor : public ProgressMonitor {
// 	public:
// 		using ProgressMonitor::ProgressMonitor;
// };
// 


// Entry point for registration of the Cadabra Python module. 
// This registers the main Ex class which wraps Cadabra expressions, as well
// as the various algorithms that can act on these and the properties that can
// be attached to Cadabra patterns.

PYBIND11_MODULE(cadabra2, m)
	{
	// Declare the Kernel object for Python so we can store it in the local Python context.
	// We add a 'cadabra2.__cdbkernel__' object to the main module scope, and will 
	// pull that into the interpreter scope in the 'cadabra2_default.py' file.
	pybind11::enum_<Kernel::scalar_backend_t>(m, "scalar_backend_t")
		.value("sympy",       Kernel::scalar_backend_t::sympy)
		.value("mathematica", Kernel::scalar_backend_t::mathematica)
		.export_values()
		;

	pybind11::class_<Kernel>(m, "Kernel", pybind11::dynamic_attr())
		.def(pybind11::init<>())
		.def_readonly("scalar_backend", &Kernel::scalar_backend);
	
	Kernel* kernel = create_scope();
	m.attr("__cdbkernel__") = pybind11::cast(kernel);
	
	// Interface the Stopwatch class
	pybind11::class_<Stopwatch>(m, "Stopwatch")
		.def(pybind11::init<>())
		.def("start", &Stopwatch::start)
		.def("stop", &Stopwatch::stop)
		.def("reset", &Stopwatch::reset)
		.def("seconds", &Stopwatch::seconds)
		.def("useconds", &Stopwatch::useconds)
		.def("__str__", [](const Stopwatch& s) {
				std::stringstream ss;
				ss << s;
				return ss.str();
				});
	
	// Make our profiling class known to the Python world.
	pybind11::class_<ProgressMonitor>(m, "ProgressMonitor")\
		.def(pybind11::init<>())
		.def("print", &ProgressMonitor::print)
		.def("totals", &ProgressMonitor_totals_helper);
	
	pybind11::class_<ProgressMonitor::Total>(m, "Total")
		.def_readonly("name", &ProgressMonitor::Total::name)
		.def_readonly("call_count", &ProgressMonitor::Total::call_count)
//		.def_readonly("time_spent", &ProgressMonitor::Total::time_spent_as_long)
		.def_readonly("total_steps", &ProgressMonitor::Total::total_steps)
		.def("__str__", &ProgressMonitor::Total::str);

	// Declare the Ex object to store expressions and manipulate on the Python side.
	// We do not allow initialisation/construction except through the two 
	// make_Ex_from_... functions, which take care of creating a '_' reference
   // on the Python side as well.

//	pybind11::class_<Ex, std::shared_ptr<Ex> > pyEx("Ex", pybind11::no_init);
	pybind11::class_<Ex, std::shared_ptr<Ex> >(m, "Ex")
		.def(pybind11::init(&construct_Ex_from_string))
		.def(pybind11::init(&construct_Ex_from_string_2))
		.def(pybind11::init(&construct_Ex_from_int))
		.def(pybind11::init(&construct_Ex_from_int_2))
		.def("__str__",     &Ex_str_)
		.def("_latex_",     &Ex_latex_)
		.def("__repr__",    &Ex_repr_)
		.def("__eq__",      &__eq__Ex_Ex)
		.def("__eq__",      &__eq__Ex_int)
		.def("_sympy_",     &Ex_to_Sympy)		
		.def("sympy_form",  &Ex_to_Sympy_string)
		.def("mma_form",    &Ex_to_MMA, pybind11::arg("unicode")=true)    // standardize on this
		.def("input_form",  &Ex_to_input) 
		.def("__getitem__", &Ex_getitem)
		.def("__getitem__", &Ex_getitem_string)		
		.def("__getitem__", &Ex_getslice)
		.def("__setitem__", &Ex_setitem)
		.def("__len__",     &Ex_len)
		.def("head",        &Ex_head)
		.def("mult",        &Ex_mult)
		.def("__iter__", [](std::shared_ptr<Ex> ex) {
				return pybind11::make_iterator(ex->begin(), ex->end());
				})
		.def("state",       &Ex::state)
		.def("reset",       &Ex::reset_state)
		.def("changed",     &Ex::changed_state)
		.def(pybind11::self + pybind11::self)
		.def(pybind11::self - pybind11::self);

	pybind11::class_<ExNode>(m, "ExNode")
		.def("__iter__",        &ExNode::iter)
		.def("__next__",        &ExNode::next, pybind11::return_value_policy::reference_internal)
		.def("__getitem__",     &ExNode::getitem_string)
		.def("indices",         &ExNode::indices)
		.def("args",            &ExNode::args)				
		.def("replace",         &ExNode::replace)
		.def_property("name",   &ExNode::get_name, &ExNode::set_name)
		;
	
	pybind11::enum_<Algorithm::result_t>(m, "result_t")
		.value("checkpointed", Algorithm::result_t::l_checkpointed)
		.value("changed", Algorithm::result_t::l_applied)
		.value("unchanged", Algorithm::result_t::l_no_action)
		.value("error", Algorithm::result_t::l_error)
		.export_values()
		;

	// Inspection algorithms and other global functions which do not fit into the C++
   // framework anymore.

	m.def("kernel", [](pybind11::kwargs dict) {
			Kernel *k=get_kernel_from_scope();			
			for(auto& item: dict) {
				std::string key=item.first.cast<std::string>();
				std::string val=item.second.cast<std::string>();				
				if(key=="scalar_backend") {
					if(val=="sympy")            k->scalar_backend=Kernel::scalar_backend_t::sympy;
					else if(val=="mathematica") k->scalar_backend=Kernel::scalar_backend_t::mathematica;
					else throw ArgumentException("scalar_backend must be 'sympy' or 'mathematica'.");
					}
				else {
					throw ArgumentException("unknown argument '"+key+"'.");
					}
				}
			});
	m.def("tree", &print_tree);
	m.def("init_ipython", &init_ipython);
	m.def("properties", &list_properties);
	m.def("map_sympy", &map_sympy_wrapper,
			pybind11::arg("ex"),
			pybind11::arg("function")="",
			pybind11::return_value_policy::reference_internal);
#ifdef MATHEMATICA_FOUND
	m.def("map_mma",   &map_mma_wrapper,
			pybind11::arg("ex"),
			pybind11::arg("function")="",
			pybind11::return_value_policy::reference_internal);
#endif
	
	m.def("create_scope", &create_scope, 
			pybind11::return_value_policy::take_ownership);
	m.def("create_scope_from_global", &create_scope_from_global,
			pybind11::return_value_policy::take_ownership);			
	m.def("create_empty_scope", &create_empty_scope,
			pybind11::return_value_policy::take_ownership);			

	// Algorithms which spit out a new Ex (or a list of new Exs), instead of 
	// modifying the existing one. 

	m.def("terms", &terms);

	m.def("lhs", &lhs);
	m.def("rhs", &rhs);

	// We do not use implicitly_convertible to convert a string
	// parameter to an Ex object automatically (it never
	// worked). However, we wouldn't want to do this either, because we
	// now use a clear construction: the cadabra python modifications
	// interpret $...$ as a mathematical expression and turn it into an
	// Ex declaration.

	// Algorithms with only the Ex as argument.
	def_algo_1<canonicalise>("canonicalise", m);
	def_algo_1<collect_components>("collect_components", m);
	def_algo_1<collect_factors>("collect_factors", m);
 	def_algo_1<collect_terms>("collect_terms", m);
 	def_algo_1<combine>("combine", m);
	def_algo_1<decompose_product>("decompose_product", m);
	def_algo_1<distribute>("distribute", m);
	def_algo_1<eliminate_kronecker>("eliminate_kronecker", m);
 	def_algo_1<expand>("expand", m);
	def_algo_1<expand_delta>("expand_delta", m);
	def_algo_1<expand_diracbar>("expand_diracbar", m);
	def_algo_1<expand_power>("expand_power", m);
	def_algo_1<flatten_sum>("flatten_sum", m);
	def_algo_1<indexsort>("indexsort", m);
	def_algo_1<lr_tensor>("lr_tensor", m);
	def_algo_1<product_rule>("product_rule", m);
	def_algo_1<reduce_delta>("reduce_delta", m);
//	def_algo_1<reduce_sub>("reduce_sub", m);
	def_algo_1<sort_product>("sort_product", m);
	def_algo_1<sort_spinors>("sort_spinors", m);
	def_algo_1<sort_sum>("sort_sum", m);
	def_algo_1<tabdimension>("tab_dimension", m);	
	def_algo_1<young_project_product>("young_project_product", m);

	m.def("complete", &dispatch_ex<complete, Ex>, 
		 pybind11::arg("ex"),pybind11::arg("add"),
		  pybind11::arg("deep")=false,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("drop_weight", &dispatch_ex<drop_weight, Ex>, 
		 pybind11::arg("ex"),pybind11::arg("condition"),
		  pybind11::arg("deep")=false,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("eliminate_metric", &dispatch_ex<eliminate_metric, Ex>, 
			pybind11::arg("ex"),
			pybind11::arg("preferred")=std::make_shared<Ex>(),
			pybind11::arg("deep")=true,
			pybind11::arg("repeat")=false,
			pybind11::arg("depth")=0,
			pybind11::return_value_policy::reference_internal );

	m.def("keep_weight", &dispatch_ex<keep_weight, Ex>, 
		 pybind11::arg("ex"),pybind11::arg("condition"),
		  pybind11::arg("deep")=false,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("integrate_by_parts", &dispatch_ex<integrate_by_parts, Ex>, 
		 pybind11::arg("ex"),pybind11::arg("away_from"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("young_project_tensor", &dispatch_ex<young_project_tensor, bool>, 
		 pybind11::arg("ex"),pybind11::arg("modulo_monoterm")=false,
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("join_gamma",  &dispatch_ex<join_gamma, bool, bool>, 
		 pybind11::arg("ex"),pybind11::arg("expand")=true,pybind11::arg("use_gendelta")=false,
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("einsteinify", &dispatch_ex<einsteinify, Ex>,
			pybind11::arg("ex"), pybind11::arg("metric")=std::make_shared<Ex>(),
		  pybind11::arg("deep")=false,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
		  
	m.def("evaluate", &dispatch_ex<evaluate, Ex, bool, bool>,
			pybind11::arg("ex"), pybind11::arg("components")=Ex(), pybind11::arg("rhsonly")=false, pybind11::arg("simplify")=true,
		  pybind11::arg("deep")=false,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
		  
	m.def("keep_terms", &dispatch_ex<keep_terms, std::vector<int> >,
		 pybind11::arg("ex"), 
		  pybind11::arg("terms"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("young_project", &dispatch_ex<young_project, std::vector<int>, std::vector<int> >, 
		 pybind11::arg("ex"),
		  pybind11::arg("shape"), pybind11::arg("indices"), 
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("order", &dispatch_ex<order, Ex, bool>, 
		 pybind11::arg("ex"),
		  pybind11::arg("factors"), pybind11::arg("anticommuting")=false,
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("simplify", &dispatch_ex<simplify>, 
			pybind11::arg("ex"),
			pybind11::arg("deep")=false,
			pybind11::arg("repeat")=false,
			pybind11::arg("depth")=0,
			pybind11::return_value_policy::reference_internal );

	m.def("order", &dispatch_ex<order, Ex, bool>, 
		 pybind11::arg("ex"),
		  pybind11::arg("factors"), pybind11::arg("anticommuting")=false,
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("epsilon_to_delta", &dispatch_ex<epsilon_to_delta, bool>,
		 pybind11::arg("ex"),
		  pybind11::arg("reduce")=true,
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("rename_dummies", &dispatch_ex<rename_dummies, std::string, std::string>, 
		 pybind11::arg("ex"),
		  pybind11::arg("set")="", pybind11::arg("to")="",
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("sym", &dispatch_ex<sym, Ex, bool>, 
		 pybind11::arg("ex"),
		  pybind11::arg("items"), pybind11::arg("antisymmetric")=false,
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("asym", &dispatch_ex<sym, Ex, bool>, 
		 pybind11::arg("ex"),
		  pybind11::arg("items"), pybind11::arg("antisymmetric")=true,
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );

	m.def("factor_in", &dispatch_ex<factor_in, Ex>, 
		 pybind11::arg("ex"),
		  pybind11::arg("factors"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("factor_out", &dispatch_ex<factor_out, Ex, bool>, 
		 pybind11::arg("ex"),
		  pybind11::arg("factors"),pybind11::arg("right")=false,
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("fierz", &dispatch_ex<fierz, Ex>, 
		 pybind11::arg("ex"),
		  pybind11::arg("spinors"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("substitute", &dispatch_ex<substitute, Ex>, 
		 pybind11::arg("ex"),
		  pybind11::arg("rules"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("take_match", &dispatch_ex<take_match, Ex>, 
		 pybind11::arg("ex"),
		  pybind11::arg("rules"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("replace_match", &dispatch_ex<replace_match>, 
		 pybind11::arg("ex"),
		  pybind11::arg("deep")=false,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("rewrite_indices", &dispatch_ex<rewrite_indices, Ex, Ex>, 
		 pybind11::arg("ex"),pybind11::arg("preferred"),pybind11::arg("converters"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("vary", &dispatch_ex_preorder<vary, Ex>, 
		 pybind11::arg("ex"),
		  pybind11::arg("rules"),
		  pybind11::arg("deep")=false,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("split_gamma", &dispatch_ex<split_gamma, bool>, 
		 pybind11::arg("ex"),
		  pybind11::arg("on_back"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	m.def("split_index", &dispatch_ex<split_index, Ex>, 
		 pybind11::arg("ex"),
		  pybind11::arg("rules"),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	
	m.def("unwrap", &dispatch_ex<unwrap, Ex>, 
		 pybind11::arg("ex"),
			pybind11::arg("wrapper")=Ex(),
		  pybind11::arg("deep")=true,pybind11::arg("repeat")=false,pybind11::arg("depth")=0,
		 pybind11::return_value_policy::reference_internal );
	
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

	pybind11::class_<BaseProperty, std::shared_ptr<BaseProperty>>(m, "Property");

	def_prop<Accent>(m);
	def_prop<AntiCommuting>(m);
	def_prop<AntiSymmetric>(m);
	def_prop<Coordinate>(m);
	def_prop<Commuting>(m);
	def_prop<CommutingAsProduct>(m);
	def_prop<CommutingAsSum>(m);
	def_prop<DAntiSymmetric>(m);
	def_prop<Depends>(m);
	def_prop<Derivative>(m);
	def_prop<Diagonal>(m);
	def_prop<DifferentialForm>(m);
	def_prop<Distributable>(m);
	def_prop<DiracBar>(m);
	def_prop<EpsilonTensor>(m);
	def_prop<ExteriorDerivative>(m);
	def_prop<FilledTableau>(m);
	def_prop<GammaMatrix>(m);
	def_prop<ImaginaryI>(m);	
	def_prop<ImplicitIndex>(m);	
	def_prop<IndexInherit>(m);
	def_prop<Indices>(m);	
	def_prop<Integer>(m);
	def_prop<InverseMetric>(m);
	def_prop<KroneckerDelta>(m);
	def_prop<LaTeXForm>(m);
	def_prop<Matrix>(m);
	def_prop<Metric>(m);
	def_prop<NonCommuting>(m);
	def_prop<NumericalFlat>(m);
	def_prop<PartialDerivative>(m);
	def_prop<RiemannTensor>(m);
	def_prop<SatisfiesBianchi>(m);
	def_prop<SelfAntiCommuting>(m);
	def_prop<SelfCommuting>(m);
	def_prop<SelfNonCommuting>(m);
	def_prop<SortOrder>(m);
	def_prop<Spinor>(m);
	def_prop<Symbol>(m);
	def_prop<Symmetric>(m);
	def_prop<Tableau>(m);
	def_prop<TableauSymmetry>(m);
	def_prop<Traceless>(m);
	def_prop<Vielbein>(m);
	def_prop<InverseVielbein>(m);		
	def_prop<Weight>(m);
	def_prop<WeightInherit>(m);
	def_prop<WeylTensor>(m);

	// Register exceptions.
	
	pybind11::register_exception<ConsistencyException>(m, "ConsistencyException");
	pybind11::register_exception<ArgumentException>(m, "ArgumentException");
	pybind11::register_exception<ParseException>(m, "ParseException");
	pybind11::register_exception<RuntimeException>(m, "RuntimeException");
	pybind11::register_exception<NonScalarException>(m, "NonScalarException");
	pybind11::register_exception<InternalError>(m, "InternalError");
	pybind11::register_exception<NotYetImplemented>(m, "NotYetImplemented");
	}

std::string replace_all(std::string str, const std::string& old, const std::string& new_s)
   {
   if(!old.empty()){
	   size_t pos = str.find(old);
	   while ((pos = str.find(old, pos)) != std::string::npos) {
		   str=str.replace(pos, old.length(), new_s);
		   pos += new_s.length();
		   }
	   }
    return str;
   }
