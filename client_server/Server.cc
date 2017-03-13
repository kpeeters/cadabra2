

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
//#include <future>
//#include <chrono>
#include <boost/regex.hpp>
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/algorithm/string/replace.hpp>
#include <json/json.h>  
//#include <boost/shared_ptr.hpp>
//#include <boost/make_shared.hpp>

#include "Snoop.hh"
#include "CdbPython.hh"
#include "Server.hh"
#include "SympyCdb.hh"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

Server::Server()
	{
	// FIXME: we do not actually do anything with this.
	socket_name="tcp://localhost:5454";
	init();
	}

Server::Server(const std::string& socket)
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
//	std::cout << "Python wrote: " << str << std::endl;
	collect+=str;
   }

void Server::CatchOutput::clear()
	{
//	std::cout << "Python clear" << std::endl;
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

void Server::init()
	{
	started=false;

	Py_Initialize();
	main_module = boost::python::import("__main__");
	main_namespace = main_module.attr("__dict__");

	// Make the C++ CatchOutput class visible on the Python side.

   boost::python::class_<Server::CatchOutput>("CatchOutput")
	   .def("write", &Server::CatchOutput::write)
	   .def("clear", &Server::CatchOutput::clear)
    	;

	// FIXME: Why does 's=Stopwatch()' not work in a notebook cell?
	boost::python::class_<Stopwatch>("Stopwatch")
		.def("start", &Stopwatch::start)
		.def("stop",  &Stopwatch::stop)
		.def("reset", &Stopwatch::reset)
		.def("seconds", &Stopwatch::seconds)
		.def("useconds", &Stopwatch::useconds);
	
	boost::python::class_<Server, boost::noncopyable>("Server")
		.def("send", &Server::send)
		.def("architecture", &Server::architecture);

	std::string stdOutErr =
		"import sys\n"
		"server=0\n"
		"def setup_catch(cO, cE, sE):\n"
		"   global server\n"
		"   sys.stdout=cO\n"
		"   sys.stderr=cE\n"
		"   server=sE\n";
	run_string(stdOutErr, false);

	// Setup the C++ output catching objects and setup the Python side to
	// use these as stdout and stderr streams.

	boost::python::object setup_catch = main_module.attr("setup_catch");
	try {
		setup_catch(boost::ref(catchOut), boost::ref(catchErr), boost::ref(*this));
		}
	catch(boost::python::error_already_set& ex) {
		snoop::log(snoop::fatal) << "Failed to initialise Python bridge." << snoop::flush;
		PyErr_Print();
		throw;
		}


	// Call the Cadabra default initialisation script.

//	std::string startup = "import site; execfile(site.getsitepackages()[0]+'/cadabra2_defaults.py')";
//	std::string startup = "import imp; execfile(imp.find_module('cadabra2_defaults')[1])";
	std::string startup = "import imp; f=open(imp.find_module('cadabra2_defaults')[1]); code=compile(f.read(), 'cadabra2_defaults.py', 'exec'); exec(code); f.close()"; 
	run_string(startup);
	}

void Server::start_sympy_stopwatch()
	{
	sympy_stopwatch.start();
	}

void Server::stop_sympy_stopwatch()
	{
	sympy_stopwatch.stop();
	}

std::string Server::run_string(const std::string& blk, bool handle_output)
	{
//	std::cerr << "RUN_STRING" << std::endl;
	// snoop::log("run") << blk << snoop::flush;

	std::string result;

	// Preparse input block.
	auto newblk = cadabra::cdb2python(blk);

	// std::cerr << "PREPARSED:\n " << newblk << std::endl;
	// snoop::log("preparsed") << newblk << snoop::flush;

	// Run block. Catch output.
	try {
		boost::python::object ignored = boost::python::exec(newblk.c_str(), main_namespace);
		std::string object_classname = boost::python::extract<std::string>(ignored.attr("__class__").attr("__name__"));
		// snoop::log("info") << "Run finished" << snoop::flush;
//		std::cout << "exec returned a " << object_classname << std::endl;
		/*
		boost::python::object catchobj = main_module.attr("catchOut");
		boost::python::object valueobj = catchobj.attr("value");
		result = boost::python::extract<std::string>(valueobj);
		catchobj.attr("clear")();
		*/

		if(handle_output) {
			result = catchOut.str();
			catchOut.clear();
			}
		}
	catch(boost::python::error_already_set& ex) {
		// Make Python print error to stderr and catch it.
		PyErr_Print();
		/*
		  boost::python::object catchobj = main_module.attr("catchErr");
		  boost::python::object valueobj = catchobj.attr("value");
		  std::string err = boost::python::extract<std::string>(valueobj);
		*/
		std::string err;
		if(handle_output) {
			err = catchErr.str();
			catchErr.clear();
			// std::cerr << "ERROR: " << err << std::endl;
//		catchobj.attr("clear")();
			}
		throw std::runtime_error(err);
		}
   std::cerr << "------------" << std::endl;

	server_stopwatch.stop();
	return result;
	}

void Server::on_socket_init(websocketpp::connection_hdl hdl, boost::asio::ip::tcp::socket & s) 
	{
	boost::asio::ip::tcp::no_delay option(true);
	s.set_option(option);
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
	auto it = connections.find(hdl);
	// snoop::log(snoop::info) << "Connection " << it->second.uuid << " close." << snoop::flush;
	connections.erase(hdl);
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

	while(true) {
		std::unique_lock<std::mutex> lock(block_available_mutex);
		while(block_queue.size()==0) 
			block_available.wait(lock);

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
			server_stopwatch.stop();
			snoop::log(snoop::info) << "Python runtime exception" << snoop::flush;
			// On error we remove all other blocks from the queue.
			lock.lock();
			std::queue<Block> empty;
			std::swap(block_queue, empty);
			lock.unlock();
			block.error = ex.what();
			on_block_error(block);
			}
		catch(std::exception& ex) {
			server_stopwatch.stop();
			snoop::log(snoop::info) << "System exception" << snoop::flush;
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
		snoop::log(snoop::warn) << "Message from unknown connection." << snoop::flush;
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
		snoop::log(snoop::error) << "Cannot parse message " << json_msg << snoop::flush;
		return;
		}

	const Json::Value content = root["content"];
	const Json::Value header  = root["header"];
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
		snoop::log(snoop::warn) << "Job stop requested." << snoop::flush;
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
	send(blk.output, "output");
	}

void Server::send(const std::string& output, const std::string& msg_type)
	{
	if(msg_type=="output") 
		std::cerr << "Cell " << msg_type << " timing: " << server_stopwatch << " (in python: " << sympy_stopwatch << ")" << std::endl;
	// Make a JSON message.
	Json::Value json, content, header;
	
	header["parent_id"]=(Json::Value::UInt64)current_id;
	header["parent_origin"]="client";
	header["cell_id"]=1; //FIXME
	header["cell_origin"]="server";
	header["time_total_microseconds"]=std::to_string(server_stopwatch.seconds()*1e6L + server_stopwatch.useconds());
	header["time_sympy_microseconds"]=std::to_string(sympy_stopwatch.seconds()*1e6L  + sympy_stopwatch.useconds());
	content["output"]=output;

	json["header"]=header;
	json["content"]=content;
	json["msg_type"]=msg_type;
	
	std::ostringstream str;
	str << json << std::endl;

	send_json(str.str());
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
	
	header["parent_id"]=(Json::Value::UInt64)blk.cell_id;
	header["parent_origin"]="client";
	header["cell_id"]=1; // FIXME;
	header["cell_origin"]="server";
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
	
	header["parent_id"]=(Json::Value::UInt64)blk.cell_id;
	header["parent_origin"]="client";
	header["cell_id"]=1; // FIXME
	header["cell_origin"]="server";
	content["output"]=blk.error;

	json["header"]=header;
	json["content"]=content;
	json["msg_type"]="fault";

	std::ostringstream str;
	str << json << std::endl;
	// std::cerr << "cadabra-server: sending kernel crash report, " << str.str() << std::endl;

	wserver.send(blk.hdl, str.str(), websocketpp::frame::opcode::text);
	}

void Server::run() 
	{
	try {
		wserver.clear_access_channels(websocketpp::log::alevel::all);
		wserver.clear_error_channels(websocketpp::log::elevel::all);
		
		wserver.set_socket_init_handler(bind(&Server::on_socket_init, this, ::_1,::_2));
		wserver.set_message_handler(bind(&Server::on_message, this, ::_1, ::_2));
		wserver.set_open_handler(bind(&Server::on_open,this,::_1));
		wserver.set_close_handler(bind(&Server::on_close,this,::_1));
		
		wserver.init_asio();
		wserver.set_reuse_addr(true);
		wserver.listen(0);
		wserver.start_accept();
		websocketpp::lib::asio::error_code ec;
		auto p = wserver.get_local_endpoint(ec);
		std::cout << p.port()  << std::endl;
		
		// std::cerr << "cadabra-server: spawning job thread "  << std::endl;
		runner = std::thread(std::bind(&Server::wait_for_job, this));
		
		wserver.run();
		}
	catch(websocketpp::exception& ex) {
		std::cerr << "cadabra-server: websocket exception " << ex.code() << " " << ex.what() << std::endl;
		}
	}
