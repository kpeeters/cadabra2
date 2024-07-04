
#include "properties/DerivativeOp.hh"
#include "Props.hh"
#include "Kernel.hh"

using namespace cadabra;

bool DerivativeOp::parse(Kernel&, std::shared_ptr<Ex>, keyval_t& keyvals)
	{
	keyval_t::const_iterator ki=keyvals.begin();
	if(ki!=keyvals.end()) return false;
	return true;
	}

std::string DerivativeOp::name() const
	{
	return "DerivativeOp";
	}
