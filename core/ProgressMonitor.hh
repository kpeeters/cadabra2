
#pragma once

#include <string>


class ProgressMonitor {
	public:
		virtual void group(std::string name="") =0; 
		virtual void progress(int n, int total) =0;
};
