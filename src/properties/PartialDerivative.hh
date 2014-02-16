
#pragma once

#include "CoreProps.hh"
#include "properties/Derivative.hh"

class PartialDerivative : public Derivative, virtual public property {
   public :
      virtual ~PartialDerivative() {};
      virtual std::string name() const;

      virtual unsigned int size(exptree&, exptree::iterator) const;
      virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

