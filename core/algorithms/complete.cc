
#include "algorithms/complete.hh"
#include "Functional.hh"
#include "properties/Metric.hh"
#include "properties/InverseMetric.hh"
#include "properties/Determinant.hh"
#include "properties/Trace.hh"
#include "SympyCdb.hh"

using namespace cadabra;

complete::complete(const Kernel& k, Ex& e, Ex& g)
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

	iterator bg=goal.begin();

	const InverseMetric *invmetric = kernel.properties.get<InverseMetric>(bg);
	if(invmetric) {
		Ex metric(bg);
		Ex::iterator ind1=metric.child(metric.begin(), 0);
		Ex::iterator ind2=metric.child(metric.begin(), 1);
		ind1->flip_parent_rel();
		ind2->flip_parent_rel();		
		
      sympy::invert_matrix(kernel, metric, tr, bg);
		res = result_t::l_applied;
		}
	const Determinant *det = kernel.properties.get<Determinant>(bg);
	if(det) {
		Ex metric(det->obj);
		sympy::determinant(kernel, metric, tr, bg);
		res = result_t::l_applied;
		}
	const Trace *trace = kernel.properties.get<Trace>(bg);
	if(trace) {
		Ex metric(trace->obj);
		sympy::trace(kernel, metric, tr, bg);
		res = result_t::l_applied;
		}

	return res;
	}
