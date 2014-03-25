
#include "PreClean.hh"
#include "algorithms/reduce_sub.hh"

void pre_clean(Kernel& k, exptree& ex, exptree::iterator it)
	{
	ratrewrite rr(k, ex);
	rr.apply_recursive(it);
	reduce_sub rsub(k, ex);
	rsub.apply_recursive(it);
	}

ratrewrite::ratrewrite(Kernel& k, exptree& tr)
	: Algorithm(k, tr)
	{
	}

bool ratrewrite::can_apply(exptree::iterator st)
	{
	if(*st->name!="1" && st->is_unsimplified_rational() 
		/* && st->fl.parent_rel!=str_node::p_sub && st->fl.parent_rel!=str_node::p_super */) return true;
	else return false;
	}

Algorithm::result_t ratrewrite::apply(exptree::iterator& st)
	{
	multiplier_t num(*st->name);
	st->name=name_set.insert("1").first;
	multiply(st->multiplier,num);
	expression_modified=true;
	return l_applied;
	}

