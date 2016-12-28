
#include "DifferentialForm.hh"

std::string DifferentialForm::name() const
	{
	return "DifferentialForm";
	}

bool DifferentialForm::parse(const Kernel&, keyval_t& keyvals)
	{
	for(auto& kv: keyvals) {
		if(kv.first=="degree") {
			degree = Ex(kv.second);
			}
		}
	return true;
	}
