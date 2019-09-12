#include <fstream>
#include <iostream>
#include "DataCell.hh"

int main(int argc, char **argv)
	{
	if(argc<3) {
		std::cerr << "Usage: cadabra2latex [cadabra notebook] [latex file]\n\n";
		std::cerr << "Convert a Cadabra v2 notebook to a standalone LaTeX file (plus images).\n";
		return -1;
		}

	std::string cdb_file=argv[1], latex_file=argv[2];

	std::ifstream file(cdb_file);
	std::string content, line;
	while(std::getline(file, line))
		content+=line;

	cadabra::DTree doc;
	JSON_deserialise(content, doc);
	std::size_t dotpos = latex_file.rfind('.');
	std::string base = latex_file.substr(0, dotpos);
	std::string latex = export_as_LaTeX(doc, base);

	std::ofstream latexfile(latex_file);
	latexfile << latex;

	return 0;
	}
