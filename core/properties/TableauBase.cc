
#include "properties/TableauBase.hh"

int TableauBase::get_indexgroup(const Properties& pr, exptree& tr, exptree::iterator it, int indexnum) const
	{
	const TableauBase *pd;
	for(;;) {
		pd=pr.get<TableauBase>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		} 
//	txtout << "now at " << *it->name << std::endl;

	unsigned int siz=size(pr, tr, it);
	assert(siz==1); // FIXME: does not work yet for multi-tab symmetries
	tab_t tmptab=get_tab(pr, tr, it, 0);
//	debugout << "searching indexgroup for " << *it->name <<  std::endl;
	if(tmptab.number_of_rows()==1) return 0;

	std::pair<int,int> loc=tmptab.find(indexnum);
//	debugout << "searching indexgroup " << loc.second << std::endl;
	assert(loc.first!=-1);
	return loc.second;
	}

bool TableauBase::is_simple_symmetry(const Properties& pr, exptree& tr, exptree::iterator it) const
	{
	const TableauBase *pd;
	for(;;) {
		pd=pr.get<TableauBase>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		} 

	for(unsigned int i=0; i<size(pr, tr, it); ++i) {
		tab_t tmptab=get_tab(pr, tr, it, i);
		if((tmptab.number_of_rows()==1 || tmptab.row_size(0)==1) && tmptab.selfdual_column==0)
			return true;
		}
	return false;
	}

