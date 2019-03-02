#include <iostream>
#include <memory>

#include "cadabra-jupyter-kernel.hh"
#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"

int main(int argc, char* argv[])
	{
	std::string file_name = (argc == 1) ? "connection.json" : argv[2];
	xeus::xconfiguration config = xeus::load_configuration(file_name);
	
	using interpreter_ptr = std::unique_ptr<cadabra::CadabraJupyter>;
	interpreter_ptr interpreter = interpreter_ptr(new cadabra::CadabraJupyter());
	xeus::xkernel kernel(config, "kpeeters", std::move(interpreter));
	std::cout << "starting kernel" << std::endl;
	kernel.start();
	
	return 0;
	}
