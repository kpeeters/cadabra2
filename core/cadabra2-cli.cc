#include <iostream>
#include <fstream>
#include <sstream>
#include <linenoise.hpp>
#ifndef _WIN32
#include <sys/types.h>
#include <pwd.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#include <glibmm/miscutils.h>
#endif
#include <regex>
#include <internal/string_tools.h>
#include "cadabra2-cli.hh"
#include "CdbPython.hh"
#include "Config.hh"
#include "InstallPrefix.hh"
#include "PreClean.hh"
#include "Config.hh"
#include "pythoncdb/py_helpers.hh"

#include <boost/dll.hpp>
#include <locale>
#include <codecvt>

using namespace linenoise;
namespace py = pybind11;

std::string replace_all(std::string const& original, std::string const& from, std::string const& to )
	{
	std::string results;
	std::string::const_iterator end = original.end();
	std::string::const_iterator current = original.begin();
	std::string::const_iterator next = std::search( current, end, from.begin(), from.end() );
	while ( next != end ) {
		results.append( current, next );
		results.append( to );
		current = next + from.size();
		next = std::search( current, end, from.begin(), from.end() );
		}
	results.append( current, next );
	return results;
	}

Shell::Shell(Flags flags)
	: globals(py::reinterpret_steal<py::dict>(nullptr))
	, flags(flags)
	{
	bool no_colour = flags & Flags::NoColour;
	colour_error   = no_colour ? "" : "\033[31m";
	colour_warning = no_colour ? "" : "\033[33m";
	colour_info    = no_colour ? "" : "\033[36m";
	colour_success = no_colour ? "" : "\033[32m";
	colour_reset   = no_colour ? "" : "\033[0m";

	if (!(flags & Flags::NoReadline)) {
		set_histfile();
		LoadHistory(histfile.c_str());
		SetMultiLine(true);
		}

	site_path = cadabra::install_prefix_of_module();
	globals = py::globals();
	if (!globals) {
		throw std::runtime_error("Could not fetch globals");
		}
	sys = py::module::import("sys");
	main_module = pybind11::module::import("__main__");
	main_namespace = main_module.attr("__dict__");

	// // Get sys module and append site path
	// py::list py_path = sys.attr("path");
	// py_path.append(site_path);
	py_stdout = sys.attr("stdout");
	py_stderr = sys.attr("stderr");
	}

Shell::~Shell()
	{
	if (!(flags & Flags::NoReadline))
		SaveHistory(histfile.c_str());
	}

void Shell::interact()
	{
	// Run cadabra2_defaults.py
	try {
		execute_file(site_path + "/cadabra2_defaults.py", false);
		}
	catch (const ExitRequest& err) {
		throw ExitRequest("Error encountered while initializing the interpreter");
		}

	// Print startup info banner
	if (!(flags & Flags::NoBanner)) {
		write_stdout("Cadabra " CADABRA_VERSION_FULL " (build " CADABRA_VERSION_BUILD
						 " dated " CADABRA_VERSION_DATE ")");
		write_stdout("Copyright (C) " COPYRIGHT_YEARS "  Kasper Peeters <info@cadabra.science>");
		write_stdout("Using SymPy version " + str(evaluate("sympy.__version__")));
		}

	// Input-output loop
	bool(*get_input)(const std::string&, std::string&);
	if (flags & Flags::NoReadline)
		get_input = [](const std::string& prompt, std::string& line) {
			std::cout << prompt;
			std::getline(std::cin, line);
			if (std::cin.eof()) {
				std::cin.clear();
				return true;
				}
			else {
				return false;
				}
			};
	else
		get_input = [](const std::string& prompt, std::string& line) {
			return Readline(prompt.c_str(), line);
			};

	std::string curline;
	while (true) {
		using namespace std::placeholders;
		if (!(flags & Flags::NoReadline))
			SetCompletionCallback(std::bind(&Shell::set_completion_callback, this, _1, _2));
		if (collect.empty()) {
			if (get_input(get_ps1(), curline)) {
				PyErr_SetNone(PyExc_KeyboardInterrupt);
				handle_error();
				}
			try {
				process_ps1(curline);
				}
			catch (py::error_already_set& err) {
				handle_error(err);
				}
			}
		else {
			if (get_input(get_ps2(), curline)) {
				PyErr_SetNone(PyExc_KeyboardInterrupt);
				handle_error();
				}
			try {
				process_ps2(curline);
				}
			catch (py::error_already_set& err) {
				handle_error();
				}
			}

		if (!(flags & Flags::NoReadline))
			AddHistory(curline.c_str());
		}
	}

pybind11::object Shell::evaluate(const std::string& code, const std::string& filename)
	{
	auto co = py::reinterpret_steal<py::object>(
		Py_CompileString(code.c_str(), filename.c_str(), Py_eval_input));
	if (!co)
		throw py::error_already_set();

	auto res = py::reinterpret_steal<py::object>(PyEval_EvalCode(co.ptr(), globals.ptr(), globals.ptr()));
	if (!res)
		throw py::error_already_set();
	return res;
	}

void Shell::execute(const std::string& code, const std::string& filename)
	{
	auto co = py::reinterpret_steal<py::object>(
		Py_CompileString(code.c_str(), filename.c_str(), Py_file_input));
	if (!co)
		throw py::error_already_set();

	auto res = py::reinterpret_steal<py::object>(PyEval_EvalCode(co.ptr(), globals.ptr(), globals.ptr()));
	if (!res)
		throw py::error_already_set();
	}

void Shell::execute_file(const std::string& filename, bool preprocess)
	{
	boost::filesystem::path abs_path = boost::filesystem::absolute(filename);
	std::string abs_path_str = abs_path.parent_path().string();
	py::list py_path = sys.attr("path");
	py_path.append(abs_path_str);
	
	bool display = !(flags & Flags::IgnoreSemicolons);
	std::string code;
	std::ifstream ifs(filename);
	if (!ifs.is_open()) {
		std::string message = "Could not open file " + filename;
		PyErr_SetString(PyExc_FileNotFoundError, message.c_str());
		handle_error();
		throw ExitRequest("Script ended execution due to an uncaught exception");
		}
	else {
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		code = buffer.str();
		}
	if (preprocess) {
		// FIXME: this duplicates code in Server.cc; separate out so we always stay consistent.
		auto python_path = cadabra::install_prefix_of_module();
		std::string startup =
			"f=open(r'" + python_path + "/cadabra2_defaults.py'); "
			"code=compile(f.read(), 'cadabra2_defaults.py', 'exec'); "
			"exec(code); f.close() ";
		py::exec(startup, main_namespace);

		std::string error;
//		code = "import cadabra2\nfrom cadabra2 import *\nfrom cadabra2_defaults import *\n\n" + cadabra::cdb2python_string(code, display, error);
		code = cadabra::cdb2python_string(code, display, error);
		}

	try {
		py::exec(code, main_namespace); // globals, globals);
		}
	catch (py::error_already_set& err) {
		handle_error(err);
		throw ExitRequest("Script ended execution due to an uncaught exception: "+code);
		}
	}

void Shell::interact_file(const std::string& filename, bool preprocess)
	{
	std::ifstream ifs(filename);
	if (!ifs.is_open())
		throw ExitRequest("Could not open file " + filename);

	try {
		execute(
			"import cadabra2\n"
			"from cadabra2 import *\n"
			"from cadabra2_defaults import *\n"
			"__cdbkernel__ = cadabra2.__cdbkernel__");
		}
	catch (py::error_already_set& err) {
		handle_error(err);
		throw ExitRequest("Error occurred during script initialization");
		}

	std::string curline;
	while (std::getline(ifs, curline)) {
		if (!collect.empty() && curline.find_first_not_of(" \t") == 0) {
			try {
				process_ps2("");
				}
			catch (py::error_already_set& err) {
				handle_error();
				throw ExitRequest("Script ended execution due to an uncaught exception");
				}
			}
		if (collect.empty()) {
			write_stdout(get_ps1() + curline);
			try {
				process_ps1(curline);
				}
			catch (py::error_already_set& err) {
				handle_error();
				throw ExitRequest("Script ended execution due to an uncaught exception");
				}
			}
		else {
			write_stdout(get_ps2() + curline);
			try {
				process_ps2(curline);
				}
			catch (py::error_already_set& err) {
				handle_error();
				throw ExitRequest("Script ended execution due to an uncaught exception");
				}
			}
		}

	if (!collect.empty()) {
		try {
			process_ps2("");
			}
		catch (py::error_already_set& err) {
			handle_error();
			throw ExitRequest("Script ended execution due to an uncaught exception");
			}
		}
	}

void Shell::write_stdout(const std::string& text, const std::string& end, bool flush)
	{
	try {
		py_stdout.attr("write")(text + end);
		if (flush)
			py_stdout.attr("flush")();
		}
	catch (py::error_already_set& err) {
		std::cerr << err.what() << '\n';
		}
	}

void Shell::write_stderr(const std::string& text, const std::string& end, bool flush)
	{
	try {
		py_stderr.attr("write")(text + end);
		if (flush)
			py_stderr.attr("flush")();
		}
	catch (py::error_already_set& err) {
		std::cerr << err.what() << '\n';
		}
	}

void Shell::set_histfile()
	{
	std::string homedir = "~";
#ifdef _WIN32
	const char* userprofile = getenv("USERPROFILE");
	if (userprofile) {
		homedir = userprofile;
		}
	else {
		const char* homedrive = getenv("HOMEDRIVE");
		const char* homepath = getenv("HOMEPATH");
		if (homedrive && homepath) {
			homedir = homedrive;
			homedir += homepath;
			}
		}
#else
	const char* home = getenv("HOME");
	if (home) {
		homedir = home;
		}
	else {
		auto pw = getpwuid(getuid());
		if (pw) {
			homedir = pw->pw_dir;
			}
		}
#endif

	if (homedir.back() == '/' || homedir.back() == '\\')
		histfile = homedir + ".cadabra2_history";
	else
		histfile = homedir + "/.cadabra2_history";
	}

std::string Shell::str(const py::handle& obj)
	{
	return py::str(obj); //.str().cast<std::string>();
	}

std::string Shell::repr(const py::handle& obj)
	{
	auto uni = py::reinterpret_steal<py::object>(PyObject_Repr(obj.ptr()));
	if (!uni) {
		handle_error();
		return "";
		}
	return str(uni);
	}

std::string Shell::sanitize(std::string s)
	{
   replace_all(s, "\\", "\\\\");
	return s;
	}

void Shell::process_ps1(const std::string& line)
	{
	// Convert cadabra to python
	bool display = !(flags & Flags::IgnoreSemicolons);
	cadabra::ConvertData cv;
	std::pair<std::string, std::string> res = cadabra::convert_line(line, cv, display);
	// FIXME: we need to process res.first
	const std::string& output = res.second;
	if (output == "::empty") {
		// Cadabra continuation line, add to collect
		collect += line + "\n";
		return;
		}

	try {
		py::object res = evaluate(output);
		if (!res.is_none()) {
			write_stdout(str(res));
			globals["_"] = res;
			}
		}
	catch (py::error_already_set& eval_err) {
		if (eval_err.matches(PyExc_SyntaxError)) {
			// Might have valid Python, but needs to be executed not evaluated.
			try {
				execute(output);
				}
			catch (py::error_already_set& exec_err) {
				bool need_more = false;
				// Check if we need more input. The approach taken is from the Python codeop library:
				// https://github.com/python/cpython/blob/8c93a63c03ddc789040a6ad50a18af1df7764884/Lib/codeop.py#L19
				// First compile with a newline appended. If it compiles, then we need more input.
				std::string output_with_nl = output + "\n";
				auto co_with_nl = py::reinterpret_steal<py::object>(
					Py_CompileString(output_with_nl.c_str(), "<internal>", Py_file_input));
				if (co_with_nl) {
					need_more = true;
					}
				else {
					// Recompile with two newlines appended. Compare the error generated by the two compilations:
					// if they are different then we need more, otherwise it is a genuine error
					py::error_already_set err1; // Save&clear error from co_with_nl
					std::string output_with_nlnl = output + "\n\n";
					auto co_with_nlnl = py::reinterpret_steal<py::object>(
						Py_CompileString(output_with_nlnl.c_str(), "<internal>", Py_file_input));
					py::error_already_set err2; // Save&clear error from co_with_nlnl
					need_more = (repr(err1.type()) != repr(err2.type()) || repr(err1.value()) != repr(err2.value()));
					}

				if (need_more)
					collect += "\n";
				else
					handle_error(exec_err);
				}
			}
		else {
			handle_error(eval_err);
			}
		}
	}

void Shell::process_ps2(const std::string& line)
	{
	if (!line.empty()) {
		collect += line + "\n";
		return;
		}

	bool display = !(flags & Flags::IgnoreSemicolons);
	std::string error;
	std::string code = cadabra::cdb2python_string(collect, display, error);
	collect.clear();
	execute(code);
	}

void Shell::set_completion_callback(const char* buffer, std::vector<std::string>& completions)
	{
	int len = strlen(buffer);
	if (len == 0 || !isalnum(buffer[len - 1]))
		return;
	int pos = len - 1;
	while (pos > 0) {
		if (!isalnum(buffer[pos - 1]))
			break;
		--pos;
		}

	std::string partial(buffer + pos, len - pos);
	for (const auto& item : globals) {
		auto key = str(item.first);
		if (key.rfind(partial, 0) == 0)
			completions.push_back(buffer + key.substr(partial.size()));
		}
	}

std::string Shell::get_ps1()
	{
	if(py::hasattr(sys, "ps1")==false)
		sys.attr("ps1") = "> ";
	auto ps1 = sys.attr("ps1");
	return str(ps1);
	}

std::string Shell::get_ps2()
	{
	if(pybind11::hasattr(sys, "ps2")==false)
		sys.attr("ps2") = ". ";
	auto ps2 = sys.attr("ps2");
	return str(ps2);
	}

void Shell::handle_error()
	{
	if (PyErr_Occurred()) {
		py::error_already_set err;
		handle_error(err);
		}
	}

void Shell::handle_error(py::error_already_set& err)
	{
	if(err.matches(PyExc_SystemExit)) {
		auto value = err.value();
		if(PyExceptionInstance_Check(value.ptr())) {
			py::object code = py::reinterpret_borrow<py::object>(value.ptr()).attr("code");
			if (code) {
				value = py::reinterpret_borrow<py::object>(code);
				}
			}
		if (!value || value.is_none()) {
			throw ExitRequest{};
			}
		else if(PyLong_Check(value.ptr())) {
			throw ExitRequest{ static_cast<int>(PyLong_AsLong(value.ptr())) };
			}
		else {
			throw ExitRequest{ str(value) };
			}
		}
	else {
		err.restore();
		PySys_WriteStderr("%.1000s", colour_error);
		PyErr_Print();
		write_stderr(colour_reset, "", true);
		}

	collect.clear();
	}

ExitRequest::ExitRequest()
	: code(0) {}

ExitRequest::ExitRequest(int code)
	: code(code) {}

ExitRequest::ExitRequest(const std::string& message)
	: code(1), message(message) {}

const char* ExitRequest::what() const noexcept
	{
	return message.c_str();
	}

Shell::Flags& operator |= (Shell::Flags& lhs, Shell::Flags rhs)
	{
	lhs = static_cast<Shell::Flags>(
		static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs)
											  );
	return lhs;
	}

Shell::Flags operator | (Shell::Flags lhs, Shell::Flags rhs)
	{
	return lhs |= rhs;
	}

bool operator & (Shell::Flags lhs, Shell::Flags rhs)
	{
	return static_cast<bool>(
		static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs)
									 );
	}

void help()
	{
	std::cout <<
		"cadabra2-cli: command line interface to cadabra2\n"
		"Usage: cadabra2-cli [OPTIONS] [script1 [script2 [...]]\n"
		"  -h      --help                Display this help message and exit\n"
		"  -v      --version             Display the version number and exit\n"
		"  -i      --interactive         Force interactive mode even if scripts are\n"
		"                                provided (will be started in the same Python\n"
		"                                context as the last script)\n"
		"  -l      --linebyline          Run scripts as though the input to an\n"
		"                                interactive session\n"
		"  -f      --fatal               When a SystemExit exception is raised, do not\n"
		"                                continue to execute any more scripts\n"
		"  -q      --quiet               Do not display startup banner\n"
		"  -n      --ignore-semicolons   Do not replace semicolons with calls to\n"
		"                                display(...)\n"
		"  -r      --noreadline          Do not use readline libraries for input\n"
		"  -w      --nocolor             Do not colourize output\n"
		"          --nocolour\n"
		"  -V      --verbose             Print extra debugging information";
	}

void version()
	{
	std::cout << CADABRA_VERSION_FULL << ' ' << CADABRA_VERSION_BUILD << '\n';
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

	Shell::Flags flags = Shell::Flags::None;
	bool force_interactive = false;
	bool line_by_line = false;
	bool fatal = false;
	bool verbose = false;

	for (const auto& opt : opts) {
		if (opt == "i" || opt == "interactive") {
			force_interactive = true;
			}
		else if (opt == "l" || opt == "linebyline") {
			line_by_line = true;
			}
		else if (opt == "f" || opt == "fatal") {
			fatal = true;
			}
		else if (opt == "q" || opt == "quiet") {
			flags |= Shell::Flags::NoBanner;
			}
		else if (opt == "n" || opt == "ignore-semicolons") {
			flags |= Shell::Flags::IgnoreSemicolons;
			}
		else if (opt == "h" || opt == "help") {
			help();
			return 0;
			}
		else if (opt == "v" || opt == "version") {
			version();
			return 0;
			}
		else if (opt == "V" || opt == "verbose") {
			verbose = true;
			}
		else if (opt == "w" || opt == "nocolour" || opt == "nocolor") {
			flags |= Shell::Flags::NoColour;
			}
		else if (opt == "r" || opt == "noreadline") {
			flags |= Shell::Flags::NoReadline;
			}
		else {
			std::cerr << "Unrecognised option \"" << opt << "\"\n";
			help();
			return 1;
			}
		}

	if (!scripts.empty())
		flags |= Shell::Flags::NoBanner;

	Shell shell(flags);
	int last_exit_code = 0;

	for (const auto& script : scripts) {
		try {
			if (line_by_line)
				shell.interact_file(script, true);
			else
				shell.execute_file(script, true);
			last_exit_code = 0;
			}
		catch (const ExitRequest& err) {
			if (err.code != 0) {
				if (!err.message.empty())
					shell.write_stderr(err.message, "\n", true);
				last_exit_code = err.code;
				}
			if (verbose)
				shell.write_stderr("Script exited with code " + std::to_string(err.code), "\n", true);
			if (fatal)
				return err.code;
			}
		}
	if (scripts.empty() || force_interactive) {
		try {
			shell.interact();
			}
		catch (const std::logic_error& err) {
			shell.write_stderr("\nTerminated by user interrupt.\n");
			return 0;
			}
		catch (const ExitRequest& err) {
			if (err.code != 0) {
				if (!err.message.empty())
					shell.write_stderr(err.message, "\n", true);
				last_exit_code = err.code;
				}
			if (verbose)
				shell.write_stderr("Exited with code " + std::to_string(err.code), "\n", true);
			return err.code;
			}
		}

	return last_exit_code;
	}
