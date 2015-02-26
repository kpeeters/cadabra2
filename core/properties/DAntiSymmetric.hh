
#pragma once

#include "properties/TableauBase.hh"

class DAntiSymmetric : public TableauBase, virtual public property  {
	public:
		virtual ~DAntiSymmetric() {};
		virtual std::string name() const override;

		virtual unsigned int size(const Properties&, exptree&, exptree::iterator) const override;
		virtual tab_t        get_tab(const Properties&, exptree&, exptree::iterator, unsigned int) const override;
};

