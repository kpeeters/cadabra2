
#include "ProgressMonitor.hh"
#include <iostream>

ProgressMonitor::ProgressMonitor()
	{
	}

ProgressMonitor::~ProgressMonitor()
	{
	}

ProgressMonitor::Block::Block()
	: step(0), total_steps(0)
	{
	started=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	}

ProgressMonitor::Total::Total()
	: call_count(0), time_spent(0), total_steps(0)
	{
	}

void ProgressMonitor::group(std::string name)
	{
	if(name=="") {
		Block& blk=call_stack.top();
		auto fnd = totals.find(blk.name);
		Total& tot=fnd->second;

		tot.name=blk.name;
		auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());		
		tot.time_spent  += (now - blk.started);
		tot.total_steps += blk.total_steps;
		call_stack.pop();
		}
	else {
		// Insert an entry on the call stack.
		Block blk;
		blk.name=name;
		call_stack.push(blk);

		// Also insert an entry to the total stack for this group if there
		// isn't one already.
		auto fnd=totals.find(name);
		if(fnd==totals.end()) {
			Total tot;
			tot.call_count=1;
			tot.name=name;
			totals[name]=tot;
			}
		else {
			fnd->second.call_count++;
			}
		}
	}

void ProgressMonitor::progress(int n, int total)
	{
	call_stack.top().step=n;
	call_stack.top().total_steps=total;
	}

void ProgressMonitor::print() const
	{
	for(auto& t: totals) {
		const Total& tot=t.second;
		std::cerr << tot.name << ": "
					 << tot.call_count << " calls, "
//					 << (long)tot.time_spent << " ms, "
					 << tot.total_steps << " steps" << std::endl;
		}
	}
