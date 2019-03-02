
#pragma once

#include <vector>

/// A class to help iterating over all values of multiple objects.
/// Templated over the type of the values. See test_multiindex.cc
/// for a sample use case.

template<class T>
class MultiIndex {
	public:
		typedef std::vector<T> values_type;
		std::vector<values_type> values;

		void start()
			{
			current_pos=std::vector<std::size_t>(values.size(), 0);
			end_=false;
			}

		bool end() const
			{
			return end_;
			}

		MultiIndex& operator++()
			{
			current_pos[0]++;
			std::size_t ci=0;
			while(current_pos[ci] == values[ci].size()) {
				if(ci==current_pos.size()-1) {
					end_=true;
					return *this;
					}
				current_pos[ci++]=0;
				current_pos[ci]++;
				}
			return *this;
			}

		const T& operator[](std::size_t i)
			{
			return values[i][current_pos[i]];
			}

	private:
		std::vector<std::size_t> current_pos;
		bool end_;
	};


