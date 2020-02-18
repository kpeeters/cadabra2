#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include "CdbPython.hh"
#include "cadabra2-cli.hh"
#include "Config.hh"
#include "InstallPrefix.hh"

namespace py = pybind11;
using namespace py::literals;

#define CHECK_BIT(var, pos) 
#define NO_BANNER 1
#define IGNORE_SEMICOLON 2

Shell::Shell(unsigned int flags)
	: ps1("> ")
	, ps2(". ")
	, flags(flags)
{
	restart();
}

void Shell::restart()
{
	guard = std::make_unique<py::scoped_interpreter>();
	locals.clear();
	is_running = true;
	return_code = 0;
}

int Shell::interact()
{
	std::string module_path = cadabra::install_prefix() + "/share/cadabra2/python";

	py::exec("import sys; sys.path.append('" + module_path + "')");
	py::eval_file(module_path + "/cadabra2_defaults.py");

	if (!(flags & NO_BANNER)) {
		py::print("Cadabra " CADABRA_VERSION_FULL " (build " CADABRA_VERSION_BUILD " dated " CADABRA_VERSION_DATE ")"); 
		py::print("Copyright (C) " COPYRIGHT_YEARS "  Kasper Peeters <kasper.peeters@phi-sci.com>"); 
		py::print("Using SymPy version", py::eval("sympy.__version__"));
	}

	std::string curline;
	is_running = true;
	while (is_running) {
		if (collect.empty()) {
			std::cout << ps1;
			std::getline(std::cin, curline);
			process_ps1(curline);
		}
		else {
			std::cout << ps2;
			std::getline(std::cin, curline);
			process_ps2(curline);
		}
	}

	return return_code;
}

int Shell::run_script(const std::string& filename)
{
	std::ifstream f(filename);
	if (!f.is_open()) {
		std::cout << "No file: " << filename << "\n";
		return -1;
	}
	std::stringstream buf;
	buf << f.rdbuf();
	auto code = cadabra::cdb2python(buf.str(), !(flags & IGNORE_SEMICOLON));
	try {
		execute(code);
	}
	catch (py::error_already_set& err) {
		std::string msg = err.what();
		process_error(msg);
		return (return_code == 0 ? -1 : return_code);
	}
	return 0;
}

void Shell::execute(const std::string& line)
{
	py::exec(line, py::globals(), locals);
}

bool Shell::try_evaluate(const std::string& s)
{
	try {
		py::print("About to evaluate: ", s);
		auto res = py::eval(s, py::globals(), locals);
		py::print("Evaluated!");
		if (!res.is_none()) {
			py::print("Result was not None, printing...");
			py::print(res);
			locals["_"] = res;
		}
		return true;
	}
	catch (py::error_already_set& err) {
		std::string msg = err.what();
		if (msg.find("SyntaxError") == 0) {
			return false;
		}
		else {
			process_error(msg);
			return true;
		}

	}
}

void Shell::process_ps1(const std::string& line)
{
	std::string lhs, rhs, op, indent;
	std::string output = cadabra::convert_line(line, lhs, rhs, op, indent, false);
	if (output == "::empty") {
		collect += line + "\n";
		return;
	}

	if (try_evaluate(output))
		return;

	try {
		execute(output);
	}
	catch (const py::error_already_set& err) {
		std::string msg = err.what();
		if (msg.find("SyntaxError") == 0 && msg.find("unexpected EOF while parsing") != std::string::npos) {
			collect += line + "\n";
		}
		else {
			process_error(msg);
		}
	}
}

void Shell::process_ps2(const std::string& line)
{
	if (!line.empty()) {
		collect += line + "\n";
		return;
	}

	try {
		execute(cadabra::cdb2python_string(collect, !(flags & IGNORE_SEMICOLON)));
		collect.clear();
	}	
	catch (py::error_already_set& err) {
		std::string msg = err.what();
		process_error(msg);
	}
}

void Shell::process_error(const std::string& msg)
{
	if (msg.find("SystemExit") == 0) {
		process_system_exit(msg);
	}
	else if (msg.find("SyntaxError") == 0) {
		process_syntax_error(msg);
	}
	else {
		py::print(msg);
	}

	collect.clear();
}

void Shell::process_syntax_error(const std::string& msg)
{
	static const std::regex r(R"(SyntaxError: \((["'])((?:\\\1|(?:(?!\1)).)*)(\1), \((["'])((?:\\\4|(?:(?!\4)).)*)(\4), (\d+), (\d+), (["'])((?:\\\9|(?:(?!\9)).)*)(\9)\)\))");
	
	std::smatch sm;
	if (!std::regex_match(msg, sm, r)) {
		py::print(msg);
		return;
	}

	std::string error = sm[2];
	std::string file = sm[5];
	//int row = std::stoi(sm[7]);
	int col = std::stoi(sm[8]);
	std::string line = sm[10];

	py::print("SyntaxError:", error);
	py::print(line);
	py::print(std::string(col > 0 ? col - 1 : 0, '-') + "^");
}

void Shell::process_system_exit(const std::string& msg)
{
	static const size_t startpos = std::string("SystemExit: ").size();
	is_running = false;
	std::string res = msg.substr(startpos, msg.find_first_of("\r\n") - startpos);

	try {
		return_code = std::stoi(res);
	}
	catch (std::invalid_argument&) {
		if (res == "None") {
			return_code = 0;
		}
		else {
			py::print(res);
			return_code = 1;
		}
	}
}

void usage()
{

}

int main(int argc, char* argv[])
{
	// Collect arguments
	std::vector<std::string> opts, scripts;
	bool accept_opts = true;
	for (int i = 1; i < argc; ++i) {
		char* arg = argv[i];
		int len = strlen(arg);
		if (len == 0)
			continue;
		if (arg[0] == '-') {
			if (len == 1) {
				accept_opts = false;
			}
			else if (arg[1] == '-') {
				opts.emplace_back(arg + 2);
			}
			else {
				for (int j = 1; j < len; ++j)
					opts.emplace_back(1, arg[j]);
			}
		}
		else {
			scripts.emplace_back(arg);
		}
	}

	unsigned int flags = 0;
	bool force_interactive = false;
	bool chain_scripts = false;

	for (const auto& opt : opts) {
		if (opt == "i" || opt == "interactive") {
			force_interactive = true;
		}
		else if (opt == "q" || opt == "quiet") {
			flags |= NO_BANNER;
		}
		else if (opt == "n" || opt == "no-display") {
			flags |= IGNORE_SEMICOLON;
		}
		else if (opt == "h" || opt == "help") {
			usage();
			return 0;
		}
		else if (opt == "c" || opt == "chain") {
			chain_scripts = true;
		}
	}

	Shell shell(flags);

	for (const auto& script : scripts) {
		if (!chain_scripts)	
			shell.restart();
		int res = shell.run_script(script);
		if (res != 0)
			return res;
	}
	if (scripts.empty()|| force_interactive)
		return shell.interact();
}