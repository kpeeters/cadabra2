
#include "Functional.hh"
#include "algorithms/evaluate.hh"
#include <functional>

evaluate::evaluate(Kernel& k, Ex& tr, const Ex& ind_values, const Ex& components)
	: Algorithm(k, tr)
	{
	// Preparse the arguments.
	collect_index_values(ind_values);	
	prepare_replacement_rules(components);
	}

bool evaluate::can_apply(iterator) 
	{
	return true;
	}

Algorithm::result_t evaluate::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	// Descend down the tree.
	Ex::post_order_iterator walk=it;
	walk.descend_all();

	do {
		if(*(walk->name)=="\\sum")       handle_sum(walk);
		else if(*(walk->name)=="\\prod") handle_prod(walk);

		++walk;
		} while(walk!=it);

	return res;
	}


void evaluate::collect_index_values(const Ex& ind_values)
	{
	auto it=ind_values.begin(ind_values.begin());

	cadabra::do_list(ind_values, it, [&](Ex::iterator ind) {
			auto name=ind_values.begin(ind);
			sibling_iterator vals=name;
			++vals;
			const Indices *indprop = kernel.properties.get<Indices>(name);
			if(indprop==0)
				throw ArgumentException("evaluate: Index "+ *(name->name) + " does not have an Indices property");
			
			index_values[indprop]=cadabra::make_list(Ex(vals));
			});
	}

void evaluate::prepare_replacement_rules(const Ex& components)
	{
	// We need to be able to match A_{t t} to A_{m n} knowing that m,n
	// take values including t. In order to do this, we look at the
	// component replacement rule, and then do an inverse lookup from the 
	// component values that we see there to the possible index name.
	// Typically (when indices take values in numbers and we have more than
	// one index set) this gives multiple options from the index value to
	// the index that can take this value.

	std::map<Ex, const Indices *> index_candidates;
	
	cadabra::do_list(components, components.begin(), [&](Ex::iterator c) {
			index_map_t ind_free, ind_dummy;
			auto pat = components.begin(c);
			classify_indices(pat, ind_free, ind_dummy);
			std::cerr << "free indices in " << *pat->name << ": ";
			for(auto in: ind_free)
				std::cerr << *in.second->name << ", ";
			std::cerr << std::endl;
			});
	}

void evaluate::handle_sum(iterator it)
	{
	index_map_t ind_free, ind_dummy;

	// First find the values that all indices will need to take.
	classify_indices(it, ind_free, ind_dummy);
	for(auto i: ind_free) {
		const Indices *prop = kernel.properties.get<Indices>(i.second);
		if(prop==0)
			throw ArgumentException("evaluate: Index "+*(i.second->name)+" does not have an Indices property");

		auto rep=index_values.find(prop);
		if(rep==index_values.end())
			throw ArgumentException("evaluate: No values for index "+*(i.second->name)+" given");

		std::cerr << "index " << *(i.second->name) << " takes values ";
		auto listnode = rep->second.begin();
		sibling_iterator sib = rep->second.begin(listnode);
		while(sib!=rep->second.end(listnode)) {
			std::cerr << *sib->name << ", ";
			++sib;
			}
		std::cerr << std::endl;
		}
	
	// Then determine the component values of all terms. Instead of
	// looping over all possible index value sets, we look straight at
	// the substitution rules, and check that these are required for
	// some index values (this is where symmetry arguments should come
	// in as well).
	//
	// In the example, we see A_{m n} in the expression, and we see
	// that the rules for A_{t t} and A_{r r} match this tensor.
	
	
	}

void evaluate::handle_prod(iterator it)
	{
	}
