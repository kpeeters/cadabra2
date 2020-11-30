
#pragma once

#include <string>
#include <chrono>
#include <stack>
#include <map>
#include <vector>

/// \ingroup core
///
/// Object keeping track of time spent in nested execution blocks,
/// and keeping track of out-of-band messages produced by these
/// blocks. The messaging facility is currently experimental.

class ProgressMonitor {
	public:
		ProgressMonitor();
		virtual ~ProgressMonitor();

		/// Start a new named group, or close the innermost one in case
		/// the `name` argument is empty. All time spent inbetween calls
		/// to group open/close calls is accumulated in a `Totals`
		/// object, one for each group name. These `Totals` blocks can
		/// be retrieved from the `totals` function.
		/// FIXME: call this `block`.
		
		void group(std::string name="");

		/// Set the progress of the current top-level block to be `n`
		/// out of `total` steps. It is possible to change `totals` at
		/// every call (e.g. in situations where the algorithm cannot
		/// possibly figure out how many total steps there are to take.
		
		void progress(int n, int total);

		/// Log out-of-band messages to the current block.

		void message(const std::string&);
		
		/// Generate debug output on `cerr`.
		
		void print() const;


		/// Object to accumulate total time and call counts for a
		/// particular named execution group.
		
		class Total {
			public:
				Total();

				std::string               name;
				size_t                    call_count;
				std::chrono::milliseconds time_spent;
				int                       total_steps;
				std::vector<std::string>  messages;

				long                      time_spent_as_long() const;

				bool operator==(const Total& other) const;

				std::string               str() const;
			};

		std::vector<Total> totals() const;

	private:

		/// A single element of the nested `group` call stack.  Every
		/// time a `group` function is called with a non-empty name,
		/// a new `Block` is pushed onto the call stack. When `group`
		/// is called with an empty name, the topmost `Block` is popped
		/// from the stack, and its sub-totals added to the `Total`
		/// element which thus collects data from multiple executions
		/// of identically-named groups.
		
		class Block {
			public:
				Block();

				std::string               name;
				std::chrono::milliseconds started;
				int                       step, total_steps;
				std::vector<std::string>  messages;                
			};

		std::stack<Block>            call_stack;
		std::map<std::string, Total> call_totals;
	};


