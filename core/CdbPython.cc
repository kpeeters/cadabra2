
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "CdbPython.hh"

#ifndef CDBPYTHON_NO_NOTEBOOK
#include "DataCell.hh"
#include "json/json.h"
#endif

std::string cadabra::escape_quotes(const std::string& line)
	{
	return "''"+line+"''";
	//	std::string ret=std::replace_all_copy(line, "'", "\\'");
	//	std::replace_all(ret, "\"", "\\\"");
	//	return ret;
	}

std::string cadabra::cdb2python(const std::string& in_name, bool display)
	{
	// Read the file into a string.
	std::ifstream ifs(in_name);
	std::stringstream buffer;
	buffer << ifs.rdbuf();

	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);

	std::ostringstream ofs;
	ofs << "# cadabra2 package, auto-compiled " << std::put_time(&tm, "%F %T") << '\n'
	    << "# Do not modify - changing the timestamp of this file may cause import errors\n"
	    << "# Original file location: " << in_name << '\n'
	    << "import cadabra2\n"
	    << "import imp\n"
	    << "from cadabra2 import *\n"
		 << "from cadabra2_defaults import *\n"
	    << "__cdbkernel__ = cadabra2.__cdbkernel__\n"
	    << "temp__all__ = dir() + ['temp__all__']\n\n"
	    << "def display(ex):\n"
	    << "   pass\n\n";

	ofs << cdb2python_string(buffer.str(), display);

	ofs << '\n'
	    << "del locals()['display']\n\n"
	    << "try:\n"
	    << "    __all__\n"
	    << "except NameError:\n"
	    << "    __all__  = list(set(dir()) - set(temp__all__))\n";

	return ofs.str();
	}

std::string cadabra::cdb2python_string(const std::string& blk, bool display)
	{
	std::stringstream str(blk);
	std::string line;
	std::string newblk;
	std::string lhs, rhs, op, indent;
	while(std::getline(str, line, '\n')) {
		std::string res=cadabra::convert_line(line, lhs, rhs, op, indent, display);
		// std::cerr << "preparsed : " + res << std::endl;
		if(res!="::empty")
			newblk += res+'\n';
		}
	return newblk;
	}

std::string cadabra::convert_line(const std::string& line, std::string& lhs, std::string& rhs, std::string& op, std::string& indent, bool display)
	{
	std::string ret;

	std::regex imatch("([\\s]*)([^\\s].*[^\\s])([\\s]*)");
	std::cmatch mres;

	std::string indent_line, end_of_line, line_stripped;
	if(std::regex_match(line.c_str(), mres, imatch)) {
		indent_line=std::string(mres[1].first, mres[1].second);
		end_of_line=std::string(mres[3].first, mres[3].second);
		line_stripped=std::string(mres[2]);
		}
	else {
		indent_line="";
		end_of_line="\n";
		line_stripped=line;
		}

	if(line_stripped.size()==0) {
		return "";
		}

	// Do not do anything with comment lines.
	if(line_stripped[0]=='#') return line;

	// Bare ';' gets replaced with 'display(_)'.
	if(line_stripped==";") {
		if(display)
			return indent_line+"display(_)";
		else
			return indent_line;
		}

	// 'lastchar' is either a Cadabra termination character, or empty.
	// 'line_stripped' will have that character stripped, if present.
	std::string lastchar = line_stripped.substr(line_stripped.size()-1,1);
	if(lastchar=="." || lastchar==";" || lastchar==":") {
		if(lhs!="") {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			rhs += line_stripped;
			ret = indent + lhs + " = Ex(r'" + escape_quotes(rhs) + "')";
			if(op==":=") {
				if(ret[ret.size()-1]!=';')
					ret+=";";
				ret+=" _="+lhs;
				}
			if(lastchar!=".")
				if(display)
					ret = ret + "; display("+lhs+")";
			indent="";
			lhs="";
			rhs="";
			op="";
			return ret;
			}
		}
	else {
		// If we are a Cadabra continuation, add to the rhs without further processing
		// and return an empty line immediately.
		if(lhs!="") {
			rhs += line_stripped+" ";
			return "::empty";
			}
		}

	// Add '__cdbkernel__' as first argument of post_process if it doesn't have that already.
	std::regex postprocmatch(R"(def post_process\(([^_]))");
	line_stripped = std::regex_replace(line_stripped, postprocmatch, "def post_process(__cdbkernel__, $1");

	// Replace $...$ with Ex(...).
	std::regex dollarmatch(R"(\$([^\$]*)\$)");
	line_stripped = std::regex_replace(line_stripped, dollarmatch, "Ex(r'''$1''', False)", std::regex_constants::match_default | std::regex_constants::format_default );

	// Replace 'converge(ex):' with 'server.progress('converge'); ex.reset(); while ex.changed(): server.progress(); server.end_progress();' properly indented.
	std::regex converge_match(R"(([ ]*)converge\(([^\)]*)\):)");
	std::smatch converge_res;
	if(std::regex_match(line_stripped, converge_res, converge_match)) {
		ret = indent_line+std::string(converge_res[1])+std::string(converge_res[2])+".reset(); _="+std::string(converge_res[2])+"\n"
		      + indent_line+std::string(converge_res[1])+"while "+std::string(converge_res[2])+".changed():";
		return ret;
		}

	size_t found = line_stripped.find(":=");
	if(found!=std::string::npos) {
		// If the last character is not a Cadabra terminator, start a capture process.
		if(lastchar!="." && lastchar!=";" && lastchar!=":") {
			indent=indent_line;
			lhs=line_stripped.substr(0,found);
			rhs=line_stripped.substr(found+2);
			op=":=";
			return "::empty";
			}
		else {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			ret = indent_line + line_stripped.substr(0,found) + " = Ex(r'"
			      + escape_quotes(line_stripped.substr(found+2)) + "')";
			std::string objname = line_stripped.substr(0,found);
			ret = ret + "; _="+objname;
			if(lastchar==";" && indent_line.size()==0 && display)
				ret = ret + "; display("+objname+")";
			}
		}
	else {   // {a,b,c}::Indices(real, parent=holo);
		found = line_stripped.find("::");
		if(found!=std::string::npos) {
			std::regex amatch(R"(([a-zA-Z]+)(\(.*\))?[;\.:]*)");
			std::smatch ares;
			std::string subline=line_stripped.substr(found+2); // need to store the copy, not feed it straight into regex_match!
			if(std::regex_match(subline, ares, amatch)) {
				auto propname = std::string(ares[1]);
				auto argument = std::string(ares[2]);
				if(argument.size()>0) { // declaration with arguments
					argument=argument.substr(1,argument.size()-2);
					ret = indent_line + "__cdbtmp__ = "+propname
					      +"(Ex(r'"+escape_quotes(line_stripped.substr(0,found))
					      +"'), Ex(r'" +escape_quotes(argument) + "') )";
					}
				else {
					// no arguments
					line_stripped=line_stripped.substr(0,line_stripped.size()-1);
					ret = indent_line + "__cdbtmp__ = " + line_stripped.substr(found+2)
					      + "(Ex(r'"+escape_quotes(line_stripped.substr(0,found))+"'), Ex(r''))";
					}
				if(lastchar==";" && display)
					ret += "; display(__cdbtmp__)";
				}
			else {
				// inconsistent, who knows what will happen...
				ret = line; // inconsistent; you are asking for trouble.
				}
			}
		else {
			if(lastchar==";" && display)
				ret = indent_line + "_ = " + line_stripped + " display(_)";
			else
				ret = indent_line + line_stripped;
			}
		}
	return ret+end_of_line;
	}

#ifndef CDBPYTHON_NO_NOTEBOOK

std::string cadabra::cnb2python(const std::string& in_name, bool for_standalone)
	{
	// Read the file into a Json object and get the cells. We go through
	// a proper notebook read because that way we can benefit from automatic
	// Jupyter -> Cadabra conversion.
	std::ifstream ifs(in_name);
	std::string content, line;
	while(std::getline(ifs, line))
		content+=line;
	cadabra::DTree doc;
	cadabra::JSON_deserialise(content, doc);

	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);

	// Loop over input cells, compile them and write to python file
	std::ostringstream ofs;
	ofs << "# cadabra2 package, auto-compiled " << std::put_time(&tm, "%F %T") << '\n'
	    << "# Do not modify - changing the timestamp of this file may cause import errors\n"
	    << "# Original file location: " << in_name << '\n'
	    << "import cadabra2\n"
	    << "import imp\n"
	    << "from cadabra2 import *\n"
		 << "from cadabra2_defaults import *\n"
	    << "__cdbkernel__ = cadabra2.__cdbkernel__\n"
	    << "temp__all__ = dir() + ['temp__all__']\n\n"
	    << "def display(ex):\n"
	    << "   pass\n\n";

	//	    << "with open(imp.find_module('cadabra2_defaults')[1]) as f:\n"
	//	    << "   code = compile(f.read(), 'cadabra2_defaults.py', 'exec')\n"
	//	    << "   exec(code)\n\n";

	// FIXME: this only does a single layer of cells below the top-level
	// 'document' cell; need recursing, in principle.
	auto docit=doc.begin();
	for(auto cell=doc.begin(docit); cell!=doc.end(docit); ++cell) {
		bool ioi = cell->ignore_on_import;
		if(for_standalone || !ioi) {
			if(cell->cell_type==cadabra::DataCell::CellType::python) {
				std::stringstream s, temp;
				s << cell->textbuf; // cell["source"].asString();
				std::string line, lhs, rhs, op, indent;
				while (std::getline(s, line)) {
					auto res = convert_line(line, lhs, rhs, op, indent, for_standalone);
					if(res!="::empty")
						ofs << res << '\n';
					}
				}
			}
		cell.skip_children();
		}
	// Ensure only symbols defined in this file get exported
	ofs << '\n'
	    << "del locals()['display']\n\n"
	    << "try:\n"
	    << "    __all__\n"
	    << "except NameError:\n"
	    << "    __all__  = list(set(dir()) - set(temp__all__))\n";

	return ofs.str();
	}


// std::string cadabra::cnb2python(const std::string& name)
// 	{
// 	// Only compile if the notebook is newer than the compiled package
// 	struct stat f1, f2;
// 	if (stat(std::string(name + ".cnb").c_str(), &f1) == 0 && stat(std::string(name + ".py").c_str(), &f2) == 0) {
// 		if (f1.st_mtime < f2.st_mtime)
// 			return "";
// 		}
//
// 	// Read the file into a Json object and get the cells
// 	std::ifstream ifs(name + ".cnb");
// 	Json::Value nb;
// 	ifs >> nb;
// 	Json::Value& cells = nb["cells"];
//
// 	// Loop over input cells, compile them and write to python file
//
// 	std::ostringstream ofs;
// 	std::time_t t = std::time(nullptr);
// 	std::tm tm = *std::localtime(&t);
// 	ofs << "# cadabra2 package, auto-compiled " << std::put_time(&tm, "%F %T") << '\n'
// 	    << "import cadabra2\n"
// 	    << "import imp\n"
// 	    << "from cadabra2 import *\n"
// 	    << "__cdbkernel__ = cadabra2.__cdbkernel__\n"
// 	    << "temp__all__ = dir() + ['temp__all__']\n\n"
// 	    << "def display(ex):\n"
// 	    << "   pass\n\n";
//
// //	    << "with open(imp.find_module('cadabra2_defaults')[1]) as f:\n"
// //	    << "   code = compile(f.read(), 'cadabra2_defaults.py', 'exec')\n"
// //	    << "   exec(code)\n\n";
//
// 	for (auto cell : cells) {
// 		if (cell["cell_type"] == "input") {
// 			std::stringstream s, temp;
// 			s << cell["source"].asString();
// 			std::string line, lhs, rhs, op, indent;
// 			while (std::getline(s, line))
// 				ofs << convert_line(line, lhs, rhs, op, indent) << '\n';
// 		}
// 	}
// 	// Ensure only symbols defined in this file get exported
// 	ofs << '\n'
// 	    << "del locals()['display']\n\n"
// 	    << "try:\n"
// 	    << "    __all__\n"
// 	    << "except NameError:\n"
// 	    << "    __all__  = list(set(dir()) - set(temp__all__))\n";
//
// 	return ofs.str();
// 	}
//

#endif