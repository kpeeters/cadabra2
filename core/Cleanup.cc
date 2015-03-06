
#include "Cleanup.hh"
#include "Algorithm.hh"
#include "algorithms/prodcollectnum.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/flatten_sum.hh"
#include "properties/Derivative.hh"

void cleanup_dispatch(Kernel& kernel, exptree& tr, exptree::iterator& it)
	{
	if(*it->name=="\\prod")            cleanup_productlike(kernel, tr, it);
	else if(*it->name=="\\sum")        cleanup_sumlike(kernel, tr, it);
	else if(*it->name=="\\expression") cleanup_expressionlike(kernel, tr, it);

	const Derivative *der = kernel.properties.get<Derivative>(it);
	if(der) cleanup_derivative(kernel, tr, it);
	}

void cleanup_productlike(Kernel& k, exptree&tr, exptree::iterator& it)
	{
	assert(*it->name=="\\prod");

	// Remove children which are 1
   // Collect all multipliers
	prodcollectnum pc(k, tr);
	pc.apply(it);
	}

void cleanup_sumlike(Kernel& k, exptree&tr, exptree::iterator& it)
	{
	assert(*it->name=="\\sum");

	// Flatten sums which are supposed to be flat.
	flatten_sum fs(k,tr);
	fs.apply(it);

	// Remove children which are 0
	exptree::sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(sib->is_zero())
			sib=tr.erase(sib);
		else
			++sib;
		}

   // Collect all equal terms.
	collect_terms ct(k, tr);
	ct.apply(it);
	}

void cleanup_expressionlike(Kernel& k, exptree&tr, exptree::iterator& it)
	{
	assert(*it->name=="\\expression");

	// Can only have one child which contains actual expression data;
	// all others are meta-data like \asymimplicit. If this child has
	// zero multiplier, simplify.

	exptree::sibling_iterator sib=tr.begin(it);
	if(sib->is_zero()) {
		// FIXME: duplicate of node_zero in Algorithm.
		tr.erase_children(sib);
		sib->name=name_set.insert("1").first;
		}
	}

void cleanup_derivative(Kernel& k, exptree& tr, exptree::iterator& it)
	{
	// Nested derivatives with the same name should be flattened.
std::cout << "cleanup_derivative" << std::endl;

if(tr.number_of_children(it)- Algorithm::number_of_indices(k.properties, it)==1) {
std::cout << "==1 " << *it->name << " " << *tr.begin(it)->name << std::endl;
		if(it->name == tr.begin(it)->name) {
			std::cout << "should flatten derivative" << std::endl;
			}
		}
	
	}

void cleanup_expression(exptree& tr)
	{
	exptree::iterator it=tr.begin();
	cleanup_expression(tr,it);
	}

void cleanup_expression(exptree& tr, exptree::iterator& it)
	{
	// V2: bring back to life

// //	txtout << "cleanup called on " << *it->name << std::endl;
// //	tr.print_recursive_treeform(txtout, it);
// //	txtout << "---" << std::endl;
// 	
// 	ratrewrite rr(tr, tr.end());
// 	rr.apply_recursive(it,false);
// 
// //	txtout << "before rsub" << std::endl;
// //	tr.print_recursive_treeform(txtout, it);
// //	txtout << "---" << std::endl;
// 
// 	reduce_sub rsub(tr, tr.end());
// 	rsub.apply_recursive(it, false);
// 
// //	txtout << "before rdiv" << std::endl;
// //	tr.print_recursive_treeform(txtout, it);
// //	txtout << "---" << std::endl;
// 
// 	reduce_div rdiv(tr, tr.end());
// 	rdiv.apply_recursive(it, false);
// 
// //	txtout << "after rdiv" << std::endl;
// //	tr.print_recursive_treeform(txtout, it);
// //	txtout << "---" << std::endl;
// 
// 	cleanup_sums_products(tr,it);
// 
// //	tr.print_recursive_treeform(txtout, it);
// //	txtout << "---" << std::endl;
// 
// //	cleanup_nests_below(tr, tr.begin());
// //	cleanup_nests(tr,it); // FIXME: enabling this is pointless as 'it' points to \expression
	}

void cleanup_sums_products(exptree& tr, exptree::iterator& it)
	{
// 	sumflatten sf(tr, tr.end());
// 	sf.make_consistent_only=true;
// 	sf.apply_recursive(it, false);
// 	prodflatten pf(tr, tr.end());
// 	pf.make_consistent_only=true;
// 	pf.apply_recursive(it, false);
// //	collect_terms ct(tr, tr.end()); // This collect_terms makes A+A input become 2*A automatically
// //	ct.apply_recursive(it,false);   // which goes against the spirit of cdb; now disabled.
// 	prodcollectnum pc(tr, tr.end());
// 	pc.apply_recursive(it,false);
// //	ct.apply_recursive(it,false);
	}

void cleanup_nests_below(exptree&tr, exptree::iterator it, bool ignore_bracket_types)
	{
 	if(!tr.is_valid(it)) return;
 	exptree::iterator now=it;
 	if(it==tr.end()) return;
 	exptree::iterator stop=now;
 	stop.skip_children();
 	++stop;
 	++now; // We are not allowed to touch the content at 'it', only content below it.
 
 	while(now!=stop) {
 		cleanup_nests(tr, now, ignore_bracket_types);
 // Iterators should always be valid when we return here, so this test is not required.	  
 //		if(tr.is_valid(now)==false)
 //			break;
 		++now;
 		}
	}

void cleanup_nests(exptree&tr, exptree::iterator &it, bool ignore_bracket_types)
	{
 	if(!tr.is_valid(it)) return;
 	if(!tr.is_valid(tr.parent(it))) return;

// //	tr.print_recursive_treeform(txtout, tr.begin());
 	if(*(it->name)=="\\prod") {
 		assert(tr.parent(it)!=tr.end());
		if(*(tr.parent(it)->name)=="\\prod" && (ignore_bracket_types || tr.begin(it)->fl.bracket==it->fl.bracket) ) {
			multiplier_t fac=*(tr.parent(it)->multiplier)*(*it->multiplier);
			tr.parent(it)->multiplier=rat_set.insert(fac).first;
			tr.flatten(it);
 			it=tr.erase(it);
			//         it=tr.parent(tr.erase(it)); // CHECK: OK?
			}
		return;
 		}

 	if(*(it->name)=="\\sum") {
 		assert(tr.parent(it)!=tr.end());
		//		txtout << "*** " << *tr.parent(it)->name << std::endl;
		//		txtout << tr.begin(it)->fl.bracket << " " << it->fl.bracket << std::endl;
 		if(*(tr.parent(it)->name)=="\\sum" && (ignore_bracket_types || tr.begin(it)->fl.bracket==it->fl.bracket) ) {
 			// WARNING, this is a copy of code in sumflatten!
 			exptree::sibling_iterator facs=tr.begin(tr.parent(it));
 			str_node::bracket_t btype_par=facs->fl.bracket;
 			exptree::sibling_iterator terms=tr.begin(it);
 			while(terms!=tr.end(it)) {
 				multiplier_t tfac=(*terms->multiplier)*(*it->multiplier);
 				terms->multiplier=rat_set.insert(tfac).first;
 				terms->fl.bracket=btype_par;
				++terms;
				}
 			tr.flatten(it);
 			// FIXME: this is dangerous:
 			it=tr.parent(tr.erase(it));
 			}
 		return;
 		}
// 	const PartialDerivative *der=properties::get<PartialDerivative>(it);
// 	if(der) {
// 		// take constants outside
// 		multiply(it->multiplier, *(tr.begin()->multiplier));
// 		one(tr.begin()->multiplier);
// 		// flatten nested diffs
// 		assert(tr.parent(it)!=tr.end());
// 		der=properties::get<PartialDerivative>(tr.parent(it));
// 		if(der && tr.parent(it)->name==it->name && tr.number_of_indices(it)>0) { 
// 			multiplier_t fac=*(tr.parent(it)->multiplier)*(*it->multiplier);
// 			tr.parent(it)->multiplier=rat_set.insert(fac).first;
// 			tr.flatten(it);
// 			it=tr.erase(it);
// //			it=tr.parent(it); // CHECK: OK?
// 			}
// 		return;
// 		}
	}
