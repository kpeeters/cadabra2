
#include "Compare.hh"
#include "NEvaluator.hh"
#include "NInterpolatingFunction.hh"
#include "Exceptions.hh"
#include <cmath>

using namespace cadabra;


// void NEvaluator::find_common_subexpressions(std::vector<Ex *>)
// 	{
// 	// Compute the hash value of every subtree, and collect matches.
// 	// Then compare subtrees with equal hash to find common subtrees.
// 	}

// #define DEBUG __FILE__
#include "Debug.hh"

constexpr double const_pi = 3.14159265358979323846;

NEvaluator::NEvaluator()
	{
	}

NEvaluator::NEvaluator(Ex::iterator ex_it)
	: ex(ex_it)
	{
	}

void NEvaluator::set_function(Ex::iterator ex_it)
	{
	ex = Ex(ex_it);
	}

void NEvaluator::set_lookup_function(lookup_function_t f)
	{
	lookup_function = f;
	}

NTensor NEvaluator::evaluate()
	{
	find_variable_locations();

	// The vector below pairs LaTeX strings which can appear in the
	// cadabra input to function names in the C++ standard library.

	using complex_func = std::complex<double>(*)(const std::complex<double>&);
	// const double eps = 1e-10;
	
	const std::vector<std::pair<nset_t::iterator, complex_func>> elementary
		= { // Trigonometric functions.

		{ name_set.find("\\sin"), std::sin<double> },
		{ name_set.find("\\cos"), std::cos<double> },
		{ name_set.find("\\tan"), std::tan<double> },

		{ name_set.find("\\csc"), [](const std::complex<double>& x) { return 1.0/std::sin<double>(x); } },
		{ name_set.find("\\sec"), [](const std::complex<double>& x) { return 1.0/std::cos<double>(x); } },
		{ name_set.find("\\cot"), [](const std::complex<double>& x) { return std::cos<double>(x)/std::sin<double>(x); } },

		{ name_set.find("\\arcsin"), std::asin<double>},
		{ name_set.find("\\arccos"), std::acos<double>},
		{ name_set.find("\\arctan"), std::atan<double>},
		
		// Hyperbolic functions.
		
		{ name_set.find("\\sinh"), std::sinh<double>},
		{ name_set.find("\\cosh"), std::cosh<double>},
		{ name_set.find("\\tanh"), std::tanh<double>},
		
		{ name_set.find("\\csch"), [](const std::complex<double>& x) { return 1.0/std::sinh<double>(x); } },
		{ name_set.find("\\sech"), [](const std::complex<double>& x) { return 1.0/std::cosh<double>(x); } },
		{ name_set.find("\\coth"), [](const std::complex<double>& x) { return std::cosh<double>(x)/std::sinh<double>(x); } },
		
		{ name_set.find("\\arcsinh"), std::asinh<double>},
		{ name_set.find("\\arccosh"), std::acosh<double>},
		{ name_set.find("\\arctanh"), std::atanh<double>},

		// Logarithmic and exponential functions.
		
		{ name_set.find("\\log"),     std::log10<double>},
		{ name_set.find("\\log2"),    [](const std::complex<double>& x) { return std::log<double>(x) / std::log<double>(2.0); } },
		{ name_set.find("\\ln"),      std::log<double>  },
		{ name_set.find("\\exp"),     std::exp<double>  },
		{ name_set.find("\\sqrt"),    std::sqrt<double> },

		// Real and imaginary parts.

		{ name_set.find("\\Re"),      [](const std::complex<double>& x) { return std::complex<double>(x.real(), 0); } },
		{ name_set.find("\\Im"),      [](const std::complex<double>& x) { return std::complex<double>(x.imag(), 0); } },

		// Steps, absolute values and so on.
		
		{ name_set.find("\\abs"),    [](const std::complex<double>& x) { return std::complex<double>(std::abs<double>(x), 0.0); } } ,
//		{ name_set.find("\\floor"),  [eps](const std::complex<double>& x) {
//			if(std::abs(x.imag()) > eps)
//				throw ConsistencyException("Function floor called on complex number.");
//			return std::complex<double>(std::floor(x.real()), 0.0);
//			} },
		{ name_set.find("\\sign"),   [](const std::complex<double>& x) {
			if(x.real()==0)     return  std::complex<double>(0.0,  0.0);
			else if(x.real()<0) return  std::complex<double>(-1.0, 0.0);
			else                return  std::complex<double>(1.0,  0.0);
			} }
	}; 

	const auto n_pow  = name_set.find("\\pow");
	const auto n_prod = name_set.find("\\prod");
	const auto n_sum  = name_set.find("\\sum");
	const auto n_pi   = name_set.find("\\pi");
	const auto n_bigO = name_set.find("\\bigO");

	NTensor lastval(0);

	auto it = ex.begin_post();
	while(it != ex.end_post()) {

		// First check interpolating functions.
		
		if(std::holds_alternative<std::shared_ptr<NInterpolatingFunction>>(it->content)) {
			auto nif = std::get<std::shared_ptr<NInterpolatingFunction>>(it->content);
			if(lastval.is_real()==false)
				throw ConsistencyException("NEvaluator: cannot feed complex number to NInterpolatingFunction.");

			lastval = variable_values[0].values.broadcast(fullshape, 0);
#ifdef DEBUG
			std::cerr << "Evaluating interpolating function for " << lastval.values.size() << " values" << std::endl;
#endif
			// Feed all values in the NTensor lastval through the interpolating function.
			for(size_t i=0; i<lastval.values.size(); ++i) {
				lastval.values[i] = nif->evaluate(lastval.values[i].real());
				}
			subtree_values.insert(std::make_pair(it, lastval) );
			++it;
			continue;
			}

		// Either this node is known in the subtree value map,
		// or this node is a function which combines the values
		// of child nodes.

		auto fnd = subtree_values.find(Ex::iterator(it));
		if(fnd!=subtree_values.end()) {
#ifdef DEBUG
			std::cerr << it << " has value " << fnd->second << std::endl;
			// Note: the above will, for a scalar value, display the
			// broadcast value for `fullshape` as computed below
			// in `find_variable_locations`; so [[[1]]] is perfectly
			// normal if there are three variables defined.
#endif
			lastval  = fnd->second;
			}
		else {
			bool found_elementary=false;
			if(it->is_rational() || it->is_double()) {
				lastval = NTensor(to_double(*it->multiplier));
				found_elementary=true;
				}
			else {
				for(const auto& el: elementary) {
					if(it->name == el.first) {
						auto arg    = ex.begin(it);
#ifdef DEBUG
						std::cerr << *el.first << " has " << ex.number_of_children(it) << " child nodes" << std::endl;
						std::cerr << "need to apply " << *el.first << " to " << arg << std::endl;
#endif
						auto argit  = subtree_values.find(arg);
						auto argval = NTensor(argit->second);
#ifdef DEBUG
						std::cerr << " argument equals " << argval
									 << "; stored had multiplier " << *(argit->first->multiplier) << std::endl;
#endif
						// Any expressions are stored without multiplier, so we
						// now need to first multiply-through with the current
						// multiplier.
						argval  *= to_double((*arg->multiplier)/(*argit->first->multiplier));
						lastval  = argval.apply(el.second);
						lastval *= to_double(*it->multiplier);
						found_elementary=true;
						break;
						}
					}
				}
			
			if(found_elementary==false) {
				if(it->name==n_prod) {
					for(auto cit = ex.begin(it); cit!=ex.end(it); ++cit) {
						auto cfnd = subtree_values.find(Ex::iterator(cit));
						if(cfnd==subtree_values.end())
							throw std::logic_error("Inconsistent value tree.");
						if(cit==ex.begin(it))
							lastval = cfnd->second;
						else
							lastval *= cfnd->second;
						}
					lastval *= to_double(*it->multiplier);
					}
				else if(it->name==n_sum) {
					for(auto cit = ex.begin(it); cit!=ex.end(it); ++cit) {
						auto cfnd = subtree_values.find(Ex::iterator(cit));
						if(cfnd==subtree_values.end())
							throw std::logic_error("Inconsistent value tree.");
						
						if(cit==ex.begin(it)) 
							lastval = cfnd->second;
						else
							lastval += cfnd->second;
						}
					lastval *= to_double(*it->multiplier);
					}
				else if(it->name==n_pow) {
					auto cit1  = Ex::begin(it);
					auto cit2  = cit1;
					++cit2;
					
					auto cfnd1 = subtree_values.find(Ex::iterator(cit1));
					auto cfnd2 = subtree_values.find(Ex::iterator(cit2));
					if(cfnd1==subtree_values.end() || cfnd2==subtree_values.end())
						throw std::logic_error("Inconsistent value tree at exponentiation node.");

					lastval = cfnd1->second.pow( cfnd2->second );
					lastval *= to_double(*it->multiplier);
					// throw std::logic_error("Value unknown for subtree special function.");
					}
				else if(it->name==n_pi) {
#ifdef DEBUG
					std::cerr << "found \\pi" << std::endl;
#endif 
					lastval  = NTensor(const_pi * to_double(*it->multiplier));
					}
				else if(it->name==n_bigO) {
					lastval = NTensor(0);
					}
				else {
					// Try variable substitution rules.
					bool found=false;
					for(const auto& var: variable_values) {
						// std::cerr << "Comparing " << var.first << " with " << *it << std::endl;
						Ex no_multiplier(it);
						auto mult = *it->multiplier;
						one( no_multiplier.begin()->multiplier );
						no_multiplier.begin()->fl.parent_rel = str_node::p_none;
						if(var.variable == no_multiplier) {
#ifdef DEBUG
							std::cerr << "found " << *(no_multiplier.begin()->name) << " with multiplier "
										 << mult << std::endl;
#endif
							lastval = var.values;
							lastval *= to_double(mult);
							subtree_values.insert(std::make_pair(it, lastval) );
							// std::cerr << "We know the value of " << *it << std::endl;
							found=true;
							break;
							}
						}

					// Is this perhaps a string representing a float?
					if(!found) {
						try {
							lastval = NTensor(std::stod(*it->name));
							subtree_values.insert(std::make_pair(it, lastval));
							}
						catch(const std::invalid_argument& err) {
							// Last resort: lookup value using the external lookup function.
							try {
								if(lookup_function)
									lastval = NTensor(lookup_function(Ex(it)));
								else 
									throw std::logic_error("Value unknown for subtree with head "+(*it->name)+".");
								}
							catch(const std::exception& ex) {
								throw std::logic_error("Value unknown for subtree with head "+(*it->name)+".");
								}
							}
						catch(const std::out_of_range& err) {
							throw std::logic_error("Value "+(*it->name)+" does not fit in a float.");
							}
						}
					}
				}
			subtree_values.insert(std::make_pair(it, lastval));
			}

		++it;
		}

	DEBUGLN( if(lastval.shape.size()==1 && lastval.shape[0]==1)
					std::cerr << "lastval needs broadcasting, shape[0] = " << lastval.shape[0] << std::endl; );

	if(lastval.is_scalar()) {
		// The evaluation code above never pulled in any of the variables,
		// so the numerical value was never broadcast to the shape of the
		// output that we want. Do that now.
		lastval = NTensor(fullshape, lastval.at());
		}
		
	DEBUGLN( std::cerr << "evaluate returns " << lastval << std::endl; );

	return lastval;
	}

void NEvaluator::set_variable(const Ex& var, const NTensor& val)
	{
	// Ensure that we only store one entry in variable_values for
	// every variable.

	if(*var.begin()->multiplier != 1)
		throw ConsistencyException("NEvaluator::set_variable: variables should not have multipliers.");
	
	auto it = variable_values_locs.find(var);
	if(it == variable_values_locs.end()) {
		// Create new entry.
		variable_values_locs[var] = variable_values.size();
		variable_values.push_back( VariableValues({var, NTensor(val), {} }) );
		}
	else {
		// Overwrite if the variable was already present.
		variable_values[it->second] = VariableValues({var, NTensor(val), {} });
		}
	}

void NEvaluator::find_variable_locations()
	{
#ifdef DEBUG
	for(auto& var: variable_values) {
		std::cerr << "variable " << var.variable << std::endl;
		std::cerr << "values ";
		for(const auto& val: var.values.values)
			std::cerr << val << ", ";
		std::cerr << std::endl;
		}
#endif
	// FIXME: we don't really need this anymore, as we do everything
	// with broadcasting. Well, not quite true. We do use the
	// subtree_values below to fetch the value of variables, as
	// subtree_values is the place where these have been broadcast
	// to the full shape. 
	for(auto& var: variable_values) {
		var.locations.clear();
		auto it = ex.begin_post();
		while(it != ex.end_post()) {
			// FIXME: also find variables in NInterpolatingFunctions.
			Ex no_multiplier(it);
			auto mult = *it->multiplier;
			one( no_multiplier.begin()->multiplier );
			no_multiplier.begin()->fl.parent_rel = str_node::p_none;
			if(var.variable == no_multiplier)
				var.locations.push_back(it);
			++it;
			}
		// std::cerr << "Variable " << var.variable << " at " << var.locations.size() << " places" << std::endl;
		}

	// Now insert subtree values which are such that for every
	// variable node we have an NTensor which is broadcast to the
	// shape of the full variable set NTensor.

	fullshape.clear();
	for(const auto& var: variable_values) {
		if(var.values.shape.size()!=1)
			throw ConsistencyException("NTensor: all variables should be specified as one-dimensional arrays.");

		fullshape.push_back(var.values.shape[0]);
		// std::cerr << var.values.shape[0] << ", ";
		}
	// std::cerr << std::endl;

	subtree_values.clear();
	for(size_t v=0; v<variable_values.size(); ++v) {
		const auto& var = variable_values[v];
		for(const auto& it: var.locations) {
			auto bc_val = var.values.broadcast( fullshape, v );
			bc_val *= to_double(*it->multiplier);
			subtree_values.insert(std::make_pair(it, bc_val ) );
			// std::cerr << bc_val << std::endl;
			}
		}

	// std::cerr << "ready" << std::endl;
	}
