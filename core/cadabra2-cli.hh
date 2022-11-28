#include <string>
#include <memory>

// Work around MSVC linking problem
#ifdef _DEBUG
#define CADABRA_CLI_DEBUG_MARKER
#undef _DEBUG
#endif
//#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
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
		NoReadline       = 0x08,
	};

	Shell(Flags flags);
	~Shell();

	void restart();
	void interact();
	pybind11::object evaluate(const std::string& code, const std::string& filename = "<stdin>");
	void execute(const std::string& code, const std::string& filename = "<stdin>");
	void execute_file(const std::string& filename, bool preprocess = true);
	void interact_file(const std::string& filename, bool preprocess = true);

	void write_stdout(const std::string& text, const std::string& end = "\n", bool flush = false);
	void write_stderr(const std::string& text, const std::string& end = "\n", bool flush = false);

private:
	void set_histfile();
	std::string histfile;
	std::string site_path;

	std::string str(const pybind11::handle& obj);
	std::string repr(const pybind11::handle& obj);
	std::string sanitize(std::string s);

	void process_ps1(const std::string& line);
	void process_ps2(const std::string& line);
	void set_completion_callback(const char* buffer, std::vector<std::string>& completions);

	std::string get_ps1();
	std::string get_ps2();

	void handle_error();
	void handle_error(pybind11::error_already_set& err);

	pybind11::dict globals;
	pybind11::object sys;
	pybind11::object py_stdout, py_stderr;
	std::string collect;

	const char* colour_error;
	const char* colour_warning;
	const char* colour_info;
	const char* colour_success;
	const char* colour_reset;
	Flags flags;
	};

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
