
#include "algorithms/eliminate_vielbein.hh"
#include "properties/Vielbein.hh"

using namespace cadabra;

eliminate_vielbein::eliminate_vielbein(const Kernel& k, Ex& e, Ex& pref, bool redundant)
	: eliminate_converter(k, e, pref, redundant)
	{
	}

bool eliminate_vielbein::is_conversion_object(iterator fit) const 
	{
	const Vielbein        *vb=kernel.properties.get<Vielbein>(fit);
	const InverseVielbein *ivb=kernel.properties.get<InverseVielbein>(fit);
   
	if(vb || ivb)  return true;
	else return false;
	}

