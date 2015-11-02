
#include "properties/LaTeXForm.hh"

std::string LaTeXForm::name() const
	{
	return "LaTeXForm";
	}

std::string LaTeXForm::unnamed_argument() const
	{
	return "latex";
	}

bool LaTeXForm::parse(const Properties& p, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("latex");
	if(kv!=keyvals.end()) {
		latex_=*(kv->second->name);
		}
	// Strip quotes. FIXME: handle errors.
	latex_=latex_.substr(1,latex_.size()-2);
	return true;
	}

std::string LaTeXForm::latex_form() const
	{
	return latex_;
	}

