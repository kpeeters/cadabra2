
#pragma once

#include "Props.hh"

class DependsBase : virtual public property {
   public:
      /// Returns a tree of objects on which the given object depends.
      virtual Ex dependencies(Ex::iterator) const=0;
};

