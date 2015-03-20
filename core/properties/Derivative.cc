
#include "properties/Derivative.hh"
#include "Props.hh"

unsigned int Derivative::size(const Properties& properties, exptree& tr, exptree::iterator it) const
	{
	it=properties.head<Derivative>(it);

	int ret=0;
	exptree::sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end() && sib->is_index()) ++sib;
	const TableauBase *tb=properties.get<TableauBase>(sib);
	if(tb)
		ret+=tb->size(properties, tr,sib);
	return ret;
	}

multiplier_t Derivative::value(const Properties& properties, exptree::iterator it, const std::string& forcedlabel) const
	{
//	txtout << "!?!?" << std::endl;
	multiplier_t ret=0;

	exptree::sibling_iterator sib=it.begin();
	while(sib!=it.end()) {
		const WeightBase *gnb=properties.get_composite<WeightBase>(sib, forcedlabel);
		if(gnb) {
			multiplier_t tmp=gnb->value(properties, sib, forcedlabel);
			if(sib->is_index()) ret-=tmp;
			else                ret+=tmp;
//			txtout << *sib->name << " = " << tmp << std::endl;
			}
		++sib;
		}
	return ret;
	}

TableauBase::tab_t Derivative::get_tab(const Properties& properties, exptree& tr, exptree::iterator it, unsigned int num) const
	{
	it=properties.head<Derivative>(it);

	bool indices_first=tr.begin(it)->is_index();
	exptree::sibling_iterator argnode=tr.begin(it);
	unsigned int number_of_indices=0; // number of indices before the argument
	while(argnode->is_index()) { 
		std::cout << *argnode->name << std::endl;
		++argnode; 
		++number_of_indices; 
		}

	// Right now we only propagate information of a child node if it does
	// not contain a sum or product. FIXME: should handle more general info?
	// (note: this should, if at all, be handled by the product node which should
	// inherit TableauSymmetry and collect info from below, but this will make
	// still make things tricky when it comes to object exchange).

	// FIXME: should really use index iterators
//	unsigned int arg_indices=tr.number_of_children(argnode);
//	txtout << "for : " << *it->name << std::endl;
//	txtout << "indices first " << indices_first << std::endl;
//	txtout << arg_indices << " indices on argument" << std::endl;
//	txtout << number_of_indices << " direct indices" << std::endl;


   // symmetry of the argument on which \diff acts
//		txtout << "computing rettab" << std::endl;

	std::cout << *argnode->name << std::endl;

	const TableauBase *tb=properties.get<TableauBase>(argnode);
	assert(tb);
	unsigned int othertabs=tb->size(properties, tr, argnode);
	assert(num<othertabs);
	TableauBase::tab_t rettab=tb->get_tab(properties, tr, argnode, num);
	if(indices_first) { // have to renumber the tableau
		for(unsigned int rows=0; rows<rettab.number_of_rows(); ++rows)
			for(unsigned int cols=0; cols<rettab.row_size(rows); ++cols) {
				rettab(rows,cols)+=number_of_indices;
//				txtout << "C " << rows << "," << cols << ": " << rettab(rows,cols) << std::endl;
				}
		}
	return rettab;
	}

std::string Derivative::name() const
	{
	return "Derivative";
	}

