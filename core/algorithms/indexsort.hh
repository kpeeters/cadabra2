
#include "Algorithm.hh"
#include "properties/TableauBase.hh"

class indexsort : public Algorithm {
	public:
		indexsort(Kernel&, Ex&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		

		class less_indexed_treenode {
			public:
				less_indexed_treenode(Kernel&, Ex&, iterator it);
				bool operator()(unsigned int, unsigned int) const;
			private:
				Kernel&           kernel;
				Ex&          tr;  
				Ex::iterator it;
		};
	private:
		const TableauBase     *tb;
};

