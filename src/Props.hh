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

/* 
   The storage.cc and props.cc files form one module which does not
   depend on any other file in the system (except of course on the gmp
   library).

*/

#pragma once

#include <map>
#include <list>
#include "Storage.hh"

class pattern { 
	public:
		pattern();
		pattern(const exptree&);

		bool match(const exptree::iterator&, bool ignore_parent_rel=false) const;
		bool children_wildcard() const;

		exptree obj;
};

bool operator<(const pattern& one, const pattern& two);
//bool operator==(const pattern& one, const pattern& two);

class keyval_t {
	public:
		typedef std::pair<std::string, exptree::iterator> kvpair_t;
		typedef std::list<kvpair_t>                       kvlist_t;

		typedef kvlist_t::const_iterator const_iterator;
		typedef kvlist_t::iterator       iterator;
		typedef kvpair_t value_type;
		
		const_iterator find(const std::string&) const;
		iterator       find(const std::string&);
		const_iterator begin() const;
		const_iterator end() const;
		void           push_back(const kvpair_t&);
		void           erase(iterator);

	private:
		kvlist_t keyvals;
};

/// Base class for all properties, handling argument parsing and defining the
/// interface.
class property_base {
	public:
		virtual ~property_base() {};
		virtual bool        core_parse(keyval_t&);
		virtual bool        parse(exptree&, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals);
		virtual std::string name() const=0;
		virtual void        display(std::ostream&) const;
		bool                preparse_arguments(exptree::iterator prop, keyval_t& keyvals);
		virtual std::string unnamed_argument() const;

		// To compare properties we sometimes need to compare their variables, not only
		// their type. The following function needs to be overridden in all properties
		// for which comparison by type is not sufficient to establish equality.
		//
		//   id_match:    only one of these properties can be registered, but their data is not the same
		//   exact_match: these properties are exactly identical
		enum match_t { no_match, id_match, exact_match };
		virtual match_t equals(const property_base *) const;
	private:
		bool                parse_one_argument(exptree::iterator arg, keyval_t& keyvals);
};

/// Placeholder.
/// \bug Should be merged with property_base.
class property : public property_base {
	public:
};

class labelled_property : virtual public property {
	public:
		virtual bool core_parse(keyval_t&);
		std::string label;
};

class list_property : public property_base {
	public:
};



class IndexInherit : virtual public property {
	public: 
		virtual std::string name() const { return std::string("IndexInherit"); };
};

// FIXME: The Inherit<...> template should be deprecated in favour of the 
// [...]Base classes, which actually allow for a computation, instead of dumb
// copying of the properties of the first child.

template<class T>
class Inherit {
	public:
		virtual ~Inherit() {};
		virtual std::string name() const { return std::string("Stay Away"); };
};

class PropertyInherit : virtual public property {
	public: 
		virtual std::string name() const { return std::string("PropertyInherit"); };
};

template<class T>
property_base *create_property()
	{
	return new T;
	}

class properties {
	public:
		// Registering property types.
		class registered_property_map_t {
			public:
				~registered_property_map_t();

				typedef std::map<std::string, property_base* (*)()> internal_property_map_t;
				typedef internal_property_map_t::iterator iterator;

				internal_property_map_t store;
		};

		static void                          register_property(property_base* (*)());
		static void                          register_properties();
		static registered_property_map_t     registered_properties;

		// Registering properties.
		// When inserting a property or list_property, ownership of the
		// property gets transferred to this singleton class.
		typedef std::pair<pattern *, const property_base *>                     pat_prop_pair_t;

		/// Pattern-property map indexed on the name_only part of the head of the pattern,
		/// for rapid lookup.
		typedef std::multimap<nset_t::iterator, pat_prop_pair_t, nset_it_less>  property_map_t;
		/// FIXME: the above contains an iterator, which we now take to be pointing to an element
		/// in the obj tree of the pattern of the pat_prop_pair_t. However, that is brittle...
		typedef std::multimap<const property_base *, pattern *>                 pattern_map_t;

		static void            insert_prop(const exptree&, const property *);
		static void            insert_list_prop(const std::vector<exptree>&, const list_property *);
		static void            clear();

		/// The following two maps own the pointers to the properties and patterns stored 
		/// in them; use clear() to clean up. Note that pointers can sit in in more than one
		/// entry in this map (when they are pointing to list_property objects, which are
		/// shared between patterns). 
		static property_map_t  props;
		static pattern_map_t   pats;   // for list properties, objects are stored here in order

		// Normal search: given a pattern, get its property if any.
		template<class T> static const T*  get(exptree::iterator, bool ignore_parent_rel=false); // Shorthand for get_composite
		template<class T> static const T*  get();
		template<class T> static const T*  get_composite(exptree::iterator, bool ignore_parent_rel=false);
		template<class T> static const T*  get_composite(exptree::iterator, int& serialnum, bool doserial=true, bool ignore_parent_rel=false);
		// Ditto for labelled properties
		template<class T> static const T*  get_composite(exptree::iterator, const std::string& label);
		template<class T> static const T*  get_composite(exptree::iterator, int& serialnum, const std::string& label, bool doserial=true);
		// For list properties: given two patterns, get a common property.
		template<class T> static const T*  get_composite(exptree::iterator, exptree::iterator, bool ignore_parent_rel=false);
		template<class T> static const T*  get_composite(exptree::iterator, exptree::iterator, int&, int&, bool ignore_parent_rel=false);

		// Get the outermost node which has the given property attached, i.e. go down through
		// all (if any) nodes which have just inherited the property.
		template<class T> exptree::iterator static head(exptree::iterator, bool ignore_parent_rel=false);

		// Search through pointers
		static bool has(const property_base *, exptree::iterator);
		// Find serial number of a pattern in a given list property
		static int  serial_number(const property_base *, const pattern *);

		// Inverse search: given a property type, get a pattern which has this property.
		// When given an iterator, it starts to search in the property
		// map from this particular point. Note: this searches on property type, not exact property.
//		template<class T>
//		static property_map_t::iterator      get_pattern(property_map_t::iterator=props.begin());

		// Equivalent search: given a node, get a pattern of equivalents.
//		static property_map_t::iterator      get_equivalent(exptree::iterator, 
//																	  property_map_t::iterator=props.begin());		
};

template<class T>
const T* properties::get(exptree::iterator it, bool ignore_parent_rel)
	{
	return get_composite<T>(it, ignore_parent_rel);
	}

template<class T>
const T* properties::get_composite(exptree::iterator it, bool ignore_parent_rel)
	{
	int tmp;
	return get_composite<T>(it, tmp, false, ignore_parent_rel);
	}

template<class T>
const T* properties::get_composite(exptree::iterator it, int& serialnum, bool doserial, bool ignore_parent_rel)
	{
	const T* ret=0;
	bool inherits=false;

	std::pair<property_map_t::iterator, property_map_t::iterator> pit=props.equal_range(it->name_only());
	
	// First look for properties of the node itself. Go through the loop twice:
	// once looking for patterns which do not have wildcards, and then looking
	// for wildcard patterns.
	bool wildcards=false;
	for(;;) {
		property_map_t::iterator walk=pit.first;
		while(walk!=pit.second) {
			if(wildcards==(*walk).second.first->children_wildcard()) {
//				std::cout << "searching " << *it->name << std::endl;
//				std::cout << "comparing " << *(walk->second.first->obj.begin()->name) << std::endl;
				if((*walk).second.first->match(it, ignore_parent_rel)) { // match found
//					std::cout << "found match" << std::endl;
					ret=dynamic_cast<const T *>((*walk).second.second);
					if(ret) { // found! determine serial number
//						std::cout << "found property" << std::endl;
						if(doserial) {
							std::pair<pattern_map_t::iterator, pattern_map_t::iterator> 
								pm=pats.equal_range((*walk).second.second);
							serialnum=0;
							while(pm.first!=pm.second) { 
								if((*pm.first).second==(*walk).second.first)
									break;
								++serialnum;
								++pm.first;
								}
							}
						break;
						}
//					else 						std::cout << "NOT found property" << std::endl;
					if(dynamic_cast<const PropertyInherit *>((*walk).second.second)) 
						inherits=true;
					else if(dynamic_cast<const Inherit<T> *>((*walk).second.second)) 
						inherits=true;
					}
//				else std::cout << "NOT found match" << std::endl;
				}
			++walk;
			}
		if(!wildcards && !ret) {
//			std::cout << "not yet found, switching to wildcards" << std::endl;
			wildcards=true;
			}
		else {
//			std::cout << "found match" << std::endl;
			break;
			}
		} 

	// If no property was found, figure out whether a property is inherited from a child node.
	if(!ret && inherits) {
		exptree::sibling_iterator sib=it.begin();
		while(sib!=it.end()) {
			const T* tmp=get_composite<T>((exptree::iterator)(sib), serialnum, doserial);
			if(tmp) {
				ret=tmp;
				break;
				}
			++sib;
			}
		}

	return ret;
	}

template<class T>
const T* properties::get_composite(exptree::iterator it, const std::string& label)
	{
	int tmp;
	return get_composite<T>(it, tmp, label, false);
	}

template<class T>
const T* properties::get_composite(exptree::iterator it, int& serialnum, const std::string& label, bool doserial)
	{
	const T* ret=0;
	bool inherits=false;
	std::pair<property_map_t::iterator, property_map_t::iterator> pit=props.equal_range(it->name);

	// First look for properties of the node itself. Go through the loop twice:
	// once looking for patterns which do not have wildcards, and then looking
	// for wildcard patterns.
	bool wildcards=false;
	for(;;) {
		property_map_t::iterator walk=pit.first;
		while(walk!=pit.second) {
			if(wildcards==(*walk).second.first->children_wildcard()) {
				if((*walk).second.first->match(it)) { // match found
					ret=dynamic_cast<const T *>((*walk).second.second);
					if(ret) { // found! determine serial number
						if(ret->label!=label && ret->label!="all") 
							ret=0;
						else {
							if(doserial) 
								serialnum=serial_number( (*walk).second.second, (*walk).second.first );
							break;
							}
						}
					if(dynamic_cast<const PropertyInherit *>((*walk).second.second))
						inherits=true;
					else if(dynamic_cast<const Inherit<T> *>((*walk).second.second)) 
						inherits=true;
					}
				}
			++walk;
			}
		if(!wildcards) wildcards=true;
		else break;
		}
		
	// If no property was found, figure out whether a property is inherited from a child node.
	if(!ret && inherits) {
		exptree::sibling_iterator sib=it.begin();
		while(sib!=it.end()) {
			const T* tmp=get_composite<T>((exptree::iterator)(sib), serialnum, label, doserial);
			if(tmp) {
				ret=tmp;
				break;
				}
			++sib;
			}
		}
	return ret;
	}

template<class T>
const T* properties::get_composite(exptree::iterator it1, exptree::iterator it2, bool ignore_parent_rel)
	{
	int tmp1, tmp2;
	return get_composite<T>(it1,it2,tmp1,tmp2, ignore_parent_rel);
	}

template<class T>
const T* properties::get_composite(exptree::iterator it1, exptree::iterator it2, int& serialnum1, int& serialnum2, bool ignore_parent_rel)
	{
	const T* ret1=0;
	const T* ret2=0;
	bool found=false;

	bool inherits1=false, inherits2=false;
	std::pair<property_map_t::iterator, property_map_t::iterator> pit1=props.equal_range(it1->name);
	std::pair<property_map_t::iterator, property_map_t::iterator> pit2=props.equal_range(it2->name);

	property_map_t::iterator walk1=pit1.first;
	while(walk1!=pit1.second) {
		if((*walk1).second.first->match(it1, ignore_parent_rel)) { // match for object 1 found
			ret1=dynamic_cast<const T *>((*walk1).second.second);
			if(ret1) { // property of the right type found for object 1
				property_map_t::iterator walk2=pit2.first;
				while(walk2!=pit2.second) {
					if((*walk2).second.first->match(it2, ignore_parent_rel)) { // match for object 1 found
						ret2=dynamic_cast<const T *>((*walk2).second.second);
						if(ret2) { // property of the right type found for object 2
							if(ret1==ret2) { 
								serialnum1=serial_number( (*walk1).second.second, (*walk1).second.first );
								serialnum2=serial_number( (*walk2).second.second, (*walk2).second.first );
								found=true;
								goto done;
								}
							}
						}
					if(dynamic_cast<const PropertyInherit *>((*walk2).second.second))
						inherits2=true;
					++walk2;
					}
				}
			if(dynamic_cast<const PropertyInherit *>((*walk1).second.second))
				inherits1=true;
			}
		++walk1;
		}
		
	// If no property was found, figure out whether a property is inherited from a child node.
	if(!found && (inherits1 || inherits2)) {
		exptree::sibling_iterator sib1, sib2;
		if(inherits1) sib1=it1.begin();
		else          sib1=it1;
		bool keepgoing1=true;
		do {    // 1
			bool keepgoing2=true;
			if(inherits2) sib2=it2.begin();
			else          sib2=it2;
			do { // 2
				const T* tmp=get_composite<T>((exptree::iterator)(sib1), (exptree::iterator)(sib2), serialnum1, serialnum2, ignore_parent_rel);
				if(tmp) {
					ret1=tmp;
					found=true;
					goto done;
					}
				if(!inherits2 || ++sib2==it2.end())
					keepgoing2=false;
				} while(keepgoing2);
			if(!inherits1 || ++sib1==it1.end())
				keepgoing1=false;
			} while(keepgoing1);
		}

	done:
	if(!found) ret1=0;
	return ret1;
	}

template<class T>
const T* properties::get()
	{
	const T* ret=0;
	// FIXME: hack
	nset_t::iterator nit=name_set.insert(std::string("")).first;
	std::pair<property_map_t::iterator, property_map_t::iterator> pit=
		props.equal_range(nit);
	while(pit.first!=pit.second) {
		ret=dynamic_cast<const T *>((*pit.first).second.second);
		if(ret) break;
		++pit.first;
		}
	return ret;
	}

template<class T>
exptree::iterator properties::head(exptree::iterator it, bool ignore_parent_rel)
	{
	exptree::iterator dn=it;
	for(;;) {
		if(get<PropertyInherit>(dn, ignore_parent_rel)) {
			dn=dn.begin();
			}
		else {
			assert(get<T>(dn));
			break;
			}
		}
	return dn;
	}

//template<class PropType>
//properties::property_map_t::iterator properties::get_pattern(property_map_t::iterator it)
//	{
//	++it;
//	while(it!=props.end()) {
//		if(typeid( *(it->second.second) ) == typeid(PropType) ) {
//			return it;
//			}
//		++it;
//		}
//	return it;
//	}


/* There are three special properties which are required at a much lower level
	than the other ones, because they are used for comparison of exptrees:
*/

class Symbol : public property {
	public:
		virtual std::string name() const;

		static const Symbol *get(exptree::iterator, bool ignore_parent_rel=false);
};

//class SymbolBase : public property {
//
//};

class Coordinate : public property {
	public:
		virtual std::string name() const;
};

class Indices : public list_property {
	public:
		Indices();
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;
		virtual std::string unnamed_argument() const { return "name"; };
		virtual match_t equals(const property_base *) const;
		
		std::string set_name, parent_name;
		enum position_t { free, fixed, independent } position_type;
		exptree     values;
};

class SortOrder : public list_property {
	public:
		virtual std::string name() const;
		virtual match_t equals(const property_base *) const;
};

class ImplicitIndex : virtual public property {
	public:
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;
		virtual std::string unnamed_argument() const { return "name"; };
		virtual void display(std::ostream& str) const;

		std::vector<std::string> set_names;
};

class Distributable : virtual public  property {
	public:
		virtual ~Distributable() {};
		virtual std::string name() const;
};

class Accent : public PropertyInherit, public IndexInherit, virtual public property {
	public:
		virtual std::string name() const;
};

class DiracBar : public Accent, public Distributable, virtual public property {
	public:
		virtual std::string name() const;
};

class CommutingAsProduct : virtual public property {
	public:
		virtual std::string name() const;
};

class CommutingAsSum : virtual public property {
	public:
		virtual std::string name() const;
};

class CommutingBehaviour : virtual public list_property {
	public:
		virtual int sign() const=0;
		virtual match_t equals(const property_base *) const;
};

class SelfCommutingBehaviour : virtual public property {
	public:
		virtual int sign() const=0;
};


