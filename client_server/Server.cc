
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
#include <json/json.h>

#include <internal/uuid.h>

#include "Config.hh"
//#ifndef ENABLE_JUPYTER
//#include "Snoop.hh"
//#endif
#include "CdbPython.hh"
#include "SympyCdb.hh"

//#define DEBUG 1


using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

Server::Server()
//	: return_cell_id(std::numeric_limits<uint64_t>::max()/2)
	{
	boost::uuids::uuid authentication_uuid = boost::uuids::random_generator()();
	authentication_token = boost::uuids::to_string( authentication_uuid );
	// FIXME: we do not actually do anything with this.
	socket_name="tcp://localhost:5454";
	init();
	}

Server::Server(const std::string& socket)
//	: return_cell_id(std::numeric_limits<uint64_t>::max()/2)
	{
	socket_name=socket;
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
	.def("architecture", &Server::architecture);
	}

void Server::init()
	{
	started=false;

	main_module = pybind11::module::import("__main__");
	main_namespace = main_module.attr("__dict__");

	// Make the C++ CatchOutput class visible on the Python side.

	auto python_path = cadabra::install_prefix()+"/share/cadabra2/python";

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


	// Call the Cadabra default initialisation script.

	//	pybind11::eval_file(PYTHON_SITE_PATH"/cadabra2_defaults.py");
	//	HERE: should use pybind11::eval_file instead, much simpler.
	//
	std::string startup =
	   "f=open(r'" + python_path + "/cadabra2_defaults.py'); "
	   "code=compile(f.read(), 'cadabra2_defaults.py', 'exec'); "
	   "exec(code); f.close()";
	run_string(startup);

#ifdef DEBUG
	std::cerr << "Server::init: completed" << std::endl;
#endif
	}

std::string Server::run_string(const std::string& blk, bool handle_output)
	{
	//std::cerr << "RUN_STRING" << std::endl;
	// snoop::log("run") << blk << snoop::flush;

	std::string result;

	// Preparse input block.
	auto newblk = cadabra::cdb2python_string(blk, true);

	//	std::cerr << "PREPARSED:\n" << newblk << std::endl;
	// snoop::log("preparsed") << newblk << snoop::flush;

	// Run block. Catch output.
	try {
		//		pybind11::object ignored = pybind11::eval<pybind11::eval_statements>(newblk.c_str(), main_namespace);
#ifdef DEBUG
		std::cerr << "executing..." << std::endl;
		std::cerr << newblk << std::endl;
#endif
		PyErr_Clear();
		pybind11::exec(newblk.c_str(), main_namespace);
#ifdef DEBUG
		std::cerr << "exec done" << std::endl;
#endif
		//		std::string object_classname = ignored.attr("__class__").attr("__name__").cast<std::string>();
		//		std::cerr << "" << std::endl;

		if(handle_output) {
			result = catchOut.str();
			catchOut.clear();
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
		std::string reason=ex.what();
		ex.restore();
		throw std::runtime_error(reason);
		}

	server_stopwatch.stop();
	return result;
	}

void Server::on_socket_init(websocketpp::connection_hdl, boost::asio::ip::tcp::socket & /* s */)
	{
	boost::asio::ip::tcp::no_delay option(true);
	// FIXME: this used to work in older websocketpp
//	s.lowest_layer().set_option(option);
	}

Server::Connection::Connection()
	{
	uuid = boost::uuids::random_generator()();
	}

void Server::on_open(websocketpp::connection_hdl hdl)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);
	Connection con;
	con.hdl=hdl;
	// snoop::log(snoop::info) << "Connection " << con.uuid << " open." << snoop::flush;
	connections[hdl]=con;
	}

void Server::on_close(websocketpp::connection_hdl hdl)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);
	//	auto it = connections.find(hdl);
	// snoop::log(snoop::info) << "Connection " << it->second.uuid << " close." << snoop::flush;
	connections.erase(hdl);

	if(exit_on_disconnect)
		exit(-1);
	}

int quit(void *)
	{
	PyErr_SetInterrupt();
	return -1;
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
			current_hdl=block.hdl;
			current_id =block.cell_id;
			block.output = run_string(block.input);
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
			block.error=ex.what();
			on_kernel_fault(block);
			// Keep running
			}
		}
	}

void Server::stop_block()
	{
	PyGILState_STATE state = PyGILState_Ensure();
	//	PyThreadState_SetAsyncExc ?
	Py_AddPendingCall(&quit, NULL);
	PyGILState_Release(state);
	}

Server::Block::Block(websocketpp::connection_hdl h, const std::string& str, uint64_t id_)
	: hdl(h), input(str), cell_id(id_)
	{
	}

void Server::on_message(websocketpp::connection_hdl hdl, WebsocketServer::message_ptr msg)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);

	auto it = connections.find(hdl);
	if(it==connections.end()) {
//#ifndef ENABLE_JUPYTER
//		snoop::log(snoop::warn) << "Message from unknown connection." << snoop::flush;
//#endif
		return;
		}

	//	std::cout << "Message from " << it->second.uuid << std::endl;

	dispatch_message(hdl, msg->get_payload());
	}

void Server::dispatch_message(websocketpp::connection_hdl hdl, const std::string& json_msg)
	{
	//	std::cout << json_msg << std::endl;

	Json::Value  root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( json_msg, root );
	if ( !parsingSuccessful ) {
//#ifndef ENABLE_JUPYTER
//		snoop::log(snoop::error) << "Cannot parse message " << json_msg << snoop::flush;
//#endif
		return;
		}

	// Check that this message is authenticated.
	std::string auth_token = root["auth_token"].asString();
	if(auth_token!=authentication_token) {
		std::cerr << "Received block with incorrect authentication token: " << auth_token << "." << std::endl;
		return;
		}

	const Json::Value content    = root["content"];
	const Json::Value header     = root["header"];
	std::string msg_type = header["msg_type"].asString();
	// std::cerr << "received msg_type |" << msg_type << "|" << std::endl;

	if(msg_type=="execute_request") {
		std::string code = content.get("code","").asString();
		// std::cerr << code << std::endl;
		uint64_t id = header.get("cell_id", 0).asUInt64();
		std::unique_lock<std::mutex> lock(block_available_mutex);
		block_queue.push(Block(hdl, code, id));
		block_available.notify_one();
		}
	else if(msg_type=="execute_interrupt") {
		std::unique_lock<std::mutex> lock(block_available_mutex);
		stop_block();
		//		std::cout << "clearing block queue" << std::endl;
		std::queue<Block> empty;
		std::swap(block_queue, empty);
		//snoop::log(snoop::warn) << "Job stop requested." << snoop::flush;
		}
	else if(msg_type=="init") {
		// Stop any running blocks.
		std::unique_lock<std::mutex> lock(block_available_mutex);
		stop_block();
		std::queue<Block> empty;
		std::swap(block_queue, empty);

		}
	else if(msg_type=="exit") {
		exit(-1);
		}
	}

void Server::on_block_finished(Block blk)
	{
	send(blk.output, "output", 0, true); // last in sequence
	}

bool Server::handles(const std::string& otype) const
	{
	if(otype=="latex_view" || otype=="image_png" || otype=="verbatim") return true;
	return false;
	}

uint64_t Server::send(const std::string& output, const std::string& msg_type, uint64_t parent_id, bool last)
	{
	//	if(msg_type=="output")
	//		std::cerr << "Cell " << msg_type << " timing: " << server_stopwatch << " (in python: " << sympy_stopwatch << ")" << std::endl;
	// Make a JSON message.
	Json::Value json, content, header;

	auto return_cell_id=cadabra::generate_uuid<Json::UInt64>();
	if(parent_id==0)
		header["parent_id"]=(Json::Value::UInt64)current_id;
	else
		header["parent_id"]=(Json::Value::UInt64)parent_id;

	header["parent_origin"]="client";
	header["cell_id"]=(Json::Value::UInt64)return_cell_id;
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

void Server::send_json(const std::string& msg)
	{
	//	std::cerr << "*** sending message " << msg << std::endl;
	std::lock_guard<std::mutex> lock(ws_mutex);
	wserver.send(current_hdl, msg, websocketpp::frame::opcode::text);
	}

void Server::on_block_error(Block blk)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);

	// Make a JSON message.
	Json::Value json, content, header;

	auto return_cell_id=cadabra::generate_uuid<Json::UInt64>();
	header["parent_id"]=(Json::Value::UInt64)blk.cell_id;
	header["parent_origin"]="client";
	header["cell_id"]=(Json::Value::UInt64)return_cell_id;
	header["cell_origin"]="server";
	header["last_in_sequence"]=true;
	content["output"]=blk.error;

	json["header"]=header;
	json["content"]=content;
	json["msg_type"]="error";

	std::ostringstream str;
	str << json << std::endl;
	// std::cerr << "cadabra-server: sending error, " << str.str() << std::endl;

	wserver.send(blk.hdl, str.str(), websocketpp::frame::opcode::text);
	}

void Server::on_kernel_fault(Block blk)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);

	// Make a JSON message.
	Json::Value json, content, header;

	auto return_cell_id=cadabra::generate_uuid<Json::UInt64>();
	header["parent_id"]=(Json::Value::UInt64)blk.cell_id;
	header["parent_origin"]="client";
	header["cell_id"]=(Json::Value::UInt64)return_cell_id;
	header["cell_origin"]="server";
	header["last_in_sequence"]=true;
	content["output"]=blk.error;

	json["header"]=header;
	json["content"]=content;
	json["msg_type"]="fault";

	std::ostringstream str;
	str << json << std::endl;
	// std::cerr << "cadabra-server: sending kernel crash report, " << str.str() << std::endl;

	wserver.send(blk.hdl, str.str(), websocketpp::frame::opcode::text);
	}

void Server::run(int port, bool eod)
	{
	exit_on_disconnect=eod;
	try {
		wserver.clear_access_channels(websocketpp::log::alevel::all);
		wserver.clear_error_channels(websocketpp::log::elevel::all);

		wserver.init_asio();
		wserver.set_reuse_addr(true);

		wserver.set_socket_init_handler(bind(&Server::on_socket_init, this, ::_1,::_2));
		wserver.set_message_handler(bind(&Server::on_message, this, ::_1, ::_2));
		wserver.set_open_handler(bind(&Server::on_open,this,::_1));
		wserver.set_close_handler(bind(&Server::on_close,this,::_1));

#ifdef DEBUG
		std::cerr << "going to listen" << std::endl;
#endif
		wserver.listen(websocketpp::lib::asio::ip::tcp::v4(), port);
#ifdef DEBUG
		std::cerr << "going to accept" << std::endl;
#endif
		wserver.start_accept();
		websocketpp::lib::asio::error_code ec;
		auto p = wserver.get_local_endpoint(ec);
		std::cout << p.port()  << std::endl;
		std::cout << authentication_token << std::endl;

		// std::cerr << "cadabra-server: spawning job thread "  << std::endl;
		runner = std::thread(std::bind(&Server::wait_for_job, this));

		pybind11::gil_scoped_release release;
		wserver.run();
		}
	catch(websocketpp::exception& ex) {
		std::cerr << "cadabra-server: websocket exception " << ex.code() << " " << ex.what() << std::endl;
		}
	}

