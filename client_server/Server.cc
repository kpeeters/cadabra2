

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <chrono>
#include <regex>
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <jsoncpp/json/json.h>

#include "Server.hh"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

Server::Server()
	{
	socket_name="tcp://*:5454";
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

void Server::init()
	{
	started=false;

	pre_parse(" hello ");
	pre_parse("   hello ");

	Py_Initialize();
	main_module = boost::python::import("__main__");
	main_namespace = main_module.attr("__dict__");

	// Python pre-amble to capture stdout and stderr into a string.
	std::string stdOutErr =
		"import sys\n"
		"class CatchOut:\n"
		"   def __init__(self):\n"
		"      self.value = ''\n"
		"   def write(self, txt):\n"
		"      self.value += txt\n"
		"   def clear(self):\n"
		"      self.value = ''\n"
		"catchOut = CatchOut()\n"
		"sys.stdout = catchOut\n"
		"class CatchErr:\n"
		"   def __init__(self):\n"
		"      self.value = ''\n"
		"   def write(self, txt):\n"
		"      self.value += txt\n"
		"   def clear(self):\n"
		"      self.value = ''\n"
		"catchErr = CatchErr()\n"
		"sys.stderr = catchErr\n";

	run_string(stdOutErr);

	std::string startup = "import site; execfile(site.getsitepackages()[0]+'/cadabra2_defaults.py');";

	run_string(startup);
	}

std::string Server::pre_parse(const std::string& line)
	{
	std::string ret;

	try {
		std::regex imatch("([\\s]*)([^\\s].*[^\\s])([\\s]*)");
		std::smatch mres;
		std::string indent_line, end_of_line;
		if(std::regex_match(line, mres, imatch)) {
			indent_line=mres[1];
			end_of_line=mres[3];
			}

		std::string line_stripped=mres[2];
		size_t found = line_stripped.find(":=");
		if(found!=std::string::npos) {
			ret = indent_line + line_stripped.substr(0,found) + " = Ex(r'" 
				+ line_stripped.substr(found+2) + "')";
			}
		else {
			found = line_stripped.find("::");
			if(found!=std::string::npos) {
				std::regex amatch("([a-zA-Z]*)(.*)");
				std::smatch ares;
				if(std::regex_match(line_stripped.substr(found+2), ares, amatch)) {
					if(std::string(ares[2]).size()>0) {
						ret = indent_line + "__cdbtmp__ = "+std::string(ares[1])
							+"(Ex(r'"+line_stripped.substr(0,found)
							+"'), Ex('"+std::string(ares[2]).substr(1,std::string(ares[2]).size()-2)+"') )";
						}
					else {
						std::cerr << "no arguments" << std::endl;
						ret = indent_line + "__cdbtmp__ = " + line_stripped.substr(found+2) 
							+ "(Ex(r'"+line_stripped.substr(0,found)+"'))";
						}
					}
				else {
					assert(1==0); // inconsistent
					}
				}
			else {
				ret = line;
				}
			}
		}
	catch(std::regex_error& ex) {
		std::cerr << ex.what() << " " << ex.code() << std::endl;
		}

	return ret;
	}

std::string Server::run_string(const std::string& blk)
	{
	std::string result;

	// Preparse input block line-by-line.
	std::stringstream str(blk);
	std::string line;
	std::string newblk;
	while(std::getline(str, line, '\n')) {
		newblk += pre_parse(line)+'\n';
		}
	std::cout << newblk << std::endl;

	// Run block. Catch output.
	try {
		boost::python::object ignored = boost::python::exec(newblk.c_str(), main_namespace);
		boost::python::object catchobj = main_module.attr("catchOut");
		boost::python::object valueobj = catchobj.attr("value");
		result = boost::python::extract<std::string>(valueobj);
		catchobj.attr("clear")();
		}
	catch(boost::python::error_already_set& ex) {
		// Make Python print error to stderr and catch it.
		PyErr_Print();
		boost::python::object catchobj = main_module.attr("catchErr");
		boost::python::object valueobj = catchobj.attr("value");
		std::string err = boost::python::extract<std::string>(valueobj);
		std::cerr << err << std::endl;
		catchobj.attr("clear")();
		throw std::runtime_error(err);
		}
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
 	std::cout << "connection " << con.uuid << " open" << std::endl;
	connections[hdl]=con;
	}

void Server::on_close(websocketpp::connection_hdl hdl)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);    
	auto it = connections.find(hdl);
	std::cout << "connection " << it->second.uuid << " closed" << std::endl;	
	connections.erase(hdl);
	}

int quit(void *)
	{
	PyErr_SetInterrupt();
	return -1;
	}

void Server::wait_for_job()
	{
	// Infinite loop, waiting for the master thread to signal that a new block is
   // available, and processing it.

	std::cout << "waiting for blocks" << std::endl;

	while(true) {
		std::unique_lock<std::mutex> lock(block_available_mutex);
		block_available.wait(lock);
		if(block_queue.size()>0) {
			std::cout << "going to run: " << block_queue.front().input << std::endl;
			Block block = block_queue.front();
			block_queue.pop();
			try {
				block.output = run_string(block.input);
				on_block_finished(block);
				}
			catch(std::runtime_error& ex) {
				block.error = ex.what();
				on_block_error(block);
				}
			}
		}
	}

void Server::stop_block() 
	{
	PyGILState_STATE state = PyGILState_Ensure();
	Py_AddPendingCall(&quit, NULL);
	PyGILState_Release(state);
	}

Server::Block::Block(websocketpp::connection_hdl h, const std::string& str, uint64_t id_)
	: hdl(h), input(str), id(id_)
	{
	}

void Server::on_message(websocketpp::connection_hdl hdl, WebsocketServer::message_ptr msg) 
	{
	std::lock_guard<std::mutex> lock(ws_mutex);    

	auto it = connections.find(hdl);
	if(it==connections.end()) {
		std::cout << "Message from unknown connection" << std::endl;
		return;
		}

	std::cout << "Message from " << it->second.uuid << std::endl;
	
	dispatch_message(hdl, msg->get_payload());
	}

void Server::dispatch_message(websocketpp::connection_hdl hdl, const std::string& json_msg)
	{
	std::cout << json_msg << std::endl;

	Json::Value  root;   // will contains the root value after parsing.
	Json::Reader reader;
	bool parsingSuccessful = reader.parse( json_msg, root );
	if ( !parsingSuccessful ) {
		std::cout << "cannot parse message" << std::endl;
		return;
		}

	const Json::Value content = root["content"];
	const Json::Value header  = root["header"];
	std::cout << "got header" << std::endl;
	std::string msg_type = header["msg_type"].asString();
	std::cout << "received msg_type |" << msg_type << "|" << std::endl;

	if(msg_type=="execute_request") {
		std::string code = content.get("code","").asString();
		uint64_t id = header.get("cell_id", 0).asUInt64();
		std::unique_lock<std::mutex> lock(block_available_mutex);
		block_queue.push(Block(hdl, code, id));
		block_available.notify_one();
		}
	else if(msg_type=="execute_interrupt") {
		stop_block();
		std::cout << "job stop requested" << std::endl;
		}
	}

void Server::on_block_finished(Block blk)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);    
	
	// Make a JSON message.
	Json::Value json, content, header;
	
	header["cell_id"]=(Json::Value::UInt64)blk.id;
	content["output"]=blk.output;

	json["header"]=header;
	json["content"]=content;

	std::ostringstream str;
	str << json << std::endl;
	std::cout << "sending " << str.str() << std::endl;

	wserver.send(blk.hdl, str.str(), websocketpp::frame::opcode::text);
	}

void Server::on_block_error(Block blk)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);    
	
	// Make a JSON message.
	Json::Value json, content, header;
	
	header["cell_id"]=(Json::Value::UInt64)blk.id;
	content["error"]=blk.error;

	json["header"]=header;
	json["content"]=content;

	std::ostringstream str;
	str << json << std::endl;
	std::cout << "sending error: " << str.str() << std::endl;

	wserver.send(blk.hdl, str.str(), websocketpp::frame::opcode::text);
	}

void Server::run() 
	{
	wserver.clear_access_channels(websocketpp::log::alevel::all);
	wserver.clear_error_channels(websocketpp::log::elevel::all);

	wserver.set_socket_init_handler(bind(&Server::on_socket_init, this, ::_1,::_2));
	wserver.set_message_handler(bind(&Server::on_message, this, ::_1, ::_2));
   wserver.set_open_handler(bind(&Server::on_open,this,::_1));
	wserver.set_close_handler(bind(&Server::on_close,this,::_1));
	
	wserver.init_asio();
	wserver.set_reuse_addr(true);
	wserver.listen(9002);
	wserver.start_accept();

	runner = std::thread(std::bind(&Server::wait_for_job, this));
	
	wserver.run();
	}
