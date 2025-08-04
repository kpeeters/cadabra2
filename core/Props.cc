/*

Cadabra: a field-theory motivated computer algebra system.
Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

This program is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "Props.hh"
#include "Exceptions.hh"
#include "Compare.hh"
#include <stdlib.h>
// #include <typeinfo>
#include <iostream>
#include <sstream>
#include "properties/Indices.hh"
#include <vector>

// #define DEBUG 1

using namespace cadabra;

pattern::pattern()
	{
	}

pattern::pattern(const Ex& o)
	: obj(o)
	{
	}

bool pattern::match(const Properties& properties, const Ex::iterator& it, bool ignore_parent_rel, bool ignore_properties) const
	{
	Ex_comparator comp(properties);
	return match_ext(properties, it, comp, ignore_parent_rel, ignore_properties);
	}

bool pattern::match_ext(const Properties& properties, const Ex::iterator& it, Ex_comparator& comp, bool ignore_parent_rel, bool ignore_properties) const
	{
	// Special case for range wildcards.
	// FIXME: move this to Compare.cc (see the FIXME there)

	// std::cerr << "Attempting to match " << Ex(it) << " to " << Ex(obj) << std::endl;

	if(it->name==obj.begin()->name && children_wildcard()) {
		Ex::iterator hm=obj.begin(obj.begin());
		if(Ex::number_of_children(hm)==0) {
			return true; // # without arguments
			}
		Ex::iterator hmarg=hm.begin();
		Ex::iterator seqarg=hm;
		const Indices *ind=0;

		if(*hmarg->name=="\\comma" || *hmarg->name!="\\sequence") {
			Ex::iterator stt=hmarg;
			if(*hmarg->name=="\\comma") {
				stt=hmarg.begin();
				seqarg=hmarg.begin();
				seqarg.skip_children();
				++seqarg;
				}
			ind=properties.get<Indices>(stt, true);
			}
		else seqarg=hmarg;

		if(seqarg!=hm) {
			Ex::sibling_iterator seqit=seqarg.begin();
			unsigned int from=to_long(*seqit->multiplier);
			++seqit;
			unsigned int to  =to_long(*seqit->multiplier);

			if(Ex::number_of_children(it)<from ||
			      Ex::number_of_children(it)>to )
				return false;
			}

		if(ind!=0) {
			Ex::sibling_iterator indit=it.begin();
			while(indit!=it.end()) {
				const Indices *gi=properties.get<Indices>(indit, true);
				if(gi!=ind) {
					return false;
					}
				++indit;
				}
			}

		return true;
		}

	// Cases without range wildcard.  Compare making full use of
	// property information. Note the order of the arguments to
	// 'equal_subtree': the first argument is supposed to be a
	// pattern, the second an expression which is to be matched.

#ifdef DEBUG
	std::cerr << "vvvvvvv " << ignore_properties << std::endl;
#endif
	comp.clear();
	Ex_comparator::match_t res=
		comp.equal_subtree(obj.begin(), it,
								 ignore_properties?Ex_comparator::useprops_t::never:Ex_comparator::useprops_t::not_at_top,
								 ignore_parent_rel);
#ifdef DEBUG
	std::cerr << "pattern::match: comparing" << Ex(it) <<  " with " << Ex(obj) << " = " << static_cast<int>(res) << std::endl;
	std::cerr << "^^^^^^" << std::endl;
#endif

	if(is_in(res, {
	Ex_comparator::match_t::subtree_match,
	Ex_comparator::match_t::match_index_less,
	Ex_comparator::match_t::match_index_greater,
	Ex_comparator::match_t::node_match
	} )) {
		return true;
		}

	return false;
	}

bool pattern::children_wildcard() const
	{
	if(Ex::number_of_children(obj.begin())==1)
		if(obj.begin(obj.begin())->is_range_wildcard())
			return true;
	return false;
	}

bool Properties::has(const property *pb, Ex::iterator it)
	{
	// Does Ex::iterator `it` possess property *pb?
	
	auto pdit = props_dict.find(it->name_only());
	if (pdit == props_dict.end()) {
		return false;
		}
	
	// Look for property *pb
	prop_pat_typemap_t::iterator pptit = pdit->second.find(typeid(*pb));
	if (pptit == pdit->second.end()) {
		return false;
		}

	// Check if any prop_pat pair matches
	for (const prop_pat_pair_t& prop_pat : pptit->second) {
		if (prop_pat.second->match(*this, it)) {
			return true;
			}
		}

	return false;
	}


void Properties::clear()
	{
	// Clear and free the property lists. Since pointers to properties can
	// be shared, we use the pats_dict map and make sure that we only free each
	// property* pointer once.

	for (const auto& [_, this_pats] : pats_dict) {
		auto it=this_pats.begin();
		const property *previous=0;
		while(it!=this_pats.end()) {
			if(previous!=it->first) {
				previous=it->first;
				delete it->first;
				}
			delete it->second;
			++it;
			}
		}
	props_dict.clear();
	pats_dict.clear();
	}

Properties::registered_property_map_t::~registered_property_map_t()
	{
	// FIXME: V2
	}

template<typename T>
void Properties::registered_property_map_t::register_type() {
	T obj;
	// obj is an actual property object (e.g. AntiCommuting) and not a pointer
	std::type_index typeidx = typeid(obj);
	auto it = types_to_names_.find(typeidx);
	if (it == types_to_names_.end()) {
		types_to_names_[typeidx] = obj.name();
		names_to_types_.emplace(obj.name(),typeidx);
	}
}

void Properties::registered_property_map_t::register_type(const property* prop) {
	std::type_index typeidx(typeid(*prop));
	auto it = types_to_names_.find(typeidx);
	if (it == types_to_names_.end()) {
		types_to_names_[typeidx] = prop->name();
		names_to_types_.emplace(prop->name(),typeidx);
	}
}


void Properties::register_property(property* (*fun)(), const std::string& name)
	{
	registered_properties.store[name]=fun;
	}

template<typename T>
void Properties::register_property_type()
	{
	registered_properties.register_type<T>();
	}

void Properties::register_property_type(const property* prop)
	{
	registered_properties.register_type(prop);
	}

keyval_t::const_iterator keyval_t::find(const std::string& key) const
	{
	keyval_t::const_iterator it=keyvals.begin();
	while(it!=keyvals.end()) {
		if(it->first==key)
			break;
		++it;
		}
	return it;
	}

keyval_t::iterator keyval_t::find(const std::string& key)
	{
	keyval_t::iterator it=keyvals.begin();
	while(it!=keyvals.end()) {
		if(it->first==key)
			break;
		++it;
		}
	return it;
	}

keyval_t::const_iterator keyval_t::begin() const
	{
	return keyvals.begin();
	}

keyval_t::const_iterator keyval_t::end() const
	{
	return keyvals.end();
	}

void keyval_t::push_back(const kvpair_t& kv)
	{
	keyvals.push_back(kv);
	}

void keyval_t::erase(iterator it)
	{
	keyvals.erase(it);
	}


property::property(bool h)
	: hidden_(h)
	{
	}

void property::hidden(bool h)
	{
	hidden_=h;
	}

bool property::hidden() const
	{
	return hidden_;
	}

bool property::parse(Kernel&, keyval_t&)
	{
	return true;
	}

bool property::parse(Kernel& kernel, std::shared_ptr<Ex>, keyval_t& keyvals)
	{
	// The default is to run the 'parse' without 'ex', as most properties
	// do not implement the new interface.
	return parse(kernel, keyvals);
	}

void property::validate(Kernel&, std::shared_ptr<Ex>) const
	{
	}

bool property::parse_one_argument(Ex::iterator arg, keyval_t& keyvals)
	{
	if(*arg->name=="\\equals") {
		Ex::sibling_iterator key=arg.begin();
		if(key==arg.end()) return false;
		Ex::sibling_iterator val=key;
		++val;
		if(val==arg.end()) return false;
		keyvals.push_back(keyval_t::value_type(*arg.begin()->name, Ex(val)));
		}
	else {
		if(unnamed_argument()!="") {
			// std::cerr << unnamed_argument() << " unnamed " << *arg->name << std::endl;
			keyvals.push_back(keyval_t::value_type(unnamed_argument(), Ex(arg)));
			}
		else return false;
		}
	return true;
	}

bool property::parse_to_keyvals(const Ex& tr, keyval_t& keyvals)
	{
	if(tr.begin()==tr.end()) return true;

	auto it=tr.begin();

	//	std::cout << "parsing to keyvals" << std::endl;
	if(*(it)->name!="\\comma") { // one argument
		if(parse_one_argument(it, keyvals)==false)
			return false;
		}
	else {
		Ex::sibling_iterator sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			if(parse_one_argument(sib, keyvals)==false)
				return false;
			++sib;
			}
		}

	// for(auto it=keyvals.begin(); it!=keyvals.end(); ++it)
	//   	std::cerr << (*it).first << " = " << *(*it).second->name << std::endl;
	return true;
	}


void property::latex(std::ostream& str) const
	{
	str << name();
	}

std::string property::unnamed_argument() const
	{
	return "";
	}

list_property::match_t list_property::equals(const property *) const
	{
	return exact_match;
	}

bool labelled_property::parse(Kernel&, std::shared_ptr<Ex>, keyval_t& keyvals)
	{
	keyval_t::const_iterator lit=keyvals.find("label");
	if(lit!=keyvals.end()) {
		label=*lit->second.begin()->name;
		return true;
		}
	else {
		// FIXME: not all labelled properties have an actual label, e.g.
		// Derivative derives from WeightBase but not all derivatives need
		// a label. If we throw an exception here, those properties fail
		// to run.
		//		throw ArgumentException("Need a 'label' parameter for property.");
		return false;
		}
	}

//bool operator<(const pattern& one, const pattern& two)
//	{
//	return tree_less(one.obj, two.obj);
////	if(*(one.obj.begin()->name)<*(two.obj.begin()->name)) return true;
////	return false;
//	}

//bool operator==(const pattern& one, const pattern& two)
//	  {
//	  return one.obj==two.obj; // FIXME: handle dummy indices
//	  }

void Properties::insert_prop(const Ex& et, const property *pr) {
	//	assert(pats.find(pr)==pats.end()); // identical properties have to be assigned through insert_list_prop

	// Create the pattern from et
	pattern *pat = new pattern(et);

	// Make sure there is no existing property of the same type matching pat
	auto tmp = props_dict.find(pat->obj.begin()->name_only());
	if (tmp != props_dict.end() ) {
		auto possible_dups = PropertyFilter<false>(&tmp->second);
		for (iterator dup_it = possible_dups.begin(); dup_it != possible_dups.end();) {
			// std::cerr << "Accessing: " << dup_it->first << '\n';
			if (typeid(*dup_it->first) != typeid(*pr)) {
				dup_it.next_proptype();
				continue;
			}
			// A given pattern can only have one property of any given type. The following
			// triggers on entries in the props map which match the pattern to be inserted
			// and are of the same type as pr.
			if( dup_it->second->match(*this, et.begin())) {
				// If this is a labelled property, is the label different from the one on the
				// property we are trying to insert?
				const labelled_property *lp    = dynamic_cast<const labelled_property *>(pr);
				const labelled_property *lpold = dynamic_cast<const labelled_property *>(dup_it->first);

				if(!lp || !lpold || lp->label==lpold->label) {
					// The to-be-inserted property cannot co-exist on this pattern with the
					// one that is currently associated to the pattern. Remove it.
					pattern        *oldpat  = dup_it->second;
					const property *oldprop = dup_it->first;

					// If the new property instance is the same as the old one, we can stop
					// (this happens if a pattern is accidentally repeated in a property assignment).
					if(oldprop==pr) {
						delete pat;
						return;
						}

					// Erase the pattern->property entry, and delete the pattern.
					// FIXME: store pattern by value. (why?)
					iterator old_dup_it = dup_it;
					++dup_it;
					// std::cerr << "Freeing: " << old_dup_it->first << '\n';
					erase(old_dup_it->first, old_dup_it->second);

					// Remove the property->pattern entry. Only delete the property
					// if it is no longer associated to any other pattern.
					// FIXME:
					//   {A, B}::SelfAntiCommuting.
					//   {A}::SelfAntiCommuting.
					//   {B}::SelfAntiCommuting.
					// leads to two properties SelfAntiCommuting, which are identical.
					// We need a way to compare properties and decide when they are
					// identical, or when they can coexist, or something like that.
					
					// FIXME: SelfAntiCommuting is not a list property, so above should be fine.
				} else {
					++dup_it;
				}
			} else {
				++dup_it;
			}
		}
	}

	register_property_type(pr);
	// Add to the props_dict
	props_dict[pat->obj.begin()->name_only()][typeid(*pr)].emplace(pr,pat);
	// Add to the pats_dict
	pats_dict[typeid(*pr)].emplace(pr, pat);
}


/*
void Properties::insert_prop_old(const Ex& et, const property *pr)
	{
	//	assert(pats.find(pr)==pats.end()); // identical properties have to be assigned through insert_list_prop

	// FIXME: is it really necessary to store this by pointer? We are in any case
	// not cleaning this up correctly yet.
	pattern *pat=new pattern(et);

	// pit iterates over pat_prop pairs matching name_only()
	std::pair<property_map_t::iterator, property_map_t::iterator> pit=
	   props.equal_range(pat->obj.begin()->name_only());

	property_map_t::iterator first_nonpattern=pit.first;

	while(pit.first!=pit.second) {
		// keep track of the first non-pattern element
		if(Ex::number_of_children((*pit.first).second.first->obj.begin())==1)
			if((*pit.first).second.first->obj.begin().begin()->is_range_wildcard())
				++first_nonpattern;
		
		// A given pattern can only have one property of any given type. The following
		// triggers on entries in the props map which match the pattern to be inserted.
		if((*pit.first).second.first->match(*this, et.begin())) {

			// Does this entry in props give a property of the same type as the one we
			// are trying to insert?
			const property *tmp = (*pit.first).second.second;
			if(typeid(*pr)==typeid(*tmp)) {

				// If this is a labelled property, is the label different from the one on the
				// property we are trying to insert?
				const labelled_property *lp   =dynamic_cast<const labelled_property *>(pr);
				const labelled_property *lpold=dynamic_cast<const labelled_property *>(pit.first->second.second);

				if(!lp || !lpold || lp->label==lpold->label) {

					// The to-be-inserted property cannot co-exist on this pattern with the
					// one that is currently associated to the pattern. Remove it.

					pattern        *oldpat =pit.first->second.first;
					const property *oldprop=pit.first->second.second;

					// If the new property instance is the same as the old one, we can stop
					// (this happens if a pattern is accidentally repeated in a property assignment).
					if(oldprop==pr) {
						delete pat;
						return;
						}

					// Erase the pattern->property entry, and delete the pattern.
					// FIXME: store pattern by value.

					// erase <oldpat, oldprop> pattern from props
					props.erase(pit.first);
					delete oldpat;

					// Remove the property->pattern entry. Only delete the property
					// if it is no longer associated to any other pattern.
					// FIXME:
					//   {A, B}::SelfAntiCommuting.
					//   {A}::SelfAntiCommuting.
					//   {B}::SelfAntiCommuting.
					// leads to two properties SelfAntiCommuting, which are identical.
					// We need a way to compare properties and decide when they are
					// identical, or when they can coexist, or something like that.
					
					// erase <oldpat, oldprop> pattern from props
					for(auto pi=pats.begin(); pi!=pats.end(); ++pi) {
						if((*pi).first==oldprop && (*pi).second==oldpat) {
							//							std::cerr << "found old entry, deleting" << std::endl;
							pats.erase(pi);
							break;
							}
						}
					if(pats.find(oldprop)==pats.end()) {
						//						std::cerr << "no other references" << std::endl;
						delete oldprop;
						}

					break;
					}
				}
			}
		++pit.first;
		}

	pats.insert(pattern_map_t::value_type(pr, pat));
	// std::cerr << "inserting for " << *(pat->obj.begin()->name) << std::endl;
	props.insert(property_map_t::value_type(pat->obj.begin()->name_only(), pat_prop_pair_t(pat,pr)));
	}
*/

// Insert a list property into the kernel. 
const list_property* Properties::insert_list_prop(const std::vector<Ex>& its, const list_property* pr)
	{
	assert(its.size()>0);
	
	auto& pats = pats_dict[typeid(*pr)];
	assert(pats.find(pr)==pats.end()); // identical properties have to be assigned through insert_list_prop

	/* Description of below code

	List properties in Cadabra include (currently) Indices, SortOrder, CommutingBehaviour (and derived classes).
	Calling `equals` on another list property will yield `no_match`, `id_match`, or `exact_match`.
	Upon insertion of a list property, we look for existing properties of the same type.
	
	If we find one of the same type and `equals` returns `exact_match`, we discard (and delete)
	the list_property pointer. This ensures e.g. that there is only one
	Indices property with the label "vector". The code then continues, as if this property was the
	one passed to `insert_list_prop`.

	If we find a list property that returns `id_match`, we delete that older list property.
	This happens e.g. if we have Indices(vector, position=free) and then create
	Indices(vector, position=fixed). This invalidates all the prior free vector indices.

	If `no_match`, we continue as normal.

	The is the current state of affairs (except `equals` was declared in `property` rather than `list_property`.)

	However, a problem arises (as discussed below). In the older code, the following failed:	
			{a,b,c,d,e}::Indices(vector).
			{a,b,c}::Indices(spinor).
	This should make a,b,c spinor indices, and keep d,e as vector indices.
	Instead, both sets were kept.
	Similarly,
			{a,b,c}::Indices(vector).
			{a,b,c,d,e,f}::Indices(spinor).
	This should make all indices spinor indices.
	Instead, both sets were kept.

	Eventually we should fix this. It is somewhat subtle as it requires we do something else.
	A fourth match type is needed that would indicate we need to look at the properties and
	eliminate incompatible types.

	*/

	// If 'pr' is exactly equal to an existing property, we should use that one instead of
	// introducing a duplicate.
	// Alternatively, if 'pr' has id_match with an existing property, we need to remove all property assignments
	// for the existing one.
	auto pit=pats.begin();
	while(pit!=pats.end()) {
		const property *tmp = (*pit).first;
		// if(typeid(*tmp)==typeid(*pr))  // unnecessary now
		list_property::match_t match_type = pr->equals((*pit).first);
		if (match_type == list_property::exact_match) {
			delete pr;
			pr=static_cast<const list_property *>( (*pit).first );
			// Because pr is passed by reference, the caller maintains a valid pointer.
			break;
		} else if (match_type == list_property::id_match) {
			erase((*pit).first);
			break;
		}
		++pit;
	}
	
	// Now register the list property.
	register_property_type(pr);

	for(size_t i=0; i<its.size(); ++i) {
		pattern *pat=new pattern(its[i]);

		// Removing properties causes more problems than it solves (the only reason
		// for overwriting a list property is to change the SortOrder, which is
		// rarely useful). So we just insert the new property regardless.

		//		// Pointers to properties are shared, so we need to delete them only once.
		//
		//		std::pair<property_map_t::iterator, property_map_t::iterator> pit=
		//			props.equal_range(its[i]->name);
		//
		//		while(pit.first!=pit.second) {
		//			if((*pit.first).second.first->match(its[i])) { // found the pattern 'its[i]' in the property list
		//				if(typeid(*pr)==typeid(*(*pit.first).second.second)) {
		////						txtout << "found a property for " << *(its[i]->name) << std::endl;
		////						Ex::print_recursive_treeform(txtout, its[i]);
		//
		//					pattern  *oldpat=pit.first->second.first;
		//					const property *oldprop=pit.first->second.second;
		//
		////					props.erase(pit.first); THIS
		//
		//					// Delete only those entries in the pattern map which are related to
		//					// this particular pattern _and_ this particular property
		//					std::pair<pattern_map_t::iterator, pattern_map_t::iterator> patrange=
		//						pats.equal_range(oldprop);
		//					while(patrange.first!=patrange.second) {
		//						if(patrange.first->first==oldprop && patrange.first->second==oldpat) {
		////								  txtout << "erasing property for " << *(oldpat->headnode) << std::endl;
		////							pats.erase(patrange.first); // THIS
		//							break;
		//							}
		//						++patrange.first;
		//						}
		////					delete oldpat; THIS
		//					break;
		//					}
		//				}
		//			++pit.first;
		//			}

		// Now register the property.
		//		txtout << "registering " << *(pat->headnode) << std::endl;
		
		props_dict[pat->obj.begin()->name_only()][typeid(*pr)].emplace(pr, pat);
		pats.emplace(pr, pat);
		}
	return pr;
	}


int Properties::serial_number(const property *listprop, const pattern *pat) const
	{
	int serialnum=0;
	auto it = pats_dict.find(typeid(*listprop));
	if (it == pats_dict.end()) {
		return serialnum;
	}
	auto pm=it->second.equal_range(listprop);
	while(pm.first!=pm.second) {
		if((*pm.first).second==pat)
			break;
		++serialnum;
		++pm.first;
		}
	return serialnum;
	}


/*

			{a,b,c,d,e}::Indices(vector).
			{a,b,c}::Indices(spinor).

This should make a,b,c spinor indices, and keep d,e as vector indices.


			{a,b,c}::Indices(vector).
			{d,e}::Indices(vector).

This should make all of a,b,c,d,e vector indices.


			{a,b,c}::Indices(vector).
			{a,b,c,d,e,f}::Indices(spinor).

This should make all indices spinor indices.


			{a,b,c,d,e}::Indices(vector, position=free).
			{a,b,c}::Indices(vector, position=fixed).

You can only have one type of index for each name, so this declaration implies that
d,e should have their property removed.


*/


// Insert a property for the given pattern Ex. Determines whether the property
// is a list property or a normal one, and dispatches accordingly.
// Returns a pointer to the new property. (For a list property, this may differ
// from the original pointer.)

const property* Properties::master_insert(Ex proptree, const property *thepropbase)
	{
	// std::ostringstream str;

	Ex::sibling_iterator st=proptree.begin();

	const list_property *thelistprop=dynamic_cast<const list_property *>(thepropbase);
	if(thelistprop) { // a list property
		std::vector<Ex> objs;
		if(*st->name=="\\comma") {
			Ex::sibling_iterator sib=proptree.begin(st);
			while(sib!=proptree.end(st)) {
				if(sib->fl.parent_rel!=str_node::p_property) {
					objs.push_back(Ex(sib));
					}
				++sib;
				}
			}
		if(objs.size()<2)
			throw ConsistencyException("A list property cannot be assigned to a single object.");

		// FIXME: we special-case Indices, as those pass a list of objects with parent_rel==p_none,
		// but we need the patterns to have parent_rel set to p_sub and p_super in order to avoid
		// special cases in the pattern matcher later.
		// DOCME: the above
		if(dynamic_cast<const Indices *>(thelistprop)) {
			std::vector<Ex> objs2;
			for(auto& obj: objs) {
				Ex obj2(obj);
				obj2.begin()->fl.parent_rel=str_node::p_super;
				objs2.push_back(obj2);
				}
			for(auto& obj: objs) {
				Ex obj2(obj);
				obj2.begin()->fl.parent_rel=str_node::p_sub;
				objs2.push_back(obj2);
				}
			thelistprop = insert_list_prop(objs2, thelistprop);
			}
		else {
			thelistprop = insert_list_prop(objs, thelistprop);
			}
		}
	else {   // a normal property
		const property *theprop=thepropbase;
		assert(theprop);
		if(*st->name=="\\comma") {
			Ex::sibling_iterator sib=proptree.begin(st);
			while(sib!=proptree.end(st)) {
				if(sib->fl.parent_rel!=str_node::p_property) {
					//	std::cerr << "inserting property for " << Ex(sib) << std::endl;
					insert_prop(Ex(sib), theprop);
					}
				++sib;
				}
			}
		else {
			insert_prop(Ex(st), theprop);
			}
		}
	// return str.str();
	if (thelistprop) {
		return dynamic_cast<const property *>(thelistprop);
	} else {
		return thepropbase;
	}
	}

bool Properties::check_label(const property* p, const std::string& label) const
	{
	return true;
	}

bool Properties::check_label(const labelled_property* p, const std::string& label) const
	{
	return (p->label==label || p->label=="all");
	}
	
Ex_comparator *Properties::create_comparator() const
	{
	return new Ex_comparator(*this);
	}

void Properties::destroy_comparator(Ex_comparator *c) const
	{
	delete c;
	}


// Completely erase a property.
void Properties::erase(const property* p) {
	// Make a list of matching patterns
	auto pad_it = pats_dict.find(typeid(*p));
	if (pad_it == pats_dict.end())
		return;
	
	// FIXME: Above should perhaps throw an exception? Property not found or some such.

	std::vector<nset_t::iterator> pattern_names;
	auto patterns = pad_it->second.equal_range(p);	
	for (auto it = patterns.first; it != patterns.second; ++it) {
		// Store all the pattern names
		pattern_names.push_back(it->second->obj.begin()->name_only());
		// Free all the patterns
		delete it->second;
	}

	// Delete all the entries in pats_dict
	pad_it->second.erase(p);

	// Delete all the entries in props_dict
	for (auto pattern_name : pattern_names) {
		props_dict[pattern_name][typeid(*p)].erase(p);
	}

	delete p;
}

// Erase a property and related pattern.
void Properties::erase(const property* prop, pattern* pat) {
	auto name = pat->obj.begin()->name_only();
	int num_found = 0;

	auto pad_it = pats_dict.find(typeid(*prop));
	if (pad_it == pats_dict.end()) {
		throw ConsistencyException("Properties erase failure: Cannot find property of matching type to property/pattern pair to erase.");
	}
	

	// Eliminate from props_dict (where they are first keyed by name of pattern)
	auto pdit = props_dict.find(name);
	if (pdit != props_dict.end()) {
		auto ppit = pdit->second.find(typeid(*prop));
		if (ppit != pdit->second.end()) {
			auto& named_typed_pairs = ppit->second;
			for (auto pairs_it = named_typed_pairs.begin(); pairs_it != named_typed_pairs.end(); ) {
				if (pairs_it->first == prop && pairs_it->second == pat) {
					pairs_it = named_typed_pairs.erase(pairs_it);
					++num_found;
				} else {
					++pairs_it;
				}
			}
		}
	}

	//	Look over all prop/pat pairs of matching types and erase them
	auto& all_typed_pairs = pad_it->second;
	for (auto pairs_it = all_typed_pairs.begin(); pairs_it != all_typed_pairs.end(); ) {
		if (pairs_it->first == prop && pairs_it->second == pat) {
			pairs_it = all_typed_pairs.erase(pairs_it);
			++num_found;
		} else {
			++pairs_it;
		}
	}

	// Does prop have any more patterns associated with it?
	if (all_typed_pairs.find(prop) == all_typed_pairs.end()) {
		delete prop;
	}
	if (num_found != 2) {
		throw ConsistencyException("Properties erase failure: Inconsistent numbers of property/patterns erased.");
	}

	delete pat;
}

std::pair<const property*, std::vector<const pattern*> > Properties::lookup_property(const property* sus) const {
	// Warning: sus may be invalid
	std::pair<const property*, std::vector<const pattern*>> ret = {nullptr, {}};

	for (const auto& [_, pats] : pats_dict) {
		auto range = pats.equal_range(sus);
		if (range.first == range.second) continue;

		// property is found, so sus is valid
		ret.first = sus;
		for (auto it = range.first; it != range.second; ++it) {
			ret.second.push_back(it->second);
		}
		break;
	}
	return ret;
}