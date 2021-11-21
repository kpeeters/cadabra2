
#include "ProgressMonitor.hh"
#include <iostream>
#include <sstream>
#include <assert.h>

ProgressMonitor::ProgressMonitor(std::function<void(const std::string&, int, int)> report, int report_level)
	: report(report)
	, report_level(report_level)
	{
	}

ProgressMonitor::~ProgressMonitor()
	{
	}

ProgressMonitor::Block::Block(const std::string& name, int level)
	: name(name), step(0), total_steps(0), level(level)
	{
	started=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	}

ProgressMonitor::Total::Total()
	: call_count(0), time_spent(0), total_steps(0)
	{
	}

std::string ProgressMonitor::Total::str() const
	{
	std::ostringstream s;

	s << name << ": " << call_count << " calls, " << total_steps << " steps, " << time_spent.count() << " ms";
	for(const auto& msg: messages)
		s << "; " << msg;

	return s.str();
	}

void ProgressMonitor::group(std::string name, int total, int level)
	{
	if(name=="") {
		Block& blk=call_stack.top();
		level = blk.level;
		auto fnd = call_totals.find(blk.name);
		Total& tot=fnd->second;

		tot.name=blk.name;
		auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		tot.time_spent  += (now - blk.started);
		tot.total_steps += blk.total_steps;
		tot.messages.reserve(tot.messages.size() + std::distance(blk.messages.begin(), blk.messages.end()));
		tot.messages.insert(tot.messages.end(), blk.messages.begin(), blk.messages.end());

		call_stack.pop();
		}
	else {
		if (level < 0) {
			if (call_stack.empty())
				level = report_level;
			else
				level = call_stack.top().level;
		}

		// Insert an entry on the call stack.
		Block blk(name, level);
		blk.total_steps = total;
		call_stack.push(blk);

		// Also insert an entry to the total stack for this group if there
		// isn't one already.
		auto fnd=call_totals.find(name);
		if(fnd==call_totals.end()) {
			Total tot;
			tot.call_count=1;
			tot.name=name;
			call_totals[name]=tot;
			}
		else {
			fnd->second.call_count++;
			}
		}

	if (report && level >= report_level) {
		if (call_stack.size() == 0)
			report("Idle", 0, 0);
		else
			report(call_stack.top().name, call_stack.top().step, call_stack.top().total_steps);
		}
	}

void ProgressMonitor::progress()
	{
	assert(!call_stack.empty());
	progress(call_stack.top().step + 1);
	}

void ProgressMonitor::progress(int n)
	{
	assert(!call_stack.empty());
	progress(n, call_stack.top().total_steps);
	}

void ProgressMonitor::progress(int n, int total)
	{
	assert(!call_stack.empty());
	call_stack.top().step=n;
	call_stack.top().total_steps=total;

	if (report && call_stack.top().level >= report_level)
		report(call_stack.top().name, n, total);
	}

void ProgressMonitor::message(const std::string& msg)
	{
	call_stack.top().messages.push_back(msg);
	}

long ProgressMonitor::Total::time_spent_as_long() const
	{
	return time_spent.count();
	}

void ProgressMonitor::print() const
	{
	for(auto& t: call_totals) {
		const Total& tot=t.second;
		std::cerr << tot.name << ": "
		          << tot.call_count << " calls, "
		          //					 << (long)tot.time_spent << " ms, "
		          << tot.total_steps << " steps" << std::endl;
		for(const auto& msg: tot.messages)
			std::cerr << "  " << msg << std::endl;
		}
	}

std::vector<ProgressMonitor::Total> ProgressMonitor::totals() const
	{
	std::vector<Total> res;
	for(auto& t: call_totals)
		res.push_back(t.second);

	return res;
	}

bool ProgressMonitor::Total::operator==(const Total& other) const
	{
	if(name==other.name &&
	      call_count==other.call_count &&
	      time_spent==other.time_spent &&
	      total_steps==other.total_steps) return true;
	return false;
	}

ScopedProgressGroup::ScopedProgressGroup(ProgressMonitor* pm, const std::string& name, int total, int level)
	: pm(pm)
	{
	if (pm)
		pm->group(name, total, level);
	}

ScopedProgressGroup::~ScopedProgressGroup()
	{
	if (pm)
		pm->group();
	}

void ScopedProgressGroup::progress()
	{
	if (pm)
		pm->progress();
	}

void ScopedProgressGroup::progress(int n)
	{
	if (pm)
		pm->progress(n);
	}

void ScopedProgressGroup::progress(int n, int total)
	{
	if (pm)
		pm->progress(n, total);
	}
