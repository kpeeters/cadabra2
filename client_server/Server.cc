
#include <signal.h>
#include "Server.hh"
#include "InstallPrefix.hh"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <regex>

#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/algorithm/string/replace.hpp>

#include <internal/uuid.h>
#include <internal/string_tools.h>

#include "Config.hh"
//#ifndef ENABLE_JUPYTER
//#include "Snoop.hh"
//#endif
#include "CdbPython.hh"
#include "SympyCdb.hh"
#include "pythoncdb/py_helpers.hh"

// #define DEBUG 1


bool interrupt_block=false;

Server::Server()
//	: return_cell_id(std::numeric_limits<uint64_t>::max()/2)
	{
	boost::uuids::uuid authentication_uuid = boost::uuids::random_generator()();
	authentication_token = boost::uuids::to_string( authentication_uuid );
	// FIXME: we do not actually do anything with this.
	init();
	}

Server::Server(const std::string& socket)
//	: return_cell_id(std::numeric_limits<uint64_t>::max()/2)
	{
	init();
	}

Server::~Server()
	{

	}

Server::CatchOutput::CatchOutput()
	{
	}

Server::CatchOutput::CatchOutput(const CatchOutput&)
	{
	}

void Server::CatchOutput::write(const std::string& str)
	{
	// std::cerr << "Python wrote: " << str << std::endl;
	collect+=str;
	}

void Server::CatchOutput::clear()
	{
	// std::cerr << "Python clear" << std::endl;
	collect="";
	}

std::string Server::CatchOutput::str() const
	{
	return collect;
	}

std::string Server::architecture() const
	{
	return "client-server";
	}

PYBIND11_EMBEDDED_MODULE(cadabra2_internal, m)
	{
	//   auto cadabra_module = pybind11::module::import("cadabra2");

	pybind11::class_<Server::CatchOutput>(m, "CatchOutput")
	.def("write", &Server::CatchOutput::write)
	.def("clear", &Server::CatchOutput::clear)
	;

	pybind11::class_<Server>(m, "Server")
	.def("send", &Server::send)
	.def("handles", &Server::handles)
	.def("architecture", &Server::architecture)
	.def("send_progress_update", &Server::send_progress_update);
	}

std::string parse_error(const std::string& error, const std::string& input)
{
	try {
		// Find syntax errors
		std::regex syntax_error(R"(SyntaxError: \('([^']+)', \('<string>', (\d+), (\d+),.*)");
		std::smatch sm;
		if (std::regex_match(error, sm, syntax_error)) {
			std::string error_type = sm[1];
			size_t line_no = stoi(sm[2]) - 1;
			size_t col_no = stoi(sm[3]);
			return
				"SyntaxError: " + error_type + "\n" +
				"Line " + std::to_string(line_no) + ", column " + std::to_string(col_no) + "\n" +
				nth_line(input, line_no - 1) + "\n" +
				std::string(col_no > 1 ? col_no - 2 : 0, ' ') + "^";
		}

		// Find other errors
		std::regex exception_name(R"(([a-zA-Z_][a-zA-Z0-9_]*):.*)");
		std::string first_line = nth_line(error, 0);
		if (std::regex_match(first_line, sm, exception_name)) {
			std::string name = sm[1];
			std::regex line_no(R"(<string>\((\d+)\): <module>)");
			std::smatch lm;
			if (std::regex_search(error, lm, line_no)) {
				size_t l = stoi(lm[1]) - 1;
				return std::regex_replace(error, line_no, "Notebook Cell (Line " + std::to_string(l) + "): " + nth_line(input, l - 1));
			}
		}

		return error;
	}
	catch (std::exception& e) {
		return error;
	}
}

void Server::init()
	{
	started=false;

	main_module = pybind11::module::import("__main__");
	main_namespace = main_module.attr("__dict__");

	// Make the C++ CatchOutput class visible on the Python side.

	auto python_path = cadabra::install_prefix_of_module();

	// FIXME: since the logic above *requires* that we can find the
	// `cdb.main` module, we will already have the correct path
	// set. So appending it once more below is useless.
	
	std::string stdOutErr =
	   "import sys\n"
	   "sys.path.append(r'"+python_path+"')\n"

	   "from cadabra2_internal import Server, CatchOutput\n"
	   "server=0\n"
	   "def setup_catch(cO, cE, sE):\n"
	   "   global server\n"
	   "   sys.stdout=cO\n"
	   "   sys.stderr=cE\n"
	   "   server=sE\n";
	run_string(stdOutErr, false);

	// Setup the C++ output catching objects and setup the Python side to
	// use these as stdout and stderr streams.

	pybind11::object setup_catch = main_module.attr("setup_catch");
	try {
		setup_catch(std::ref(catchOut), std::ref(catchErr), std::ref(*this));
		}
	catch(pybind11::error_already_set& ex) {
//#ifndef ENABLE_JUPYTER
//		snoop::log(snoop::fatal) << "Failed to initialise Python bridge." << snoop::flush;
//#endif
		PyErr_Print();
		throw;
		}

	// Get the Python thread id.
	std::string code_get_id = "import threading; print(threading.get_native_id())";
	std::string main_thread_id_str = run_string(code_get_id);
	main_thread_id = std::stol(main_thread_id_str);
	// std::cerr << "Server: main python thread id = " << main_thread_id << std::endl;
	// std::cerr << "Server: python_path = " << python_path << std::endl;
	
	// Call the Cadabra default initialisation script.

	//	pybind11::eval_file(python_path + "/cadabra2_defaults.py");
	//	HERE: should use pybind11::eval_file instead, much simpler.
	//
	std::string startup =
	   "f=open(r'" + python_path + "/cadabra2_defaults.py'); "
	   "code=compile(f.read(), 'cadabra2_defaults.py', 'exec'); "
	   "exec(code); f.close() ";
	run_string(startup);

#ifdef DEBUG
	std::cerr << "Server::init: completed" << std::endl;
#endif
	}

int InterruptCheck(PyObject* obj, _frame* frame, int what, PyObject* arg)
	{
	std::cerr << "Server: interruptcheck" << std::endl;
	if(interrupt_block) {
		PyErr_SetString(PyExc_KeyboardInterrupt, "Stop script");
		interrupt_block = false;
		}
	
	return 0;
	}

std::string Server::run_string(const std::string& blk, bool handle_output, bool extract_variables,
										 std::set<std::string> remove_assignments)
	{
	//std::cerr << "RUN_STRING" << std::endl;
	// snoop::log("run") << blk << snoop::flush;

	std::string result, newblk;
	
	// Run block. Catch output.
	try {
		// Preparse input block.
		// std::cerr << "RAW:\n" << blk << std::endl;
		std::string error;
		newblk = cadabra::cdb2python_string(blk, true, error);
		
		// std::cerr << "PREPARSED:\n" << newblk << std::endl;
		// snoop::log("preparsed") << newblk << snoop::flush;
		
		run_string_variables.clear();
		if(error.size()==0) {
			// If the preparsing found an error, do not attempt anything
			// else; just run it and let Python report the error.
			if(extract_variables) {
				cadabra::variables_in_code(newblk, run_string_variables);
				// std::cerr << "----" << std::endl;
				// for(const auto& name: run_string_variables)
				// 	std::cerr << "contains " << name << std::endl;
				}
			for(const std::string& var: remove_assignments) {
				newblk = cadabra::remove_variable_assignments(newblk, var);
				}
			// std::cerr << "REMOVED:\n" << newblk << std::endl;
			}
		
#ifdef DEBUG
		std::cerr << "executing..." << std::endl;
		std::cerr << newblk << std::endl;
#endif
		PyErr_Clear();
//		PyEval_SetTrace(InterruptCheck, NULL);		
		pybind11::exec(newblk.c_str(), main_namespace);
//		PyEval_SetTrace(NULL, NULL);		
#ifdef DEBUG
		std::cerr << "exec done" << std::endl;
#endif
		//		std::string object_classname = ignored.attr("__class__").attr("__name__").cast<std::string>();
		//		std::cerr << "" << std::endl;

		if(handle_output) {
			result = catchOut.str();
			catchOut.clear();
			std::string result_err = catchErr.str();
			if(result_err!="") 
				std::cerr << "catchErr: " << result_err << std::endl;
			catchErr.clear();
			}
		}
	catch(pybind11::error_already_set& ex) {
#ifdef DEBUG
		std::cerr << "Server::run_string: exception " << ex.what() << std::endl;
#endif
		// On macOS and with the current conda tools,
		// you can never exit from this block: throwing or simply
		// exiting with 'return ""' makes things hang.
		// The solution is to ex.restore(), see
		//    https://github.com/pybind/pybind11/issues/1490
		// Note: the restore() has the side effect of making the
		// error come back on any future pybind11::exec() call.
		std::string reason=parse_error(ex.what(), newblk);
		ex.restore();
		if(reason.substr(0, 17)=="KeyboardInterrupt") {
			auto loc = reason.find("At:");
			reason = "Interrupted a" + reason.substr(loc+1);
			}
//		std::cerr << "gobbling " << catchOut.str() << std::endl;
//		catchOut.clear();
		throw std::runtime_error(reason);
		}

	server_stopwatch.stop();
	return result;
	}

//void Server::on_socket_init(websocketpp::connection_hdl, boost::asio::ip::tcp::socket & /* s */)
//	{
//	boost::asio::ip::tcp::no_delay option(true);
//	// FIXME: this used to work in older websocketpp
////	s.lowest_layer().set_option(option);
//	}

Server::Connection::Connection()
	{
	uuid = boost::uuids::random_generator()();
	}

void Server::on_open(websocket_server::id_type ws_id)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);
	Connection con;
	con.ws_id = ws_id;
	// snoop::log(snoop::info) << "Connection " << con.uuid << " open." << snoop::flush;
	connections[ws_id]=con;
	}

void Server::on_close(websocket_server::id_type ws_id)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);
	//	auto it = connections.find(hdl);
	// snoop::log(snoop::info) << "Connection " << it->second.uuid << " close." << snoop::flush;
	connections.erase(ws_id);

	if(exit_on_disconnect)
		exit(-1);
	}

int quit(void *)
	{
	std::cerr << "Server: setting python interrupt." << std::endl;
	PyErr_SetInterrupt();
	std::cerr << "Server: python interrupt set." << std::endl;
	return -1;
	}

void Server::wait_for_websocket()
	{
	try {
		wserver.set_message_handler(std::bind(&Server::on_message, this,
														 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		wserver.set_connect_handler(std::bind(&Server::on_open, this,     std::placeholders::_1));
		wserver.set_disconnect_handler(std::bind(&Server::on_close, this, std::placeholders::_1));

		wserver.listen(run_on_port);

		auto p = wserver.get_local_port();
		std::cout << p  << std::endl;
		std::cout << authentication_token << std::endl;
		wserver.run();
		}
	catch(std::exception& ex) {
		std::cerr << "Server::wait_for_websocket: exception " << ex.what() << std::endl;
		throw;
		}
	}

void Server::wait_for_job()
	{
	// Infinite loop, waiting for the master thread to signal that a new block is
	// available, and processing it. Blocks are always processed sequentially
	// even though new ones may come in before previous ones have finished.

	// snoop::log(snoop::info) << "Waiting for blocks" << snoop::flush;

#ifdef DEBUG
	std::cerr << "Server::wait_for__job: start" << std::endl;
#endif

#ifndef CDB_DONT_ACQUIRE_GIL
	// FIXME: why do we need this for the normal Cadabra server, but does
	// it hang in the Jupyter server? If you drop this from the normal
	// server, it will crash soon below.
	pybind11::gil_scoped_acquire acquire;
#endif

	while(true) {
#ifdef DEBUG
		std::cerr << "Server::wait_for__job: locking" << std::endl;
#endif
		std::unique_lock<std::mutex> lock(block_available_mutex);
		while(block_queue.size()==0) {
#ifdef DEBUG
			std::cerr << "Server::wait_for__job: waiting" << std::endl;
#endif
			block_available.wait(lock);
			}

		Block block = block_queue.front();
		block_queue.pop();
		lock.unlock();

		server_stopwatch.reset();
		server_stopwatch.start();

		try {
			// We are done with the block_queue; release the lock so that the
			// master thread can push new blocks onto it.
			// snoop::log(snoop::info) << "Block finished running" << snoop::flush;
			server_stopwatch.stop();
			current_ws_id   = block.ws_id;
			current_id      = block.cell_id;
			block.output    = run_string(block.input, true, true, block.remove_variable_assignments);
			block.variables = run_string_variables;
			on_block_finished(block);
			}
		catch(std::runtime_error& ex) {
#ifdef DEBUG
			std::cerr << "Exception caught, acquiring lock" << std::endl;
#endif
			server_stopwatch.stop();
			// snoop::log(snoop::info) << "Python runtime exception" << snoop::flush;
			// On error we remove all other blocks from the queue.
			lock.lock();
#ifdef DEBUG
			std::cerr << "Lock acquired" << std::endl;
#endif
			std::queue<Block> empty;
			std::swap(block_queue, empty);
			lock.unlock();
			block.output = catchOut.str();
			catchOut.clear();
			block.error = ex.what();
			on_block_error(block);
			}
		catch(std::exception& ex) {
			server_stopwatch.stop();
			// snoop::log(snoop::info) << "System exception" << snoop::flush;
			lock.lock();
			std::queue<Block> empty;
			std::swap(block_queue, empty);
			lock.unlock();
			block.output=catchOut.str();
			catchOut.clear();
			block.error=ex.what();
			on_kernel_fault(block);
			// Keep running
			}
		}
	}

void Server::stop_block()
	{
//	interrupt_block=true;
	std::cerr << "Server: sending SIGINT to python thread." << std::endl;
	PyErr_SetInterrupt();

	// PyGILState_STATE state = PyGILState_Ensure();
	// //	PyThreadState_SetAsyncExc ?
	// Py_AddPendingCall(&quit, NULL);
	// PyGILState_Release(state);

//	PyGILState_STATE state = PyGILState_Ensure();
//	std::cerr << "Server: make thread " << main_thread_id << " raise exception" << std::endl;
//	PyThreadState_SetAsyncExc(main_thread_id, PyExc_Exception);
//	PyGILState_Release(state);
	}

Server::Block::Block(websocket_server::id_type ws_id_, const std::string& str, uint64_t id_, const std::string& msg_type_)
	: ws_id(ws_id_), msg_type(msg_type_), input(str), cell_id(id_)
	{
	nlohmann::json content, header;
	response["header"]=header;
	response["content"]=content;
	response["msg_type"]=msg_type;
	}

void Server::on_message(websocket_server::id_type ws_id, const std::string& msg,
								const websocket_server::request_type& req, const std::string& ip_address)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);

	auto it = connections.find(ws_id);
	if(it==connections.end()) {
//#ifndef ENABLE_JUPYTER
//		snoop::log(snoop::warn) << "Message from unknown connection." << snoop::flush;
//#endif
		return;
		}

	//	std::cout << "Message from " << it->second.uuid << std::endl;

	dispatch_message(ws_id, msg);
	}

void Server::dispatch_message(websocket_server::id_type ws_id, const std::string& json_msg)
	{
	//	std::cout << json_msg << std::endl;

	nlohmann::json root;
	try {
		root = nlohmann::json::parse(json_msg);
		}
	catch(nlohmann::json::exception& ex) {
//#ifndef ENABLE_JUPYTER
//		snoop::log(snoop::error) << "Cannot parse message " << json_msg << snoop::flush;
//#endif
		return;
		}

	// Check that this message is authenticated.
	std::string auth_token = root.value("auth_token", "");
	if(auth_token!=authentication_token) {
		std::cerr << "Received block with incorrect authentication token: " << auth_token << "." << std::endl;
		return;
		}

	const auto& content    = root["content"];
	const auto& header     = root["header"];
	std::string msg_type = header["msg_type"].get<std::string>();
	// std::cerr << "received msg_type |" << msg_type << "|" << std::endl;

	if(msg_type=="execute_request") {
		std::string code = content.value("code","");
		// std::cerr << code << std::endl;
		uint64_t id = header.value("cell_id", uint64_t(0));
		std::unique_lock<std::mutex> lock(block_available_mutex);
		Block block(ws_id, code, id, msg_type);
		if(content.count("remove_variable_assignments")==1) {
			block.remove_variable_assignments.insert(content.value("remove_variable_assignments", ""));
			}
		block.response["header"]["parent_origin"]="client";
		block.response["header"]["parent_id"]=id;
		block_queue.push(block);
		block_available.notify_one();
		}
	else if(msg_type=="execute_interrupt") {
		std::unique_lock<std::mutex> lock(block_available_mutex);
		std::cerr << "Server: requesting python thread stop." << std::endl;
		stop_block();
		std::cerr << "Server: clearing block queue." << std::endl;
		std::queue<Block> empty;
		std::swap(block_queue, empty);
		std::cerr << "Server: block queue cleared." << std::endl;
		//snoop::log(snoop::warn) << "Job stop requested." << snoop::flush;
		}
	else if(msg_type=="init") {
		// Stop any running blocks.
		std::unique_lock<std::mutex> lock(block_available_mutex);
		stop_block();
		std::queue<Block> empty;
		std::swap(block_queue, empty);
		}
	else if(msg_type=="complete") {
		// Schedule a block which runs code to complete the given string.
		std::string str=root["string"].get<std::string>();
		int alternative=root["alternative"].get<int>();
		std::string todo="print(__cdbkernel__.completer.complete(\""+str+"\", "+std::to_string(alternative)+"))";

		uint64_t id = header.value("cell_id", uint64_t(0));
		Block blk(ws_id, todo, id, "completed");
		blk.response["header"]["cell_id"]=id;
		blk.response["content"]["original"]=str;
		blk.response["content"]["position"]=root["position"].get<int>();
		blk.response["content"]["alternative"]=alternative;

		std::unique_lock<std::mutex> lock(block_available_mutex);
		block_queue.push(blk);
		block_available.notify_one();
		}
	else if(msg_type=="exit") {
		exit(-1);
		}
	}

void Server::on_block_finished(Block block)
	{
	auto& header  = block.response["header"];
	auto& content = block.response["content"];

	if(block.msg_type=="completed") {
		// FIXME: need a better way to get the result out of python, so we can spot None
		// while keeping the possibility to complete 'No' -> 'None'.
		std::string res=block.output;
		if(res.size()>0 && res[res.size()-1]=='\n')
			res=res.substr(0, res.size()-1);
		if(res=="None")
			res="";
		block.response["content"]["completed"]=res;
		}
	else {
		header["cell_origin"]="server";
		header["cell_id"]=cadabra::generate_uuid<uint64_t>();
		header["time_total_microseconds"]=std::to_string(server_stopwatch.seconds()*1e6L + server_stopwatch.useconds());
		header["time_sympy_microseconds"]=std::to_string(sympy_stopwatch.seconds()*1e6L  + sympy_stopwatch.useconds());
		header["last_in_sequence"]=block.error.empty(); // If this is the output followed by an error, it's not the last output cell for the running block.
		content["output"]=block.output;
		block.response["msg_type"]="output";
		}

	// Inform the notebook about the variables referenced in this block.
	content["variables"]=block.variables;

	std::ostringstream str;
	str << block.response << std::endl;
	send_json(str.str());
	}

bool Server::handles(const std::string& otype) const
	{
	if(otype=="latex_view" || otype=="image_png" || otype=="verbatim") return true;
	return false;
	}

uint64_t Server::send(const std::string& output, const std::string& msg_type,
							 uint64_t parent_id, uint64_t cell_id, bool last)
	{
	// This is the function exposed to the Python side; not used
	// directly in the server to send block output back to the client
	// (that's all handled by on_block_finished above).

	// std::cerr << "Send: " << msg_type << ", " << output.substr(0, std::min(size_t(40), output.size())) << std::endl;
	
	nlohmann::json json, header, content;

	uint64_t return_cell_id=cell_id;
	if(return_cell_id==0)
		return_cell_id=cadabra::generate_uuid<uint64_t>();
	
	if(parent_id==0)
		header["parent_id"]=current_id;
	else
		header["parent_id"]=parent_id;

	header["parent_origin"]="client";
	header["cell_id"]=return_cell_id;
	header["cell_origin"]="server";
	header["time_total_microseconds"]=std::to_string(server_stopwatch.seconds()*1e6L + server_stopwatch.useconds());
	header["time_sympy_microseconds"]=std::to_string(sympy_stopwatch.seconds()*1e6L  + sympy_stopwatch.useconds());
	header["last_in_sequence"]=last;

	content["output"]=output;

	json["header"]=header;
	json["content"]=content;
	json["msg_type"]=msg_type;

	std::ostringstream str;
	str << json << std::endl;

	send_json(str.str());

	return return_cell_id;
	}

void Server::send_progress_update(const std::string& msg, int n, int total)
	{
	nlohmann::json json, content, header;

	header["parent_id"] = 0;
	header["parent_origin"] = "client";
	header["cell_id"] = 0;
	header["cell_origin"] = "server";

	content["msg"] = msg;
	content["n"] = n;
	content["total"] = total;

	json["header"] = header;
	json["content"] = content;
	json["msg_type"] = "progress_update";

	std::ostringstream str;
	str << json << std::endl;

	send_json(str.str());
	}

void Server::send_json(const std::string& msg)
	{
	//	std::cerr << "*** sending message " << msg << std::endl;
	std::lock_guard<std::mutex> lock(ws_mutex);
	wserver.send(current_ws_id, msg);
	}

void Server::on_block_error(Block blk)
	{
	if(blk.output!="")
		on_block_finished(blk);
	
	std::lock_guard<std::mutex> lock(ws_mutex);

	// Make a JSON message.
	nlohmann::json json, content, header;

	auto return_cell_id=cadabra::generate_uuid<uint64_t>();
	header["parent_id"]=blk.cell_id;
	header["parent_origin"]="client";
	header["cell_id"]=return_cell_id;
	header["cell_origin"]="server";
	header["last_in_sequence"]=true;
	content["output"]=blk.error;

	json["header"]=header;
	json["content"]=content;
	json["msg_type"]="error";

	std::ostringstream str;
	str << json << std::endl;
	// std::cerr << "cadabra-server: sending error, " << str.str() << std::endl;

	wserver.send(blk.ws_id, str.str());
	}

void Server::on_kernel_fault(Block blk)
	{
	if(blk.output!="")
		on_block_finished(blk);
	
	std::lock_guard<std::mutex> lock(ws_mutex);

	// Make a JSON message.
	nlohmann::json json, content, header;

	auto return_cell_id=cadabra::generate_uuid<uint64_t>();
	header["parent_id"]=blk.cell_id;
	header["parent_origin"]="client";
	header["cell_id"]=return_cell_id;
	header["cell_origin"]="server";
	header["last_in_sequence"]=true;
	content["output"]=blk.error;

	json["header"]=header;
	json["content"]=content;
	json["msg_type"]="fault";

	std::ostringstream str;
	str << json << std::endl;
	// std::cerr << "cadabra-server: sending kernel crash report, " << str.str() << std::endl;

	wserver.send(blk.ws_id, str.str());
	}

void Server::run(int port, bool eod)
	{
	exit_on_disconnect = eod;
	run_on_port = port;

	// Python has to be running on the main thread, otherwise
	// it cannot receive signals. So we spawn the websocket
	// listener on a separate thread, and then do the blocking
	// "wait for python code to execute" loop on the main
	// thread.

//	std::thread::id tmp= std::this_thread::get_id();	
//	main_thread_id = *(unsigned *)&tmp;
//	std::cerr << "Server: main_thread_id = " << main_thread_id << std::endl;
	runner = std::thread(std::bind(&Server::wait_for_websocket, this));

	wait_for_job();
//		pybind11::gil_scoped_release release;
	}
