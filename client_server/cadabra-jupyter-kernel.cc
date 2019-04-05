
#include "Config.hh"
#include <iostream>
#include "cadabra-jupyter-kernel.hh"
#include "xeus/xguid.hpp"
#include <boost/algorithm/string.hpp>

// #define DEBUG 1

cadabra::CadabraJupyter::CadabraJupyter()
	: Server()
	{
	runner = std::thread(std::bind(&Server::wait_for_job, this));
	pybind11::gil_scoped_release release;
	}

void cadabra::CadabraJupyter::configure_impl()
	{
	auto handle_comm_opened = [](xeus::xcomm&& comm, const xeus::xmessage&) {
		std::cerr << "Comm opened for target: " << comm.target().name() << std::endl;
		};
	comm_manager().register_comm_target("echo_target", handle_comm_opened);
//	using function_type = std::function<void(xeus::xcomm&&, const xeus::xmessage&)>;
#ifdef DEBUG
	std::cerr << "CadabraJupyter configured" << std::endl;
#endif
	}

xjson cadabra::CadabraJupyter::execute_request_impl(int execution_counter,
      const std::string& code,
      bool silent,
      bool store_history,
      xjson /* user_expressions */,
      bool allow_stdin)
	{
#ifdef DEBUG
	std::cerr << "Received execute_request" << std::endl;
	std::cerr << "execution_counter: " << execution_counter << std::endl;
	std::cerr << "code: " << code << std::endl;
	std::cerr << "silent: " << silent << std::endl;
	std::cerr << "store_history: " << store_history << std::endl;
	std::cerr << "allow_stdin: " << allow_stdin << std::endl;
	std::cerr << std::endl;
#endif

	std::unique_lock<std::mutex> lock(block_available_mutex);
	websocketpp::connection_hdl hdl;
	block_queue.push(Block(hdl, code, execution_counter));
	block_available.notify_one();

	// The 'wait_for_job' function which runs in a separate thread will take
	// care of executing the 'code'. If anything in 'code' uses 'display',
	// it will run the 'send' function below. At the end of the code
	// execution, a final output block will be sent by 'Server::on_block_finished'.

	xjson result;
	result["status"] = "ok";
	return result;
	}

void cadabra::CadabraJupyter::on_block_error(Block blk)
	{
#ifdef DEBUG
	std::cerr << "error: " << blk.error << std::endl;
#endif
	std::vector<std::string> traceback;
	// FIXME: This does not show the error, for some reason...
	publish_execution_error("Exception", blk.error, traceback);
	xjson pub_data;
	pub_data["text/markdown"] = blk.error;
//	xjson extra_data;
//	extra_data["dummy"] = "dummy";
	// FIXME: ... so we send it again as a message.
	publish_execution_result(current_id, std::move(pub_data), 0); // std::move(extra_data));
	}

uint64_t cadabra::CadabraJupyter::send(const std::string& output, const std::string& msg_type, uint64_t parent_id, bool last)
	{
#ifdef DEBUG
	std::cerr << "received: " << msg_type << " " << output << std::endl;
#endif
	if(output.size()>0) {
		if(msg_type=="verbatim" || msg_type=="output") {
			xjson pub_data;
			pub_data["text/markdown"] = output;
//			xjson extra_data;
//			extra_data["dummy"] = "dummy";
			publish_execution_result(current_id, std::move(pub_data), 0); // std::move(extra_data));
			}
		else if(msg_type=="latex_view") {
			xjson pub_data;
			std::string tmp=output;
			boost::replace_all(tmp, "\\begin{dmath*}", "$");
			boost::replace_all(tmp, "\\end{dmath*}", "$");
			pub_data["text/markdown"] = tmp;
//			xjson extra_data;
//			extra_data["dummy"] = "dummy";
			publish_execution_result(current_id, std::move(pub_data), 0); // std::move(extra_data));
			}
		}
	return current_id;
	}

xjson cadabra::CadabraJupyter::complete_request_impl(const std::string& code,
      int cursor_pos)
	{
#ifdef DEBUG
	std::cerr << "Received complete_request" << std::endl;
	std::cerr << "code: " << code << std::endl;
	std::cerr << "cursor_pos: " << cursor_pos << std::endl;
	std::cerr << std::endl;
#endif
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
#ifdef DEBUG
	std::cerr << "Received inspect_request" << std::endl;
	std::cerr << "code: " << code << std::endl;
	std::cerr << "cursor_pos: " << cursor_pos << std::endl;
	std::cerr << "detail_level: " << detail_level << std::endl;
	std::cerr << std::endl;
#endif
	xjson result;
	result["status"] = "ok";
	result["found"] = false;
	return result;
	}

xjson cadabra::CadabraJupyter::is_complete_request_impl(const std::string& code)
	{
#ifdef DEBUG
	std::cerr << "Received is_complete_request" << std::endl;
	std::cerr << "code: " << code << std::endl;
	std::cerr << std::endl;
#endif
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


