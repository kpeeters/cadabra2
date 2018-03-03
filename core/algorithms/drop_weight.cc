
#include "algorithms/drop_weight.hh"
#include "Cleanup.hh"
#include "Props.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"

using namespace cadabra;

drop_keep_weight::drop_keep_weight(const Kernel& k, Ex& tr, Ex& a)
	: Algorithm(k, tr), arg(a)
	{
	if(arg.begin()==arg.end()) 
		throw ArgumentException("drop_keep_weight: need 'weight=rational' argument.");

	if(Ex::number_of_children(arg.begin())!=2)
		throw ArgumentException("drop_keep_weight: need 'weight=rational' argument.");

	if(*(arg.begin()->name)!="\\equals")
		throw ArgumentException("drop_keep_weight: need 'weight=rational' argument.");
	}


// This algorithm acts on nodes which have Weight or inherit Weight.
// It only acts when the parent does _not_ have or inherit weight.
// This makes sure that we act on sums and products which are not
// themselves terms or factors in a sum or product.

bool drop_keep_weight::can_apply(iterator st)
	{
	sibling_iterator argit=arg.begin(arg.begin());
	label=*argit->name;
	++argit;
	weight=*argit->multiplier;
	
	// std::cerr << "dk: " << label << " = " << weight << std::endl;

	const WeightInherit *gmnpar=0;
	const Weight        *wghpar=0;

	gmn=kernel.properties.get_composite<WeightInherit>(st, label);
	wgh=kernel.properties.get_composite<Weight>(st, label);
	if(tr.is_head(st)==false) {
		gmnpar=kernel.properties.get_composite<WeightInherit>(tr.parent(st), label);
		wghpar=kernel.properties.get_composite<Weight>(tr.parent(st), label);
		}

	// std::cerr << *st->name << ": " << gmn << ", " << wgh << ", " << gmnpar << " " << std::endl;
	if(gmn!=0 || wgh!=0) {
		bool ret = (gmnpar==0 && wghpar==0);
		// std::cerr << "can_apply " << ret << std::endl;
		return ret;
		}

	return false;
	}

Algorithm::result_t drop_keep_weight::do_apply(iterator& it, bool keepthem)
	{
	Algorithm::result_t res=result_t::l_no_action;

	if(gmn) {
		if(gmn->combination_type==WeightInherit::multiplicative) {
			if((keepthem==true && weight!=gmn->value(kernel, it, label)) || 
				(keepthem==false && weight==gmn->value(kernel, it, label))) {
				zero(it->multiplier);
				}
			}
		else {
			sibling_iterator sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				const WeightBase *gnb=kernel.properties.get_composite<WeightBase>(sib, label);
				if(gnb) {
					// std::cerr << "WeightBase for child " << Ex(sib) << std::endl;
					multiplier_t val;
					bool no_val=false;
					try {
						val=gnb->value(kernel, sib, label);
						// std::cerr << *sib->name << " has weight " << val << std::endl;
						}
					catch(WeightInherit::WeightException& we) {
						// std::cerr << *sib->name << " has undeterminable weight " << std::endl;
						// If we cannot determine the weight of this term because this is a sum of
						// terms with different weights: keep when in @drop, drop when in @keep.
						no_val=true;
						}
					if( (no_val==false && ( (keepthem==true && weight!=val) || (keepthem==false && weight==val) ) ) 
						 || (no_val==true && keepthem==true) ) {
						sib=tr.erase(sib);
						}
					else ++sib;
					}
				else {
					// std::cerr << "NO WeightBase for child " << Ex(sib) << std::endl;
					if( (keepthem==true && weight!=0) || (keepthem==false && weight==0) ) {
						sib=tr.erase(sib);
						}
					else ++sib;
					}
				}
			if(tr.number_of_children(it)==0)
				zero(it->multiplier);
			else if(tr.number_of_children(it)==1) {
				tr.flatten(it);
				it=tr.erase(it);
				}
			res=result_t::l_applied;
			}
		}
	else {
		assert(wgh);
		if((keepthem==true && weight!=wgh->value(kernel, it, label)) || 
			(keepthem==false && weight==wgh->value(kernel, it, label))) {
			res = result_t::l_applied;
			zero(it->multiplier);
			}
		}
	
	cleanup_dispatch(kernel, tr, it);

	return res;
	}

drop_weight::drop_weight(const Kernel& k, Ex& e, Ex& a)
	: drop_keep_weight(k, e, a)
	{
	}

Algorithm::result_t drop_weight::apply(iterator& it)
	{
	return do_apply(it, false);
	}

keep_weight::keep_weight(const Kernel& k, Ex& e, Ex& a)
	: drop_keep_weight(k, e, a)
	{
	}

Algorithm::result_t keep_weight::apply(iterator& it)
	{
	return do_apply(it, true);
	}
