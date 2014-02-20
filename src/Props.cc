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
#include "CoreProps.hh"
#include "Exceptions.hh"
//#include "modules/dummies.hh"
#include <stdlib.h>
#include <typeinfo>
#include <iostream>

Properties::registered_property_map_t::~registered_property_map_t()
	{
	Properties::clear();
// 	internal_property_map_t::iterator it=store.begin();
//	while(it!=store.end()) {
//		delete (*it).second;
//		++it;
//		}
	}

pattern::pattern()
	{
	}

pattern::pattern(const exptree& o)
	: obj(o)
	{
	}

bool pattern::match(const exptree::iterator& it, bool ignore_parent_rel) const
	{
	// Special case for range wildcards.
	// FIXME: move this to storage.cc (see the FIXME there)

	if(it->name==obj.begin()->name && children_wildcard()) {
		exptree::iterator hm=obj.begin(obj.begin());
		if(exptree::number_of_children(hm)==0) {
			return true; // # without arguments
			}
		exptree::iterator hmarg=hm.begin();
		exptree::iterator seqarg=hm; 
		const Indices *ind=0;

		if(*hmarg->name=="\\comma" || *hmarg->name!="\\sequence") {
			exptree::iterator stt=hmarg;
			if(*hmarg->name=="\\comma") {
				stt=hmarg.begin();
				seqarg=hmarg.begin();
				seqarg.skip_children();
				++seqarg;
				}
			ind=Properties::get<Indices>(stt, true);
			}
		else seqarg=hmarg;

		if(seqarg!=hm) {
			exptree::sibling_iterator seqit=seqarg.begin();
			unsigned int from=to_long(*seqit->multiplier);
			++seqit;
			unsigned int to  =to_long(*seqit->multiplier);

			if(exptree::number_of_children(it)<from ||
				exptree::number_of_children(it)>to ) 
				return false;
			}
		
		if(ind!=0) {
			exptree::sibling_iterator indit=it.begin();
			while(indit!=it.end()) {
				const Indices *gi=Properties::get<Indices>(indit, true);
				if(gi!=ind) {
					return false;
					}
				++indit;
				}
			}
		
		return true;
		}

	// Cases without range wildcard.
//	txtout << "comparing " << *it->name << " " << *obj.begin()->name << std::endl;
//	exptree::print_recursive_treeform(txtout, it);
//	exptree::print_recursive_treeform(txtout, obj.begin());
	int res=subtree_compare(it, obj.begin(), ignore_parent_rel?0:-2, true, 0);
//	txtout << res << std::endl;
	if(abs(res)<=1) {
//		 txtout << "match!" << std::endl;
		 return true;
		 }

	return false;
	}

bool pattern::children_wildcard() const	
	{
	if(exptree::number_of_children(obj.begin())==1) 
		if(obj.begin(obj.begin())->is_range_wildcard())
			return true;
	return false;
	}

bool Properties::has(const property_base *pb, exptree::iterator it) 
	{
	std::pair<property_map_t::iterator, property_map_t::iterator> pit=props.equal_range(it->name);
	while(pit.first!=pit.second) {
//		txtout << *it->name << std::endl;
//		txtout << typeid(pit.first->second.second).name() << " versus " 
//				 << typeid(pb).name() << std::endl;
		if(typeid(*(pit.first->second.second))==typeid(*pb) && 
			pit.first->second.first->match(it))  // match found
			return true;
		++pit.first;
		}
	return false;
	}

void Properties::clear() 
	{
	// Clear and free the property lists. Since pointers to properties can
	// be shared, we use the pats map and make sure that we only free each
	// property* pointer once.
	pattern_map_t::const_iterator it=pats.begin();
	const property_base *previous=0;
	while(it!=pats.end()) {
		 if(previous!=it->first) {
			  previous=it->first;
			  delete it->first;
			  }
		 delete it->second;
		 ++it;
		 }
	props.clear();
	pats.clear();
	}

void Properties::register_property(property_base* (*fun)())
	{
	property_base *tmp=fun(); // need a property object of this type temporarily to retrieve the name
	registered_properties.store[tmp->name()]=fun;
	delete tmp; 
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


bool property_base::parse(exptree& tr, exptree::iterator, exptree::iterator arg, keyval_t& keyvals)
	{
	if(tr.number_of_children(arg)==0) return true;
//	txtout << name() << ": should not have any arguments." << std::endl;
	return false;
	}

bool property_base::parse_one_argument(exptree::iterator arg, keyval_t& keyvals)
	{
	if(*arg->name=="\\equals") {
		exptree::sibling_iterator key=arg.begin();
		if(key==arg.end()) return false;
		exptree::sibling_iterator val=key;
		++val;
		if(val==arg.end()) return false;
		keyvals.push_back(keyval_t::value_type(*arg.begin()->name, val));
		}
	else {
		if(unnamed_argument()!="") {
			keyvals.push_back(keyval_t::value_type(unnamed_argument(), arg));
			}
		else return false;
		}
	return true;
	}

bool property_base::preparse_arguments(exptree::iterator prop, keyval_t& keyvals) 
	{
	if(exptree::number_of_children(prop)==0) return true;
	if(exptree::number_of_children(prop)>1) return false;
	if(*prop.begin()->name!="\\comma") { // one argument
		if(parse_one_argument(prop.begin(), keyvals)==false)
			return false;
		}
	else {
		exptree::sibling_iterator sib=prop.begin().begin();
		while(sib!=prop.begin().end()) {
			if(parse_one_argument(sib, keyvals)==false)
				return false;
			++sib;
			}
		}
	return true;
	}


void property_base::display(std::ostream& str) const
	{ 
	str << name() << "(";
	}

std::string property_base::unnamed_argument() const
	{
	return "";
	}

bool property_base::core_parse(keyval_t& keyvals)
	{
	return true;
	}

property_base::match_t property_base::equals(const property_base *) const
	{
	return exact_match;
	}

bool labelled_property::core_parse(keyval_t& keyvals)
	{
	keyval_t::const_iterator lit=keyvals.find("label");
	if(lit!=keyvals.end()) {
		label=*lit->second->name;
		return true;
		}
	else {
//		txtout << "This property needs a label." << std::endl;
		return false;
		}
	}
	
bool operator<(const pattern& one, const pattern& two)
	{
	return tree_less(one.obj, two.obj);
//	if(*(one.obj.begin()->name)<*(two.obj.begin()->name)) return true;
//	return false;
	}

//bool operator==(const pattern& one, const pattern& two)
//	  {
//	  return one.obj==two.obj; // FIXME: handle dummy indices
//	  }

void Properties::insert_prop(const exptree& et, const property *pr)
	{
//	assert(pats.find(pr)==pats.end()); // identical properties have to be assigned through insert_list_prop

	pattern *pat=new pattern(et);

	std::pair<property_map_t::iterator, property_map_t::iterator> pit=
		props.equal_range(pat->obj.begin()->name_only());

	property_map_t::iterator first_nonpattern=pit.first;

	while(pit.first!=pit.second) {
		// keep track of the first non-pattern element
		if(exptree::number_of_children((*pit.first).second.first->obj.begin())==1) 
			if((*pit.first).second.first->obj.begin().begin()->is_range_wildcard()) 
				++first_nonpattern;
			
		if((*pit.first).second.first->match(et.begin())) { // match found
			if(typeid(*pr)==typeid(*(*pit.first).second.second)) {
				const labelled_property *lp   =dynamic_cast<const labelled_property *>(pr);
				const labelled_property *lpold=dynamic_cast<const labelled_property *>(pit.first->second.second);
				if(!lp || !lpold || lp->label==lpold->label) {
//					txtout << "Removing previously set property." << std::endl;
					pattern  *oldpat=pit.first->second.first;
					const property_base *oldprop=pit.first->second.second;
					props.erase(pit.first);
					pats.erase(oldprop);
					delete oldpat;
					delete oldprop;
					break;
					}
				}
			}
		++pit.first;
		}

	pats.insert(pattern_map_t::value_type(pr, pat));
	Properties::props.insert(property_map_t::value_type(pat->obj.begin()->name_only(), pat_prop_pair_t(pat,pr)));
	}

void Properties::insert_list_prop(const std::vector<exptree>& its, const list_property *pr)
	{
	assert(pats.find(pr)==pats.end()); // identical properties have to be assigned through insert_list_prop
	assert(its.size()>0);

	// If 'pr' is exactly equal to an existing property, we should use that one instead of 
	// introducing a duplicate.
	pattern_map_t::iterator fit=pats.begin();
	while(fit!=pats.end()) {
		if(typeid(*(*fit).first)==typeid(*pr))
			if(pr->equals((*fit).first)==property_base::exact_match) {
				pr=static_cast<const list_property *>( (*fit).first );
				break;
				}
		++fit;
		}
	
	// If 'pr' has id_match with an existing property, we need to remove all property assignments
	// for the existing one, except when there is an exact_match.
	const property_base *to_delete_property=0;
	pattern_map_t::iterator pit=pats.begin();
	while(pit!=pats.end()) {
		if(typeid(*(*pit).first)==typeid(*pr))
			if(pr->equals((*pit).first)==property_base::id_match) {
				to_delete_property = (*pit).first;
				break;
				}
		++pit;
		}
	if(to_delete_property) {
		pats.erase(to_delete_property);
		property_map_t::iterator it=props.begin();
		while(it!=props.end()) {
			property_map_t::iterator nxt=it;
			++nxt;
			if((*it).second.second==to_delete_property) props.erase(it);
			it=nxt;
			}
		}
	
	
	// Now register the list property.

	for(unsigned int i=0; i<its.size(); ++i) {
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
////						exptree::print_recursive_treeform(txtout, its[i]);
//					
//					pattern  *oldpat=pit.first->second.first;
//					const property_base *oldprop=pit.first->second.second;
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
		pats.insert(pattern_map_t::value_type(pr, pat));
		Properties::props.insert(property_map_t::value_type(pat->obj.begin()->name_only(), pat_prop_pair_t(pat,pr)));
		}
	}


int Properties::serial_number(const property_base *listprop, const pattern *pat)
	{
	int serialnum=0;

	std::pair<pattern_map_t::iterator, pattern_map_t::iterator> 
		pm=pats.equal_range(listprop);
	serialnum=0;
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

std::string Symbol::name() const
	{
	return "Symbol";
	}

const Symbol *Symbol::get(exptree::iterator it, bool ignore_parent_rel) 
	{
	if(*it->name=="\\sum") {
		// Check whether all siblings have the Symbol property.
		exptree::sibling_iterator sib=it.begin();
		const Symbol *s=0;
		while(sib!=it.end()) {
			s = Properties::get<Symbol>(sib, ignore_parent_rel);
			if(!s)
				break;
			++sib;
			}
		return s;
		}
	else return Properties::get<Symbol>(it, ignore_parent_rel);
	}

std::string Coordinate::name() const
	{
	return "Coordinate";
	}

Indices::Indices()
	: position_type(free)
	{
	}

std::string Indices::name() const
	{
	return "Indices";
	}

property_base::match_t Indices::equals(const property_base *other) const
	{
	const Indices *cast_other = dynamic_cast<const Indices *>(other);
	if(cast_other) {
		 if(set_name == cast_other->set_name) {
			  if(parent_name == cast_other->parent_name && position_type == cast_other->position_type)
					return exact_match;
			  else
					return id_match;
			  }
		 return no_match;
		 }
	return property_base::equals(other);
	}

bool Indices::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals)
	{
	keyval_t::const_iterator ki=keyvals.begin();
	while(ki!=keyvals.end()) {
		if(ki->first=="name") {
			if(*ki->second->multiplier!=1) {
				throw std::logic_error("Indices: use quotes to label names when they start with a number.");
				}
			set_name=*ki->second->name;
			}
		else if(ki->first=="parent") {
			parent_name=*ki->second->name;
			}
		else if(ki->first=="position") {
			if(*ki->second->name=="free")
				position_type=free;
			else if(*ki->second->name=="fixed")
				position_type=fixed;
			else if(*ki->second->name=="independent")
				position_type=independent;
			else throw ConsistencyException("Position type should be fixed, free or independent.");
			}
		else if(ki->first=="values") {
			values=*ki->second;
			if(*values.begin()->name!="\\comma") 
				throw ConsistencyException("Key 'values' of property 'Indices' needs a list as value.");
			}
		else throw ConsistencyException("Property 'Indices' does not accept key '"+ki->first+"'.");
		++ki;
		}

	return true;
	}

property_base::match_t SortOrder::equals(const property_base *) const
	{
	return no_match; // you can have as many of these as you like
	}

std::string SortOrder::name() const
	{
	return "SortOrder";
	}

std::string ImplicitIndex::name() const
	{
	return "ImplicitIndex";
	}

bool ImplicitIndex::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals)
	{
	keyval_t::const_iterator ki=keyvals.begin();
	while(ki!=keyvals.end()) {
		if(ki->first=="name") {
			if(*ki->second->multiplier!=1) {
				throw std::logic_error("ImplicitIndex: use quotes to label names when they start with a number.");
				}
			set_names.push_back(*ki->second->name);
			}
		else throw ConsistencyException("Property 'ImplicitIndex' does not accept key '"+ki->first+"'.");
		++ki;
		}

	return true;
	}

void ImplicitIndex::display(std::ostream& str) const
	{
	property::display(str);
	for(size_t n=0; n<set_names.size(); ++n) {
		if(n>0) str << ", ";
		str << set_names[n];
		}
	}

std::string CommutingAsProduct::name() const
	{
	return "CommutingAsProduct";
	}

std::string CommutingAsSum::name() const
	{
	return "CommutingAsSum";
	}

property_base::match_t CommutingBehaviour::equals(const property_base *) const
	{
	return no_match; // you can have as many of these as you like
	}

