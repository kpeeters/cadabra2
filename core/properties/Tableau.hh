
#pragma once

#include "Props.hh"

class Tableau : public property {
	public:
		virtual ~Tableau() {};
		virtual std::string name() const;
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);

		int dimension;
};

