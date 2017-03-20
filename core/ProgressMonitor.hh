
#pragma once

#include <string>
#include <chrono>
#include <stack>
#include <map>
#include <vector>

class ProgressMonitor {
	public:
		ProgressMonitor();
		virtual ~ProgressMonitor();

		virtual void group(std::string name=""); 
		virtual void progress(int n, int total);

		void print() const;

		class Total {
			public: 
				Total();
				
				std::string               name;
				size_t                    call_count;
				std::chrono::milliseconds time_spent;
				int                       total_steps;

				long                      time_spent_as_long() const;

				bool operator==(const Total& other) const;
		};

		std::vector<Total> totals() const;
		
	private:
		class Block {
			public:
				Block();

				std::string               name;
				std::chrono::milliseconds started;
				int                       step, total_steps;
		};

		std::stack<Block>            call_stack;
		std::map<std::string, Total> call_totals;
};


