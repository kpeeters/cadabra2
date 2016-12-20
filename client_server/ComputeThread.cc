
#include <string>
#include <iostream>
#include <sstream>
#include "ComputeThread.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"
#include "Actions.hh"
#include "popen2.hh"
#include <sys/types.h>
#include <signal.h>
#include <glibmm/spawn.h>

using namespace cadabra;
typedef websocketpp::client<websocketpp::config::asio_client> client;

ComputeThread::ComputeThread()
	: gui(0), docthread(0), connection_is_open(false), restarting_kernel(false), server_pid(0), 
	  server_stdout(0), server_stderr(0)
	{
   // The ComputeThread constructor (but _not_ the run() member!) is
	// always run on the gui thread, so we can grab the gui thread id
	// here.

	gui_thread_id=std::this_thread::get_id();
	}

ComputeThread::~ComputeThread()
	{
	if(server_stdout!=0) {
		close(server_stdout);
		close(server_stderr);
		Glib::spawn_close_pid(server_pid);
		server_pid=0;
		server_stdout=0;
		server_stderr=0;
		}
	}

void ComputeThread::set_master(GUIBase *b, DocumentThread *d)
	{
	gui=b;
	docthread=d;
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
	// boost::asio::ip::resolver_query_base::flags(0);

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

	std::ostringstream uristr;
	uristr << "ws://127.0.0.1:" << port;
	websocketpp::lib::error_code ec;
	connection = wsclient.get_connection(uristr.str(), ec);
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
	try_spawn_server();
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

		if(server_stdout!=0) {
			close(server_stdout);
			close(server_stderr);
			Glib::spawn_close_pid(server_pid);
			server_pid=0;
			server_stdout=0;
			server_stderr=0;
			}
//		kill(server_pid, SIGKILL);
// 		if(server_stdout)
//			pclose2(server_stdout, server_pid); 
		}
	}

void ComputeThread::all_cells_nonrunning()
	{
	for(auto it: running_cells) {
		std::shared_ptr<ActionBase> rs_action = 
			std::make_shared<ActionSetRunStatus>(it.second, false);
		docthread->queue_action(rs_action);
		}
	gui->process_data();
	gui->on_kernel_runstatus(false);
	running_cells.clear();
	}

void ComputeThread::on_fail(websocketpp::connection_hdl hdl) 
	{
	std::cerr << "cadabra-client: connection failed" << std::endl;
	connection_is_open=false;
	all_cells_nonrunning();
	if(gui && server_pid!=0) {
		close(server_stdout);
		close(server_stderr);
		Glib::spawn_close_pid(server_pid);
//		kill(server_pid, SIGKILL);
		server_pid=0;
		server_stdout=0;
		server_stderr=0;
		gui->on_network_error();
		}

	try_spawn_server();
	try_connect();
	}

using SlotSpawnChildSetup = sigc::slot<void()>;

void ComputeThread::try_spawn_server()
	{
	// Startup the server. First generate a UUID, pass this to the
	// starting server, then use this UUID to get access to the server
	// port.

	std::cerr << "cadabra-client: spawning server" << std::endl;

	std::vector<std::string> argv, envp;
#if defined(_WIN32) || defined(_WIN64)
	argv.push_back("cadabra-server.exe");
#else
	argv.push_back("cadabra-server");
#endif
	Glib::Pid pid;
	std::string wd("");
	
	Glib::spawn_async_with_pipes(wd, argv, envp,
										  Glib::SPAWN_DEFAULT|Glib::SPAWN_SEARCH_PATH,
										  sigc::slot<void>(),
										  &pid,
										  0,
										  &server_stdout,
										  &server_stderr);
	
	char buffer[100];
	FILE *f = fdopen(server_stdout, "r");
	if(fscanf(f, "%100s", buffer)!=1) {
		throw std::logic_error("Failed to read port from server.");
		}
	port = atoi(buffer);
	}

void ComputeThread::on_open(websocketpp::connection_hdl hdl) 
	{
	connection_is_open=true;
	restarting_kernel=false;
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
	if(gui) {
		if(restarting_kernel) gui->on_disconnect("restarting kernel");
		else                  gui->on_disconnect("not connected");
		}

	sleep(1); // do not cause a torrent...
	try_connect();
	}

DTree::iterator ComputeThread::find_cell_by_id(DataCell::id_t id, bool remove) 
	{
	auto it=running_cells.find(id);
	if(it==running_cells.end())
		throw std::logic_error("Cannot find cell with id = "+std::to_string(id.id));

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
	DataCell::id_t parent_id;
	parent_id.id = header["parent_id"].asUInt64();
	if(header["parent_origin"].asString()=="client")
		parent_id.created_by_client=true;
	else
		parent_id.created_by_client=false;
	DataCell::id_t cell_id;
	cell_id.id = header["cell_id"].asUInt64();
	if(header["cell_origin"].asString()=="client")
		cell_id.created_by_client=true;
	else
		cell_id.created_by_client=false;

	try {
		// If this is the stdout or stderr of the block, it has finished execution.
		// All other output cells are generated while the cell is executing, and 
		// do not yet mean that execution is finished.
		bool finished=(msg_type.asString()=="output" || msg_type.asString()=="error");

		//std::cerr << "cadabra-client: find " << (long)parent_id.id << " for " << msg_type.asString() << std::endl;
		auto it = find_cell_by_id(parent_id, finished);
		if(finished) {
			std::shared_ptr<ActionBase> rs_action = 
				std::make_shared<ActionSetRunStatus>(it, false);
			docthread->queue_action(rs_action);
			}

		if(content["output"].asString().size()>0) {
			if(msg_type.asString()=="output") {
				std::string output = "\\begin{verbatim}"+content["output"].asString()+"\\end{verbatim}";
				
				// Stick an AddCell action onto the stack. We instruct the
				// action to add this result output cell as a child of the
				// corresponding input cell.
				DataCell result(cell_id, DataCell::CellType::output, output);
				
				// Finally, the action to add the output cell.
				std::shared_ptr<ActionBase> action = 
					std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
				docthread->queue_action(action);
				}
			else if(msg_type.asString()=="verbatim") {
				std::string output = "\\begin{verbatim}"+content["output"].asString()+"\\end{verbatim}";
				
				// Stick an AddCell action onto the stack. We instruct the
				// action to add this result output cell as a child of the
				// corresponding input cell.
				DataCell result(cell_id, DataCell::CellType::verbatim, output);
				
				// Finally, the action to add the output cell.
				std::shared_ptr<ActionBase> action = 
					std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
				docthread->queue_action(action);
				}
			else if(msg_type.asString()=="latex_view") {
				// std::cerr << "received latex cell " << content["output"].asString() << std::endl;
				DataCell result(cell_id, DataCell::CellType::latex_view, content["output"].asString());
				std::shared_ptr<ActionBase> action = 
					std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
				docthread->queue_action(action);
				}
			else if(msg_type.asString()=="error") {
				std::string error = "{\\color{red}{\\begin{verbatim}"+content["output"].asString()
					+"\\end{verbatim}}}";
				if(msg_type.asString()=="fault") {
					error = "{\\color{red}{Kernel fault}}\\begin{small}"+error+"\\end{small}";
					}
				
				// Stick an AddCell action onto the stack. We instruct the
				// action to add this result output cell as a child of the
				// corresponding input cell.
				DataCell result(cell_id, DataCell::CellType::error, error);
				
				// Finally, the action.
				std::shared_ptr<ActionBase> action = 
					std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
				docthread->queue_action(action);
				
				// Position the cursor in the cell that generated the error. All other cells on 
				// the execute queue have been cancelled by the server.
				std::shared_ptr<ActionBase> actionpos =
					std::make_shared<ActionPositionCursor>(it, ActionPositionCursor::Position::in);
				docthread->queue_action(actionpos);
				
				// FIXME: iterate over all cells and set the running flag to false.
				}
			else if(msg_type.asString()=="image_png") {
				DataCell result(cell_id, DataCell::CellType::image_png, content["output"].asString());
				std::shared_ptr<ActionBase> action = 
					std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
				docthread->queue_action(action);
				}
			else {
				std::cerr << "cadabra-client: received cell we did not expect: " 
							 << msg_type.asString() << std::endl;
				}
			}
		}
	catch(std::logic_error& ex) {
		// WARNING: if the server sends
		std::cerr << "cadabra-client: trouble processing server response: " << ex.what() << std::endl;
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
	assert(gui_thread_id==std::this_thread::get_id());

	if(connection_is_open==false)
		return;

	const DataCell& dc=(*it);

	// std::cout << "cadabra-client: ComputeThread going to execute " << dc.textbuf << std::endl;


	// Position the cursor in the next cell so this one will not 
	// accidentally get executed twice.
	std::shared_ptr<ActionBase> actionpos =
		std::make_shared<ActionPositionCursor>(it, ActionPositionCursor::Position::next);
	docthread->queue_action(actionpos);
	gui->process_data();
	
	// For a code cell, construct a server request message and then
	// send the cell to the server.
	if(it->cell_type==DataCell::CellType::python) {
		running_cells[dc.id()]=it;

		// Schedule an action to update the running status of this cell.
		std::shared_ptr<ActionBase> rs_action = 
			std::make_shared<ActionSetRunStatus>(it, true);
		docthread->queue_action(rs_action);

		Json::Value req, header, content;
		header["uuid"]="none";
		header["cell_id"]=(Json::UInt64)dc.id().id;
		if(dc.id().created_by_client)
			header["cell_origin"]="client";
		else
			header["cell_origin"]="server";
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
		DataCell result(DataCell::CellType::latex_view, it->textbuf);
		
		std::shared_ptr<ActionBase> action = 
			std::make_shared<ActionAddCell>(result, it, ActionAddCell::Position::child);
		docthread->queue_action(action);
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

	restarting_kernel=true;

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
