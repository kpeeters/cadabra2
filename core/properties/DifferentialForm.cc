
#include "DifferentialForm.hh"

using namespace cadabra;

DifferentialForm::~DifferentialForm()
	{
	}

std::string DifferentialForm::name() const
	{
	return "DifferentialForm";
	}

bool DifferentialForm::parse(Kernel&, keyval_t& keyvals)
	{
	for(auto& kv: keyvals) {
		if(kv.first=="degree") {
			degree_ = Ex(kv.second);
			}
		}
	return true;
	}

Ex DifferentialForm::degree(const Properties&, Ex::iterator) const
	{
	return degree_;
	}
