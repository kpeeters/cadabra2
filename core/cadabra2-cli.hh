#include <pybind11/embed.h>
#include <string>
#include <memory>

class Shell
{
public:
    Shell(unsigned int flags = 0);

    void restart();
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

	pybind11::dict locals;
    std::unique_ptr<pybind11::scoped_interpreter> guard;

    bool is_running;
	int return_code;

	std::string ps1, ps2;
	std::string collect;

    unsigned int flags;
};
