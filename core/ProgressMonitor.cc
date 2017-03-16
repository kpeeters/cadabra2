
#include "ProgressMonitor.hh"

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

void ProgressMonitor::group(std::string name)
	{
	if(name=="") {
		call_stack.pop();
		}
	else {
		Block blk;
		blk.name=name;
		call_stack.push(blk);
		}
	}

void ProgressMonitor::progress(int n, int total)
	{
	call_stack.top().step=n;
	call_stack.top().total_steps=total;
	}
