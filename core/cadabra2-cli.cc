#include <iostream>
#include <fstream>
#include <sstream>
#include <linenoise.hpp>
#ifndef _WIN32
#include <sys/types.h>
#include <pwd.h>
#endif
#include <regex>
#include <internal/string_tools.h>
#include "cadabra2-cli.hh"
#include "CdbPython.hh"
#include "Config.hh"
#include "InstallPrefix.hh"
#include "PreClean.hh"

using namespace linenoise;

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
	: globals(NULL)
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

	restart();
}

Shell::~Shell()
{
	if (!(flags & Flags::NoReadline))
		SaveHistory(histfile.c_str());
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

void Shell::restart()
{
	// Cleanup previous session
	Py_Finalize();

	// Start new session
	Py_Initialize();
	globals = PyEval_GetGlobals();
	if (!globals) {
		PyObject* main = PyImport_ImportModule("__main__");
		if (!main)
			throw std::runtime_error("Could not access __main__ frame");
		globals = PyModule_GetDict(main);
		if (!globals)
			throw std::runtime_error("Could not fetch globals");
		Py_DECREF(main);
	}

	sys = PyImport_ImportModule("sys");
	std::string module_path = cadabra::install_prefix() + "/share/cadabra2/python";
	PyObject* module_path_str = PyUnicode_FromString(module_path.c_str());
	PyList_Append(PyObject_GetAttrString(sys, "path"), module_path_str);
	Py_XDECREF(module_path_str);
}

void Shell::interact()
{
	// Run cadabra2_defaults.py
	if (!execute_file(cadabra::install_prefix() + "/share/cadabra2/python/cadabra2_defaults.py", false)) {
		handle_error();
		throw ExitRequest("Error encountered while initializing the interpreter");
	}

	// Print startup info banner
	if (!(flags & Flags::NoBanner)) {
		write("Cadabra " CADABRA_VERSION_FULL " (build " CADABRA_VERSION_BUILD
		          " dated " CADABRA_VERSION_DATE ")");
		write("Copyright (C) " COPYRIGHT_YEARS "  Kasper Peeters <kasper.peeters@phi-sci.com>");
		write("Using SymPy version " + evaluate_to_string("sympy.__version__"));
	}

	// Input-output loop
	bool(*get_input)(const std::string&, std::string&);
	if (flags & Flags::NoReadline)
		get_input = [] (const std::string& prompt, std::string& line) {
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
		get_input = [] (const std::string& prompt, std::string& line) {
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
			if (!process_ps1(curline))
				handle_error();
		}
		else {
			if (get_input(get_ps2(), curline)) {
				PyErr_SetNone(PyExc_KeyboardInterrupt);
				handle_error();
			}
			if (!process_ps2(curline))
				handle_error();
		}

		if (!(flags & Flags::NoReadline))
			AddHistory(curline.c_str());
	}
}

void Shell::interact_file(const std::string& filename, bool preprocess)
{
	std::ifstream ifs(filename);
	if (!ifs.is_open())
		throw ExitRequest("Could not open file " + filename);

	if (!execute("import cadabra2; from cadabra2 import *; from cadabra2_defaults import *; __cdbkernel__ = cadabra2.__cdbkernel__")) {
		handle_error();
		throw ExitRequest("Error occurred during script initialization");
	}

	std::string curline;
	while (std::getline(ifs, curline)) {
		if (!collect.empty() && curline.find_first_not_of(" \t") == 0) {
			if (!process_ps2("")) {
				handle_error();
				throw ExitRequest("Script ended execution due to an uncaught exception");
			}
		}
		if (collect.empty()) {
			write(get_ps1() + curline);
			if (!process_ps1(curline)) {
				handle_error();
				throw ExitRequest("Script ended execution due to an uncaught exception");
			}
		}
		else {
			write(get_ps2() + curline);
			if (!process_ps2(curline)) {
				handle_error();
				throw ExitRequest("Script ended execution due to an uncaught exception");
			}
		}
	}

	if (!collect.empty()) {
		if (!process_ps2("")) {
			handle_error();
			throw ExitRequest("Script ended execution due to an uncaught exception");
		}
	}
}

void Shell::write(const std::string& text, const char* end, const char* stream, bool flush)
{
	PyObject* pystream = PyObject_GetAttrString(sys, stream);
	PyObject_CallMethod(pystream, "write", "s", text.c_str());
	PyObject_CallMethod(pystream, "write", "s", end);
	if (flush)
		PyObject_CallMethod(pystream, "flush", NULL);
	Py_XDECREF(pystream);
}

void Shell::write(PyObject* obj, const char* end, const char* stream, bool flush)
{
	PyObject* pystream = PyObject_GetAttrString(sys, stream);
	PyObject_CallMethod(pystream, "write", "O", obj);
	PyObject_CallMethod(pystream, "write", "s", end);
	if (flush)
		PyObject_CallMethod(pystream, "flush", NULL);
	Py_XDECREF(pystream);
}

std::string Shell::to_string(PyObject* obj)
{
	PyObject* obj_str = PyObject_Str(obj);
	PyObject* obj_utf = PyUnicode_AsEncodedString(obj_str, "utf-8", "~E~");
	std::string res(PyBytes_AS_STRING(obj_utf));
	Py_XDECREF(obj_str);
	Py_XDECREF(obj_utf);
	return res;
}

std::string Shell::sanitize(std::string s)
{
   replace_all(s, "\\", "\\\\");
	return s;
}


PyObject* Shell::evaluate(const std::string& code, const std::string& filename)
{
	PyObject* co = Py_CompileString(code.c_str(), filename.c_str(), Py_eval_input);
	if (!co)
		return NULL;

	PyObject* res = PyEval_EvalCode(co, globals, globals);
	Py_DECREF(co);

	return res;
}

std::string Shell::evaluate_to_string(const std::string& code, const std::string& error_val)
{
	PyObject* res = evaluate(code);
	if (res) {
		std::string s = to_string(res);
		Py_DECREF(res);
		return s;
	}
	else {
		clear_error();
		return error_val;
	}
}

bool Shell::execute(const std::string& code, const std::string& filename)
{
	PyObject* co = Py_CompileString(code.c_str(), filename.c_str(), Py_file_input);
	if (!co)
		return false;

	PyObject* res = PyEval_EvalCode(co, globals, globals);
	Py_DECREF(co);

	return res;
}

bool Shell::execute_file(const std::string& filename, bool preprocess)
{
	bool display = !(flags & Flags::IgnoreSemicolons);
	std::string code;
	if (preprocess)
		code = cadabra::cdb2python(filename, display);
	else {
		std::ifstream ifs(filename);
		if (!ifs.is_open()) {
			std::string message = "Could not open file " + filename;
			PyErr_SetString(PyExc_FileNotFoundError, message.c_str());
			return false;
		}
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		code = buffer.str();
	}

	PyObject* co = Py_CompileString(code.c_str(), filename.c_str(), Py_file_input);
	if (!co)
		return false;

	PyObject* res = PyEval_EvalCode(co, globals, globals);
	Py_DECREF(co);

	return res;
}

bool Shell::process_ps1(const std::string& line)
{
	// Convert cadabra to python
	bool display = !(flags & Flags::IgnoreSemicolons);
	std::string lhs, rhs, op, indent;
	std::string output = cadabra::convert_line(line, lhs, rhs, op, indent, display);
	if (output == "::empty") {
		// Cadabra continuation line, add to collect
		collect += line + "\n";
		return true;
	}

	PyObject* res = evaluate(output);
	if (res) {
		if (res != Py_None) {
			write(res);
			PyDict_SetItemString(globals, "_", res);
			Py_DECREF(res);
		}
		return true;
	}
	else if (is_syntax_error()) {
		clear_error();
		if (execute(output)) {
			return true;
		}
		else if (is_eof_error()) {
			collect += line + "\n";
			clear_error();
			return true;
		}
	}
	return false;
}

bool Shell::process_ps2(const std::string& line)
{
	if (!line.empty()) {
		collect += line + "\n";
		return true;
	}

	bool display = !(flags & Flags::IgnoreSemicolons);
	std::string code = cadabra::cdb2python_string(collect, display);

	auto res = execute(code);
	collect.clear();
	return res;
}

std::string Shell::get_ps1()
{
	PyObject* ps1 = PyObject_GetAttrString(sys, "ps1");
	if (!ps1) {
		ps1 = PyUnicode_FromString("> ");
		PyObject_SetAttrString(sys, "ps1", ps1);
	}
	auto res = to_string(ps1);
	Py_XDECREF(ps1);
	return res;
}

std::string Shell::get_ps2()
{
	PyObject* ps2 = PyObject_GetAttrString(sys, "ps2");
	if (!ps2) {
		ps2 = PyUnicode_FromString(". ");
		PyObject_SetAttrString(sys, "ps2", ps2);
	}
	auto res = to_string(ps2);
	Py_XDECREF(ps2);
	return res;
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
	PyObject* keys = PyDict_Keys(globals);
	for (int i = 0; i < PyList_Size(keys); ++i) {
		std::string key = to_string(PyList_GetItem(keys, i));
		if (key.size() > partial.size()) {
			if (key.substr(0, partial.size()) == partial) {
				completions.push_back(buffer + key.substr(partial.size()));
			}
		}
	}
	Py_XDECREF(keys);
}

bool Shell::is_syntax_error()
{
	return
		PyErr_Occurred() &&
		PyErr_ExceptionMatches(PyExc_SyntaxError);
}

bool Shell::is_eof_error()
{
	bool res = false;

	PyObject *type, *value, *traceback;
	PyErr_Fetch(&type, &value, &traceback);
	if (to_string(PyTuple_GetItem(value, 0)) == "unexpected EOF while parsing")
		res = true;
	PyErr_Restore(type, value, traceback);

	return res;
}

void Shell::handle_error()
{
	if (PyErr_Occurred()) {
		if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
			PyObject *type, *value, *traceback;
			PyErr_Fetch(&type, &value, &traceback);
			if (PyExceptionInstance_Check(value)) {
				_Py_Identifier PyId_code;
				PyId_code.next = 0;
				PyId_code.string = "code";
				PyId_code.object = 0;
				PyObject* code = _PyObject_GetAttrId(value, &PyId_code);
				if (code) {
					Py_DECREF(value);
					value = code;
				}
			}

			if (value == NULL || value == Py_None)
				throw ExitRequest{};
			else if (PyLong_Check(value))
				throw ExitRequest{ static_cast<int>(PyLong_AsLong(value)) };
			else
				throw ExitRequest{ to_string(value) };
		}
		else {
			PySys_WriteStderr(colour_error);
			PyErr_Print();
			write(colour_reset, "", "stderr", true);
		}
	}
	PyErr_Clear();
	collect.clear();
}

void Shell::clear_error()
{
	PyErr_Clear();
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
		"  -h      --help                Display this help message and exit\n"
		"  -c      --chain               Share the same Python context between\n"
		"                                scripts and interactive mode\n"
		"  -r      --noreadline          Do not use readline libraries for input\n"
		"  -w      --nocolor             Do not colourize output\n"
		"          --nocolour\n"
		"  -v      --verbose             Print extra debugging information";
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
	bool chain_scripts = false;
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
		else if (opt == "c" || opt == "chain") {
			chain_scripts = true;
		}
		else if (opt == "v" || opt == "verbose") {
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

	for (const auto& script : scripts) {
		if (!chain_scripts)
			shell.restart();
		try {
			if (line_by_line)
				shell.interact_file(script, true);
			else
				shell.execute_file(script, true);
		}
		catch (const ExitRequest& err) {
			if (err.code != 0) {
				if (!err.message.empty())
					shell.write(err.message, "\n", "stderr", true);
			}
			if (verbose)
				shell.write("Script exited with code " + std::to_string(err.code), "\n", "stderr", true);
			if (fatal)
				return err.code;
		}
	}
	if (scripts.empty() || force_interactive) {
		try {
			shell.interact();
		}
		catch (const ExitRequest& err) {
			if (err.code != 0) {
				if (!err.message.empty())
					shell.write(err.message, "\n", "stderr", true);
			}
			if (verbose)
				shell.write("Exited with code " + std::to_string(err.code), "\n", "stderr", true);
			return err.code;
		}
	}

	return 0;
}
