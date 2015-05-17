#pragma once

#include "Props.hh"

class FilledTableau : public property {
	public:
		virtual ~FilledTableau() {};
		virtual std::string name() const;
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);

		int dimension;
};

