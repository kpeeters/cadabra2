#include "nevaluate.hh"
#include "NEvaluator.hh"

using namespace cadabra;

nevaluate::nevaluate(const Kernel& k, Ex& tr, const std::vector<std::pair<Ex, NTensor>>& values_)
	: Algorithm(k, tr), values(values_)
	{
	}

bool nevaluate::can_apply(iterator it)
	{
	return true;
	}

Algorithm::result_t nevaluate::apply(iterator& it)
	{
	result_t res = result_t::l_no_action;

	NEvaluator evaluator(*it);

	for(const auto& var: values) {
		evaluator.set_variable(var.first, var.second);
		}

	return res;
	}
