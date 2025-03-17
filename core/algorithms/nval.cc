#include "nval.hh"
#include "NEvaluator.hh"
#include "NIntegrator.hh"

using namespace cadabra;

nval::nval(const Kernel& k, Ex& tr, NEvaluator& ev)
	: Algorithm(k, tr), evaluator(ev)
	{
	}

bool nval::can_apply(iterator it)
	{
	if(*it->name=="\\int") return true;
	return false;
	}

Algorithm::result_t nval::apply(iterator& it)
	{
	if(*it->name=="\\int") {
		sibling_iterator arg=tr.begin(it);
		Ex igrand(arg);
		Ex range(++arg);

		// std::cerr << range << " is the range" << std::endl;
		NIntegrator ig(igrand.begin());
		ig.evaluator = evaluator;
		ig.evaluator.set_function(igrand.begin());
		
		sibling_iterator r=tr.begin(range.begin());
		Ex ivar(r);
		double from = to_double(*(++r)->multiplier);
		double to   = to_double(*(++r)->multiplier);

		// std::cerr << ivar << " is the ivar " << from << " - " << to << std::endl;
		ig.set_range(ivar, from, to);
		auto nt = std::make_shared<NTensor>( ig.integrate() );
		auto mult = *it->multiplier;
		node_one(it);
		
		(*nt) *= to_double(mult);
		it->content = nt;
		}
	
	return result_t::l_applied;
	}
