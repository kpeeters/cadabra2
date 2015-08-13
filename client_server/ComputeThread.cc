
#include <string>
#include <iostream>
#include <sstream>
#include "ComputeThread.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"
#include "Actions.hh"
#include "spawn.h"

using namespace cadabra;
typedef websocketpp::client<websocketpp::config::asio_client> client;

ComputeThread::ComputeThread(GUIBase *g, DocumentThread& dt)
	: gui(g), docthread(dt), connection_is_open(false), server_pid(0)
	{
   // The ComputeThread constructor is always run on the main thread,
	// so we can grab the main thread id here.

	main_thread_id=std::this_thread::get_id();
	}

ComputeThread::~ComputeThread()
	{
	}

void ComputeThread::init() 
	{
	// Setup the WebSockets client.

	wsclient.init_asio();
	wsclient.start_perpetual();
	}

void ComputeThread::try_connect()
	{
	using websocketpp::lib::bind;

	// Make the resolver work when there is no network up at all, only localhost.
	// https://svn.boost.org/trac/boost/ticket/2456
	// Not sure why this works here, as the compiler claims this statement does
	// not have any effect.
   // No longer necessary with websocketpp-0.6.0.
	//
   //	boost::asio::ip::resolver_query_base::flags(0);

	wsclient.clear_access_channels(websocketpp::log::alevel::all);
	wsclient.clear_error_channels(websocketpp::log::elevel::all);

	wsclient.set_open_handler(bind(&ComputeThread::on_open, 
											 this, websocketpp::lib::placeholders::_1));
	wsclient.set_fail_handler(bind(&ComputeThread::on_fail, 
											 this, websocketpp::lib::placeholders::_1));
	wsclient.set_close_handler(bind(&ComputeThread::on_close, 
											  this, websocketpp::lib::placeholders::_1));
	wsclient.set_message_handler(bind(&ComputeThread::on_message, this, 
												 websocketpp::lib::placeholders::_1,
												 websocketpp::lib::placeholders::_2));

	std::string uri = "ws://localhost:9002";
	websocketpp::lib::error_code ec;
	connection = wsclient.get_connection(uri, ec);
	if (ec) {
		std::cerr << "cadabra-client: websocket connection error " << ec.message() << std::endl;
		return;
		}

	our_connection_hdl = connection->get_handle();
	wsclient.connect(connection);
	std::cerr << "cadabra-client: connect done" << std::endl;
	}

void ComputeThread::run()
	{
	init();
	try_connect();

	// Enter run loop, which will never terminate anymore. The on_fail and on_close
	// handlers will re-try to establish connections when they go bad.

	wsclient.run();
	}

void ComputeThread::terminate()
	{
	wsclient.stop();

	// If we have started the server ourselves, stop it now so we do 
	// not leave mess behind.
	// http://riccomini.name/posts/linux/2012-09-25-kill-subprocesses-linux-bash/

	if(server_pid!=0) {
		std::cerr << "cadabra-client: killing server" << std::endl;
		kill(server_pid, SIGKILL);
		}
	}

void ComputeThread::on_fail(websocketpp::connection_hdl hdl) 
	{
	std::cerr << "cadabra-client: connection failed" << std::endl;
	connection_is_open=false;
	if(gui && server_pid!=0)
		gui->on_network_error();

	try_spawn_server();
	sleep(1);
	try_connect();
	}

void ComputeThread::try_spawn_server()
	{
	// Startup the server. First generate a UUID, pass this to the
	// starting server, then use this UUID to get access to the server
	// port.

	std::cerr << "cadabra-client: spawning server" << std::endl;
	const char * const sargv[] = {"sh", "-c", "exec cadabra-server", NULL};
	int status;
	status = posix_spawn(&server_pid, "/bin/sh", NULL, NULL, 
								const_cast<char * const *>(sargv), NULL); //environ);
	}

void ComputeThread::on_open(websocketpp::connection_hdl hdl) 
	{
	connection_is_open=true;
	if(gui)
		gui->on_connect();

//	// now it is safe to use the connection
//	std::string msg;
//
////	if(stopit) {
////		msg = 
////			"{ \"header\":   { \"uuid\": \"none\", \"msg_type\": \"execute_interrupt\" },"
////			"  \"content\":  { \"code\": \"print(42)\n\"} "
////			"}";
////		} 
////	else {
//		msg = 
//			"{ \"header\":   { \"uuid\": \"none\", \"msg_type\": \"execute_request\" },"
//			"  \"content\":  { \"code\": \"import time\nprint(42)\ntime.sleep(10)\n\"} "
//			"}";
////		}
//
////	c->send(hdl, "import time\nfor i in range(0,10):\n   print('this is python talking '+str(i))\nex=Ex('A_{m n}')\nprint(str(ex))", websocketpp::frame::opcode::text);
//	c->send(hdl, msg, websocketpp::frame::opcode::text);
	}

void ComputeThread::on_close(websocketpp::connection_hdl hdl) 
	{
	std::cerr << "cadabra-client: connection closed" << std::endl;
	connection_is_open=false;
	if(gui)
		gui->on_disconnect();

	sleep(1);
	try_connect();
	}

DTree::iterator ComputeThread::find_cell_by_id(uint64_t id, bool remove) 
	{
	auto it=running_cells.find(id);
	if(it==running_cells.end())
		throw std::logic_error("Cannot find cell by id");

	DTree::iterator ret = (*it).second;
	if(remove)
		running_cells.erase(it);

	return ret;
	}

void ComputeThread::on_message(websocketpp::connection_hdl hdl, message_ptr msg) 
	{
	client::connection_ptr con = wsclient.get_con_from_hdl(hdl);
//	std::cerr << msg->get_payload() << std::endl;

	// Parse the JSON message.
	Json::Value  root;
	Json::Reader reader;
	bool success = reader.parse( msg->get_payload(), root );
	if ( !success ) {
		std::cerr << "cadabra-client: cannot parse message" << std::endl;
		return;
		}
	const Json::Value header   = root["header"];
	const Json::Value content  = root["content"];
	const Json::Value msg_type = root["msg_type"];
	uint64_t id = header["cell_id"].asUInt64();

	auto it = find_cell_by_id(id, true);
	std::shared_ptr<ActionBase> rs_action = 
		std::make_shared<ActionSetRunStatus>(it, false);
	docthread.queue_action(rs_action);

	if(msg_type.asString()=="response") {
		std::string output = "\\begin{equation*}"+content["output"].asString()+"\\end{equation*}";
		if(output!="\\begin{equation*}\\end{equation*}") {
			std::cerr << "cadabra-client: ComputeThread received response from server" << std::endl;

			// Stick an AddCell action onto the stack. We instruct the
			// action to add this result output cell as a child of the
			// corresponding input cell.
			DataCell result(DataCell::CellType::output, output);
			
			// Finally, the action to add the output cell.
			std::shared_ptr<ActionBase> action = 
				std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
			docthread.queue_action(action);
			}
		}
	else {
		std::cout << "Generating ERROR cell" << std::endl;
		std::string error = "{\\color{red}{\\begin{verbatim}"+content["error"].asString()+"\\end{verbatim}}}";
		if(msg_type.asString()=="fault") {
			error = "{\\color{red}{Kernel fault}}\\begin{small}"+error+"\\end{small}";
			}

		// Stick an AddCell action onto the stack. We instruct the
		// action to add this result output cell as a child of the
		// corresponding input cell.
		DataCell result(DataCell::CellType::output, error);
		
		// Finally, the action.
		std::shared_ptr<ActionBase> action = 
			std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
		docthread.queue_action(action);

		// Position the cursor in the cell that generated the error. All other cells on 
		// the execute queue have been cancelled by the server.
		std::shared_ptr<ActionBase> actionpos =
			std::make_shared<ActionPositionCursor>(it, ActionPositionCursor::Position::in);
		docthread.queue_action(actionpos);

		// FIXME: iterate over all cells and set the running flag to false.
		}

	// Update kernel busy indicator depending on number of running cells.
	if(number_of_cells_executing()>0)
		gui->on_kernel_runstatus(true);
	else
		gui->on_kernel_runstatus(false);

	gui->process_data();

	}

void ComputeThread::execute_cell(DTree::iterator it)
	{
	// This absolutely has to be run on the main GUI thread.
	assert(main_thread_id==std::this_thread::get_id());

	if(connection_is_open==false)
		return;

	const DataCell& dc=(*it);

	std::cout << "cadabra-client: ComputeThread going to execute " << dc.textbuf << std::endl;


	// Position the cursor in the next cell so this one will not 
	// accidentally get executed twice.
	std::shared_ptr<ActionBase> actionpos =
		std::make_shared<ActionPositionCursor>(it, ActionPositionCursor::Position::next);
	docthread.queue_action(actionpos);
	gui->process_data();
	
	// For a code cell, construct a server request message and then
	// send the cell to the server.
	if(it->cell_type==DataCell::CellType::python) {
		running_cells[dc.id()]=it;

		// Schedule an action to update the running status of this cell.
		std::shared_ptr<ActionBase> rs_action = 
			std::make_shared<ActionSetRunStatus>(it, true);
		docthread.queue_action(rs_action);

		Json::Value req, header, content;
		header["uuid"]="none";
		header["cell_id"]=(Json::UInt64)dc.id();
		header["msg_type"]="execute_request";
		req["header"]=header;
		content["code"]=dc.textbuf;
		req["content"]=content;

		std::ostringstream str;
		str << req << std::endl;
		wsclient.send(our_connection_hdl, str.str(), websocketpp::frame::opcode::text);
		gui->on_kernel_runstatus(true);
		}

	// For a LaTeX cell, immediately request a new latex output cell to be displayed.
	if(it->cell_type==DataCell::CellType::latex) {
		// Stick an AddCell action onto the stack. We instruct the
		// action to add this result output cell as a child of the
		// corresponding input cell.
		DataCell result(DataCell::CellType::output, it->textbuf);
		
		std::shared_ptr<ActionBase> action = 
			std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
		docthread.queue_action(action);
		}
	}

int ComputeThread::number_of_cells_executing() const
	{
	return running_cells.size();
	}

void ComputeThread::stop()
	{
	if(connection_is_open==false)
		return;

	Json::Value req, header, content;
	header["uuid"]="none";
	header["msg_type"]="execute_interrupt";
	req["header"]=header;

	std::ostringstream str;
	str << req << std::endl;
	
//	std::cerr << str.str() << std::endl;

	server_pid=0;
	wsclient.send(our_connection_hdl, str.str(), websocketpp::frame::opcode::text);
	}

void ComputeThread::restart_kernel()
	{
	if(connection_is_open==false)
		return;

	// Restarting the kernel means all previously running blocks have stopped running.
	// Inform the GUI about this.
	// FIXME: set all running flags to false
	gui->on_kernel_runstatus(false);

	std::cerr << "cadabra-client: restarting kernel" << std::endl;
	Json::Value req, header, content;
	header["uuid"]="none";
	header["msg_type"]="exit";
	req["header"]=header;

	std::ostringstream str;
	str << req << std::endl;
	
//	std::cerr << str.str() << std::endl;

	wsclient.send(our_connection_hdl, str.str(), websocketpp::frame::opcode::text);
	}
