
#pragma once

#include "CoreProps.hh"
#include "properties/Distributable.hh"

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

