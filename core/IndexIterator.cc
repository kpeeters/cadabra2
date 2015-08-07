
#include "IndexIterator.hh"
#include "properties/IndexInherit.hh"

index_iterator::index_iterator(const Properties& k)
	: iterator_base(), properties(&k)
	{
	}

index_iterator index_iterator::create(const Properties& k, const iterator_base& other)
	{
	index_iterator ret(k);
	ret.node=other.node;
	ret.halt=other;
	ret.walk=other;
	ret.roof=other;

	ret.halt.skip_children();
	++ret.halt;
	ret.operator++(); 
	return ret;
	}

index_iterator::index_iterator(const index_iterator& other) 
	: iterator_base(other.node), halt(other.halt), walk(other.walk), roof(other.roof), properties(other.properties)
	{
	}

bool index_iterator::operator!=(const index_iterator& other) const
	{
	if(other.node!=this->node) return true;
	else return false;
	}

bool index_iterator::operator==(const index_iterator& other) const
	{
	if(other.node==this->node) return true;
	else return false;
	}

// \bar{\prod{A}{B}} 's indices are undefined, as \bar inherits
// the Product property of \prod. So the worst-case scenario is
// of the type \bar{\hat{A_\mu}} in which the objects with Inherit
// property are strictly nested. However, we can also have
// things like \bar{\diff{\diff{A_\mu}_{\nu}}_{\rho}}, for which 
// we have to collect indices at multiple levels.

/*
  \bar{?}::Accent.
  \bar{\diff{\diff{A_\mu}_{\nu}}_{\rho}};
  @indexlist(%);
  \diff{\diff{A_{\mu}}_{\nu}}_{\rho};
  @indexlist(%);
  \diff{\diff{A}_{\nu}}_{\rho};
  @indexlist(%);
  \bar{\psi_{m}} * \Gamma_{q n p} * \psi_{m} * H_{n p q};
  @indexlist(%);
  q*A_{d c b a};
  @indexlist(%);
  A_{d c b a}*q;
  @indexlist(%);
  \diff{\phi}_s A_\mu \diff{\phi}_t;
  @indexlist(%);
  \Gamma_{a b c};
  @indexlist(%);
  \diff{\sin(x_\mu)}_{\nu};
  @indexlist(%);
  \equals{A_{i}}{B_{i j} Z_{j}};
  @indexlist(%);

*/
index_iterator& index_iterator::operator+=(unsigned int num)
	{
	while(num != 0) {
		--num;
		operator++();
		}
	return *this;
	}


index_iterator& index_iterator::operator++()
	{
	assert(this->node!=0);
	
	// Increment the iterator. As long as we are at an inherit
	// node, keep incrementing. As long as the parent does not inherit,
   // and as long as we are not at the top node,
	// skip children. As long as we are not at an index, keep incrementing.

	const IndexInherit *this_inh=0, *parent_inh=0;
	while(walk!=halt) {
		this_inh=properties->get<IndexInherit>(walk);
		
		if(this_inh==0 && (walk!=roof && walk.node->parent!=0)) {
			parent_inh=properties->get<IndexInherit>(walk.node->parent);
			if(parent_inh==0)
				walk.skip_children();
			}
		
		++walk;

		if(walk!=halt)
			 if(walk->is_index()) 
				  break;
//		if(this_inh==false && walk->is_index())
//			break;
		}
	if(walk==halt) {
		this->node=0;
		return *this;
		}
	else {
		this->node=walk.node;
		}

	return *this;
	}

index_iterator index_iterator::begin(const Properties& properties, const iterator_base& it, int offset) 
	{
	index_iterator ret=index_iterator::create(properties,it);
	if(offset>0)
		ret+=offset;
	return ret;
	}

index_iterator index_iterator::end(const Properties& properties, const iterator_base& it) 
	{
	index_iterator tmp=create(properties, it);
	tmp.node=0;

	return tmp;
	}

index_iterator& index_iterator::operator=(const index_iterator& other)
	{
	iterator_base::operator=(other);
	halt=other.halt;
	walk=other.walk;
	roof=other.roof;
	properties=other.properties;
	return *this;
	}

size_t number_of_indices(const Properties& pr, Ex::iterator it)
	{
	unsigned int res=0;
	index_iterator indit=index_iterator::begin(pr, it);
	while(indit!=index_iterator::end(pr, it)) {
		++res;
		++indit;
		}
	return res;
	}
