
// Convert a Cadabra .cnb notebook to a Jupyter notebook. input file to pure Python .py, by doing the
// pre-processing stage.

#include <iostream>
#include <CdbPython.hh>
#include <fstream>
#include "nlohmann/json.hpp"
#include "DataCell.hh"

int main(int argc, char **argv)
	{
	if(argc<2) {
		std::cerr << "Usage: cadabra2ipynb [cadabra notebook] [jupyter notebook]\n\n";
		std::cerr << "Convert a Cadabra notebook to a Jupyter notebook.\n"
		          << "If the Jupyter notebook name is not given, output goes to standard out.\n";
		return -1;
		}

	std::string cdb_file, python_file;
	int n=1;
	while(n<argc) {
		if(cdb_file=="")
			cdb_file=argv[n];
		else
			python_file=argv[n];
		++n;
		}

	auto from=cdb_file.find_last_of("/");
	++from;
	auto to  =cdb_file.find_last_of(".");
	std::string t=cdb_file.substr(from, to-from);
	t[0]=toupper(t[0]);
	std::string title="Cadabra manual: "+t;

	std::ifstream file(cdb_file);
	if(!file.is_open()) {
		std::cerr << "cadabra2ipynb: failed to open " << cdb_file << std::endl;
		return -1;
		}
	nlohmann::json content;
	file >> content;

	auto python = cadabra::cnb2ipynb(content);

	if(python_file!="") {
		std::ofstream pythonfile(python_file);
		pythonfile << python.dump(3);
		}
	else {
		std::cout << python.dump(3);
		}

	return 0;
	}
