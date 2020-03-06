#pragma once

#include <memory>
#include <array>
#include "Algorithm.hh"
#include "Adjform.hh"

namespace cadabra {

    class meld : public Algorithm
    {
        public:
            meld(const Kernel& kernel, Ex& ex);
            virtual ~meld();

            virtual bool can_apply(iterator it) override;
            virtual result_t apply(iterator& it) override;

        private:
            void cleanup(iterator it);
            AdjformEx symmetrize(Ex::iterator it);
            
            result_t do_traces(iterator it);
            result_t do_tableaux(iterator it);

            IndexMap index_map;
    };

}
