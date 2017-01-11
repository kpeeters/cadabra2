
#pragma once

#include "properties/TableauBase.hh"
#include <vector>

namespace cadabra {

	class TableauSymmetry : public TableauBase, virtual public property {
		public:
			virtual ~TableauSymmetry();
			
			virtual bool         parse(const Kernel&, keyval_t&) override;
			virtual std::string  name() const override;
			virtual void         latex(std::ostream&) const override;
			virtual unsigned int size(const Properties&, Ex&, Ex::iterator) const override;
			virtual tab_t        get_tab(const Properties&, Ex&, Ex::iterator, unsigned int) const override;
			virtual bool         only_column_exchange() const override;
			
			std::vector<tab_t>   tabs;
			
		private:
			bool only_col_;
	};

}
