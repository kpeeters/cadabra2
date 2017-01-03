
#include "Exceptions.hh"
#include "ExteriorDerivative.hh"

std::string ExteriorDerivative::name() const
	{
	return "ExteriorDerivative";
	}

Ex ExteriorDerivative::degree(const Properties& props, Ex::iterator it) const
	{
	auto sib=Ex::begin(it);
	long deg=1;
	while(sib!=Ex::end(it)) {
		const DifferentialFormBase *db = props.get<DifferentialFormBase>(sib);
		if(db) {
			auto thisdeg = db->degree(props, sib);
			if(thisdeg.is_rational()==false) {
				std::cerr << thisdeg << std::endl;
				throw NotYetImplemented("Cannot yet compute non-numerical form degrees.");
				}
			deg += to_long(thisdeg.to_rational());
			}
		++sib;
		}
	return Ex(deg);
	}
