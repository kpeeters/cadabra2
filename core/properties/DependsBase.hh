
#pragma once

#include "Props.hh"

class DependsBase : virtual public property {
   public:
      /// Returns a tree of objects on which the given object depends.
      virtual exptree dependencies(exptree::iterator) const=0;
};

