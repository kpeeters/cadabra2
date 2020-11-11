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

// Classes handling storage of property information. Actual property
// implementations are in the properties directory in separate files.

#pragma once

#include <map>
#include <list>
#include <type_traits>
#include "Storage.hh"

namespace cadabra {

	class Properties;
	class Kernel;
	class Accent;
	class Ex_comparator;
	
	class pattern {
		public:
			pattern();
			pattern(const Ex&);

			/// Match a pattern to an expression. If ignore_parent_rel is
			/// true, this will match regardless of the parent_rel of the
			/// top node. If ignore_properties is true, property
			/// information will not be used to match symbols anywhere
			/// (in which case A_{a} and A_{b} will no longer match even
			/// when 'a' and 'b' have the same Indices property, for
			/// example). The latter feature is mostly used to do pattern
			/// matching when the property for which we need it cannot
			/// rely on such child node properties, e.g. Accent; see the
			/// specialisation in get below.
			
			bool match(const Properties&, const Ex::iterator&, bool ignore_parent_rel=false, bool ignore_properties=false) const;
			bool children_wildcard() const;

			/// As `match`, but using a comparator object which is
			/// externally provided, so that the caller can use
			/// the found pattern map.
			bool match_ext(const Properties&, const Ex::iterator&, Ex_comparator& comp, bool ignore_parent_rel=false, bool ignore_properties=false) const;

			Ex obj;
		};

	/// Arguments to properties get parsed into a keyval_t structure.

	class keyval_t {
		public:
			typedef std::pair<std::string, Ex::iterator> kvpair_t;
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

	/// \ingroup core
	///
	/// Base class for all properties, handling argument parsing and
	/// defining the interface.
	///
	/// Properties can have arguments. Parsing of these is done in the
	/// properties object itself, with the use of some helper functions.
	/// Parsing is done by implementing the virtual function
	/// property::parse(const Kernel&, keyval_t&). The argument is a
	/// container class which represents the arguments passed to the
	/// property as key/value pairs keyval_t type.
	///
	/// Properties will be asked to check that they can be associated to a
	/// given pattern through the virtual property::validate(const
	/// Properties&, const Ex&) function. The
	// default implementation returns true for any pattern.
	///
	/// FIXME: the above two need to be merged, because parse may need access
	/// to the actual pattern tree, and once we are there, we may as well
	/// do checking.
	/// HOWEVER: in TableauSymmetry.cc:
	/// FIXME: we get the wrong pattern in case of a list! We should have
	/// been fed each individual item in the list, not the list itself.
	///
	/// This suggests that we should be calling once for every pattern, but
	/// that is wasteful in case we are just parsing arguments. Can we really
	/// avoid calling parse for every pattern?
	///
	/// {A_{m n}, A_{m n p}, A_{m n p q}}::TableauSymmetry(shape={2,1}, indices={0,1,2}).
	///
	/// leads to a problem, because the property needs to setup its internal
	/// structures but also verify that these can match all objects in the
	/// same way.
	///
	///
	/// Make all identical properties point to the same property object, so
	/// that normal and list properties become pretty much identical.


	class property {
		public:
			property(bool hidden=false);
			virtual ~property() {};

			// Parse the argument tree into key-value pairs. Returns false on error.
			bool                parse_to_keyvals(const Ex&, keyval_t&);

			// Use the pre-parsed arguments in key/value form to set parameters.
			// Parses universal arguments by default. Will be called once for
			// every property; assigning a non-list property to multiple patterns
			// still calls this only once.
			// FIXME: failure to call
			// into superclass may lead to problems for labelled properties.
			virtual bool        parse(Kernel&, keyval_t& keyvals);

			// New entry point, which also passes the Ex of the pattern, so that
			// the property itself can inject other properties automatically (e.g.
			// declare an InverseMetric if a Metric is declared).
			virtual bool        parse(Kernel&, std::shared_ptr<Ex>, keyval_t& keyvals);

			// Check whether the property can be associated with the pattern.
			// Throw an error if validation fails. Needs access to all other
			// declared properties so that it can understand what the pattern
			// means (which objects are indices etc.).
			virtual void        validate(const Kernel&, const Ex&) const;

			/// Display the property on the stream
			//		virtual void        display(std::ostream&) const;

			/// Generate a LaTeX representation of the property, assuming LaTeX
			/// is in text mode (so it needs dollar symbols to switch to maths).
			virtual void        latex(std::ostream&) const;

			virtual std::string name() const=0;
			virtual std::string unnamed_argument() const;

			// To compare properties we sometimes need to compare their variables, not only
			// their type. The following function needs to be overridden in all properties
			// for which comparison by type is not sufficient to establish equality.
			//
			//   id_match:    only one of these properties can be registered, but their data is not the same
			//   exact_match: these properties are exactly identical
			enum match_t { no_match, id_match, exact_match };
			virtual match_t equals(const property *) const;

			/// Properties can be hidden because they only make sense to the
			/// system; they will not be printed when the user asks for a list
			/// of properties.
			void hidden(bool h);
			bool hidden(void) const;

		private:
			bool                parse_one_argument(Ex::iterator arg, keyval_t& keyvals);
			bool hidden_;
		};

	class labelled_property : virtual public property {
		public:
			virtual bool parse(Kernel&, std::shared_ptr<Ex>, keyval_t&) override;
			std::string label;
		};

	/// Something cannot be both a list property and a normal property at
	/// the same time, so we can safely inherit without virtual.

	class list_property : public property {
		public:
		};

	/// If a property X derives from Inherit<Y>, and get<Y> is called on
	/// an object which has an X property (but no Y property), then the
	/// get<Y> will look at the non-index child of the object to see if
	/// that has a Y property.  FIXME: need to decided what to do if there
	/// are more non-index children.

	template<class T>
	class Inherit : virtual public property {
		public:
			virtual ~Inherit() {};
			virtual std::string name() const
				{
				return std::string("Stay Away");
				};
		};

	/// PropertyInherit is like Inherit<T> for all properties. This is very
	/// generic and almost never really useful.

	class PropertyInherit : virtual public property {
		public:
			virtual std::string name() const
				{
				return std::string("PropertyInherit");
				};
		};

	/// \ingroup core
	///
	/// Class holding a collection of properties attached to expressions.
	/// Symbols and expressions do not have a default meaning in
	/// Cadabra. They get their meaning by attaching properties to
	/// them. When the core manipulator calls an algorithm object, it
	/// passes an instance of the Properties class along with the
	/// expression tree on which to act, so that the algorithm can figure
	/// out what the symbols in the expression tree mean.

	class Properties {
		public:
			// Registering property types.
			class registered_property_map_t {
				public:
					~registered_property_map_t();

					typedef std::map<std::string, property* (*)()> internal_property_map_t;
					typedef internal_property_map_t::iterator iterator;

					internal_property_map_t store;
				};

			/// Registering properties.  When inserting a property or
			/// list_property, ownership of the property gets transferred to
			/// this class.

			void                          register_property(property* (*)(), const std::string& name);
			registered_property_map_t     registered_properties;
			typedef std::pair<pattern *, const property *>  pat_prop_pair_t;

			/// We keep two multi-maps: one from the pattern to the property (roughly) and
			/// one from the property to the pattern. These are both multi-maps because
			/// one pattern can have multiple properties assigned to it, and one property can
			/// be assigned to multiple properties.
			///
			/// When we delete properties, we check the pats map to see if the reference count
			/// for that property has dropped to zero.
			typedef std::multimap<nset_t::iterator, pat_prop_pair_t, nset_it_less>  property_map_t;
			typedef std::multimap<const property *, pattern *>                      pattern_map_t;

			/// Register a property for the indicated Ex. Takes both normal and list
			/// properties and works out which insert calls to make. The property ownership
			/// is transferred to us on using this call.
			std::string master_insert(Ex proptree, const property *thepropbase);

			void        clear();

			/// The following two maps own the pointers to the properties and patterns stored
			/// in them; use clear() to clean up. Note that pointers can sit in in more than one
			/// entry in this map (when they are pointing to list_property objects, which are
			/// shared between patterns).
			property_map_t  props;  // pattern -> property
			pattern_map_t   pats;   // property -> pattern; for list properties, patterns are stored here in order

			/// Normal search: given a pattern, get its property if any.
			template<class T> const T*  get(Ex::iterator, bool ignore_parent_rel=false) const;
			template<class T> const T*  get(Ex::iterator, int& serialnum, bool doserial=true, bool ignore_parent_rel=false) const;
			/// Ditto for labelled properties
			template<class T> const T*  get(Ex::iterator, const std::string& label) const;
			template<class T> const T*  get(Ex::iterator, int& serialnum, const std::string& label, bool doserial=true) const;
			/// For list properties: given two patterns, get a common property.
			template<class T> const T*  get(Ex::iterator, Ex::iterator, bool ignore_parent_rel=false) const;
			template<class T> const T*  get(Ex::iterator, Ex::iterator, int&, int&, bool ignore_parent_rel=false) const;

			/// General property finder, which will return not only the property but also
			/// the pattern which matched the given node. All 'get' functions above call
			/// this function; all functionality is contained in here.
			template<class T>
			std::pair<const T*, const pattern *> get_with_pattern(Ex::iterator, int& serialnum,
																					const std::string& label,
																					bool doserial=true, bool ignore_parent_rel=false) const;

			template<class T>
			std::pair<const T*, const pattern *> get_with_pattern_ext(Ex::iterator, Ex_comparator&, int& serialnum,
																					const std::string& label,
																					bool doserial=true, bool ignore_parent_rel=false) const;

			// Get the outermost node which has the given property attached, i.e. go down through
			// all (if any) nodes which have just inherited the property.
			template<class T> Ex::iterator head(Ex::iterator, bool ignore_parent_rel=false) const;

			// Inverse search: given a property type, get a pattern which has this property.
			// When given an iterator, it starts to search in the property
			// map from this particular point. Note: this searches on property type, not exact property.
			//		template<class T>
			//		property_map_t::iterator      get_pattern(property_map_t::iterator=props.begin());

			// Equivalent search: given a node, get a pattern of equivalents.
			//		property_map_t::iterator      get_equivalent(Ex::iterator,
			//																	  property_map_t::iterator=props.begin());

		private:
			// Insert a property. Do not use this directly, use the public
			// interface `master_insert` instead.
			void insert_prop(const Ex&, const property *);
			void insert_list_prop(const std::vector<Ex>&, const list_property *);
			bool check_label(const property *, const std::string&) const;
			bool check_label(const labelled_property *, const std::string&) const;			
			// Search through pointers
			bool has(const property *, Ex::iterator);
			// Find serial number of a pattern in a given list property
			int  serial_number(const property *, const pattern *) const;
			Ex_comparator *create_comparator() const;
			void           destroy_comparator(Ex_comparator *) const;			
		};

	template<class T>
	const T* Properties::get(Ex::iterator it, bool ignore_parent_rel) const
		{
		int tmp;
		return get<T>(it, tmp, false, ignore_parent_rel);
		}

	template<class T>
	const T* Properties::get(Ex::iterator it, int& serialnum, bool doserial, bool ignore_parent_rel) const
		{
		auto ret = get_with_pattern<T>(it, serialnum, "", doserial, ignore_parent_rel);
		return ret.first;
		}

	template<class T>
	std::pair<const T*, const pattern *> Properties::get_with_pattern(Ex::iterator it, int& serialnum, const std::string& label,
																							bool doserial, bool ignore_parent_rel) const
		{
		Ex_comparator *compptr = create_comparator();
		// FIXME: catch and rethrow all exceptions so we do not leak memory
		auto ret = get_with_pattern_ext<T>(it, *compptr, serialnum, label, doserial, ignore_parent_rel);
		destroy_comparator(compptr);
		return ret;
		}

	template<class T>
	std::pair<const T*, const pattern *> Properties::get_with_pattern_ext(Ex::iterator it, Ex_comparator& comp,
																								 int& serialnum, const std::string& label,
																								 bool doserial, bool ignore_parent_rel) const
		{
		std::pair<const T*, const pattern *> ret;
		ret.first=0;
		ret.second=0;
		bool inherits=false;

		//std::cerr << *it->name_only() << std::endl;
		//	std::cerr << props.size() << std::endl;
		std::pair<property_map_t::const_iterator, property_map_t::const_iterator> pit=props.equal_range(it->name_only());

		// First look for properties of the node itself. Go through the loop twice:
		// once looking for patterns which do not have wildcards, and then looking
		// for wildcard patterns.
		bool wildcards=false;

		// For some properties, we cannot lookup properties lower down the
		// tree, because it would lead to an endless recursion (and it would
		// not make sense anyway). At the moment, this is only for Accent.
		bool ignore_properties=false;
		if(std::is_same<T, Accent>::value) ignore_properties=true;
		
		for(;;) {
			property_map_t::const_iterator walk=pit.first;
			while(walk!=pit.second) {
				if(wildcards==(*walk).second.first->children_wildcard()) {
					// First check property type; a dynamic cast is much faster than a pattern match.
					ret.first=dynamic_cast<const T *>((*walk).second.second);
					if(ret.first) {
						if((*walk).second.first->match_ext(*this, it, comp, ignore_parent_rel, ignore_properties)) {
							ret.second=(*walk).second.first;
							if(!check_label(ret.first, label)) 
								ret.first=0;
							else {
								if(doserial) 
									serialnum=serial_number( (*walk).second.second, (*walk).second.first );
								break;
								}
							}
						}
					ret.first=0;
					if(dynamic_cast<const PropertyInherit *>((*walk).second.second))
						inherits=true;
					else if(dynamic_cast<const Inherit<T> *>((*walk).second.second))
						inherits=true;
					}
				++walk;
				}
			if(!wildcards && !ret.first) {
				//			std::cerr << "not yet found, switching to wildcards" << std::endl;
				wildcards=true;
				}
			else break;
			}

		// If no property was found, figure out whether a property is inherited from a child node.
		if(!ret.first && inherits) {
			//		std::cout << "no match but perhaps inheritance?" << std::endl;
			Ex::sibling_iterator sib=it.begin();
			while(sib!=it.end()) {
				std::pair<const T*, const pattern *> tmp=get_with_pattern<T>((Ex::iterator)(sib), serialnum, label, doserial);
				if(tmp.first) {
					ret=tmp;
					break;
					}
				++sib;
				}
			}

		//	std::cout << ret << std::endl;
		return ret;
		}

	template<class T>
	const T* Properties::get(Ex::iterator it, const std::string& label) const
		{
		int tmp;
		return get<T>(it, tmp, label, false);
		}

	template<class T>
	const T* Properties::get(Ex::iterator it, int& serialnum, const std::string& label, bool doserial) const
		{
		auto ret=get_with_pattern<T>(it, serialnum, label, doserial, false);
		return ret.first;
		}

	template<class T>
	const T* Properties::get(Ex::iterator it1, Ex::iterator it2, bool ignore_parent_rel) const
		{
		int tmp1, tmp2;
		return get<T>(it1,it2,tmp1,tmp2, ignore_parent_rel);
		}

	template<class T>
	const T* Properties::get(Ex::iterator it1, Ex::iterator it2, int& serialnum1, int& serialnum2, bool ignore_parent_rel) const
		{
		const T* ret1=0;
		const T* ret2=0;
		bool found=false;

		bool inherits1=false, inherits2=false;
		std::pair<property_map_t::const_iterator, property_map_t::const_iterator> pit1=props.equal_range(it1->name_only());
		std::pair<property_map_t::const_iterator, property_map_t::const_iterator> pit2=props.equal_range(it2->name_only());

		property_map_t::const_iterator walk1=pit1.first;
		while(walk1!=pit1.second) {
			if((*walk1).second.first->match(*this, it1, ignore_parent_rel)) { // match for object 1 found
				ret1=dynamic_cast<const T *>((*walk1).second.second);
				if(ret1) { // property of the right type found for object 1
					property_map_t::const_iterator walk2=pit2.first;
					while(walk2!=pit2.second) {
						if((*walk2).second.first->match(*this, it2, ignore_parent_rel)) { // match for object 2 found
							ret2=dynamic_cast<const T *>((*walk2).second.second);
							if(ret2) { // property of the right type found for object 2
								if(ret1==ret2 && walk1!=walk2) { // accept if properties are the same and patterns are not
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
			Ex::sibling_iterator sib1, sib2;
			if(inherits1) sib1=it1.begin();
			else          sib1=it1;
			bool keepgoing1=true;
			do {    // 1
				bool keepgoing2=true;
				if(inherits2) sib2=it2.begin();
				else          sib2=it2;
				do { // 2
					const T* tmp=get<T>((Ex::iterator)(sib1), (Ex::iterator)(sib2), serialnum1, serialnum2, ignore_parent_rel);
					if(tmp) {
						ret1=tmp;
						found=true;
						goto done;
						}
					if(!inherits2 || ++sib2==it2.end())
						keepgoing2=false;
					}
				while(keepgoing2);
				if(!inherits1 || ++sib1==it1.end())
					keepgoing1=false;
				}
			while(keepgoing1);
			}

done:
		if(!found) ret1=0;
		return ret1;
		}

	template<class T>
	Ex::iterator Properties::head(Ex::iterator it, bool ignore_parent_rel) const
		{
		Ex::iterator dn=it;
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

	}
