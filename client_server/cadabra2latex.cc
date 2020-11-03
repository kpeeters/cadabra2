#include <fstream>
#include <iostream>
#include <regex>
#include "DataCell.hh"

int main(int argc, char **argv)
	{
	if(argc<3) {
		std::cerr << "Usage: cadabra2latex [--segment] [cadabra notebook] [latex file]\n\n";
		std::cerr << "Convert a Cadabra v2 notebook to a standalone LaTeX file (plus images).\n";
		return -1;
		}

	int i=1;
	bool for_embedding=false;
	if(argc==4) {
		if(std::string(argv[1])=="--segment") {
			for_embedding=true;
			}
		++i;
		}
	std::string cdb_file=argv[i], latex_file=argv[i+1];

	std::ifstream file(cdb_file);
	std::string content, line;
	while(std::getline(file, line))
		content+=line;

	cadabra::DTree doc;
	JSON_deserialise(content, doc);
	std::size_t dotpos = latex_file.rfind('.');
	std::string base = latex_file.substr(0, dotpos);
	std::string latex = export_as_LaTeX(doc, base, for_embedding);

	if(for_embedding) {
		// Ensure all sections are numbered if this will be embedded in a larger
		// document.
		latex=std::regex_replace(latex, std::regex(R"(\\section\*\{)"), "\\section\{");
		latex=std::regex_replace(latex, std::regex(R"(\\subsection\*\{)"), "\\subsection\{");
		latex=std::regex_replace(latex, std::regex(R"(\\LaTeX\{\})"), "LaTeX{}");				
		}

	std::ofstream latexfile(latex_file);
	latexfile << latex;

	return 0;
	}
