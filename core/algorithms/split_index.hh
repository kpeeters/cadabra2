
#pragma once

#include "Algorithm.hh"

class split_index : public Algorithm {
	public:
		split_index(Kernel&, exptree&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

	private:
		const Indices *full_class, *part1_class, *part2_class;
		bool     part1_is_number, part2_is_number;
		long     num1, num2;
};
