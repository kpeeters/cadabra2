
// Convert a Cadabra .cdb input file to pure Python .py, by doing the
// pre-processing stage.

#include <iostream>
#include <CdbPython.hh>
#include <fstream>

int main(int argc, char **argv)
	{
	if(argc<2) {
		std::cerr << "Usage: cadabra2python [cadabra file] [python file]\n\n";
		std::cerr << "Convert a Cadabra v2 input file or notebook to a pure Python file.\n"
		          << "If the Python file name is not given, output goes to standard out.\n";
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
		std::cerr << "cadabra2python: failed to open " << cdb_file << std::endl;
		return -1;
		}
	std::string content, line;
	while(std::getline(file, line))
		content+=line+"\n";

	auto python = cadabra::cdb2python(content, true);

	if(python_file!="") {
		std::ofstream pythonfile(python_file);
		pythonfile << python;
		}
	else {
		std::cout << python;
		}

	return 0;
	}
