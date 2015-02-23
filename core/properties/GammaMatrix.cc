
#include "GammaMatrix.hh"

std::string GammaMatrix::name() const
	{
	return "GammaMatrix";
	}

void GammaMatrix::display(std::ostream& str) const
	{
	Matrix::display(str);
	}

bool GammaMatrix::parse(const Properties& properties, keyval_t& keyvals)
	{
	keyval_t::iterator kv=keyvals.find("metric");
	if(kv!=keyvals.end()) {
		metric=exptree(kv->second);
		keyvals.erase(kv);
		}

	ImplicitIndex::parse(properties, keyvals);

//	kv=keyvals.find("delta");
//	if(kv!=keyvals.end()) delta=exptree(kv->second);
//
	return true;
	}
