
#include "properties/LaTeXForm.hh"

std::string LaTeXForm::name() const
	{
	return "LaTeXForm";
	}

std::string LaTeXForm::unnamed_argument() const
	{
	return "latex";
	}

bool LaTeXForm::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("latex");
	if(kv!=keyvals.end()) latex=*(kv->second->name);
	// FIXME: handle errors.
	latex=latex.substr(1,latex.size()-2);
	return true;
	}

