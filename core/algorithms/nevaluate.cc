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
	NTensor ev = evaluator.evaluate();

	// Now we need to insert the NTensor into the tree.
	
	return result_t::l_applied;
	}
