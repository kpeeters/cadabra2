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

using namespace linenoise;

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
	}
}

void Shell::interact()
{
	// Run cadabra2_defaults.py
	std::string module_path = cadabra::install_prefix() + "/share/cadabra2/python";
	try {
		execute("import sys; sys.path.append('" + sanitize(module_path) + "')");
		execute("sys.ps1 = '> '; sys.ps2 = '. '");
		execute_file(module_path + "/cadabra2_defaults.py", false);
	}
	catch (const PyExceptionBase& err) {
		handle_error();
		throw ExitRequest("Error encountered whilst initializing the interpreter");
	}
	catch (const std::runtime_error& err) {
		write_stdout(err.what());
		throw ExitRequest("Error encountered whilst initializing the interpreter");
	}

	// Print startup info banner
	if (!(flags & Flags::NoBanner)) {
		write_stdout("Cadabra " CADABRA_VERSION_FULL " (build " CADABRA_VERSION_BUILD 
		          " dated " CADABRA_VERSION_DATE ")"); 
		write_stdout("Copyright (C) " COPYRIGHT_YEARS "  Kasper Peeters <kasper.peeters@phi-sci.com>"); 
		write_stdout("Using SymPy version ", "");
		PyObject* sympy_version;
		try {
			sympy_version = evaluate("sympy.__version__");
		}
		catch (const PyExceptionBase& err) {
			sympy_version = Py_None;
		}
		write_stdout(to_string(sympy_version));
		if (sympy_version != Py_None)
			Py_XDECREF(sympy_version);
	}

	// Input-output loop
	std::string curline;
	while (true) {
		using namespace std::placeholders;
		if (!(flags & Flags::NoReadline)) 
			SetCompletionCallback(std::bind(&Shell::set_completion_callback, this, _1, _2));
		if (collect.empty()) {
			auto cursor = to_string(PySys_GetObject("ps1"));
			if (!get_input(cursor, curline))
				break;
			process_ps1(curline);
		}
		else {
			auto cursor = to_string(PySys_GetObject("ps2"));
			if (!get_input(cursor, curline))
				break;
			process_ps2(curline);
		}

		if (!(flags & Flags::NoReadline))
			AddHistory(curline.c_str());
	}
}

bool Shell::get_input(const std::string& prompt, std::string& line)
{
	if (!(flags & Flags::NoReadline)) {
		return !Readline(prompt.c_str(), line);
	}
	else {
		std::cout << prompt;
		std::getline(std::cin, line);
		return true;
	}
}

void Shell::write_stdout(const std::string& str, const char* end)
{
	PySys_FormatStdout("%s%s", str.c_str(), end);
}

void Shell::write_stdout(PyObject* obj, const char* end)
{
	write_stdout(to_string(obj), end);
}

void Shell::write_stderr(const std::string& str, const char* end)
{
	PySys_FormatStderr("%s%s", str.c_str(), end);
}

void Shell::write_stderr(PyObject* obj, const char* end)
{
	write_stderr(to_string(obj), end);
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


PyObject* Shell::evaluate(const std::string& code)
{
	PyObject* co = Py_CompileString(code.c_str(), "<stdin>", Py_eval_input);
	if (!co)
		throw PySyntaxError{};

	PyObject* res = PyEval_EvalCode(co, globals, globals);
	Py_DECREF(co);

	if (!res)
		throw PyException{};

	return res;
}

void Shell::execute(const std::string& code)
{
	PyObject* co = Py_CompileString(code.c_str(), "<stdin>", Py_file_input);
	if (!co)
		throw PySyntaxError{};

	PyObject* res = PyEval_EvalCode(co, globals, globals);
	Py_DECREF(co);

	if (!res)
		throw PyException{};
}

void Shell::execute_file(const std::string& filename, bool preprocess)
{
	bool display = !(flags & Flags::IgnoreSemicolons);
	std::string code;
	if (preprocess)
		code = cadabra::cdb2python(filename, display);
	else {
		std::ifstream ifs(filename);
		if (!ifs.is_open())
			throw std::runtime_error("Could not open file " + filename);
		std::stringstream buffer;
		buffer << ifs.rdbuf();
		code = buffer.str();
	}

	PyObject* co = Py_CompileString(code.c_str(), filename.c_str(), Py_file_input);
	if (!co)
		throw PySyntaxError{};

	PyObject* res = PyEval_EvalCode(co, globals, globals);
	Py_DECREF(co);

	if (!res)
		throw PyException{};
}

void Shell::process_ps1(const std::string& line)
{
	// Convert cadabra to python
	bool display = !(flags & Flags::IgnoreSemicolons);
	std::string lhs, rhs, op, indent;
	std::string output = cadabra::convert_line(line, lhs, rhs, op, indent, display);
	if (output == "::empty") {
		// Cadabra continuation line, add to collect
		collect += line + "\n";
		return;
	}

	try {
		PyObject* res = evaluate(output);
		if (res != Py_None) {
			write_stdout(to_string(res));
			PyDict_SetItemString(globals, "_", res);
			Py_DECREF(res);
		}
	}
	catch (const PySyntaxError& err) {
		clear_error();
		try {
			execute(output);
		}
		catch (const PySyntaxError& err) {
			if (is_eof_error()) {
				clear_error();
				collect += line + "\n";
			}
			else {
				handle_error();
			}
		}
		catch (const PyException& err) {
			handle_error();
		}
	}
	catch (const PyException& err) {
		handle_error();
	}
}

void Shell::process_ps2(const std::string& line)
{
	if (!line.empty()) {
		collect += line + "\n";
		return;
	}

	bool display = !(flags & Flags::IgnoreSemicolons);
	std::string code = cadabra::cdb2python_string(collect, display);

	try {
		execute(code);
	}
	catch (const PyExceptionBase& err) {
		handle_error();
	}

	collect.clear();
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
			write_stderr(colour_error, "");
			PyErr_Print();
			write_stderr(colour_reset, "");
			fprintf(stderr, colour_reset);
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
		"  -f      --fatal               When a SystemExit exception is raised, do not\n"
		"                                continue to execute any more scripts\n"
		"  -q      --quiet               Do not display startup banner\n"
		"  -n      --ignore-semicolons   Do not replace semicolons with calls to\n"
		"                                display(...)\n"
		"  -h      --help                Display this help message and exit\n"
		"  -c      --chain               Share the same Python context between\n"
		"                                scripts and interactive mode\n"
		"  -r      --noreadline          Do not use readline libraries for input\n"
		"          --noreadline\n"
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
	bool fatal = false;
	bool chain_scripts = false;
	bool verbose = false;

	for (const auto& opt : opts) {
		if (opt == "i" || opt == "interactive") {
			force_interactive = true;
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

	Shell shell(flags);

	for (const auto& script : scripts) {
		if (!chain_scripts)	
			shell.restart();
		try {
			shell.execute_file(script);
		}
		catch (const ExitRequest& err) {
			if (err.code != 0) {
				if (!err.message.empty())
					shell.write_stderr(err.message);
			}
			if (verbose)
				shell.write_stderr("Script exited with code " + std::to_string(err.code));
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
					shell.write_stderr(err.message);
			}
			if (verbose)
				shell.write_stderr("Exited with code " + std::to_string(err.code));
			return err.code;
		}
	}

	return 0;
}
