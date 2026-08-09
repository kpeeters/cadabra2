
#include <string>
#include <iostream>
#include <locale>
#include <sstream>
#include "ComputeThread.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"
#include "Actions.hh"
#include "InstallPrefix.hh"
#include "popen2.hh"
#include <sys/types.h>
#include <chrono>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>       // TerminateProcess, in kill_server()
#else
#include <sys/wait.h>      // waitpid, in kill_server()
#endif
#include <glibmm/spawn.h>
#include "internal/unistd.h"
#include "CdbPython.hh"

using namespace cadabra;

ComputeThread::ComputeThread(int server_port, std::string token, std::string ip_address)
	: gui(0), docthread(0), connection_is_open(false), restarting_kernel(false), server_pid(0),
	  server_stdout(0), server_stderr(0), forced_server_port(server_port), forced_server_token(token),
	  forced_server_ip_address(ip_address)
	{
	// The ComputeThread constructor (but _not_ the run() member!) is
	// always run on the gui thread, so we can grab the gui thread id
	// here.

	gui_thread_id=std::this_thread::get_id();
	}

ComputeThread::~ComputeThread()
	{
	kill_server();
	}

void ComputeThread::set_master(GUIBase *b, DocumentThread *d)
	{
	gui=b;
	docthread=d;
	}

void ComputeThread::init()
	{
	// Setup the WebSockets client.
	}

void ComputeThread::try_connect()
	{
	wsclient.set_connect_handler(std::bind(&ComputeThread::on_open, this));
	wsclient.set_fail_handler(std::bind(&ComputeThread::on_fail, this, std::placeholders::_1));
	wsclient.set_close_handler(std::bind(&ComputeThread::on_close, this));
	wsclient.set_message_handler(std::bind(&ComputeThread::on_message, this, std::placeholders::_1));

	std::ostringstream uristr;
	// Never let the global locale group the digits of a port number.
	uristr.imbue(std::locale::classic());
	uristr << "ws://" << (forced_server_ip_address.empty() ? "127.0.0.1" : forced_server_ip_address) << ":" << port;
	wsclient.connect(uristr.str());

	// std::cerr << "cadabra-client: connect done" << std::endl;
	}

void ComputeThread::run()
	{
	// This does *not* run on the GUI thread.
	
	init();
	try_spawn_server();
	try_connect();

	// Enter run loop, which will never terminate anymore. The on_fail and on_close
	// handlers will re-try to establish connections when they go bad.

	wsclient.run();
	}

void ComputeThread::kill_server()
	{
	if(server_stdout!=0) {
		close(server_stdout);
		// close(server_stderr);
		server_stdout=0;
		server_stderr=0;
		}

	// Zero means we never spawned anything -- e.g. --server-port was used and
	// the server belongs to someone else. Never touch that one.
	if(server_pid==0)
		return;

#if defined(_WIN32) || defined(_WIN64)
	TerminateProcess(reinterpret_cast<HANDLE>(server_pid), 1);
#else
	// The child was spawned with DO_NOT_REAP_CHILD and Glib::spawn_close_pid()
	// does not wait on Unix, so we have to reap it ourselves or leave a zombie
	// behind for every server we ever started.
	//
	// Ask politely first: a server that is still connected is already shutting
	// down (Server::run() defaults to exit_on_disconnect=true), and one that
	// never accepted a connection will not go away on its own. Insist only if
	// it has not gone within half a second.
	bool reaped=false;
	::kill(server_pid, SIGTERM);
	for(int i=0; i<50 && !reaped; ++i) {
		if(::waitpid(server_pid, nullptr, WNOHANG)==server_pid)
			reaped=true;
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	if(!reaped) {
		::kill(server_pid, SIGKILL);
		::waitpid(server_pid, nullptr, 0);
		}
#endif

	Glib::spawn_close_pid(server_pid);
	server_pid=0;
	}

void ComputeThread::terminate()
	{
	wsclient.stop();

	// If we have started the server ourselves, stop it now so we do
	// not leave mess behind.
	// http://riccomini.name/posts/linux/2012-09-25-kill-subprocesses-linux-bash/

	// A connected server is already on its way out (Server::run() defaults to
	// exit_on_disconnect=true and wsclient.stop() above closed the connection),
	// but one that never accepted a connection would linger forever, so make
	// sure of it either way.
	kill_server();
	}

bool ComputeThread::kernel_is_connected() const
	{
	return connection_is_open;
	}

void ComputeThread::all_cells_nonrunning()
	{
	for(auto it: running_cells) {
		std::shared_ptr<ActionBase> rs_action =
		   std::make_shared<ActionSetRunStatus>(it.first, false);
		docthread->queue_action(rs_action);
		}
	if(gui) {
		gui->process_data();
		gui->on_kernel_runstatus(false);
		}
	running_cells.clear();
	}

void ComputeThread::on_fail(const boost::beast::error_code& ec)
	{
	if(!restarting_kernel) {
		std::cerr << "cadabra-client: connection to server on port " << port << " failed, " << ec.message() << std::endl;
		}
	connection_is_open=false;
	all_cells_nonrunning();
	// Whatever server we started is not usable, and try_spawn_server() below
	// will start another; get rid of this one first or every retry leaks a
	// process. A no-op when the server is not ours (--server-port).
	kill_server();

	// Not during a kernel restart: losing the connection is expected there and
	// on_close() has already told the GUI what is going on.
	if(gui && !restarting_kernel)
		gui->on_network_error();

	try_spawn_server();
	try_connect();
	}

using SlotSpawnChildSetup = sigc::slot<void()>;

void ComputeThread::try_spawn_server()
	{
	// Startup the server. First generate a UUID, pass this to the
	// starting server, then use this UUID to get access to the server
	// port.

	// std::cerr << "cadabra-client: spawning server" << std::endl;

	if(forced_server_port!=0) {
		port=forced_server_port;
		authentication_token=forced_server_token;
		return;
		}

	std::vector<std::string> argv, envp;
#if defined(_WIN32) || defined(_WIN64)
	argv.push_back("cadabra-server.exe");
#else
	const char *appdir = getenv("APPDIR");
	if(appdir) {
		// std::cerr << "This is an AppImage, APPDIR = " << appdir << std::endl;
		argv.push_back(std::string(appdir)+"/usr/bin/cadabra-server");
		}
	else {
		// std::cerr << "Not an AppImage." << std::endl;
		argv.push_back("cadabra-server");
		}
#endif
	std::string wd("");

	// See https://bugs.launchpad.net/inkscape/+bug/1662531 for things related to
	// the 'envp' argument in the call below.
	try {
		// Note: 'DO_NOT_REAP_CHILD' is required on all platforms, because we
		// call Glib::spawn_close_pid() on the pid ourselves (see terminate()).
#if GLIBMM_MAJOR_VERSION > 2 || (GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION >= 68)
  #ifdef _WIN32
		Glib::SpawnFlags flags = Glib::SpawnFlags::DO_NOT_REAP_CHILD | Glib::SpawnFlags::SEARCH_PATH
			| Glib::SpawnFlags::STDERR_TO_DEV_NULL;
  #else
		Glib::SpawnFlags flags = Glib::SpawnFlags::DO_NOT_REAP_CHILD | Glib::SpawnFlags::SEARCH_PATH;
  #endif
#else
  #ifdef _WIN32
		Glib::SpawnFlags flags = Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH
			| Glib::SPAWN_STDERR_TO_DEV_NULL;
  #else
		Glib::SpawnFlags flags = Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH;
  #endif
#endif

		Glib::spawn_async_with_pipes(wd, argv, /* envp, WITH envp, Fedora 27 fails to start python properly */
											  flags,
		                             sigc::slot<void()>(),
		                             &server_pid,
		                             0,
		                             &server_stdout,
		                             0); // We need to see stderr on the console
		//										  &server_stderr);

		char buffer[100];
		FILE *f = fdopen(server_stdout, "r");
		if(fscanf(f, "%99s", buffer)!=1) {
			throw std::logic_error("Failed to read port from server.");
			}
		port = atoi(buffer);
		if(fscanf(f, "%99s", buffer)!=1) {
			throw std::logic_error("Failed to read authentication token from server.");
			}
		authentication_token=std::string(buffer);
		// std::cerr << "auth token: " << authentication_token << std::endl;
		}
	catch(Glib::SpawnError& err) {
		std::cerr << "Failed to start server " << argv[0] << ": " << err.what() << std::endl;
		// FIXME: cannot just fall through, the server is not up!
		}
	}

void ComputeThread::on_open()
	{
	connection_is_open=true;
	restarting_kernel=false;
	if(gui) {
		gui->on_connect();
		gui->on_kernel_runstatus(false);
		}

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

void ComputeThread::on_close()
	{
	// std::cerr << "cadabra-client: connection closed" << std::endl;
	connection_is_open=false;
	all_cells_nonrunning();
	if(gui) {
		if(restarting_kernel) {
			gui->on_disconnect("restarting kernel");
			gui->on_kernel_runstatus(true);
			}
		else {
			gui->on_disconnect("not connected");
			}
		}

	sleep(1); // do not cause a torrent...
	try_connect();
	}

void ComputeThread::cell_finished_running(DataCell::id_t id)
	{
	if(id.id==0) {
		// This was code without a cell representation (run in
		// response to e.g. a slider update). Ignore.
		}
	else {
		auto it=running_cells.find(id);
		if(it==running_cells.end()) {
			throw std::logic_error("Cannot find cell with id = "+std::to_string(id.id));
			}
		
		if(it->second==1) {
			// Mark this cell as no longer running.
			std::shared_ptr<ActionBase> rs_action =
				std::make_shared<ActionSetRunStatus>(id, false);
			docthread->queue_action(rs_action);

			running_cells.erase(it);
			}
		else
			it->second -= 1;
		}
	}

void ComputeThread::on_message(const std::string& msg)
	{
	// Parse the JSON message.
	nlohmann::json root;

	try {
		root=nlohmann::json::parse(msg);
		}
	catch(nlohmann::json::exception& e) {
		std::cerr << "cadabra-client: cannot parse message." << std::endl;
		return;
		}
	if(getenv("CADABRA_SHOW_RECEIVED")) {
		std::cerr << "RECV: " << root.dump(3) << std::endl;
		}
	
	if(root.count("header")==0) {
		std::cerr << "cadabra-client: received message without 'header'." << std::endl;
		return;
		}
	if(root.count("content")==0) {
		std::cerr << "cadabra-client: received message without 'content'." << std::endl;
		return;
		}
	const nlohmann::json& header  = root["header"];
	const nlohmann::json& content = root["content"];
	const std::string msg_type    = root.value("msg_type", "");

	DataCell::id_t parent_id;
	parent_id.id = header.value("parent_id", uint64_t(0));
	if(header.value("parent_origin", "")=="client")
		parent_id.created_by_client=true;
	else
		parent_id.created_by_client=false;
	DataCell::id_t cell_id;
	cell_id.id = header["cell_id"].get<uint64_t>();
	if(header.value("cell_origin", "")=="client")
		cell_id.created_by_client=true;
	else
		cell_id.created_by_client=false;
	// std::cerr << "received cell with id " << cell_id.id << std::endl;

	// Determine if this refers to a special cell in the interactive console.
	if (interactive_cells.find(parent_id.id) != interactive_cells.end()) {
		interactive_cells.insert(cell_id.id);
		docthread->on_interactive_output(root);
	}
	else if (interactive_cells.find(cell_id.id) != interactive_cells.end()) {
		docthread->on_interactive_output(root);
		}
	else if (msg_type.find("csl_") == 0) {
		root["header"]["from_server"] = true;
		docthread->on_interactive_output(root);
		}
	else if (msg_type == "progress_update") {
		std::string msg = content.value<std::string>("msg", "Idle");
		int n = content.value<int>("n", 0);
		int total = content.value<int>("total", 0);
		// FIXME: do something with 'pulse':
		// int pulse = content.value<bool>("pulse", false);
		docthread->set_progress(msg, n, total);
	}
	else if(msg_type=="completed") {
		// std::cerr << "received completion of " << content["original"] << " -> " << content["completed"] << std::endl;

		// Finally, the action to add the output cell.
		std::string toadd=content["completed"].get<std::string>();
		if(toadd.size()>0) {
			toadd=toadd.substr(content["original"].get<std::string>().size());
			int pos=content["position"].get<int>();
			int alternative=content["alternative"].get<int>();
			std::shared_ptr<ActionBase> action =
				std::make_shared<ActionCompleteText>(cell_id, pos, toadd, alternative);
			docthread->queue_action(action);
			}
		}
	else {
		try {
			bool finished = header["last_in_sequence"].get<bool>();

			if (finished) {
				if(parent_id.id!=0) {
					// If this cell references variables, store them in the DataCell.
					if(content.count("variables")>0) {
						std::set<std::string> vars;
						for(const auto& var: content["variables"])
							vars.insert(var);
						
						std::shared_ptr<ActionBase> action =
							std::make_shared<ActionSetVariableList>(parent_id, vars);
						docthread->queue_action(action);
						}
					}
				cell_finished_running(parent_id);
				}

			if (content.count("output")>0 && content["output"].get<std::string>().size() > 0) {
				if (msg_type == "output") {
					std::string output = "\\begin{verbatim}" + content["output"].get<std::string>() + "\\end{verbatim}";

					// Stick an AddCell action onto the stack. We instruct the
					// action to add this result output cell as a child of the
					// corresponding input cell.
					DataCell result(cell_id, DataCell::CellType::output, output);

					// Finally, the action to add the output cell.
					std::shared_ptr<ActionBase> action =
					   std::make_shared<ActionAddCell>(result, parent_id, ActionAddCell::Position::child);
					docthread->queue_action(action);
					}
				else if (msg_type == "verbatim") {
					std::string output = "\\begin{verbatim}" + content["output"].get<std::string>() + "\\end{verbatim}";

					// Stick an AddCell action onto the stack. We instruct the
					// action to add this result output cell as a child of the
					// corresponding input cell.
					DataCell result(cell_id, DataCell::CellType::verbatim, output);

					// Finally, the action to add the output cell.
					std::shared_ptr<ActionBase> action =
					   std::make_shared<ActionAddCell>(result, parent_id, ActionAddCell::Position::child);
					docthread->queue_action(action);
					}
				else if (msg_type == "latex_view") {
					// std::cerr << "received latex cell " << content["output"].asString() << std::endl;
					DataCell result(cell_id, DataCell::CellType::latex_view, content["output"].get<std::string>());
					std::shared_ptr<ActionBase> action =
					   std::make_shared<ActionAddCell>(result, parent_id, ActionAddCell::Position::child);
					docthread->queue_action(action);
					}
				else if (msg_type == "input_form") {
					DataCell result(cell_id, DataCell::CellType::input_form, content["output"].get<std::string>());
					std::shared_ptr<ActionBase> action =
					   std::make_shared<ActionAddCell>(result, parent_id, ActionAddCell::Position::child);
					docthread->queue_action(action);
					}
				else if (msg_type == "error" || msg_type=="fault") {
					std::string error = "\\begin{verbatim}" + content["output"].get<std::string>()
					                    + "\\end{verbatim}";
					//if (msg_type == "fault") {
					//	error = "Kernel fault\\begin{small}" + error + "\\end{small}";
					//	}

					// Stick an AddCell action onto the stack. We instruct the
					// action to add this result output cell as a child of the
					// corresponding input cell.
					DataCell result(cell_id, DataCell::CellType::error, error);

					// Finally, the action.
					std::shared_ptr<ActionBase> action =
					   std::make_shared<ActionAddCell>(result, parent_id, ActionAddCell::Position::child);
					docthread->queue_action(action);

					// Position the cursor in the cell that generated the error. All other cells on
					// the execute queue have been cancelled by the server.
					std::shared_ptr<ActionBase> actionpos =
					   std::make_shared<ActionPositionCursor>(parent_id, ActionPositionCursor::Position::in);
					docthread->queue_action(actionpos);

					// Action has stopped, so mark all cells as non-running.
					all_cells_nonrunning();
					}
				else if (msg_type == "image_png") {
					DataCell result(cell_id, DataCell::CellType::image_png, content["output"].get<std::string>());
					std::shared_ptr<ActionBase> action =
					   std::make_shared<ActionAddCell>(result, parent_id, ActionAddCell::Position::child);
					docthread->queue_action(action);
					}
				else if (msg_type == "image_svg") {
					DataCell result(cell_id, DataCell::CellType::image_svg, content["output"].get<std::string>());
					std::shared_ptr<ActionBase> action =
					   std::make_shared<ActionAddCell>(result, parent_id, ActionAddCell::Position::child);
					docthread->queue_action(action);
					}
				else if (msg_type == "slider") {
					DataCell result(cell_id, DataCell::CellType::slider, content["output"].get<std::string>());
					std::shared_ptr<ActionBase> action =
					   std::make_shared<ActionAddCell>(result, parent_id, ActionAddCell::Position::child);
					docthread->queue_action(action);
					}
				else {
					std::cerr << "cadabra-client: received cell we did not expect: "
					          << msg_type << ": " << content << std::endl;
					}
				}
			}
		catch (std::logic_error& ex) {
			// WARNING: if the server sends
			std::cerr << "cadabra-client: trouble processing server response: " << ex.what() << std::endl;
			}
		}

	// Update kernel busy indicator depending on number of running cells.
	if(number_of_cells_executing()>0)
		gui->on_kernel_runstatus(true);
	else
		gui->on_kernel_runstatus(false);

	gui->process_data();
	}

void ComputeThread::execute_interactive(uint64_t id, const std::string& code)
	{
	assert(gui_thread_id == std::this_thread::get_id());

	if (!connection_is_open)
		return;

	if (code.substr(0, 7) == "reset()")
		return restart_kernel();

	nlohmann::json req, header, content;

	header["msg_type"]    = "execute_request";
	header["cell_id"]     = id;
	header["interactive"] = true;
	content["code"]       = code.c_str();

	req["auth_token"] = authentication_token;
	req["header"]     = header;
	req["content"]    = content;

	std::ostringstream oss;
	oss << req << std::endl;
	if(getenv("CADABRA_SHOW_SENT")) {
		std::cerr << "SENT: " << req.dump(3) << std::endl;
		}
	wsclient.send(oss.str());
	interactive_cells.insert(id);
	}

void ComputeThread::execute_cell(DTree::iterator it, std::string no_assign, std::vector<uint64_t> output_cell_ids)
	{
	// This absolutely has to be run on the main GUI thread.
	assert(gui_thread_id==std::this_thread::get_id());

	if(connection_is_open==false)
		return;

	const DataCell& dc=(*it);

	// std::cout << "cadabra-client: ComputeThread going to execute " << dc.textbuf << std::endl;

	if((it->textbuf).substr(0,7)=="reset()") {
		restart_kernel();

		std::shared_ptr<ActionBase> action =
		   std::make_shared<ActionPositionCursor>(it->id(), ActionPositionCursor::Position::next);

		docthread->queue_action(action);
		return;
		}

	// Position the cursor in the next cell so this one will not
	// accidentally get executed twice. This runs synchronously!
	std::shared_ptr<ActionBase> actionpos =
	   std::make_shared<ActionPositionCursor>(it->id(), ActionPositionCursor::Position::next);
	docthread->queue_action(actionpos);
	gui->process_data();

	// For a code cell, construct a server request message and then
	// send the cell to the server.
	if(it->cell_type==DataCell::CellType::python) {
		auto rit=running_cells.find(dc.id());
		if(rit==running_cells.end())
			running_cells[dc.id()]=1;
		else
			rit->second += 1;

		// Schedule an action to update the running status of this cell.
		std::shared_ptr<ActionBase> rs_action =
		   std::make_shared<ActionSetRunStatus>(it->id(), true);
		docthread->queue_action(rs_action);

		nlohmann::json req, header, content;
		header["uuid"]="none";
		header["cell_id"]=dc.id().id;
		if(dc.id().created_by_client)
			header["cell_origin"]="client";
		else
			header["cell_origin"]="server";
		header["msg_type"]="execute_request";
		header["output_cell_ids"]=output_cell_ids;
		req["auth_token"]=authentication_token;
		req["header"]=header;
		content["remove_variable_assignments"]=no_assign;
		content["code"]=dc.textbuf;
		req["content"]=content;

		gui->on_kernel_runstatus(true);
		std::ostringstream str;
		str << req << std::endl;
		if(getenv("CADABRA_SHOW_SENT")) {
			std::cerr << "SENT: " << req.dump(3) << std::endl;
			}
		wsclient.send(str.str());
		// NOTE: we can get a return message in on_message at any point after this,
		// it will come in on a different thread!
		}

	// For a LaTeX cell, immediately request a new latex output cell to be displayed.
	if(it->cell_type==DataCell::CellType::latex) {
		// Stick an AddCell action onto the stack. We instruct the
		// action to add this result output cell as a child of the
		// corresponding input cell.
		DataCell result(DataCell::CellType::latex_view, it->textbuf);

		std::shared_ptr<ActionBase> action =
		   std::make_shared<ActionAddCell>(result, it->id(), ActionAddCell::Position::child);
		docthread->queue_action(action);
		}
	}

void ComputeThread::update_variable_on_server(std::string variable, double value)
	{
	nlohmann::json req, header, content;
	header["uuid"]="none";
	header["cell_id"]=0;
	header["cell_origin"]="client";
	header["msg_type"]="execute_request";
	req["auth_token"]=authentication_token;
	req["header"]=header;
	content["code"]=variable + "=" + std::to_string(value);
	req["content"]=content;
	
	std::ostringstream str;
	str << req << std::endl;
	if(getenv("CADABRA_SHOW_SENT")) {
		std::cerr << "SENT: " << req.dump(3) << std::endl;
		}
	wsclient.send(str.str());
	}


int ComputeThread::number_of_cells_executing() const
	{
	return running_cells.size();
	}

void ComputeThread::stop()
	{
	if(connection_is_open==false)
		return;

	// std::cerr << "stopping existing kernel" << std::endl;
	
	nlohmann::json req, header, content;
	header["uuid"]="none";
	header["msg_type"]="execute_interrupt";
	req["auth_token"]=authentication_token;
	req["header"]=header;

	std::ostringstream str;
	str << req << std::endl;

	//	std::cerr << str.str() << std::endl;

	if(getenv("CADABRA_SHOW_SENT")) {
		std::cerr << "SENT: " << req.dump(3) << std::endl;
		}
	wsclient.send(str.str());
	// Do not yet mark cells non-running, otherwise we are unable to
	// process any error messages. Do this once the stop comes through.
	// all_cells_nonrunning();	
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

	// std::cerr << "cadabra-client: restarting kernel" << std::endl;
	nlohmann::json req, header, content;
	header["uuid"]="none";
	header["msg_type"]="exit";
	header["from_server"] = true;
	req["auth_token"]=authentication_token;
	req["header"]=header;

	std::ostringstream str;
	str << req << std::endl;

	//	std::cerr << str.str() << std::endl;

	if(getenv("CADABRA_SHOW_SENT")) {
		std::cerr << "SENT: " << req.dump(3) << std::endl;
		}
	wsclient.send(str.str());
	docthread->on_interactive_output(req);
	}

bool ComputeThread::complete(DTree::iterator it, int pos, int alternative)
	{
	if(connection_is_open==false)
		return false;

	const DataCell& dc=(*it);

	nlohmann::json req, header, content;
	header["uuid"]="none";
	header["cell_id"]=dc.id().id;
	if(dc.id().created_by_client)
		header["cell_origin"]="client";
	else
		header["cell_origin"]="server";
	header["msg_type"]="complete";
	req["auth_token"]=authentication_token;
	req["header"]=header;
	std::string todo = it->textbuf.substr(0,pos);
//	if(todo.size()>0 && todo[todo.size()-1]=='\n')
//		todo=todo.substr(0, todo.size()-1);
	// std::cerr << "to complete full: " << todo << std::endl;
	size_t lst=todo.find_last_of("\n(){}[]:\t ");
	if(lst!=std::string::npos)
		todo=todo.substr(lst+1);
	// std::cerr << "to complete strip: " << todo << std::endl;

	if(todo.size()==0)
		return false;

	req["string"]=todo;
	req["position"]=pos;
	req["alternative"]=alternative;

	std::ostringstream str;
	str << req << std::endl;

	// std::cerr << str.str() << std::endl;

	if(getenv("CADABRA_SHOW_SENT")) {
		std::cerr << "SENT: " << req.dump(3) << std::endl;
		}
	wsclient.send(str.str());

	return true;
	}
