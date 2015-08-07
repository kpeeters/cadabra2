
#pragma once

#include "properties/TableauBase.hh"
#include <vector>

class TableauSymmetry : public TableauBase, virtual public property {
	public:
		virtual ~TableauSymmetry();

		virtual bool         parse(const Properties&, keyval_t&) override;
		virtual std::string  name() const override;
		virtual void         display(std::ostream&) const override;
		virtual unsigned int size(const Properties&, Ex&, Ex::iterator) const override;
		virtual tab_t        get_tab(const Properties&, Ex&, Ex::iterator, unsigned int) const override;
		virtual bool         only_column_exchange() const override;

		std::vector<tab_t>   tabs;

	private:
		bool only_col_;
};

