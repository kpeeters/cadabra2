#include <iostream>
#include <regex>
#include "CdbPython.hh"
#include "cadabra2-cli.hh"
#include "Config.hh"
#include "InstallPrefix.hh"

namespace py = pybind11;
using namespace py::literals;


Shell::Shell(int argc, char* argv[])
	: ps1("> ")
	, ps2(". ")
{

}

int Shell::interact()
{
	pybind11::scoped_interpreter guard;
	locals.clear();

	std::string module_path = cadabra::install_prefix() + "/share/cadabra2/python";

	py::exec("import sys; sys.path.append('" + module_path + "')");
	py::print("Cadabra " CADABRA_VERSION_FULL " (build " CADABRA_VERSION_BUILD " dated " CADABRA_VERSION_DATE ")"); 
	py::print("Copyright (C) " COPYRIGHT_YEARS "  Kasper Peeters <kasper.peeters@phi-sci.com>"); 
	py::eval_file(module_path + "/cadabra2_defaults.py");
	py::print("Using SymPy version", py::eval("sympy.__version__"));

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
	return 1;
}

void Shell::execute(const std::string& line)
{
	py::exec(line, py::globals(), locals);
}

bool Shell::try_evaluate(const std::string& s)
{
	try {
		auto res = py::eval(s, py::globals(), locals);
		if (!res.is_none()) {
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
		execute(cadabra::cdb2python_string(collect, true));
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


int main(int argc, char* argv[])
{
	Shell shell(argc, argv);
	return shell.interact();
}