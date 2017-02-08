
#include "CdbPython.hh"
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

std::string cadabra::escape_quotes(const std::string& line)
	{
	return "''"+line+"''";
//	std::string ret=boost::replace_all_copy(line, "'", "\\'");
//	boost::replace_all(ret, "\"", "\\\"");
//	return ret;
	}


std::string cadabra::cdb2python(const std::string& blk)
	{
	std::stringstream str(blk);
	std::string line;
	std::string newblk;
	std::string lhs, rhs, indent;
	while(std::getline(str, line, '\n')) {
		std::string res=cadabra::convert_line(line, lhs, rhs, indent);
		// std::cerr << "preparsed : " + res << std::endl;
		if(res!="::empty")
			newblk += res+'\n';
		}
	return newblk;
	}

std::string cadabra::convert_line(const std::string& line, std::string& lhs, std::string& rhs, std::string& indent)
	{
	std::string ret;
	
	boost::regex imatch("([\\s]*)([^\\s].*[^\\s])([\\s]*)");
	boost::cmatch mres;
	
	std::string indent_line, end_of_line, line_stripped;
	if(boost::regex_match(line.c_str(), mres, imatch)) {
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
	if(line_stripped==";") return indent_line+"display(_)";

	// 'lastchar' is either a Cadabra termination character, or empty.
	// 'line_stripped' will have that character stripped, if present.
	std::string lastchar = line_stripped.substr(line_stripped.size()-1,1);
	if(lastchar=="." || lastchar==";" || lastchar==":") {
		if(lhs!="") {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			rhs += line_stripped;
			ret = indent + lhs + " = Ex(r'" + escape_quotes(rhs) + "')";
			if(lastchar!=".")
				ret = ret + "; display("+lhs+")";
			indent="";
			lhs="";
			rhs="";
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
	
	// Replace $...$ with Ex(...).
	boost::regex dollarmatch(R"(\$([^\$]*)\$)");
	line_stripped = boost::regex_replace(line_stripped, dollarmatch, "Ex\\(r'''$1''', False\\)", boost::match_default | boost::format_all);
	
	// Replace 'converge(ex):' with 'ex.reset(); while ex.changed():' properly indented.
	boost::regex converge_match(R"(([ ]*)converge\(([^\)]*)\):)");
	boost::smatch converge_res;
	if(boost::regex_match(line_stripped, converge_res, converge_match)) {
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
			return "::empty";
			}
		else {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			ret = indent_line + line_stripped.substr(0,found) + " = Ex(r'" 
				+ escape_quotes(line_stripped.substr(found+2)) + "')";
			std::string objname = line_stripped.substr(0,found);
			if(lastchar==";" && indent_line.size()==0)
				ret = ret + "; display("+objname+")";
			}
		}
	else { // {a,b,c}::Indices(real, parent=holo);
		found = line_stripped.find("::");
		if(found!=std::string::npos) {
			boost::regex amatch(R"(([a-zA-Z]+)(.*)[;\.:]*)");
			boost::smatch ares;
			std::string subline=line_stripped.substr(found+2); // need to store the copy, not feed it straight into regex_match!
			if(boost::regex_match(subline, ares, amatch)) {
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
						+ "(Ex(r'"+escape_quotes(line_stripped.substr(0,found))+"'))";
					}
				if(lastchar==";") 
					ret += "; display(__cdbtmp__)";
				}
			else {
				// inconsistent, who knows what will happen...
				ret = line; // inconsistent; you are asking for trouble.
				}
			}
		else {
			if(lastchar==";") 
				ret = indent_line + "_ = " + line_stripped + " display(_)";
			else
				ret = indent_line + line_stripped;
			}
		}
	return ret+end_of_line;
	}
