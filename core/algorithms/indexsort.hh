
#include "Algorithm.hh"
#include "properties/TableauBase.hh"

namespace cadabra {

	class indexsort : public Algorithm {
		public:
			indexsort(const Kernel&, Ex&);
			
			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);		
			
			class less_indexed_treenode {
				public:
					less_indexed_treenode(const Kernel&, Ex&, iterator it);
					bool operator()(unsigned int, unsigned int) const;
				private:
					const Kernel&           kernel;
					Ex&          tr;  
					Ex::iterator it;
			};
		private:
			const TableauBase     *tb;
	};
	
}
