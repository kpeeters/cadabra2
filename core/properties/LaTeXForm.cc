
#include "properties/LaTeXForm.hh"

using namespace cadabra;

std::string LaTeXForm::name() const
	{
	return "LaTeXForm";
	}

std::string LaTeXForm::unnamed_argument() const
	{
	return "latex";
	}

bool LaTeXForm::parse(Kernel&, keyval_t& keyvals)
	{
//	keyval_t::const_iterator kv=keyvals.find("latex");
	for(const auto& kv: keyvals)
		latex.push_back(kv.second);

	// Strip quotes. FIXME: handle errors.
//	latex_=latex_.substr(1,latex_.size()-2);
	return true;
	}

std::string LaTeXForm::latex_form() const
	{
	return *(latex.begin()->begin()->name); // FIXME: deprecated
	}

