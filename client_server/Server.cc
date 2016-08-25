

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

	boost::python::class_<Server, boost::noncopyable>("Server")
		.def("send", &Server::send);

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

std::string Server::escape_quotes(const std::string& line)
	{
	return "''"+line+"''";
//	std::string ret=boost::replace_all_copy(line, "'", "\\'");
//	boost::replace_all(ret, "\"", "\\\"");
//	return ret;
	}

std::string Server::pre_parse(const std::string& line)
	{
	std::string ret;
	
	boost::regex imatch("([\\s]*)([^\\s].*[^\\s])([\\s]*)");
	boost::cmatch mres;
	
	std::string indent_line, end_of_line, line_stripped;
	if(boost::regex_match(line.c_str(), mres, imatch)) {
		indent_line=std::string(mres[1].first, mres[1].second);
		end_of_line=std::string(mres[3].first, mres[3].second);
		line_stripped=std::string(mres[2]);
		}
	else {
		indent_line="";
		end_of_line="\n";
		line_stripped=line;
		}
	
	if(line_stripped.size()==0) {
		return "";
		}

	// Do not do anything with comment lines.
	if(line_stripped[0]=='#') return line; 
	
	// Bare ';' gets replaced with 'display(_)'.
	if(line_stripped==";") return indent_line+"display(_)";

	// 'lastchar' is either a Cadabra termination character, or empty.
	// 'line_stripped' will have that character stripped, if present.
	std::string lastchar = line_stripped.substr(line_stripped.size()-1,1);
	if(lastchar=="." || lastchar==";" || lastchar==":") {
		if(lhs!="") {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			rhs += line_stripped;
			ret = indent + lhs + " = Ex(r'" + escape_quotes(rhs) + "')";
			if(lastchar!=".")
				ret = ret + "; display("+lhs+")";
			indent="";
			lhs="";
			rhs="";
			return ret;
			}
		}
	else { 
		// If we are a Cadabra continuation, add to the rhs without further processing
		// and return an empty line immediately.
		if(lhs!="") {
			rhs += line_stripped+" ";
			return "";
			}
		}
	
	// Replace $...$ with Ex(...).
	boost::regex dollarmatch(R"(\$([^\$]*)\$)");
	line_stripped = boost::regex_replace(line_stripped, dollarmatch, "Ex\\(r'''$1''', False\\)", boost::match_default | boost::format_all);
	
	// Replace 'converge(ex):' with 'ex.reset(); while ex.changed():' properly indented.
	boost::regex converge_match(R"(([ ]*)converge\(([^\)]*)\):)");
	boost::smatch converge_res;
	if(boost::regex_match(line_stripped, converge_res, converge_match)) {
		ret = indent_line+std::string(converge_res[1])+std::string(converge_res[2])+".reset(); _="+std::string(converge_res[2])+"\n"
			 + indent_line+std::string(converge_res[1])+"while "+std::string(converge_res[2])+".changed():";
		return ret;
		}
	
	size_t found = line_stripped.find(":=");
	if(found!=std::string::npos) {
		// If the last character is not a Cadabra terminator, start a capture process.
		if(lastchar!="." && lastchar!=";" && lastchar!=":") {
			indent=indent_line;
			lhs=line_stripped.substr(0,found);
			rhs=line_stripped.substr(found+2);
			return "";
			}
		else {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			ret = indent_line + line_stripped.substr(0,found) + " = Ex(r'" 
				+ escape_quotes(line_stripped.substr(found+2)) + "')";
			std::string objname = line_stripped.substr(0,found);
			if(lastchar==";" && indent_line.size()==0)
				ret = ret + "; display("+objname+")";
			}
		}
	else { // {a,b,c}::Indices(real, parent=holo);
		found = line_stripped.find("::");
		if(found!=std::string::npos) {
			boost::regex amatch(R"(([a-zA-Z]+)(.*)[;\.:]*)");
			boost::smatch ares;
			std::string subline=line_stripped.substr(found+2); // need to store the copy, not feed it straight into regex_match!
			if(boost::regex_match(subline, ares, amatch)) {
				auto propname = std::string(ares[1]);
				auto argument = std::string(ares[2]);
				if(argument.size()>0) { // declaration with arguments
					argument=argument.substr(1,argument.size()-2);
					ret = indent_line + "__cdbtmp__ = "+propname
						+"(Ex(r'"+escape_quotes(line_stripped.substr(0,found))
						+"'), Ex(r'" +escape_quotes(argument) + "') )";
					}
				else {
					// no arguments
					line_stripped=line_stripped.substr(0,line_stripped.size()-1);
					ret = indent_line + "__cdbtmp__ = " + line_stripped.substr(found+2) 
						+ "(Ex(r'"+escape_quotes(line_stripped.substr(0,found))+"'))";
					}
				if(lastchar==";") 
					ret += "; display(__cdbtmp__)";
				}
			else {
				// inconsistent, who knows what will happen...
				ret = line; // inconsistent; you are asking for trouble.
				}
			}
		else {
			if(lastchar==";") 
				ret = indent_line + "_ = " + line_stripped + " display(_)";
			else
				ret = indent_line + line_stripped;
			}
		}
	return ret+end_of_line;
	}

std::string Server::run_string(const std::string& blk, bool handle_output)
	{
//	std::cerr << "RUN_STRING" << std::endl;
	// snoop::log("run") << blk << snoop::flush;

	std::string result;

	// Preparse input block line-by-line.
	std::stringstream str(blk);
	std::string line;
	std::string newblk;
	while(std::getline(str, line, '\n')) {
//		std::cerr << "preparsing: " + line << std::endl;
		std::string res=pre_parse(line);
		std::cerr << "preparsed : " + res << std::endl;
		newblk += res+'\n';
		}
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
	// snoop::log(snoop::info) << "Connection " << con.uuid << " open." << snoop::flush;
	connections[hdl]=con;
	}

void Server::on_close(websocketpp::connection_hdl hdl)
	{
	std::lock_guard<std::mutex> lock(ws_mutex);    
	auto it = connections.find(hdl);
	// snoop::log(snoop::info) << "Connection " << it->second.uuid << " close." << snoop::flush;
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

	// snoop::log(snoop::info) << "Waiting for blocks" << snoop::flush;

	while(true) {
		std::unique_lock<std::mutex> lock(block_available_mutex);
		while(block_queue.size()==0) 
			block_available.wait(lock);

		Block block = block_queue.front();
		block_queue.pop();
		lock.unlock();
		try {
			// We are done with the block_queue; release the lock so that the
			// master thread can push new blocks onto it.
			// snoop::log(snoop::info) << "Block finished running" << snoop::flush;
			current_hdl=block.hdl;
			current_id =block.cell_id;
			block.output = run_string(block.input);
			on_block_finished(block);
			}
		catch(std::runtime_error& ex) {
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
//	std::cout << "received msg_type |" << msg_type << "|" << std::endl;

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
	// Make a JSON message.
	Json::Value json, content, header;
	
	header["parent_id"]=(Json::Value::UInt64)current_id;
	header["parent_origin"]="client";
	header["cell_id"]=1; //FIXME
	header["cell_origin"]="server";
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
		auto p = wserver.get_acceptor()->local_endpoint();
		std::cout << p.port()  << std::endl;
		
		// std::cerr << "cadabra-server: spawning job thread "  << std::endl;
		runner = std::thread(std::bind(&Server::wait_for_job, this));
		
		wserver.run();
		}
	catch(websocketpp::exception& ex) {
		std::cerr << "cadabra-server: websocket exception " << ex.code() << " " << ex.what() << std::endl;
		}
	}
