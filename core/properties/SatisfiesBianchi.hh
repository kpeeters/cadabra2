
#pragma once

#include "properties/TableauBase.hh"

class SatisfiesBianchi : public TableauBase, virtual public property {
	public:
//		virtual bool parse(const Properties&, keyval_t&) override;
		virtual std::string name() const override;

		virtual unsigned int size(const Properties&, exptree&, exptree::iterator) const override;
		virtual tab_t        get_tab(const Properties&, exptree&, exptree::iterator, unsigned int) const override;
};

