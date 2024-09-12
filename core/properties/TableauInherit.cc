

#include "properties/TableauInherit.hh"
#include "Exceptions.hh"

using namespace cadabra;

// #define DEBUG 1

TableauInherit::~TableauInherit()
	{
	}

unsigned int TableauInherit::size(const Properties& properties, Ex& tr, Ex::iterator it) const
	{
//	it=properties.head<TableauInherit>(it);
#ifdef DEBUG
	std::cerr << "TableauInherit::size" << std::endl;
#endif

	int ret=0;
	Ex::sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it) && sib->is_index()) ++sib;
	if(sib!=tr.end(it)) {
		const TableauBase *tb=properties.get<TableauBase>(sib);
		if(tb)
			ret+=tb->size(properties, tr,sib);
		}
	
#ifdef DEBUG
	std::cerr << "TableauInherit::size: tab size = " << ret << std::endl;
#endif
	
	return ret;
	}

TableauBase::tab_t TableauInherit::get_tab(const Properties& properties, Ex& tr, Ex::iterator it, unsigned int num) const
	{
//	it=properties.head<TableauInherit>(it);

#ifdef DEBUG
	std::cerr << "TableauInherit::get_tab: " << it << std::endl;
#endif

	// Some algorithms call this without first calling `size`, so we
	// have to safeguard against that.
	if(size(properties, tr, it)==0)
		throw InternalError("TableauInherit::get_tab called with incorrect index.");
	
	//	std::cout << *it->name << " is Derivative" << std::endl;
	//	tr.print_recursive_treeform(std::cout, it);

	bool indices_first=tr.begin(it)->is_index();
	Ex::sibling_iterator argnode=tr.begin(it);
	unsigned int number_of_indices=0; // number of indices before the argument
	while(argnode->is_index()) {
		//		std::cout << *argnode->name << std::endl;
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

#ifdef DEBUG
	std::cerr << "TableauInherit::get_tab: offset indices by " << number_of_indices << std::endl;
#endif

	
	// symmetry of the argument on which \diff acts
	//		txtout << "computing rettab" << std::endl;

	//	std::cout << *argnode->name << std::endl;

	const TableauBase *tb=properties.get<TableauBase>(argnode);
	if(!tb) {
		return TableauBase::tab_t(); // empty tableau
		}
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
