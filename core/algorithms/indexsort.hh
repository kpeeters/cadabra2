
#include "Algorithm.hh"
#include "properties/TableauBase.hh"

class indexsort : public Algorithm {
	public:
		indexsort(Kernel&, exptree&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		

		class less_indexed_treenode {
			public:
				less_indexed_treenode(Kernel&, exptree&, iterator it);
				bool operator()(unsigned int, unsigned int) const;
			private:
				Kernel&           kernel;
				exptree&          tr;  
				exptree::iterator it;
		};
	private:
		const TableauBase     *tb;
};

