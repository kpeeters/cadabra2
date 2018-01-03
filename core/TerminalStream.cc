
#include "DisplayTerminal.hh"
#include "TerminalStream.hh"

using namespace cadabra;

TerminalStream::TerminalStream(const Kernel& k, std::ostream& s)
	: kernel(k), out_(s)
	{
	}

TerminalStream& TerminalStream::operator<<(const Ex& ex)
	{
	DisplayTerminal dt(kernel, ex, true);
	dt.output(out_);
	return *this;
	}

TerminalStream& TerminalStream::operator<<(std::shared_ptr<Ex> ex)
	{
	DisplayTerminal dt(kernel, *ex, true);
	dt.output(out_);
	return *this;
	}
