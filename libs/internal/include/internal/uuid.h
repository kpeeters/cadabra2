#pragma once
#include <stdlib.h>
#include <chrono>
#include <random>
#include <vector>

namespace cadabra
{
	namespace detail
	{
		inline unsigned int get_seed()
		{
			auto tp = std::chrono::system_clock::now();
			return static_cast<unsigned long>(tp.time_since_epoch().count());
		}
	}

	template <typename IntegerT>
	IntegerT generate_uuid()
	{
		static std::random_device rd;
		static std::mt19937 rng(rd());
		static std::uniform_int_distribution<IntegerT> uni(1);

		return uni(rng);
	}
}
