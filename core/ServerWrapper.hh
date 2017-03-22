
#pragma once

#include "ProgressMonitor.hh"
#include <boost/python/implicit.hpp>

/// A class which enables access to a Python-side defined Server
/// object with ProgressMonitor interface. It forwards calls
/// on the group/progress members via Boost.Python to the Python
/// defined Server object, thereby allowing the ProgressMonitor
/// functionality to be exposed to Cadabra algorithms through a
/// a single C++ interface, independent of whether it is implemented
/// in the C++ Server object or in the Python one.

class ServerWrapper : public ProgressMonitor {
	public:
		ServerWrapper();
		
		virtual void group(std::string name="") override;
		virtual void progress(int n, int total) override;

	private:
		boost::python::object server;
		boost::python::object python_group, python_progress;
};
