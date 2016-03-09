
#include "properties/DependsInherit.hh"

std::string DependsInherit::name() const
	{
	return "DependsInherit";
	}

Ex DependsInherit::dependencies(const Kernel& kernel, Ex::iterator it) const
	{
	Ex ret("\\comma");
	Ex::sibling_iterator sib=it.begin();
	while(sib!=it.end()) {
		const DependsBase *dep=kernel.properties.get_composite<DependsBase>(sib);
		if(dep) {
			Ex::iterator cn=ret.append_child(ret.begin(), dep->dependencies(kernel, sib).begin());
			ret.flatten(cn);
			ret.erase(cn);
			}
		++sib;
		}
	return ret;
	}

