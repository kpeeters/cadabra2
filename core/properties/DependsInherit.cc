
#include "properties/DependsInherit.hh"

using namespace cadabra;

std::string DependsInherit::name() const
	{
	return "DependsInherit";
	}

Ex DependsInherit::dependencies(const Kernel& kernel, Ex::iterator it) const
	{
	// std::cerr << "DependsInherit::dependencies" << std::endl;
	Ex ret("\\comma");
	Ex::sibling_iterator sib=it.begin();
	while(sib!=it.end()) {
		const DependsBase *dep=kernel.properties.get<DependsBase>(sib);
		if(dep) {
			// std::cerr << "found dep " << dep->dependencies(kernel, sib) << " for " << *sib->name << std::endl;
			Ex::iterator cn=ret.append_child(ret.begin(), dep->dependencies(kernel, sib).begin());
			ret.flatten(cn);
			ret.erase(cn);
			}
		++sib;
		}
	return ret;
	}

