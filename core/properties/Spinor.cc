
#include "Spinor.hh"
#include "Exceptions.hh"
#include "Kernel.hh"

using namespace cadabra;

Spinor::Spinor()
	: dimension(10), weyl(true), chirality(positive), majorana(true)
	{
	}

std::string Spinor::name() const
	{
	return "Spinor";
	}

bool Spinor::parse(Kernel& kernel, keyval_t& keyvals)
	{
	keyval_t::iterator ki=keyvals.find("dimension");
	if(ki!=keyvals.end()) {
		dimension=to_long(*ki->second->multiplier);
		keyvals.erase(ki);
		} else                  dimension=10;

	ki=keyvals.find("type");
	if(ki!=keyvals.end()) {
		if(*ki->second->name=="Weyl") {
			if(dimension%2!=0) {
				throw ConsistencyException("Weyl spinors require the dimension to be even.");
				}
			weyl=true;
			}
		if(*ki->second->name=="Majorana") {
			weyl=false;
			if(dimension%8==2 || dimension%8==3 || dimension%8==4)
				majorana=true;
			else {
				throw ConsistencyException("Majorana spinors require the dimension to be 2,3,4 mod 8.");
				return false;
				}
			}
		if(*ki->second->name=="MajoranaWeyl") {
			if(dimension%8==2) {
				//				txtout << "setting to MajoranaWeyl" << std::endl;
				weyl=true;
				majorana=true;
				} else {
				throw ConsistencyException("Majorana-Weyl spinors require the dimension to be 2 mod 8.");
				return false;
				}
			}
		keyvals.erase(ki);
		}

	ki=keyvals.find("chirality");
	if(ki!=keyvals.end()) {
		if(*ki->second->name=="Positive") chirality=positive;
		if(*ki->second->name=="Negative") chirality=negative;
		keyvals.erase(ki);
		}

	ImplicitIndex::parse(kernel, keyvals);

	return true;
	}

