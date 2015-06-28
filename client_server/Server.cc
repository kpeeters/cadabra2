

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <chrono>
//#include <regex>
#include <boost/regex.hpp>
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <jsoncpp/json/json.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "Server.hh"

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

	std::string stdOutErr =
		"import sys\n"
		"def setup_catch(cO, cE):\n"
		"   sys.stdout=cO\n"
		"   sys.stderr=cE\n";
	run_string(stdOutErr, false);

	// Setup the C++ output catching objects and setup the Python side to
	// use these as stdout and stderr streams.

	boost::python::object setup_catch = main_module.attr("setup_catch");
	try {
		setup_catch(boost::ref(catchOut), boost::ref(catchErr));
		}
	catch(boost::python::error_already_set& ex) {
		PyErr_Print();
		throw;
		}

	// Call the Cadabra default initialisation script.

	std::string startup = "import site; execfile(site.getsitepackages()[0]+'/cadabra2_defaults.py')";
	run_string(startup);
	}

std::string Server::pre_parse(const std::string& line)
	{
	std::string ret;

	/*	try */{
//		std::regex imatch("([\\s]*)([^\\s].*[^\\s])([\\s]*)");
//		std::smatch mres;
		boost::regex imatch("([\\s]*)([^\\s].*[^\\s])([\\s]*)");
		boost::cmatch mres;

		std::string indent_line, end_of_line;
		if(boost::regex_match(line.c_str(), mres, imatch)) {
			indent_line=std::string(mres[1].first, mres[1].second);
			end_of_line=std::string(mres[3].first, mres[3].second);
			}

		std::string line_stripped=std::string(mres[2].first, mres[2].second);

		// 'lastchar' is either a Cadabra termination character, or empty.
		// 'line_stripped' will have that character stripped, if present.
		std::string lastchar = line_stripped.substr(line_stripped.size()-1,1);
		if(lastchar!="." && lastchar!=";" && lastchar!=":")
			lastchar="";
		else 
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);

		size_t found = line_stripped.find(":=");
		if(found!=std::string::npos) {
			ret = indent_line + line_stripped.substr(0,found) + " = Ex(r'" 
				+ line_stripped.substr(found+2) + "')";
			// FIXME: add cadabra line continuations
			std::string objname = line_stripped.substr(0,found);
			if(lastchar!="." && indent_line.size()==0)
				ret = ret + "; print("+objname+")";
			}
		else {
			found = line_stripped.find("::");
			if(found!=std::string::npos) {
				std::cerr << "prop token found" << std::endl;
//				std::regex amatch("([a-zA-Z]*)(\\([.]*\\))?");
				boost::regex amatch(R"(([a-zA-Z]+)(.*)[;\.:]*)");
				boost::cmatch ares;
				if(boost::regex_match(line_stripped.substr(found+2).c_str(), ares, amatch)) {
					auto propname = std::string(ares[1].first, ares[1].second);
					if(ares[2].second>ares[2].first+1) {
						auto argument = std::string(ares[2].first+1, ares[2].second-1);
						ret = indent_line + "__cdbtmp__ = "+propname
							+"(Ex(r'"+line_stripped.substr(0,found)
							+"'), Ex('" +argument + "') )";
						}
					else {
						std::cerr << "no arguments" << std::endl;
						ret = indent_line + "_ = " + line_stripped.substr(found+2) 
							+ "(Ex(r'"+line_stripped.substr(0,found)+"'))";
						}
					if(lastchar==";") 
						ret += "; print(latex(_))";
					}
				else {
					std::cerr << "inconsistent" << std::endl;
					ret = line; // inconsistent; you are asking for trouble.
					}
				}
			else {
				// std::cerr << "no preparse" << std::endl;
				if(lastchar==";") 
					ret = indent_line + "_ = " + line_stripped + "; print(latex(_))";
				else
					ret = line;
				}
			}
		}
//	catch(std::regex_error& ex) {
//		std::cerr << ex.what() << " " << ex.code() << std::endl;
//		}

	return ret;
	}

std::string Server::run_string(const std::string& blk, bool handle_output)
	{
//	std::cerr << "RUN_STRING" << std::endl;

	std::string result;

	// Preparse input block line-by-line.
	std::stringstream str(blk);
	std::string line;
	std::string newblk;
	while(std::getline(str, line, '\n')) {
		newblk += pre_parse(line)+'\n';
		}
//	std::cerr << "PREPARSED: " << newblk << std::endl;

	// Run block. Catch output.
	try {
		boost::python::object ignored = boost::python::exec(newblk.c_str(), main_namespace);
		std::string object_classname = boost::python::extract<std::string>(ignored.attr("__class__").attr("__name__"));
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
			std::cerr << "ERROR: " << err << std::endl;
//		catchobj.attr("clear")();
			}
		throw std::runtime_error(err);
		}
   //	std::cerr << "------------" << std::endl;
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
 	std::cerr << "cadabra-server: connection " << con.uuid << " open" << std::endl;
	connections[hdl]=con;
	}

void Server::on_close(websocketpp::connection_hdl hdl)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);    
	auto it = connections.find(hdl);
	std::cout << "cadabra-server: connection " << it->second.uuid << " closed" << std::endl;	
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
   // available, and processing it. Blocks are always processed sequentially
	// even though new ones may come in before previous ones have finished.

	std::cout << "cadabra-server: waiting for blocks" << std::endl;

	while(true) {
		std::unique_lock<std::mutex> lock(block_available_mutex);
		while(block_queue.size()==0) 
			block_available.wait(lock);

		std::cout << "cadabra-server: going to run " << block_queue.front().input << std::endl;
		Block block = block_queue.front();
		block_queue.pop();
		// We are done with the block_queue; release the lock so that the
		// master thread can push new blocks onto it.
		lock.unlock();
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
		std::unique_lock<std::mutex> lock(block_available_mutex);
		stop_block();
		std::cout << "clearing block queue" << std::endl;
		std::queue<Block> empty;
		std::swap(block_queue, empty);
		std::cout << "job stop requested" << std::endl;
		}
	else if(msg_type=="exit") {
		exit(-1);
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
	json["msg_type"]="response";

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
	json["msg_type"]="error";

	std::ostringstream str;
	str << json << std::endl;
	std::cout << "sending error: " << str.str() << std::endl;

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
		wserver.listen(9002);
		wserver.start_accept();
		
		std::cerr << "cadabra-server: spawning job thread" << std::endl;
		runner = std::thread(std::bind(&Server::wait_for_job, this));
		
		std::cerr << "cadabra-server: starting websocket server" << std::endl;
		wserver.run();
		std::cerr << "cadabra-server: websocket server terminated" << std::endl;
		}
	catch(websocketpp::exception& ex) {
		std::cerr << "cadabra-server: websocket exception " << ex.code() << " " << ex.what() << std::endl;
		}
	}
