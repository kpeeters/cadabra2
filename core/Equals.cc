
#include "Equals.hh"
#include "Exceptions.hh"

using namespace cadabra;

visit::Equals::Equals(const Kernel &k, Ex &e, Ex::iterator i)
	: ReservedNode(k, e, i)
	{
	if(*top->name != "\\equals")
		throw ConsistencyException("Top not not an equality.");
	}

Ex::iterator visit::Equals::lhs() const
	{
	return tr.begin(top);
	}

Ex::iterator visit::Equals::rhs() const
	{
	sibling_iterator it = tr.begin(top);
	++it;
	return it;
	}

void visit::Equals::move_all_to_lhs()
	{
	Ex::iterator lhs_ = lhs();
	Ex::iterator rhs_ = rhs();

	if(*lhs_->name!="\\sum")
		force_node_wrap(lhs_, "\\sum");
	if(*rhs_->name!="\\sum")
		force_node_wrap(rhs_, "\\sum");

	Ex::iterator last = tr.end(lhs_);
	last.skip_children();
	--last;

	tr.reparent(lhs_, rhs_);

	last.skip_children();
	++last;
	while(last != tr.end(lhs_)) {
		multiply(last->multiplier, -1);
		last.skip_children();
		++last;
		}

	tr.append_child(rhs_, Ex(0).begin());
	}
