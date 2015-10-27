
#include "properties/LaTeXForm.hh"

std::string LaTeXForm::name() const
	{
	return "LaTeXForm";
	}

std::string LaTeXForm::unnamed_argument() const
	{
	return "latex";
	}

bool LaTeXForm::parse(Ex& tr, Ex::iterator pat, Ex::iterator prop, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("latex");
	if(kv!=keyvals.end()) latex_=*(kv->second->name);
	// FIXME: handle errors.
	latex_=latex_.substr(1,latex_.size()-2);
	return true;
	}

std::string LaTeXForm::latex_form() const
	{
	std::cerr << "printing " << latex_ << std::endl;
	return latex_;
	}

