#include <fstream>
#include <iostream>
#include "DataCell.hh"

int main(int argc, char **argv)
	{
	if(argc<3) {
		std::cerr << "Usage: cadabra2html [--segment] [cadabra notebook] [html file]\n\n";
		std::cerr << "Convert a Cadabra v2 notebook to an HTML segment or standalone HTML file.\n"
					 << "If the HTML file name is not given, output goes to standard out.\n";
		return -1;
		}

	std::string cdb_file, html_file;
	bool segment_only=false;
	bool strip_code=false;
	int n=1;
	while(n<argc) {
		if(std::string(argv[n])=="--segment")
			segment_only=true;
		else if(std::string(argv[n])=="--strip-code")
			strip_code=true;
		else if(cdb_file=="")
			cdb_file=argv[n];
		else
			html_file=argv[n];
		++n;
		}
	std::cerr << "stripping code: " << strip_code << std::endl;

	auto from=cdb_file.find_last_of("/");
	++from; 
	auto to  =cdb_file.find_last_of(".");
	std::string t=cdb_file.substr(from, to-from);
	t[0]=toupper(t[0]);
	std::string title="Cadabra manual: "+t;

	std::ifstream file(cdb_file);
	std::string content, line;
	while(std::getline(file, line)) 
		content+=line;

	cadabra::DTree doc;
	JSON_deserialise(content, doc);
	std::string html = export_as_HTML(doc, segment_only, strip_code, title);

	if(html_file!="") {
		std::ofstream htmlfile(html_file);
		htmlfile << html;
		}
	else {
		std::cout << html;
		}

	return 0;
	}
