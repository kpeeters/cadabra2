
#pragma once

#include "properties/TableauBase.hh"
#include <vector>

class TableauSymmetry : public TableauBase, virtual public property {
	public:
		virtual ~TableauSymmetry() {};
		virtual bool parse(const Properties&, keyval_t&);
		virtual std::string name() const;
		virtual void display(std::ostream&) const;

		virtual unsigned int size(exptree&, exptree::iterator) const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;

		virtual bool         only_column_exchange() const { return only_col_; };

		std::vector<tab_t>     tabs;
	private:
		bool only_col_;
};

