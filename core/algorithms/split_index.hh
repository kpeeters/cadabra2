
#pragma once

#include "Algorithm.hh"
#include "properties/Indices.hh"
#include "properties/Coordinate.hh"

namespace cadabra {

	class split_index : public Algorithm {
		public:
			split_index(const Kernel&, Ex&, Ex&);
			
			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);
			
		private:
			const Indices *full_class, *part1_class, *part2_class;
			const Coordinate *part1_coord, *part2_coord;
			bool     part1_is_number, part2_is_number;
			long     num1, num2;
			
			iterator part1_coord_node, part2_coord_node;
	};

}
