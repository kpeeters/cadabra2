
#pragma once

#include <string>
#include <chrono>
#include <stack>
#include <map>

class ProgressMonitor {
	public:
		ProgressMonitor();
		virtual ~ProgressMonitor();

		virtual void group(std::string name=""); 
		virtual void progress(int n, int total);

	private:
		class Block {
			public:
				Block();

				std::string               name;
				std::chrono::milliseconds started;
				int                       step, total_steps;
		};
		class Total {
			public: 
				Total();
				
				std::string               name;
				size_t                    call_count;
				std::chrono::milliseconds time_spent;
				int                       total_steps;
		};

		std::stack<Block>            call_stack;
		std::map<std::string, Total> totals;
};
