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

#pragma once

#include "Props.hh"
#include "YoungTab.hh"

// This file contains property classes which are central to the cadabra
// core and required for basic algorithms to work.

class Symbol : public property {
	public:
		virtual std::string name() const;

		static const Symbol *get(exptree::iterator, bool ignore_parent_rel=false);
};

class Coordinate : public property {
	public:
		virtual std::string name() const;
};

/// Property indicating that an operator is numerically flat, so that
/// numerical factors in the argument can be taken outside.

class NumericalFlat : virtual public property {
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

class IndexInherit : virtual public property {
	public: 
		virtual std::string name() const { return std::string("IndexInherit"); };
};

class DependsBase : virtual public property {
   public:
      /// Returns a tree of objects on which the given object depends.
      virtual exptree dependencies(exptree::iterator) const=0;
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

class WeightBase : virtual public labelled_property {
	public:
		virtual multiplier_t  value(exptree::iterator, const std::string& forcedlabel) const=0;
};

class TableauBase {
	public:
		virtual ~TableauBase() {};
		typedef yngtab::filled_tableau<unsigned int> tab_t;

		virtual unsigned int size(exptree&, exptree::iterator) const=0;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const=0;

		virtual bool         only_column_exchange() const { return false; };

		// Indexgroups are groups of indices which can be sorted by application
		// of single-index monoterm symmetries. E.g. R_{m n p q} -> {m,n}:0, {p,q}:1.
		int                  get_indexgroup(exptree&, exptree::iterator, int) const;

		// Is the tableau either a single column or a single row, and without 
		// duality projections?
		bool                 is_simple_symmetry(exptree&, exptree::iterator) const;
};

class Derivative : public IndexInherit, 
//                   public Inherit<DependsBase>,
//                   public Inherit<Spinor>,
//                   public Inherit<SortOrder>,
                   public CommutingAsProduct, 
                   public NumericalFlat,
                   public WeightBase,
                   public TableauBase,
                   public Distributable, virtual public property {
   public :
      virtual ~Derivative() {};
      virtual std::string name() const;

      virtual unsigned int size(exptree&, exptree::iterator) const;
      virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
      virtual multiplier_t value(exptree::iterator, const std::string& forcedlabel) const;
};

class PartialDerivative : public Derivative, virtual public property {
   public :
      virtual ~PartialDerivative() {};
      virtual std::string name() const;

      virtual unsigned int size(exptree&, exptree::iterator) const;
      virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

