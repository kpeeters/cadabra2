
#include "ReservedNode.hh"

using namespace cadabra;

visit::ReservedNode::ReservedNode(const Kernel &k, Ex &e, Ex::iterator i)
   : ExManip(k, e), top(i)
	{
	}

Ex::iterator visit::ReservedNode::node() const
	{
	return top;
	}  
