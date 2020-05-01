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
            AdjformEx symmetrize(Ex::iterator it);

				bool can_apply_traces(iterator it);
				bool can_apply_tableaux(iterator it);

            result_t apply_traces(iterator it);
            result_t apply_tableaux(iterator it);

			 bool has_TableauBase(Ex::iterator it);

				 IndexMap index_map;
    };

}
