
#pragma once

#include "properties/ImplicitIndex.hh"

class Spinor : public ImplicitIndex, virtual public property {
	public:
		Spinor();
		virtual ~Spinor() {};
		virtual std::string name() const override;
//		virtual void        display(std::ostream&) const;
		virtual bool        parse(const Kernel&, keyval_t& keyvals) override;

		int  dimension;
		bool weyl;
		enum { positive, negative } chirality;  // only in combination with weyl
		bool majorana;
};

