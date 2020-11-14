
#pragma once

#include "Algorithm.hh"
#include "IndexIterator.hh"

namespace cadabra {

	class eliminate_converter : public Algorithm {
		public:
			eliminate_converter(const Kernel&, Ex&, Ex&, bool);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		protected:
			virtual bool is_conversion_object(iterator) const=0;

		private:
			Ex          preferred;
			index_map_t ind_dummy, ind_free;
            bool        redundant;

			/// See if the conversion which turns index 'i1' into index
			/// 'i2' can be applied on the expression, so that it gets a
			/// factor in the expression closer to the 'fit' form.
			/// The two indices 'i1' and 'i2' are objects on the convertor
			/// object, so they need to appear with opposite parent rel
			/// in the expression if they are to be applied (if the index
			/// position type is 'fixed').
			bool handle_one_index(index_iterator i1, index_iterator i2, iterator fit, sibling_iterator objs);
		};


	class eliminate_metric : public eliminate_converter {
		public:
			eliminate_metric(const Kernel&, Ex&, Ex&, bool);

		protected:
			virtual bool is_conversion_object(iterator) const override;
		};

	}
