#include "nval.hh"
#include "NEvaluator.hh"
#include "NIntegrator.hh"
#include "Cleanup.hh"

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
		
		sibling_iterator r=tr.begin(range.begin());
		Ex ivar(r);
		// The integration range can be symbolic, so we need to first evaluate that
		evaluator.set_function(++r);
		auto nt_from = evaluator.evaluate();
		evaluator.set_function(++r);
		auto nt_to   = evaluator.evaluate();
		// std::cerr << nt_from.shape.size() << ", " << nt_to.shape.size() << std::endl;
		// std::cerr << nt_to.shape[0] << std::endl;
		// std::cerr << nt_to << std::endl;
		if(nt_from.shape.size()!=1 || nt_from.shape[0]!=1)
			throw ArgumentException("Lower integration limit is not a scalar.");
		if(nt_to.shape.size()!=1 || nt_to.shape[0]!=1)
			throw ArgumentException("Upper integration limit is not a scalar.");

		if(!nt_from.is_real())
			throw ArgumentException("Lower integration limit not real.");
		if(!nt_to.is_real())
			throw ArgumentException("Lower integration limit not real.");
											
		// Setup the integrator.
		NIntegrator ig(igrand.begin());
		ig.evaluator = evaluator;
		ig.evaluator.set_function(igrand.begin());

		// std::cerr << ivar << " is the ivar " << from << " - " << to << std::endl;
		ig.set_range(ivar, nt_from.at().real(), nt_to.at().real());
		auto nt = std::make_shared<NTensor>( ig.integrate() );
		auto mult = *it->multiplier;
		node_one(it);
		
		(*nt) *= to_double(mult);
		it->content = nt;

		cleanup_dispatch(kernel, tr, it);
		}
	
	return result_t::l_applied;
	}
