
#include "Weight.hh"
#include "Exceptions.hh"

using namespace cadabra;

Weight::~Weight()
	{
	}

std::string Weight::name() const
	{
	return "Weight";
	}

bool Weight::parse(Kernel& k, std::shared_ptr<Ex> ex, keyval_t& kv)
	{
	keyval_t::const_iterator kvit=kv.find("value");
	if(kvit!=kv.end()) {
		if(kvit->second.is_rational()==false)
			throw ConsistencyException("Weight: weight should be an explicit rational.");
		value_=*kvit->second.begin()->multiplier;
		}
	else value_=1;

	return WeightBase::parse(k, ex, kv);
	}

multiplier_t Weight::value(const Kernel&, Ex::iterator, const std::string& forcedlabel) const
	{
	if(forcedlabel!=label) return -1;
	return value_;
	}
