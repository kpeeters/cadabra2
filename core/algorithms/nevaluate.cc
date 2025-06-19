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
	std::cerr << "About to call evaluator.evaluate()" << std::endl;
	auto result = evaluator.evaluate();
	std::cerr << "evaluate() returned successfully" << std::endl;
	it->content = std::make_shared<NTensor>( result );
	std::cerr << "NTensor created" << std::endl;
	node_one(it);
	
	return result_t::l_applied;
	}
