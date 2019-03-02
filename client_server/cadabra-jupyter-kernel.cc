
#include "Config.hh"
#include <iostream>
#include "cadabra-jupyter-kernel.hh"
#include "xeus/xguid.hpp"
#include <boost/algorithm/string.hpp>

void cadabra::CadabraJupyter::configure_impl()
	{
	auto handle_comm_opened = [](xeus::xcomm&& comm, const xeus::xmessage&) {
		std::cout << "Comm opened for target: " << comm.target().name() << std::endl;
	};
	comm_manager().register_comm_target("echo_target", handle_comm_opened);
	using function_type = std::function<void(xeus::xcomm&&, const xeus::xmessage&)>;
	}
	
xjson cadabra::CadabraJupyter::execute_request_impl(int execution_counter,
																	 const std::string& code,
																	 bool silent,
																	 bool store_history,
																	 xjson /* user_expressions */,
																	 bool allow_stdin)
	{
	std::cout << "Received execute_request" << std::endl;
	std::cout << "execution_counter: " << execution_counter << std::endl;
	std::cout << "code: " << code << std::endl;
	std::cout << "silent: " << silent << std::endl;
	std::cout << "store_history: " << store_history << std::endl;
	std::cout << "allow_stdin: " << allow_stdin << std::endl;
	std::cout << std::endl;

	std::string out = run_string(code);
	
//	xjson pub_data;
//	pub_data["text/plain"] = out;
//	publish_execution_result(execution_counter, std::move(pub_data), xjson());

	xjson result;
	result["status"] = "ok";
	return result;
	}

uint64_t cadabra::CadabraJupyter::send(const std::string& output, const std::string& msg_type, uint64_t parent_id, bool last)
	{
	std::cerr << "received: " << msg_type << " " << output << std::endl;
	if(!last && output.size()>0) {
		if(msg_type=="verbatim") {
			xjson pub_data;
			pub_data["text/markdown"] = output;
			publish_execution_result(1, std::move(pub_data), xjson());
			}
		else if(msg_type=="latex_view") {
			xjson pub_data;
			std::string tmp=output;
			boost::replace_all(tmp, "\\begin{dmath*}", "$");
			boost::replace_all(tmp, "\\end{dmath*}", "$");
			pub_data["text/markdown"] = tmp;
			publish_execution_result(1, std::move(pub_data), xjson());
			}
		}
	}

xjson cadabra::CadabraJupyter::complete_request_impl(const std::string& code,
																	  int cursor_pos)
	{
	std::cout << "Received complete_request" << std::endl;
	std::cout << "code: " << code << std::endl;
	std::cout << "cursor_pos: " << cursor_pos << std::endl;
	std::cout << std::endl;
	xjson result;
	result["status"] = "ok";
	result["matches"] = {"a.echo1"};
	result["cursor_start"] = 2;
	result["cursor_end"] = 6;
	return result;
	}

xjson cadabra::CadabraJupyter::inspect_request_impl(const std::string& code,
																	 int cursor_pos,
																	 int detail_level)
	{
	std::cout << "Received inspect_request" << std::endl;
	std::cout << "code: " << code << std::endl;
	std::cout << "cursor_pos: " << cursor_pos << std::endl;
	std::cout << "detail_level: " << detail_level << std::endl;
	std::cout << std::endl;
	xjson result;
	result["status"] = "ok";
	result["found"] = false;
	return result;
	}

xjson cadabra::CadabraJupyter::is_complete_request_impl(const std::string& code)
	{
	std::cout << "Received is_complete_request" << std::endl;
	std::cout << "code: " << code << std::endl;
	std::cout << std::endl;
	xjson result;
	result["status"] = "complete";
	return result;
	}

xjson cadabra::CadabraJupyter::kernel_info_request_impl()
	{
	xjson result;
	result["implementation"] = "Cadabra";
	result["implementation_version"] = std::string(CADABRA_VERSION_MAJOR)+"."+CADABRA_VERSION_MINOR
		+"."+CADABRA_VERSION_PATCH;
	result["language_info"]["name"] = "cadabra";
	result["language_info"]["version"] = "2.0.0";
	result["language_info"]["mimetype"] = "text/cadabra";
	result["language_info"]["file_extension"] = ".cdb";
	return result;
	}

void cadabra::CadabraJupyter::shutdown_request_impl() {}


