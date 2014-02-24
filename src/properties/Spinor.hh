
#pragma once

#include "properties/ImplicitIndex.hh"

class Spinor : public ImplicitIndex, virtual public property {
	public:
		Spinor();
		virtual ~Spinor() {};
		virtual std::string name() const;
//		virtual void        display(std::ostream&) const;
		virtual bool        parse(exptree&, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals);

		int  dimension;
		bool weyl;
		enum { positive, negative } chirality;  // only in combination with weyl
		bool majorana;
};

