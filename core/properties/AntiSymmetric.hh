
#pragma once


#include "properties/TableauBase.hh"
#include "properties/Traceless.hh"


class AntiSymmetric : public TableauBase, public Traceless, virtual public property  {
	public:
		virtual ~AntiSymmetric() {};
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;

		virtual unsigned int size(const Properties&, exptree&, exptree::iterator) const;
		virtual tab_t        get_tab(const Properties&, exptree&, exptree::iterator, unsigned int) const;
};

