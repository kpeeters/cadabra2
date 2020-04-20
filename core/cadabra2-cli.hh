#include <string>
#include <memory>

// Work around MSVC linking problem
#ifdef _DEBUG
#define CADABRA_CLI_DEBUG_MARKER
#undef _DEBUG
#endif
#include <Python.h>
#ifdef CADABRA_CLI_DEBUG_MARKER
#define _DEBUG
#undef CADABRA_CLI_DEBUG_MARKER
#endif


class Shell
{
public:
	enum class Flags : unsigned int
	{
		None             = 0x00,
		NoBanner         = 0x01,
		IgnoreSemicolons = 0x02,
		NoColour         = 0x04,
		NoReadline       = 0x08
	};

	Shell(Flags flags);
	~Shell();

	void restart();
	void interact();
	void execute_file(const std::string& filename, bool preprocess = true);
	void execute(const std::string& code);
	PyObject* evaluate(const std::string& code);

	void write_stdout(const std::string& str, const char* end = "\n");
	void write_stdout(PyObject* obj, const char* end = "\n");
	void write_stderr(const std::string& str, const char* end = "\n");
	void write_stderr(PyObject* obj, const char* end = "\n");

private:
	void set_histfile();
	std::string histfile;

	std::string to_string(PyObject* obj);
	std::string sanitize(std::string s);

	bool get_input(const std::string& prompt, std::string& line);
	void process_ps1(const std::string& line);
	void process_ps2(const std::string& line);
	void set_completion_callback(const char* buffer, std::vector<std::string>& completions);

	bool is_syntax_error();
	bool is_eof_error();
	void handle_error();
	void clear_error();

	PyObject* globals;
	std::string collect;

	const char* colour_error;
	const char* colour_warning;
	const char* colour_info;
	const char* colour_success;
	const char* colour_reset;
	Flags flags;
	};

class PyExceptionBase : public std::exception {};
class PyException : public PyExceptionBase {};
class PySyntaxError : public PyExceptionBase {};

class ExitRequest : public std::exception
{
public:
	ExitRequest();
	ExitRequest(int code);
	ExitRequest(const std::string& message);

	virtual const char* what() const noexcept override;

	int code;
	std::string message;
};


Shell::Flags& operator |= (Shell::Flags& lhs, Shell::Flags rhs);
Shell::Flags operator | (Shell::Flags lhs, Shell::Flags rhs);
bool operator & (Shell::Flags lhs, Shell::Flags rhs);
