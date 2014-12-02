
#include "algorithms/drop_weight.hh"

drop_keep_weight::drop_keep_weight(Kernel& k, exptree& tr)
	: Algorithm(k, tr)
	{
	}


// This algorithm acts on nodes which have Weight or inherit Weight.
// It only acts when the parent does _not_ have or inherit weight.
// This makes sure that we act on sums and products which are not
// themselves terms or factors in a sum or product.

bool drop_keep_weight::can_apply(iterator st)
	{
	if(number_of_args()!=2) return false;
	sibling_iterator argit=args_begin();
	label=*argit->name;
	++argit;
	weight=*argit->multiplier;


	const WeightInherit *gmnpar=0;
	const Weight        *wghpar=0;

	gmn=properties::get_composite<WeightInherit>(st, label);
	wgh=properties::get_composite<Weight>(st, label);
	gmnpar=properties::get_composite<WeightInherit>(tr.parent(st), label);
	wghpar=properties::get_composite<Weight>(tr.parent(st), label);

//	txtout << *st->name << ": " << gmn << ", " << wgh << ", " << gmnpar << " " << std::endl;
	if(gmn!=0 || wgh!=0) {
		bool ret = (gmnpar==0 && wghpar==0);
		return ret;
		}

	return false;
	}

algorithm::result_t drop_keep_weight::do_apply(iterator& it, bool keepthem)
	{
	if(gmn) {
		if(gmn->combination_type==WeightInherit::multiplicative) {
			if((keepthem==true && weight!=gmn->value(it, label)) || (keepthem==false && weight==gmn->value(it, label))) {
				expression_modified=true;
				zero(it->multiplier);
				}
			}
		else {
			sibling_iterator sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				const WeightBase *gnb=properties::get_composite<WeightBase>(sib, label);
				if(gnb) {
					multiplier_t val;
					bool no_val=false;
					try {
						val=gnb->value(sib, label);
//						txtout << *sib->name << " has weight " << val << std::endl;
						}
					catch(WeightInherit::weight_error& we) {
//						txtout << *sib->name << " has undeterminable weight " << std::endl;
						// If we cannot determine the weight of this term because this is a sum of
						// terms with different weights: keep when in @drop, drop when in @keep.
						no_val=true;
						}
					if( (no_val==false && ( (keepthem==true && weight!=val) || (keepthem==false && weight==val) ) ) 
						 || (no_val==true && keepthem==true) ) {
						expression_modified=true;
						sib=tr.erase(sib);
						}
					else ++sib;
					}
				else {
					if( (keepthem==true && weight!=0) || (keepthem==false && weight==0) ) {
						expression_modified=true;
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
			}
		}
	else {
		assert(wgh);
		if((keepthem==true && weight!=wgh->value(it, label)) || (keepthem==false && weight==wgh->value(it, label))) {
			expression_modified=true;
			zero(it->multiplier);
			}
		}
	
	cleanup_expression(tr, it);

	return l_applied;
	}

