#include <fstream>
#include <iostream>
#include "DataCell.hh"

int main(int argc, char **argv)
	{
	if(argc<3) {
		std::cerr << "Usage: cadabra2cadabra [cadabra notebook] [cadabra/python output]\n\n";
		std::cerr << "Convert a Cadabra v2 notebook to an standalone Cadabra/Python file.\n"
		          << "If the output file name is not given, output goes to standard out.\n";
		return -1;
		}

	std::string cdb_file, py_file;
	int n=1;
	while(n<argc) {
		if(cdb_file=="")
			cdb_file=argv[n];
		else
			py_file=argv[n];
		++n;
		}

	//	auto from=cdb_file.find_last_of("/");
	//	++from;
	//	auto to  =cdb_file.find_last_of(".");
	//	std::string t=cdb_file.substr(from, to-from);
	//	t[0]=toupper(t[0]);
	//	std::string title="Cadabra manual: "+t;

	std::ifstream file(cdb_file);
	std::string content, line;
	while(std::getline(file, line))
		content+=line;

	cadabra::DTree doc;
	JSON_deserialise(content, doc);
	std::string pycode = export_as_python(doc);

	if(py_file!="") {
		std::ofstream pyfile(py_file);
		pyfile << pycode;
		} else {
		std::cout << pycode;
		}

	return 0;
	}
