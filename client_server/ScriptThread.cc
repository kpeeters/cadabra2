
#include "ScriptThread.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"
#include "Actions.hh"

#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/algorithm/string/replace.hpp>

#include <internal/uuid.h>
#include <internal/string_tools.h>

#include "nlohmann/json.hpp"

using namespace cadabra;

ScriptThread::ScriptThread(DocumentThread *d, GUIBase *g)
	: document(d), gui(g), local_port(0)
	{
	// Lock the URL (port and auth token) until the thread is
	// properly spun up.
	url_mutex.lock();
	
	boost::uuids::uuid authentication_uuid = boost::uuids::random_generator()();
	authentication_token = boost::uuids::to_string( authentication_uuid );
	}

ScriptThread::~ScriptThread()
	{
	url_mutex.unlock();
	}

void ScriptThread::on_open(websocket_server::id_type ws_id)
	{
	std::cerr << "on_open" << std::endl;

//	std::lock_guard<std::mutex> lock(ws_mutex);
//	Connection con;
//	con.ws_id = ws_id;
//	// snoop::log(snoop::info) << "Connection " << con.uuid << " open." << snoop::flush;
//	connections[ws_id]=con;
	}

void ScriptThread::on_close(websocket_server::id_type ws_id)
	{
	std::cerr << "on_close" << std::endl;

//	std::lock_guard<std::mutex> lock(ws_mutex);
//	//	auto it = connections.find(hdl);
//	// snoop::log(snoop::info) << "Connection " << it->second.uuid << " close." << snoop::flush;
//	connections.erase(ws_id);
//
//	if(exit_on_disconnect)
//		exit(-1);
	}

void ScriptThread::on_message(websocket_server::id_type ws_id, const std::string& msg,
										const websocket_server::request_type& req, const std::string& ip_address)
	{
	// std::cerr << "received: " << msg << std::endl;
	if(req.target().substr(1) != authentication_token) {
		// Unauthorised.
		return;
		}

	try {
		auto jmsg = nlohmann::json::parse(msg);
		std::cerr << "received message: " << jmsg.dump(3) << std::endl;
		std::string msg_action = jmsg.value("action", "");
		size_t      msg_serial = jmsg.value("serial", 0);
		if(msg_action=="run_all_cells") {
			// We cannot call directly into the document methods here,
			// because we are not on the main thread. So we queue an
			// action, to be dispatched later.
			
			std::shared_ptr<ActionBase> action = std::make_shared<ActionRunCell>();
			action->callback = [this, ws_id, msg_serial, msg_action]() {
				nlohmann::json msg;
				msg["status"]="completed";
				msg["serial"]=msg_serial;
				msg["action"]=msg_action;
				wserver.send(ws_id, msg.dump());
				};
			document->queue_action(action);
			gui->process_data();
			}
		else if(msg_action=="open") {
			std::string notebook = jmsg.value("notebook", "");
			
			std::shared_ptr<ActionBase> action = std::make_shared<ActionOpen>(notebook);
			action->callback = [this, ws_id, msg_serial, msg_action]() {
				nlohmann::json msg;
				msg["status"]="completed";
				msg["serial"]=msg_serial;
				msg["action"]=msg_action;
				wserver.send(ws_id, msg.dump());
				};
			document->queue_action(action);
			gui->process_data();
			}
		else if(msg_action=="add_cell") {
			std::string content = jmsg.value("content", "");

			DataCell dc(DataCell::CellType::python, content);
			DataCell::id_t ref_id;
			ref_id.id=0; // relative to current cell
			
			std::shared_ptr<ActionAddCell> action =
				std::make_shared<ActionAddCell>(dc, ref_id, ActionAddCell::Position::after);
			
			action->callback = [this, ws_id, msg_serial, msg_action]() {
				nlohmann::json msg;
				msg["status"]="completed";
				msg["serial"]=msg_serial;
				msg["action"]=msg_action;
				wserver.send(ws_id, msg.dump());
				};
			document->queue_action(action);
			gui->process_data();
			}
		}
	catch(nlohmann::json::exception& ex) {
		std::cerr << "Received unparsable message: " << msg << std::endl;
		}
	}

void ScriptThread::run()
	{
	wserver.set_message_handler(std::bind(&ScriptThread::on_message, this,
													  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	wserver.set_connect_handler(std::bind(&ScriptThread::on_open, this,     std::placeholders::_1));
	wserver.set_disconnect_handler(std::bind(&ScriptThread::on_close, this, std::placeholders::_1));
	
	wserver.listen(0); 
	local_port = wserver.get_local_port();
	
	url_mutex.unlock();
	wserver.run();
	}

uint16_t ScriptThread::get_local_port() const
	{
	std::lock_guard<std::mutex> guard(url_mutex);
	return local_port;
	}

std::string ScriptThread::get_authentication_token() const
	{
	std::lock_guard<std::mutex> guard(url_mutex);
	return authentication_token;
	}

void ScriptThread::terminate()
	{
	wserver.stop();
	}
