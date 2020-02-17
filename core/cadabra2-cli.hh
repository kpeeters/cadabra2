#include <pybind11/embed.h>
#include <string>

class Shell
{
public:
    Shell(int argc, char* argv[]);

    int interact();
    int run_script(const std::string& filename);

private:
    void execute(const std::string& code);
    bool try_evaluate(const std::string& code);

    void process_ps1(const std::string& line);
    void process_ps2(const std::string& line);

    void process_error(const std::string& msg);
    void process_syntax_error(const std::string& msg);
    void process_system_exit(const std::string& msg);

    bool is_running;
	int return_code;
	pybind11::dict locals;
	std::string ps1, ps2;
	std::string collect;
};
