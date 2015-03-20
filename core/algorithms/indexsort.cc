
#include "IndexIterator.hh"
#include "properties/TableauSymmetry.hh"
#include "algorithms/indexsort.hh"

indexsort::indexsort(Kernel& k, exptree& tr) 
	: Algorithm(k, tr), tb(0)
	{
	}

bool indexsort::can_apply(iterator st)
	{
	if(number_of_indices(kernel.properties, st)<2) return false;
	tb=kernel.properties.get<TableauBase>(st);
	if(tb) return true;
	return false;
	}

indexsort::less_indexed_treenode::less_indexed_treenode(Kernel& k, exptree& t, iterator i)
	: kernel(k), tr(t), it(i)
	{
	}

bool indexsort::less_indexed_treenode::operator()(unsigned int i1, unsigned int i2) const
	{
	return subtree_exact_less(&kernel.properties, 
									  index_iterator::begin(kernel.properties, it, i1), 
									  index_iterator::begin(kernel.properties, it,i2) );
	}

Algorithm::result_t indexsort::apply(iterator& st)
	{
//	txtout << "indexsort acting on " << *st->name << std::endl;
//	txtout << properties::get<TableauBase>(st) << std::endl;
	
	result_t res=result_t::l_no_action;
	exptree backup(st);

	for(unsigned int i=0; i<tb->size(kernel.properties, tr, st); ++i) {
		TableauSymmetry::tab_t tmptab(tb->get_tab(kernel.properties, tr, st, i));
		TableauSymmetry::tab_t origtab(tmptab);
		less_indexed_treenode comp(kernel,tr,st);
		tmptab.canonicalise(comp, false); // KP: why is this here? tb->only_column_exchange());
		TableauSymmetry::tab_t::iterator it1=origtab.begin();
		TableauSymmetry::tab_t::iterator it2=tmptab.begin();
		while(it2!=tmptab.end()) {
			if(*it1!=*it2) {
				tr.replace_index(index_iterator::begin(kernel.properties,st,*it1), 
									  index_iterator::begin(kernel.properties,backup.begin(),*it2) );
//				tr.tensor_index(st,*it1)->multiplier=backup.tensor_index(backup.begin(),*it2)->multiplier;
				res = result_t::l_applied;
				}
			++it1;
			++it2;
			}
		if(*(tr.parent(st)->name)=="\\prod") {
			multiply(tr.parent(st)->multiplier, tmptab.multiplicity*origtab.multiplicity);
			pushup_multiplier(tr.parent(st));
			}
		else {
			multiply(st->multiplier, tmptab.multiplicity*origtab.multiplicity);
			pushup_multiplier(st);
			}
		}
	return res;
	}

