
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "CdbPython.hh"
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include "Exceptions.hh"

#ifndef CDBPYTHON_NO_NOTEBOOK
#include "DataCell.hh"
#endif

// #define DEBUG __FILE__
#include "Debug.hh"

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
	    << "from cadabra2 import *\n"
		 << "from cadabra2_defaults import *\n"
	    << "__cdbkernel__ = cadabra2.__cdbkernel__\n"
	    << "temp__all__ = dir() + ['temp__all__']\n\n"
	    << "def display(ex):\n"
	    << "   pass\n\n";

	std::string error;
	ofs << cdb2python_string(buffer.str(), display, error);

	ofs << '\n'
	    << "del locals()['display']\n\n"
	    << "try:\n"
	    << "    __all__\n"
	    << "except NameError:\n"
	    << "    __all__  = list(set(dir()) - set(temp__all__))\n";

	return ofs.str();
	}

std::string cadabra::insert_prefix(const std::string& tmpblk, const std::string& tmpprefix, size_t tmpprefixloc)
	{
	// Search backwards for "[^\\]\n[\w]+" and insert the
	// prefix immediately after that.
	std::regex pattern_rx("([^\\\\])\n[ \t]+");
	auto last = tmpblk.begin();
	last += tmpprefixloc;
	std::sregex_iterator it(tmpblk.begin(), last, pattern_rx);
	std::sregex_iterator end;
	size_t lastpos = std::string::npos;
	while (it != end) {
		lastpos = it->position() + it->length();
		++it;
		}
	std::string ret;
	if(lastpos!=std::string::npos) {
		ret = tmpblk.substr(0, lastpos) + tmpprefix + tmpblk.substr(lastpos);
#ifdef DEBUG
		std::cerr << "tmpblk now: " << ret << std::endl;
#endif
		}
	else {
#ifdef DEBUG
		std::cerr << "cannot find a place to insert prefix" << std::endl;
#endif
		ret = tmpblk;
		}
	
	return ret;
	}

std::string cadabra::cdb2python_string(const std::string& blk, bool display, std::string& report_error)
	{
	std::stringstream str(blk);
	std::string line;
	std::string tmpblk;
	std::vector<std::string> newblks;
	ConvertData cv;

	std::pair<std::string, std::string> res;

	// We collect lines into blocks until they pass python
	// validation. If we ever hit an indentation error, we add
	// previous successful blocks and re-try, successively,
	// until the block compiles again.

	bool early_exit=false;
	std::string error;
		
	while(!early_exit && std::getline(str, line, '\n')) {
		res = cadabra::convert_line(line, cv, display);

//		if(res.second!="::empty")
//			tmpblk += res.first + res.second + "\n";

		if(res.second!="::empty")
			tmpblk += res.second;

		bool previous_step_removed=false; // avoid removing a line, then adding again.
		while(true) {
			int ic = is_python_code_complete(tmpblk, error);
#ifdef DEBUG
			std::cerr << "CHECK:---\n" << tmpblk << "\n---: " << ic << ", " << res.first << std::endl;
#endif
			if(ic==1) { // complete
				newblks.push_back(res.first + tmpblk + "\n");
				tmpblk = "";
				break;
				}
			if(ic==0) { // incomplete
				if(res.first.size()>0)
					tmpblk = insert_prefix(tmpblk, res.first, tmpblk.size());
				tmpblk += "\n";
				break;
				}
			if(ic==-1) { // indentation error
				if(newblks.size()==0) {
					early_exit=true;
					break;
					}
				// Grow block by adding previously ok block,
				// then try compiling again.

				tmpblk = newblks.back() + tmpblk;
				newblks.pop_back();
				if(previous_step_removed) {
					// We are about to add a line which we just removed. This is a genuine
					// indentation error. Return what we have so far and let the code runner
					// fail with proper exception info for the user (if we just throw an
					// exception with the content of 'error' it will refer to 'codeop').
					early_exit=true;
					break;
					}
				previous_step_removed=false;
				}
			if(ic==-2) { 
				// This is either a genuine syntax error, or one line
				// added too many (after a previous 'incomplete' result).
				// Remove the line from the block, then re-run that line
				// separately.
				if(tmpblk != "") {
					if(size_t pos = tmpblk.rfind('\n'); pos != std::string::npos) {
						newblks.push_back(tmpblk.substr(0, pos)+"\n");
						tmpblk = tmpblk.substr(pos+1);
						previous_step_removed=true;
						// re-run
						}
					else {
						early_exit=true;
						break;
						}
					}
				else {
					early_exit=true;
					break;
					}
				}
			}
		}

	// Collect.
#ifdef DEBUG
	std::cerr << "** at exit:" << std::endl;
	for(const auto& blk: newblks)
		std::cerr << "blk: " << blk << std::endl;
	std::cerr << "tmpblk    = " << tmpblk << std::endl;
#endif

	std::string newblk;
	for(const auto& blk: newblks)
		newblk += blk;

	// Add anything still left and cross fingers.
	newblk += tmpblk+"\n";

	if(early_exit) {
		std::cerr << "** EARLY EXIT: " << error << std::endl;
		report_error=error;
		}

	return newblk;
	}

//  1: complete
//  0: incomplete
// -1: indentation error, need backtracking
// -2: syntax error

int cadabra::is_python_code_complete(const std::string& code, std::string& error)
	{
	// The following code prints None, <code object> and <code object>.
	// Make sure that the string you feed here does *not* include the
	// newline on the last line.
	//
	// 
   //  from codeop import *
   //  
   //  str1='''def fun():
   //     print("hello")'''
   //  str2='''def fun():
   //     print("hello")
   //  '''
   //  str3='''print("hello")'''
   //  
   //  print(compile_command(str1, "<string>", "single"))
   //  print(compile_command(str2, "<string>", "single"))
   //  print(compile_command(str3, "<string>", "single"))

   //	pybind11::scoped_interpreter guard{}; // Ensures the Python interpreter is initialized.
	
	// Import the 'codeop' module.
	pybind11::object codeop = pybind11::module_::import("codeop");
	pybind11::object compile_command = codeop.attr("compile_command");

	// std::cerr << "CHECK:\n" << code << "END" << std::endl;
	try {
		pybind11::object result = compile_command(code, "<string>", "single");
		if( result.is_none() ) return 0;
		else                   return 1;
		}
	catch (pybind11::error_already_set& e) {
//		std::cerr << "EXCEPTION: " << e.what() << std::endl;
		error=e.what();
		// if (std::string(e.what()).find("multiple statements found while compiling a single") != std::string::npos) {
		// 	return 0;
		// 	}
		if (std::string(e.what()).find("unexpected EOF") != std::string::npos) {
			return -1;
			}
		if (std::string(e.what()).find("Indentation") != std::string::npos) {
			return -1;
			}
		if (std::string(e.what()).find("SyntaxError") != std::string::npos) {
			// std::cerr << e.what() << std::endl;
			return -2;
			}
		
		throw; // pass all other exceptions through
		}
	}

std::string cadabra::remove_variable_assignments(const std::string& code, const std::string& variable)
	{
//	pybind11::scoped_interpreter guard{};

static std::string removal_code = R"PYTHON(
import ast

class AssignmentRemover(ast.NodeTransformer):
    def __init__(self, var_name):
        self.var_name = var_name

    def visit_Assign(self, node):
        for target in node.targets:
            if isinstance(target, ast.Name) and target.id == self.var_name:
                return ast.Pass()
        return node

    def visit_AnnAssign(self, node):
        if isinstance(node.target, ast.Name) and node.target.id == self.var_name:
            return ast.Pass()
        return node

    def visit_AugAssign(self, node):
        if isinstance(node.target, ast.Name) and node.target.id == self.var_name:
            return ast.Pass()
        return node

def remove_assignments(code: str, var_name: str) -> str:
    try:
        # Python 3.9+
        unparse = ast.unparse
    except AttributeError:
        # Python < 3.9
        import astunparse
        unparse = astunparse.unparse

    tree = ast.parse(code)
    transformer = AssignmentRemover(var_name)
    modified_tree = transformer.visit(tree)
    ast.fix_missing_locations(modified_tree)
    return unparse(modified_tree)
)PYTHON";

	
	try {
		pybind11::exec(removal_code);
		pybind11::object remove_assignments = pybind11::globals()["remove_assignments"];
		pybind11::object result = remove_assignments(code, variable);
		return result.cast<std::string>();
		}
	catch(const pybind11::error_already_set& e) {
		std::cerr << "Python error: " << e.what() << std::endl;
		return code;
		}
	}

bool cadabra::code_contains_variable(const std::string& code, const std::string& variable)
	{
	pybind11::scoped_interpreter guard{};	
		
	static std::string testcode = R"PYTHON(
def contains_variable(code_str, variable_name):
    import ast

    class VariableVisitor(ast.NodeVisitor):
        def __init__(self):
            self.found = False
        
        def visit_Name(self, node):
            if node.id == variable_name:
                self.found = True

    try:
        tree = ast.parse(code_str)
        visitor = VariableVisitor()
        visitor.visit(tree)
        return visitor.found
    except SyntaxError:
        return False
   )PYTHON";

	try {
		pybind11::exec(testcode);
		pybind11::object contains_variable = pybind11::globals()["contains_variable"];
		pybind11::object result = contains_variable(code, variable);
		return result.cast<bool>();
		}
	catch(const pybind11::error_already_set& e) {
		std::cerr << "Python error: " << e.what() << std::endl;
		return false;
		}
	}

bool cadabra::variables_to_pull_in(const std::string& code, std::set<std::string>& variables)
	{
	std::regex pullin_rx(R"(@\(([^\)]*)\))");

	auto it = std::sregex_iterator(code.begin(), code.end(), pullin_rx);
	while(it != std::sregex_iterator()) {
		std::smatch m = *it;
		variables.insert(m[1].str());
		++it;
		}

	return true;
	}


bool cadabra::variables_in_code(const std::string& code, std::set<std::string>& variables)
	{
	// It would be great if we could parse Python code and then walk
	// the AST directly using the Python C API. However, that does not
	// seem to exist (or rather, it seems to have been removed, as the
	// `ast.h` header is no longer public). So we have to do all this
	// in Python using the `ast` module, and then move the resulting
	// variable list to the C++ side.
	
	static std::string testcode = R"CODE(
def variable_set(code_str):
    """
    Collect all variable names in a piece of Python code.
    Returns a set of names.
    """
    import ast

    class VariableVisitor(ast.NodeVisitor):
        def __init__(self):
            self.names = set()
        
        def generic_visit(self, node):
            # We still need to visit all children even if parent assignment fails
            for child in ast.iter_child_nodes(node):
                try:
                    child.parent = node
                except (TypeError, AttributeError):
                    # If we can't set parent, continue without it
                    pass
                self.visit(child)
        
        def visit_Name(self, node):
            # For names, if we can check the context do so, otherwise include the name
            try:
                parent = getattr(node, 'parent', None)
                if parent is not None:
                    # Only exclude if we're certain it's a function call
                    if isinstance(parent, ast.Call) and parent.func == node:
                        return

            except (TypeError, AttributeError):
                pass
            
            # Include the name unless we explicitly determined it's a function call
            self.names.add(node.id)

    try:
        # Parse the code into an AST
        tree = ast.parse(code_str)
        visitor = VariableVisitor()
        visitor.visit(tree)
        return visitor.names
    except SyntaxError:
        # Handle invalid Python code
        return False
)CODE";

	try {
		pybind11::exec(testcode);
		pybind11::object variable_set = pybind11::globals()["variable_set"];
		pybind11::object result = variable_set(code);

		variables = result.cast<std::set<std::string>>();

		// Add the variables inside '@(...)' wrappers.
		
		
		return true;
		}
	catch(const pybind11::error_already_set& e) {
		std::cerr << "Python error: " << e.what() << std::endl;
		return false;
		}
	}


cadabra::ConvertData::ConvertData()
	{
	}

cadabra::ConvertData::ConvertData(const std::string& lhs_, const std::string& rhs_,
											 const std::string& op_, const std::string& indent_)
	: lhs(lhs_), rhs(rhs_), op(op_), indent(indent_)
	{
	}

std::string replace_dollar_expressions(const std::string& input, 
													const std::function<std::string(const std::string&)>& replacer)
	{
	std::ostringstream result;
	bool in_single_quote = false;
	bool in_double_quote = false;
	size_t dollar_start = std::string::npos;
	
	for (size_t i = 0; i < input.length(); ++i) {
		char c = input[i];
      
		// Toggle quote state
		if (c == '"' && !in_single_quote && dollar_start == std::string::npos) {
			in_double_quote = !in_double_quote;
			result << c;
        }
		else if (c == '\'' && !in_double_quote && dollar_start == std::string::npos) {
			in_single_quote = !in_single_quote;
			result << c;
			}
		// Handle dollar signs outside of quotes
		else if (c == '$' && !in_single_quote && !in_double_quote) {
			if (dollar_start == std::string::npos) {
				// First dollar sign
				dollar_start = i;
				// Don't append the $ yet, wait until we find the matching one
            }
			else {
				// Second dollar sign, found a match
				std::string content = input.substr(dollar_start + 1, i - dollar_start - 1);
				result << replacer(content);
				dollar_start = std::string::npos;
            }
        }
		else {
			// Regular character
			if (dollar_start == std::string::npos) {
				// Not in the middle of a $...$ expression
				result << c;
            }
			// If we're between $...$, don't add anything yet
			}
		}
	
	// Handle unclosed dollar
	if (dollar_start != std::string::npos) {
		result << input.substr(dollar_start);
		}
	
	return result.str();
	}

std::pair<std::string, std::string> cadabra::convert_line(const std::string& line, ConvertData& cv, bool display)
	{
	std::string ret, prefix;

	auto& lhs    = cv.lhs;
	auto& rhs    = cv.rhs;
	auto& op     = cv.op;
	auto& indent = cv.indent;

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
		return std::make_pair(prefix, "");
		}

	// Do not do anything with comment lines.
	if(line_stripped[0]=='#') return std::make_pair(prefix, line);

	// Bare ';' gets replaced with 'display(_)' but *only* if we have no
	// preceding lines which have not finished parsing.
	if(line_stripped==";" && lhs=="") {
		if(display)
			return std::make_pair(prefix, indent_line+"display(_)");
		else
			return std::make_pair(prefix, indent_line);
		}

	// 'lastchar' is either a Cadabra termination character, or empty.
	// 'line_stripped' will have that character stripped, if present.
	std::string lastchar = line_stripped.substr(line_stripped.size()-1,1);
	if(lastchar=="." || lastchar==";" || lastchar==":") {
		if(lhs!="") {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			rhs += " "+line_stripped;
			if(lhs.find('(')==std::string::npos) {
				ret = indent + lhs + " = Ex(r'" + escape_quotes(rhs) + "')";
				}
			else {
				ret = indent + "def " + lhs + ": return Ex(r'" + escape_quotes(rhs) + "')";
				}
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
			return std::make_pair(prefix, ret);
			}
		}
	else {
		// If we are a Cadabra continuation, add to the rhs without further processing
		// and return an empty line immediately.
		if(lhs!="") {
			rhs += line_stripped+" ";
			return std::make_pair(prefix, "::empty");
			}
		}

	// Add '__cdbkernel__' as first argument of post_process if it doesn't have that already.
	std::regex postprocmatch(R"(def post_process\(([^_]))");
	line_stripped = std::regex_replace(line_stripped, postprocmatch, "def post_process(__cdbkernel__, $1");

	// Replace $...$ with Ex(...).
	auto replacement = [](const std::string& content) {
		return "Ex(r'''" + content + "''', False)";
		};
    
	line_stripped = replace_dollar_expressions(line_stripped, replacement);
	DEBUGLN( std::cerr << "line_stripped = " << line_stripped << std::endl; );
	
// 	std::regex dollarmatch(R"(\$([^\$]*)\$)");
// 	line_stripped = std::regex_replace(line_stripped, dollarmatch, "Ex(r'''$1''', False)", std::regex_constants::match_default | std::regex_constants::format_default );

	// Replace 'converge(ex):' with 'server.progress('converge'); ex.reset(); while ex.changed(): server.progress(); server.end_progress();' properly indented.
	std::regex converge_match(R"(([ ]*)converge\(([^\)]*)\):)");
	std::smatch converge_res;
	if(std::regex_match(line_stripped, converge_res, converge_match)) {
		ret = indent_line+std::string(converge_res[1])+std::string(converge_res[2])+".reset(); _="+std::string(converge_res[2])+"\n"
		      + indent_line+std::string(converge_res[1])+"while "+std::string(converge_res[2])+".changed():";
		return std::make_pair(prefix, ret);
		}

	size_t found = line_stripped.find(":=");
	if(found!=std::string::npos) {
		// If the last character is not a Cadabra terminator, start a capture process.
		if(lastchar!="." && lastchar!=";" && lastchar!=":") {
			indent=indent_line;
			lhs=line_stripped.substr(0,found);
			rhs=line_stripped.substr(found+2);
			op=":=";
			return std::make_pair(prefix, "::empty");
			}
		else {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			if(line_stripped.substr(0, found).find('(')==std::string::npos) {
				ret = indent_line + line_stripped.substr(0,found) + " = Ex(r'"
					+ escape_quotes(line_stripped.substr(found+2)) + "')";
				}
			else {
				ret = indent_line + "def " + line_stripped.substr(0,found) + ": return Ex(r'"
			      + escape_quotes(line_stripped.substr(found+2)) + "')";
				}
			std::string objname = line_stripped.substr(0,found);
			ret = ret + "; _="+objname;
			if(lastchar==";" && /* indent_line.size()==0 && */ display)
				ret = ret + "; display("+objname+")";
			}
		}
	else {   // {a,b,c}::Indices(real, parent=holo);
		found = line_stripped.find("::");
		if(found!=std::string::npos) {
			std::regex amatch(R"(([a-zA-Z]+)(\(.*\))?[ ]*[;\.:]*)");
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
			if(lastchar==";" && display) {
				prefix = "_ = " + prefix;
				ret = indent_line + line_stripped + " display(_)";
				}
			else {
				ret = indent_line + line_stripped;
				}
			}
		}
	return std::make_pair(prefix, ret+end_of_line);
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
	    << "from cadabra2 import *\n"
		 << "from cadabra2_defaults import *\n"
	    << "__cdbkernel__ = cadabra2.__cdbkernel__\n"
	    << "temp__all__ = dir() + ['temp__all__']\n\n"
	    << "def display(ex):\n"
	    << "   pass\n\n";


	// FIXME: this only does a single layer of cells below the top-level
	// 'document' cell; need recursing, in principle.
	auto docit=doc.begin();
	for(auto cell=doc.begin(docit); cell!=doc.end(docit); ++cell) {
		bool ioi = cell->ignore_on_import;
		if(for_standalone || !ioi) {
			if(cell->cell_type==cadabra::DataCell::CellType::python) {
				std::stringstream s, temp;
				s << cell->textbuf; // cell["source"].asString();
				ConvertData cv;
//				std::string line, lhs, rhs, op, indent;
				while (std::getline(s, line)) {
					std::pair<std::string, std::string> res
						= convert_line(line, cv, for_standalone); // lhs, rhs, op, indent, for_standalone);
					if(res.second!="::empty")
						ofs << res.second << '\n';
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
