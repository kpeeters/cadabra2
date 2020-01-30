
#include "Kernel.hh"
#include "PreClean.hh"
#include "Cleanup.hh"
#include "Parser.hh"
#include "Config.hh"

using namespace cadabra;

Kernel::Kernel()
	: scalar_backend(scalar_backend_t::sympy)
	{
	//	std::cerr << "Kernel() " << this << std::endl;
	}

Kernel::~Kernel()
	{
	//	std::cerr << "~Kernel() " << this << std::endl;
	}

void Kernel::inject_property(property *prop, std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param)
	{
	Ex::iterator it=ex->begin();

	if(param) {
		// std::cerr << "property with " << *param << std::endl;
		keyval_t keyvals;
		prop->parse_to_keyvals(*param, keyvals);
		prop->parse(*this, ex, keyvals);
		}
	prop->validate(*this, Ex(it));
	properties.master_insert(Ex(it), prop);
	}

std::shared_ptr<cadabra::Ex> Kernel::ex_from_string(const std::string& s)
	{
	auto ex = std::make_shared<cadabra::Ex>();
	cadabra::Parser parser1(ex, s);

	pre_clean_dispatch_deep(*this, *ex);
	cleanup_dispatch_deep(*this, *ex);
	check_index_consistency(*this, *ex, ex->begin());

	return ex;
	}

