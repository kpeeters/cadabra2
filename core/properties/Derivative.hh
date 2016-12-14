
#pragma once

#include "properties/CommutingAsProduct.hh"
#include "properties/NumericalFlat.hh"
#include "properties/WeightBase.hh"
#include "properties/TableauBase.hh"
#include "properties/Distributable.hh"
#include "properties/DependsInherit.hh"
#include "properties/IndexInherit.hh"
#include "properties/SortOrder.hh"

class Derivative : public IndexInherit, 
						 public DependsInherit,
//                   public Inherit<Spinor>,
                   public Inherit<SortOrder>,
                   public CommutingAsProduct, 
                   public NumericalFlat,
                   public WeightBase,
                   public TableauBase,
                   public Distributable, virtual public property {
   public :
      virtual ~Derivative() {};
      virtual std::string name() const override;

      virtual unsigned int size(const Properties&, Ex&, Ex::iterator) const override;
      virtual tab_t        get_tab(const Properties&, Ex&, Ex::iterator, unsigned int) const override;
      virtual multiplier_t value(const Kernel&, Ex::iterator, const std::string& forcedlabel) const override;
};

