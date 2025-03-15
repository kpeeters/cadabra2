#include "nevaluate.hh"
#include "NEvaluator.hh"

using namespace cadabra;

nevaluate::nevaluate(const Kernel& k, Ex& tr, NEvaluator& ev)
	: Algorithm(k, tr), evaluator(ev)
	{
	}

bool nevaluate::can_apply(iterator it)
	{
	return true;
	}

Algorithm::result_t nevaluate::apply(iterator& it)
	{
	// Evaluate and store the result in the tree.
	it->content = std::make_shared<NTensor>( evaluator.evaluate() );
	node_one(it);
	
	return result_t::l_applied;
	}
