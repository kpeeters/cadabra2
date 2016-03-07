
#pragma once

#include "Props.hh"
#include "Kernel.hh"

/// \ingroup core

class DependsBase : virtual public property {
   public:
      /// Returns a tree of objects on which the given object depends.
      virtual Ex dependencies(const Kernel&, Ex::iterator) const=0;
};

