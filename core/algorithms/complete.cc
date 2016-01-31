
#include "algorithms/complete.hh"
#include "Functional.hh"
#include "properties/Metric.hh"
#include "properties/InverseMetric.hh"
#include "SympyCdb.hh"

complete::complete(Kernel& k, Ex& e, Ex& g)
	: Algorithm(k, e), goal(g)
	{
	}

bool complete::can_apply(iterator it) 
	{
	return true;
	}

Algorithm::result_t complete::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	iterator bg=goal.begin(goal.begin());

	const InverseMetric *invmetric = kernel.properties.get<InverseMetric>(bg);
	if(invmetric) {
		Ex metric(bg);
      sympy::invert_matrix(kernel, metric, tr);
		res = result_t::l_applied;
		}

	return res;
	}
