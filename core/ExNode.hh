
#pragma once

#include "Storage.hh"
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

class ExNode {
   public:
      ExNode(std::shared_ptr<cadabra::Ex>);
      
      std::shared_ptr<cadabra::Ex>          ex;
      cadabra::Ex::iterator it;

      ExNode& iter();
      ExNode& next();

      std::string __str__() const;
      std::string _latex_() const;      
      
      std::string get_name() const;
      void        set_name(std::string);

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
      
      /// Erase the current node, iterator becomes invalid!
      void        erase();
      
      /// Get a new iterator which always stays
      /// below the current one.
      ExNode      getitem_string(std::string tag);

		/// Get a new iterator which only iterates over all first-level
		/// terms. If the first-level is not a sum, the iterator will
      /// only return the single node and then end.
		ExNode      terms();

		/// Get a new iterator which only iterates over all first-level
		/// factors. If the first-level is not a product, the iterator will
      /// only return the single node and then end.
		ExNode      factors();

      /// Get a new iterator which only iterates over all first-level
		/// indices.
		ExNode      indices();
      
		/// Get a new iterator which only iterates over all first-level
		/// arguments (non-indices).
		ExNode      args();
      
		/// Get a new iterator which iterates over all first-level
		/// children (a sibling iterator, in other words).
		ExNode      children();
      
      std::string tag;
      bool        indices_only, args_only, terms_only, factors_only;

      void update(bool first);
      cadabra::Ex::iterator         nxtit;
		cadabra::Ex::sibling_iterator sibnxtit;
      bool                 use_sibling_iterator;
      cadabra::Ex::iterator         topit, stopit;
};


ExNode Ex_iter(std::shared_ptr<cadabra::Ex> ex);
ExNode Ex_top(std::shared_ptr<cadabra::Ex> ex);
bool   Ex_matches(std::shared_ptr<cadabra::Ex> ex, ExNode& other);
ExNode Ex_getitem_string(std::shared_ptr<cadabra::Ex> ex, std::string tag);


 
