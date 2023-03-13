
#pragma once

#include "Kernel.hh"
#include "Storage.hh"
#include "IndexIterator.hh"
#include "IndexClassifier.hh"
#include <memory>
#include <pybind11/pybind11.h>

/// ExNode is a combination of an Ex::iterator and an interface which
/// we can use to manipulate the data pointed to by this iterator.
/// In this way, we can use
///
///   for it in ex:
///      ...
///
/// loops and still use 'it' to do things like insertion etc.
/// which requires knowing the Ex::iterator.
///
/// Iterators are much safer than in C++, because they carry the
/// tree modification interface themselves, and can thus compute
/// their next value for any destructive operation.
///
/// Note that ExNode does not really behave like a Python iterator
/// in the strict sense: it does not return copies of nodes in the
/// tree, but rather objects which know how to modify the tree.

class ExNode : public cadabra::IndexClassifier {
	public:
		ExNode(const cadabra::Kernel&, std::shared_ptr<cadabra::Ex>);

		std::shared_ptr<cadabra::Ex>          ex;
		cadabra::Ex::iterator it;

		ExNode& iter();
		ExNode& next();

		ExNode copy() const;

		std::string __str__() const;
		std::string _latex_() const;
	   std::string input_form() const;
	
		std::string get_name() const;
		void        set_name(std::string);

		/// Create a copy of the Ex pointed to by this iterator.
		cadabra::Ex get_ex() const;
		
		cadabra::str_node::parent_rel_t get_parent_rel() const;
		void                            set_parent_rel(cadabra::str_node::parent_rel_t);

		pybind11::object get_multiplier() const;
		void             set_multiplier(pybind11::object);

		/// Take a child argument out of the node and
		/// add as child of current.
		//      ExNode      unwrap(ExNode child);

		/// Replace the subtree at the current node with the given
		/// expression. Updates the iterator so that it points to the
		/// replacement subtree.
		void        replace(std::shared_ptr<cadabra::Ex> rep);

		/// Insert a subtree as previous sibling of the current node.
		ExNode      insert(std::shared_ptr<cadabra::Ex>    ins);
		ExNode      insert_it(ExNode ins);

		/// Append a subtree as a child. Return an ExNode pointing to the new child.
		ExNode      append_child(std::shared_ptr<cadabra::Ex>);
		ExNode      append_child_it(ExNode ins);

		/// Add an expression to the given node, adding a sum parent if necessary.
		ExNode      add_ex(std::shared_ptr<cadabra::Ex>);

		/// Erase the current node, iterator becomes invalid!
		void        erase();

		/// Get a new iterator which always stays
		/// below the current one.
		ExNode      getitem_string(std::string tag);
		ExNode      getitem_iterator(ExNode);

		/// Set all elements with the indicated tag to the given value.
		void        setitem_string(std::string tag, std::shared_ptr<cadabra::Ex> val);
		void        setitem_iterator(ExNode, std::shared_ptr<cadabra::Ex> val);

		/// Get a new iterator which only iterates over all first-level
		/// terms. If the first-level is not a sum, the iterator will
		/// only return the single node and then end.
		ExNode      terms();

		/// Get a new iterator which only iterates over all first-level
		/// factors. If the first-level is not a product, the iterator will
		/// only return the single node and then end.
		ExNode      factors();

		/// Get a new iterator which only iterates over all first-level indices
		/// (that is, does not iterate over inherited indices; use 'indices' for that).
		ExNode      own_indices();

		/// Get a new iterator which only iterates over all indices (whether direct
		/// or inherited). Uses an index_iterator internally (see its documentation
		/// for details on behaviour).
		ExNode      indices();

		/// Get a new iterator which only iterates over all free indices (whether direct
		/// or inherited). Uses an index_iterator internally (see its documentation
		/// for details on behaviour).
		ExNode      free_indices();

		/// Get a new iterator which only iterates over all first-level
		/// arguments (non-indices).
		ExNode      args();

		/// Get a new iterator which iterates over all first-level
		/// children (a sibling iterator, in other words).
		ExNode      children();

		std::string tag;
		bool        indices_only, args_only, terms_only, factors_only;

		/// Internal function to update the iterator to the next value.
		/// Switches behaviour depending on use_sibling_iterator, use_index_iterator,
		/// indices_only, args_only, terms_only, factors_only;
		void update(bool first);

		cadabra::Ex::iterator         nxtit;
		cadabra::Ex::sibling_iterator sibnxtit;
		cadabra::index_iterator       indnxtit;

		bool                          use_sibling_iterator;
		bool                          use_index_iterator;
		cadabra::Ex::iterator         topit, stopit;

		index_map_t                   ind_free, ind_dummy;
		index_position_map_t          ind_pos_dummy;

	private:
	};


ExNode Ex_iter(std::shared_ptr<cadabra::Ex> ex);
ExNode Ex_top(std::shared_ptr<cadabra::Ex> ex);
bool   Ex_matches(std::shared_ptr<cadabra::Ex> ex, ExNode& other);
bool   Ex_matches_Ex(std::shared_ptr<cadabra::Ex> ex, std::shared_ptr<cadabra::Ex> other);
bool   ExNode_less(ExNode& one, ExNode& two);
bool   ExNode_greater(ExNode& one, ExNode& two);
ExNode Ex_getitem_string(std::shared_ptr<cadabra::Ex> ex, std::string tag);
ExNode Ex_getitem_iterator(std::shared_ptr<cadabra::Ex> ex, ExNode);



