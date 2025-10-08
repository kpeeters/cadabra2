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
#include <typeindex>
#include <iterator>
#include <functional>

#include <cassert>

namespace cadabra {

	class Properties;
	class Kernel;
	class Accent;
	class LaTeXForm;
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
			typedef std::pair<std::string, Ex> kvpair_t;
			typedef std::list<kvpair_t>        kvlist_t;

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
			//
			// This is currently also the point where properties are allowed
			// to insert other, related properties (see e.g. `Indices`), and form
			// that reason the Kernel parameter is not const. FIXME: split and
			// then make Kernel const again).
			virtual void        validate(Kernel&, std::shared_ptr<Ex>) const;

			/// Display the property on the stream
			//		virtual void        display(std::ostream&) const;

			/// Generate a LaTeX representation of the property, assuming LaTeX
			/// is in text mode (so it needs dollar symbols to switch to maths).
			virtual void        latex(std::ostream&) const;

			virtual std::string name() const=0;
			virtual std::string unnamed_argument() const;

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
			// To compare list properties we sometimes need to compare their variables, not only
			// their type. The following function needs to be overridden in all properties
			// for which comparison by type is not sufficient to establish equality.
			//
			//   id_match:    only one of these properties can be registered, but their data is not the same
			//   exact_match: these properties are exactly identical
			enum match_t { no_match, id_match, exact_match };
			virtual match_t equals(const property *) const;
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
//				T tmp;
				// T can be abstract, so we cannot instantiate. Maybe use typeid,
				// but then we need to be careful about name mangling.
				return std::string("Inherit");
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
	
			// Class to store names and std::type_index for property objects
			class registered_property_map_t {
				public:
					~registered_property_map_t();

					typedef std::map<std::string, property* (*)()> internal_property_map_t;
					typedef internal_property_map_t::iterator iterator;

					internal_property_map_t store;

					// Modifications to registered_property_map_t.
					// FIXME: Pieces above here are old and do not seem to be used anywhere.
					// (Pieces below here are new.)

					// Register a type by template. Usage: `register_type<T>();`
					template<typename T>
					void register_type();

					// Register the type of an object. Usage: `register_type(prop);`
					void register_type(const property* prop);
					
					// Fuzzy bool type
					enum fbool {False, True, Unknown};
					fbool castable(std::type_index to, std::type_index from);

				private:
					// Dictionary from type_index to human readable name.
					std::map<std::type_index, std::string>       types_to_names_;
					// Dictionary from human readable name to type_index.
					std::multimap<std::string, std::type_index>  names_to_types_;


				};			
			/// Registering properties.  When inserting a property or
			/// list_property, ownership of the property gets transferred to
			/// this class.
			
			// FIXME: register_property does not seem to be used.
			void                          register_property(property* (*)(), const std::string& name);

			registered_property_map_t     registered_properties;
			// typedef std::pair<pattern *, const property *>  pat_prop_pair_t;
			typedef std::pair<const property * const, pattern *>  prop_pat_pair_t;

			// Register a type by template. Usage: `register_property_type<T>();`
			// Just calls `registered_properties.register_type<T>()`
			template<typename T>  void       register_property_type();

			// Register the type of an object. Usage: `register_property_type(prop);`
			// Just calls `registered_properties.register_type(obj)`
			void       register_property_type(const property* prop);



			/// We keep two multi-maps: one from the pattern to the property (roughly) and
			/// one from the property to the pattern. These are both multi-maps because
			/// one pattern can have multiple properties assigned to it, and one property can
			/// be assigned to multiple properties.
			///
			/// When we delete properties, we check the pats map to see if the reference count
			/// for that property has dropped to zero.
			
			// typedef std::multimap<nset_t::iterator, pat_prop_pair_t, nset_it_less>  property_map_t;
			// typedef std::multimap<const property *, pattern *>                      pattern_map_t;

			/// Register a property for the indicated Ex. Takes both normal and list
			/// properties and works out which insert calls to make. The property ownership
			/// is transferred to us on using this call.
			const property* master_insert(Ex proptree, const property *thepropbase);

			void        clear();

			/// The following two maps own the pointers to the properties and patterns stored
			/// in them; use clear() to clean up. Note that pointers can sit in in more than one
			/// entry in this map (when they are pointing to list_property objects, which are
			/// shared between patterns).
			// property_map_t  props;  // pattern -> property
			// pattern_map_t   pats;   // property -> pattern; for list properties, patterns are stored here in order

			typedef std::multimap<const property *, pattern *>				propmap_t;
			typedef std::map<std::type_index, propmap_t>				    typemap_t;
			typedef std::map<nset_t::iterator, typemap_t, nset_it_less>		namemap_t;

			typemap_t  pats_dict;
			namemap_t  props_dict;

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
			// template<class T>
			// property_map_t::iterator      get_pattern();
			// template<class T>
			// property_map_t::iterator      get_pattern(property_map_t::iterator);

			// Equivalent search: given a node, get a pattern of equivalents.
			//		property_map_t::iterator      get_equivalent(Ex::iterator,
			//																	  property_map_t::iterator=props.begin());
					
			/// Erases property completely.
			void erase(const property*);
			/// Erases pattern from a given property, leaving other patterns alone.
			void erase(const property*, pattern*);

			/// Helper function to lookup all patterns associated with a property.
			/// If the property is invalid, it returns a null pointer in the first slot.
			std::pair<const property*, std::vector<const pattern*> > lookup_property(const property*) const;

		private:
			// Insert a property. Do not use this directly, use the public
			// interface `master_insert` instead.
			void insert_prop(const Ex&, const property *);
			const list_property* insert_list_prop(const std::vector<Ex>&, const list_property *);
			bool check_label(const property *, const std::string&) const;
			bool check_label(const labelled_property *, const std::string&) const;			
			// Search through pointers
			bool has(const property *, Ex::iterator);
			// Find serial number of a pattern in a given list property
			int  serial_number(const property *, const pattern *) const;
			Ex_comparator *create_comparator() const;
			void           destroy_comparator(Ex_comparator *) const;			




		public:
			// typedef std::multimap<const property *, pattern *>				propmap_t;
			// typedef std::map<std::type_index, propmap_t>				    typemap_t;

			template<bool IsConst>
			class iterator_base {
			public:
				using map_t = std::conditional_t<IsConst, const typemap_t, typemap_t>;
				using outer_iterator = std::conditional_t<IsConst, typename typemap_t::const_iterator, typename typemap_t::iterator>;
				using inner_iterator = std::conditional_t<IsConst, typename propmap_t::const_iterator, typename propmap_t::iterator>;
				using iterator_category = std::forward_iterator_tag;
				using difference_type   = std::ptrdiff_t;
				using value_type = std::conditional_t<IsConst, const prop_pat_pair_t, prop_pat_pair_t>;
				using pointer = value_type*;
				using reference = value_type&;
				using self_type = iterator_base<IsConst>;

				iterator_base(bool is_end = true) {
					typemap_ = nullptr;
				}

				iterator_base(map_t* m, bool is_end_ = false): typemap_(m) {
					if (is_end_) {
						typemap_ = nullptr;
					} else {
						outer_it_ = typemap_->begin();
						skip_ahead();
					}
				}

				iterator_base(map_t* m, outer_iterator outer, inner_iterator inner)
					: typemap_(m), outer_it_(outer), inner_it_(inner) {
						if (inner_it_ == outer_it_->second.end()) {
							++outer_it_;
							skip_ahead();
						}
					}

				self_type& operator++() {
					++inner_it_;
					if (inner_it_ == outer_it_->second.end()) {
						++outer_it_;
						skip_ahead();
					}
					return *this;
				}

				self_type& next_proptype() {
					++outer_it_;
					skip_ahead();
					return *this;
				}

				self_type& next_prop() {
					const property* this_prop = inner_it_->first;
					while (typemap_ && inner_it_->first == this_prop) {
						operator++();
					}
					return *this;
				}

				std::type_index proptype() const {
					return outer_it_->first;
				}

				const propmap_t& prop_pat_pairs() const {
					return outer_it_->second;
				}

				reference operator*() const { return *inner_it_; }
				pointer operator->() const { return &(*inner_it_); }

				bool operator==(const self_type& other) const {
					return (!typemap_ && !other.typemap_) || (typemap_ == other.typemap_ && outer_it_==other.outer_it_ && inner_it_ == other.inner_it_);
				}

				bool operator!=(const self_type& other) const {
					return !(*this == other);
				}

			protected:
				map_t* typemap_;
				outer_iterator outer_it_;
				inner_iterator inner_it_;

				void skip_ahead() {
					while (outer_it_ != typemap_->end()) {
						if (outer_it_->second.empty()) {
        				    ++outer_it_;
            				continue;
						}
						inner_it_ = outer_it_->second.begin();
						return;
						/*
						if (inner_it_ != outer_it_->second.end()) return;
						++outer_it_;
						*/
					}
					// inner_it_ = inner_iterator();
					// is_end_ = true;
					typemap_ = nullptr;
				}



			};

			typedef iterator_base<false> iterator;
			typedef iterator_base<true>  const_iterator;

			// Create iterator over all property/pattern pairs
			iterator begin() {return iterator(&pats_dict);}
			const_iterator begin() const {return const_iterator(&pats_dict);}

			// Create iterator over all property/pattern pairs with a pattern matching name
			iterator begin(nset_t::iterator name) {
				auto it = props_dict.find(name);
				if (it == props_dict.end()) return iterator{true};
				else return iterator(&(it->second));
			}
			const_iterator begin(nset_t::iterator name) const {
				auto it = props_dict.find(name);
				if (it == props_dict.end()) return const_iterator{true};
				else return const_iterator(&(it->second));
			}
			
			// Create iterator over all property/pattern pairs of a specific type
			iterator begin(std::type_index type) {
				auto it = pats_dict.find(type);
				if (it == pats_dict.end()) return iterator{true};
				else return iterator(&(it->second));
			}
			const_iterator begin(std::type_index type) const {
				auto it = pats_dict.find(type);
				if (it == pats_dict.end()) return const_iterator{true};
				else return const_iterator(&(it->second));
			}

			// Return pair corresponding to begin and end of a property range
			std::pair<iterator, iterator> equal_range(const property *prop) {
				auto it = pats_dict.find(typeid(*prop));
				if (it == pats_dict.end()) return {iterator{true}, iterator{true}};
				auto range = it->second.equal_range(prop);
				return {iterator{&pats_dict, it, range.first}, iterator{&pats_dict, it, range.second}};
			}
			std::pair<const_iterator, const_iterator> equal_range(const property *prop) const {
				auto it = pats_dict.find(typeid(*prop));
				if (it == pats_dict.end()) return {const_iterator{true}, const_iterator{true}};
				auto range = it->second.equal_range(prop);
				return {const_iterator{&pats_dict, it, range.first}, const_iterator{&pats_dict, it, range.second}};
			}

			// All end iterators are the same.
			iterator end() {return iterator(/*is_end=*/ true);}
			const_iterator end() const {return const_iterator(/*is_end=*/ true);}
			

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
		std::pair<const T*, const pattern *> ret = {nullptr, nullptr};

		bool inherits = false;
		//std::cerr << *it->name_only() << std::endl;
		//	std::cerr << props.size() << std::endl;
		
		// First look for properties of the node itself. Go through the loop twice:
		// once looking for patterns which do not have wildcards, and then looking
		// for wildcard patterns.
		bool wildcards=false;

		// For some properties, we cannot lookup properties lower down the
		// tree, because it would lead to an endless recursion (and it would
		// not make sense anyway). At the moment, this is only for Accent.
		bool ignore_properties=false;
		if(std::is_same<T, Accent>::value)
			ignore_properties=true;

		// Outer loop runs first with wildcards = false, second (if needed) with wildcards = true
		
		for(;;) {
			// first pass: wildcards == false
			// second pass (optional): wildcards == true
			auto walk = begin(it->name_only());
			auto end_it = end();
			std::type_index last_type = typeid(void);

			// walk takes us through all properties that have patterns matching name
			while (walk != end_it) {
				// Upon (re)starting the loop, check whether the property type has changed.
				// If it has, check castability and skip if not castable.
				if (last_type != walk.proptype()) {
					// This is the first time we are encountering this property type in the loop.
					last_type = walk.proptype();
					ret.first = dynamic_cast<const T *>(walk->first);
					if (!ret.first) {
						// If we can't dynamically cast this property type, then move on...
						walk.next_proptype();
						continue;
					}
				}
				// We are only dealing with castable properties now.
				if(wildcards==walk->second->children_wildcard()) {
					if(walk->second->match_ext(*this, it, comp, ignore_parent_rel, ignore_properties)) {
						ret.first  = dynamic_cast<const T *>(walk->first);
						assert(ret.first != nullptr);
						ret.second = walk->second;
						if(!check_label(ret.first, label)) {
							ret.first=0;
						}
						else {
							if(doserial) 
								serialnum=serial_number( walk->first, walk->second);
							break;
						}
					}
					ret.first=0;
				}
				++walk;
			}
			if(!wildcards && !ret.first) {
				//			std::cerr << "not yet found, switching to wildcards" << std::endl;
				wildcards=true;
				// walk = begin(it->name_only()); // restarting the for loop
				}
			else break;
		}

		// Do not walk down the tree if the property cannot be passed up the tree.
		// FIXME: see issue/259.
		if(std::is_same<T, LaTeXForm>::value) {
			inherits=false;
		} else if (!ret.first) {
			auto walk = begin(it->name_only());
			auto end_it = end();
			while (walk != end_it) {
				if (dynamic_cast<const Inherit<T>*>(walk->first) || dynamic_cast<const PropertyInherit*>(walk->first)) {
					inherits = true;
					break;
				}
				walk.next_proptype();
			}
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
		bool inherits1=false, inherits2=false;
		
		// Find all common properties of it1 and it2
		auto walk1 = begin(it1->name_only());
		auto walk2 = begin(it2->name_only());
		auto end_it = end();

		if (walk1 != end_it && walk2 != end_it) {
			if (walk1.proptype() < walk2.proptype()) {
				walk1.next_proptype();
			} else if (walk2.proptype() < walk1.proptype()) {
				walk2.next_proptype();
			} else if (!dynamic_cast<const T*>(walk1->first)) {
				walk1.next_proptype();
				walk2.next_proptype();
			} else {
				// walk1 and walk2 are of the same castable property type.
				// Unlike what we do with get() above for a single iterator.
				// we dive in and process here ALL the properties of the same type.

				// Take the property maps
				auto& propmap1 = walk1.prop_pat_pairs();
				auto& propmap2 = walk2.prop_pat_pairs();
				bool match1_found = false;
				bool match2_found = false;
				// By construction, the pat_props are always sorted by property pointer.
				// This allows the iteration below to be as fast as possible.
				for (auto ppit1 = propmap1.begin(), ppit2 = propmap2.begin();
					ppit1 != propmap1.end() && ppit2 != propmap2.end(); ) {
					// Advance both iterators until their properties match
					if (ppit1->first < ppit2->first) {
						++ppit1;
						match1_found = false;
					} else if (ppit2->first < ppit1->first) {
						++ppit2;
						match2_found = false;
					} else {
						// The properties match, so see if there is a pattern match
						if (!match1_found) {
							if (ppit1->second->match(*this, it1, ignore_parent_rel)) {
								match1_found = true;
							} else {
								++ppit1;
								// assert(match1_found == false);
								continue;
							}
						}
						if (!match2_found) {
							if (ppit2->second->match(*this, it2, ignore_parent_rel)) {
								match2_found = true;
							} else {
								++ppit2;
								// assert(match2_found == false);
								continue;
							}
						}
						// assert(match1_found && match2_found)
						
						// accept if properties are the same and patterns are not
						if ((ppit1->first == ppit2->first) && (ppit1->second != ppit2->second)) {
							serialnum1=serial_number( ppit1->first, ppit1->second );
							serialnum2=serial_number( ppit2->first, ppit2->second );
							// Return the recasted property
							return dynamic_cast<const T *>( ppit1->first );
						}
						// Otherwise, continue.
						++ppit1;
						++ppit2;
					}
				}
				walk1.next_proptype();
				walk2.next_proptype();
			}
		}

		// Are any of these properties heritable?
		// FIXME: Below just uses PropertyInherit and not Inherit<T>?
		walk1 = begin(it1->name_only());
		while (walk1 != end_it) {
			if (dynamic_cast<const PropertyInherit*>(walk1->first)) {
				inherits1 = true;
				break;
			}
			walk1.next_proptype();
		}
		walk2 = begin(it2->name_only());
		while (walk2 != end_it) {
			if (dynamic_cast<const PropertyInherit*>(walk2->first)) {
				inherits2 = true;
				break;
			}
			walk2.next_proptype();
		}

		// If no property was found, figure out whether a property is inherited from a child node.
		if(inherits1 || inherits2) {
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
						return tmp;
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
		return nullptr;
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
