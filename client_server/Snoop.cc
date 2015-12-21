
#include "Snoop.hh"
#include "SnoopPrivate.hh"

#include <iostream>
#include <string.h>
#include <regex>
#include <iostream>
#include <uuid/uuid.h>
#include <chrono>
#include <ctime>
#include <sys/utsname.h>
#include <stdint.h>

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace snoop;

// Global instance.

snoop::Snoop snoop::log;
snoop::Flush snoop::flush;

using u64_millis = std::chrono::duration<uint64_t, std::milli>;
static std::chrono::time_point<std::chrono::system_clock, u64_millis> u64_to_time(uint64_t timestamp) {
    return std::chrono::time_point<std::chrono::system_clock, u64_millis>{u64_millis{timestamp}};
}

std::string safestring(const unsigned char *c)
	{
	if(c==0) return "";
	else     return std::string((const char *)c);
	}

Snoop::Snoop()
	: sync_immediately_(false)
	{
	impl=new SnoopImpl(this);
	}

SnoopImpl::SnoopImpl(Snoop *s)
	: snoop_(s), db(0), insert_statement(0), id_for_uuid_statement(0), connection_is_open(false)
   {
   }

void Snoop::init(const std::string& app_name, const std::string& app_version, const std::string& dbname, std::string server)
   {
	impl->init(app_name, app_version, dbname, server);
	}

void SnoopImpl::init(const std::string& app_name, const std::string& app_version, const std::string& dbname, std::string server)
   {
	assert(app_name.size()>0);
	

	if(db==0) { // Only initialise if database has not been opened before
		this_app_.app_name=app_name;
		this_app_.app_version=app_version;
		this_app_.pid = getpid();
		struct utsname buf;
		if(uname(&buf)==0) {
#ifdef __APPLE__
			this_app_.machine_id = std::string(buf.sysname)
				+", "+buf.nodename+", "+buf.release+", "+buf.version+", "+buf.machine;
#else
			this_app_.machine_id = std::string(buf.sysname)
				+", "+buf.nodename+", "+buf.release+", "+buf.version+", "+buf.machine+", "+buf.domainname;
#endif
			}

		auto duration =  std::chrono::high_resolution_clock::now().time_since_epoch();
		this_app_.create_millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		
		server_=server;

		int ret = sqlite3_open_v2(dbname.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if(ret) {
			throw std::logic_error("Cannot open database");
			}
		create_tables();

		// Turn off synchronous writes as they seriously degrade performance (by orders of magnitude)
		// for our single-row inserts. See
		// http://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite?rq=1
		// for more options to speed things up.

		sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, NULL);

		// If this is a client, i.e. not a SnoopServer: obtain a uuid, start the websocket listener,
		// and sync with the remote server whatever has not yet been synced in previous runs.
		
		if(this_app_.app_name!="SnoopServer") {
			obtain_uuid();
			start_websocket_client();
			// Once started, the websocket client thread will call a sync
			// in the on_client_open callback, and then set the
			// connection_is_open flag.
			}
		}
   }

void Snoop::set_sync_immediately(bool s)
	{
	sync_immediately_=s;
	}

void SnoopImpl::create_tables()
	{
	assert(db!=0);

	std::lock_guard<std::mutex> lock(sqlite_mutex);

	char *errmsg;
	if(sqlite3_exec(db, "create table if not exists apps ("
						 "id              integer primary key autoincrement,"
						 "uuid            text,"
						 "create_millis   unsigned big int,"
						 "receive_millis  unsigned big int,"
						 "pid             int,"
						 "ip_address      text,"
						 "machine_id      text,"
						 "app_name        text,"
						 "app_version     text,"
						 "server_status   int);"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create table apps");
		}
	if(sqlite3_exec(db, "create table if not exists logs ("
						 "log_id          integer primary key autoincrement,"
						 "client_log_id   integer,"
						 "id              integer references login,"
						 "create_millis   unsigned big int,"
						 "receive_millis  unsigned big int,"
						 "loc_file        text,"
						 "loc_line        integer,"
						 "loc_method      text,"
						 "type            text,"
						 "message         text,"
						 "server_status   int);"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create table logs");
		}
	}

void SnoopImpl::obtain_uuid()
	{
	assert(db!=0);

	std::lock_guard<std::mutex> lock(sqlite_mutex);

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select uuid from apps where pid=" << getpid() << " order by create_millis desc limit 1";
	
	int res = sqlite3_prepare(db, ss.str().c_str(), -1, &statement, NULL);
	if(res==SQLITE_OK) {
		int ret = sqlite3_step(statement);
		if(ret==SQLITE_ROW) {
			if(sqlite3_column_type(statement, 0)==SQLITE3_TEXT) 
				this_app_.uuid=safestring(sqlite3_column_text(statement, 0));
			else throw std::logic_error("Database inconsistency for obtain_uuid");
			}
		}
	sqlite3_finalize(statement);

	// Generate and insert a new uuid if there is no existing entry for the current pid.

	if(this_app_.uuid.size()==0) {
		uuid_t nid;
		uuid_generate(nid);
		char *uuid_string = new char[100];
		uuid_unparse(nid, uuid_string);
		this_app_.uuid=uuid_string;

		store_app_entry_without_lock(this_app_);
		}
	}

bool SnoopImpl::store_app_entry(Snoop::AppEntry& app)
	{
	assert(db!=0);

	std::lock_guard<std::mutex> lock(sqlite_mutex);

	return store_app_entry_without_lock(app);
	}

bool SnoopImpl::store_app_entry_without_lock(Snoop::AppEntry& app)
	{
	sqlite3_stmt *statement=0;
	int res = sqlite3_prepare(db, "insert into apps (uuid, create_millis, receive_millis, pid, ip_address, machine_id, "
									  "app_name, app_version, server_status) "
									  "values "
									  "(?, ?, ?, ?, ?, ?, ?, ?, ?)", 
									  -1, &statement, NULL);
	
	if(res==SQLITE_OK) {
		sqlite3_bind_text(statement,   1, app.uuid.c_str(), app.uuid.size(), 0);
		sqlite3_bind_int64(statement,  2, app.create_millis);
		sqlite3_bind_int64(statement,  3, app.receive_millis);
		sqlite3_bind_int64(statement,  4, app.pid);
		sqlite3_bind_text(statement,   5, app.ip_address.c_str(), app.ip_address.size(), 0);
		sqlite3_bind_text(statement,   6, app.machine_id.c_str(), app.machine_id.size(), 0);
		sqlite3_bind_text(statement,   7, app.app_name.c_str(), app.app_name.size(), 0);
		sqlite3_bind_text(statement,   8, app.app_version.c_str(), app.app_version.size(), 0);
		sqlite3_bind_int(statement,    9, app.server_status);
		
		sqlite3_step(statement);
		sqlite3_finalize(statement);

		app.id = sqlite3_last_insert_rowid(db);
		}
	else {
		throw std::logic_error("Failed to prepare insertion");
		}

	return false;
	}

void SnoopImpl::store_log_entry(Snoop::LogEntry& log_entry)
	{
	assert(db!=0);
	
	std::lock_guard<std::mutex> lock(sqlite_mutex);

	auto duration =  std::chrono::high_resolution_clock::now().time_since_epoch();
	log_entry.receive_millis   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

	int res=SQLITE_OK;
	if(insert_statement==0) {
		res=sqlite3_prepare_v2(db, "insert into logs "
									  "(client_log_id, id, create_millis, receive_millis, loc_file, loc_line, loc_method, "
									  " type, message, server_status) "
									  "values "
									  "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
									  -1, &insert_statement, NULL);
		}

	if(res==SQLITE_OK ) {
		sqlite3_bind_int(insert_statement,    1, log_entry.client_log_id);
		sqlite3_bind_int(insert_statement,    2, log_entry.id);
		sqlite3_bind_int64(insert_statement,  3, log_entry.create_millis);
		sqlite3_bind_int64(insert_statement,  4, log_entry.receive_millis);
		sqlite3_bind_text(insert_statement,   5, log_entry.loc_file.c_str(), log_entry.loc_file.size(), 0);
		sqlite3_bind_int(insert_statement,    6, log_entry.loc_line);
		sqlite3_bind_text(insert_statement,   7, log_entry.loc_method.c_str(), log_entry.loc_method.size(), 0);
		sqlite3_bind_text(insert_statement,   8, log_entry.type.c_str(),    log_entry.type.size(), 0);
		sqlite3_bind_text(insert_statement,   9, log_entry.message.c_str(), log_entry.message.size(), 0);
		sqlite3_bind_int(insert_statement,   10, log_entry.server_status);
		 
		sqlite3_step(insert_statement);

		log_entry.log_id = sqlite3_last_insert_rowid(db);

		sqlite3_reset(insert_statement);
		}
	else {
		throw std::logic_error("Failed to insert log entry");
		}
	}

void SnoopImpl::start_websocket_client()
	{
	using websocketpp::lib::bind;

	wsclient.clear_access_channels(websocketpp::log::alevel::all);
	wsclient.clear_error_channels(websocketpp::log::elevel::all);

	wsclient.set_open_handler(bind(&SnoopImpl::on_client_open, this, websocketpp::lib::placeholders::_1));
	wsclient.set_fail_handler(bind(&SnoopImpl::on_client_fail, this, websocketpp::lib::placeholders::_1));
	wsclient.set_close_handler(bind(&SnoopImpl::on_client_close, this, websocketpp::lib::placeholders::_1));
	wsclient.set_message_handler(bind(&SnoopImpl::on_client_message, this, 
												 websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
	wsclient.init_asio();
	wsclient.start_perpetual();

	std::string uri = "ws://"+server_;
	websocketpp::lib::error_code ec;
	connection = wsclient.get_connection(uri, ec);
	if (ec) {
		// std::cerr << "Snoop: websocket connection error " << ec.message() << std::endl;
		return;
		}

	our_connection_hdl = connection->get_handle();
	wsclient.connect(connection);
	// need to start client immediately now
	wsclient_thread=std::thread([this]{ wsclient.run(); });
	}

void Snoop::sync_with_server(bool from_wsthread)
	{
	impl->sync_with_server(from_wsthread);
	}

void SnoopImpl::sync_with_server(bool from_wsthread)
	{
	assert(server_.size()>0);

	if(!from_wsthread)
		if(!connection_is_open) 
			return;

	sync_apps_with_server(from_wsthread);
	sync_logs_with_server(from_wsthread);
	}

void Snoop::sync_apps_with_server(bool from_wsthread)
	{
	impl->sync_apps_with_server(from_wsthread);
	}

void SnoopImpl::sync_apps_with_server(bool from_wsthread)
	{
	if(!from_wsthread)
		if(!connection_is_open) 
			return;

	std::lock_guard<std::mutex> lock(sqlite_mutex);

	// Create a JSON text with all locally stored entries which have
	// a server_status field negative or zero.

	sqlite3_stmt *statement=0;
	std::ostringstream ssc;
	std::ostringstream pack;
	pack << "{ \"app\": [";
	ssc << "select id, uuid, create_millis, receive_millis, pid, ip_address, machine_id, app_name, app_version, server_status "
		 << "from apps where server_status=0";

	int sres = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
	if(sres==SQLITE_OK) {
		bool first=true;
		bool go=true;
		while(go) {
			int ret = sqlite3_step(statement);
			switch(ret) {
				case SQLITE_BUSY: 
					throw std::logic_error("Unexpected SQLITE_BUSY in sync_apps_with_server");
					break;
				case SQLITE_ROW: {
					Snoop::AppEntry le;
					// FIXME: isolate this in a separate function so we can fetch individual records more easily
					le.id               = sqlite3_column_int(statement, 0);
					le.uuid             = safestring(sqlite3_column_text(statement, 1));
					le.create_millis    = sqlite3_column_int64(statement, 2);
					le.receive_millis   = sqlite3_column_int64(statement, 3);
					le.pid              = sqlite3_column_int(statement, 4);
					le.ip_address       = safestring(sqlite3_column_text(statement, 5));
					le.machine_id       = safestring(sqlite3_column_text(statement, 6));
					le.app_name         = safestring(sqlite3_column_text(statement, 7));
					le.app_version      = safestring(sqlite3_column_text(statement, 8));
					le.server_status    = sqlite3_column_int(statement, 9);
					if(!first) pack << ", \n";
					else       first=false;
					pack << le.to_json(false);
					break;
					}
				case SQLITE_DONE: {
					go=false;
					break;
					}
				}
			}
		}
	pack << "] }";
	sqlite3_finalize(statement);

	// Before we upload, decrease the server_status flag of all these
	// rows to indicate that we have started an attempt to get the info
	// to the server.

	ssc.str("");
	ssc << "update apps set server_status=server_status-1 where server_status=0";
	sres = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
	if(sres==SQLITE_OK) {
		sqlite3_step(statement);
		sqlite3_finalize(statement);
		}
	else {
		sqlite3_finalize(statement);
		return;
		}

	// Upload to the server.
	wsclient.send(our_connection_hdl, pack.str(), websocketpp::frame::opcode::text);
	}

void Snoop::sync_logs_with_server(bool from_wsthread)
	{
	impl->sync_logs_with_server(from_wsthread);
	}

void SnoopImpl::sync_logs_with_server(bool from_wsthread)
	{
	if(!from_wsthread)
		if(!connection_is_open) 
			return;

	std::lock_guard<std::mutex> lock(sqlite_mutex);

	// Create a JSON text with all locally stored entries which have
	// a server_status field negative or zero.

	sqlite3_stmt *statement=0;
	std::ostringstream ssc;
	std::ostringstream pack;
	pack << "{ \"log\": [";
	ssc << "select log_id, client_log_id, id, create_millis, loc_file, loc_line, loc_method, type, message, server_status "
		 << "from logs where server_status=0";

	int sres = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
	if(sres==SQLITE_OK) {
		bool first=true;
		bool go=true;
		while(go) {
			int ret = sqlite3_step(statement);
			switch(ret) {
				case SQLITE_BUSY: 
					throw std::logic_error("Unexpected SQLITE_BUSY in sync_apps_with_server");
					break;
				case SQLITE_ROW: {
					Snoop::LogEntry le;
					le.log_id           = sqlite3_column_int(statement, 0);
					le.client_log_id    = sqlite3_column_int(statement, 1);
					le.id               = sqlite3_column_int(statement, 2);
					le.uuid             = this_app_.uuid;
					le.create_millis    = sqlite3_column_int64(statement, 3);
					le.loc_file         = safestring(sqlite3_column_text(statement, 4));
					le.loc_line         = sqlite3_column_int(statement, 5);
					le.loc_method       = safestring(sqlite3_column_text(statement, 6));
					le.type             = safestring(sqlite3_column_text(statement, 7));
					le.message          = safestring(sqlite3_column_text(statement, 8));
					le.server_status    = sqlite3_column_int(statement, 9);
					if(!first) pack << ", \n";
					else       first=false;
					pack << le.to_json(false);
					break;
					}
				case SQLITE_DONE: {
					go=false;
					break;
					}
				}
			}
		}
	pack << "] }";
	sqlite3_finalize(statement);

	// Before we upload, decrease the server_status flag of all these
	// rows to indicate that we have started an attempt to get the info
	// to the server.

	ssc.str("");
	ssc << "update logs set server_status=server_status-1 where server_status=0";
	sres = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
	if(sres==SQLITE_OK) {
		sqlite3_step(statement);
		sqlite3_finalize(statement);
		}
	else {
		sqlite3_finalize(statement);
		return;
		}

	// Upload to the server.

	wsclient.send(our_connection_hdl, pack.str(), websocketpp::frame::opcode::text);
	}

std::vector<Snoop::AppEntry> SnoopImpl::get_app_registrations(std::string uuid_filter)
	{
	std::lock_guard<std::mutex> lock(sqlite_mutex);

	sqlite3_stmt *statement=0;

	std::ostringstream ssc;
	ssc << "select id, uuid, create_millis, receive_millis, pid, ip_address, machine_id, "
		"app_name, app_version, server_status from apps";
	if(uuid_filter.size()>0) 
		ssc << " where uuid=?";

	int sres = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
	if(uuid_filter.size()>0) 
		sqlite3_bind_text(statement, 1, uuid_filter.c_str(), uuid_filter.size(), 0);

	std::vector<Snoop::AppEntry> entries;
	if(sres==SQLITE_OK) {
		bool go=true;
		while(go) {
			int ret = sqlite3_step(statement);
			switch(ret) {
				case SQLITE_ROW: {
					Snoop::AppEntry ae;
					ae.id               = sqlite3_column_int(statement, 0);
					ae.uuid             = safestring(sqlite3_column_text(statement, 1));
					ae.create_millis    = sqlite3_column_int64(statement, 2);
					ae.receive_millis   = sqlite3_column_int64(statement, 3);
					ae.pid              = sqlite3_column_int(statement, 4);
					ae.ip_address       = safestring(sqlite3_column_text(statement, 5));
					ae.machine_id       = safestring(sqlite3_column_text(statement, 6));
					ae.app_name         = safestring(sqlite3_column_text(statement, 7));
					ae.app_version      = safestring(sqlite3_column_text(statement, 8));
					ae.server_status    = sqlite3_column_int(statement, 9);
					entries.push_back(ae);
					break;
					}
				case SQLITE_DONE: {
					go=false;
					break;
					}
				}
			}
		}
	else throw std::logic_error("Failed to prepare statement for get_app_registrations");

	sqlite3_finalize(statement);

	return entries;
	}

void SnoopImpl::on_client_open(websocketpp::connection_hdl)
	{
 	// std::cerr << "Snoop: connection to " << server_  << " open " << std::this_thread::get_id() << std::endl;
	sync_with_server(true);
	connection_is_open=true;
	// std::cerr << "Snoop: sync'ed with server" << std::endl;
	}

void SnoopImpl::on_client_fail(websocketpp::connection_hdl)
	{
	// std::cerr << "Snoop: connection failed" << std::endl;	
	}

void SnoopImpl::on_client_close(websocketpp::connection_hdl)
	{
	connection_is_open=false;
	// std::cerr << "Snoop: connection closed" << std::endl;	
	}

void SnoopImpl::on_client_message(websocketpp::connection_hdl, WebsocketClient::message_ptr msg) 
	{
	Json::Value  root;
	Json::Reader reader;
	if(!reader.parse( msg->get_payload(), root )) 
		throw std::logic_error("Cannot parse LogEntry from JSON");
	
	// Determine whether this is a log or an app message.

	std::lock_guard<std::mutex> lock(sqlite_mutex);

	if(root.isMember("log_stored")) {
		// Mark all log entries for which we have received confirmation from the 
		// server that they have been stored with 'server_status=1', to prevent
		// us from re-uploading them again. 

		sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

		sqlite3_stmt *statement=0;
		std::ostringstream ssc;
		ssc << "update logs set server_status=1 where log_id=?";
		int ret = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
		if(ret!=SQLITE_OK) 
			throw std::logic_error("Failed to prepare statement for on_client_message");
		
		Json::Value entries=root["log_stored"];
		for(auto entry: entries) {
			sqlite3_bind_int(statement, 1, entry.asInt());
			sqlite3_step(statement);
			sqlite3_reset(statement);
			}
		sqlite3_finalize(statement);

		sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
		}

	if(root.isMember("app_stored")) {
		// Mark all app entries for which we have received confirmation from the 
		// server that they have been stored with 'server_status=1', to prevent
		// us from re-uploading them again. 

		sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

		sqlite3_stmt *statement=0;
		std::ostringstream ssc;
		ssc << "update apps set server_status=1 where id=?";
		int ret = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
		if(ret!=SQLITE_OK) 
			throw std::logic_error("Failed to prepare statement for on_client_message");
		
		Json::Value entries=root["app_stored"];
		for(auto entry: entries) {
			sqlite3_bind_int(statement, 1, entry.asInt());
			sqlite3_step(statement);
			sqlite3_reset(statement);
			}
		sqlite3_finalize(statement);

		sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
		}
	
	}


Snoop::~Snoop()
	{
	if(impl) delete impl;
	}

SnoopImpl::~SnoopImpl()
   {
	if(db!=0) { // If the database is not open the wsclient won't be running either
		wsclient.stop();
		wsclient_thread.join();
		}

	if(insert_statement!=0) {
		sqlite3_finalize(insert_statement);
		}
	if(id_for_uuid_statement!=0) {
		sqlite3_finalize(id_for_uuid_statement);
		}
	if(db!=0) {
		sqlite3_close(db);
		db=0;
		}
   }


Snoop& Snoop::operator()(const std::string& type, std::string fl, int loc, std::string method) 
	{
	return impl->operator()(type, fl, loc, method);
	}

Snoop& SnoopImpl::operator()(const std::string& type, std::string fl, int loc, std::string method) 
	{
	assert(this_app_.app_name.size()>0);

	this_log_.type=type;
	this_log_.loc_file=fl;
	this_log_.loc_line=loc;
	this_log_.loc_method=method;

	return *snoop_;
	}

Snoop& Snoop::operator<<(const Flush& f)
	{
	return impl->operator<<(f);
	}

Snoop& SnoopImpl::operator<<(const Flush&)
   {
	assert(this_app_.app_name.size()>0);

	// Fill in the remaining fields of the LogEntry to be stored/sent.

	auto duration =  std::chrono::high_resolution_clock::now().time_since_epoch();
	auto millis   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	this_log_.create_millis = millis;
	this_log_.message       = snoop_->out_.str();
	this_log_.server_status = 0;

	store_log_entry(this_log_);
	if(snoop_->sync_immediately_)
		sync_logs_with_server();

   snoop_->out_.str("");

	this_log_.loc_file="";
	this_log_.loc_line=-1;
	this_log_.loc_method="";
	this_log_.type="";
	this_log_.message="";

   return *snoop_;
   }

Snoop::LogEntry::LogEntry()
	: log_id(0), client_log_id(0), id(0), create_millis(0), receive_millis(0), loc_line(0), server_status(0)
	{
	}

Snoop::LogEntry::LogEntry(int log_id_, int client_log_id_, int c1, const std::string& c1b,
								  uint64_t c2, uint64_t c2b, const std::string& c3, int c4, const std::string& c5,
								  const std::string& c6, const std::string& c7, int c8)
	: log_id(log_id_), client_log_id(client_log_id_), id(c1), uuid(c1b), 
	  create_millis(c2), receive_millis(c2b), loc_file(c3), loc_line(c4), loc_method(c5), 
	  type(c6), message(c7), server_status(c8)
	{
	}

std::string Snoop::LogEntry::to_json(bool human_readable) const
	{
	Json::Value json;
	
	json["log_id"]=log_id;
	json["client_log_id"]=client_log_id;
	json["id"]=id;
	json["uuid"]=uuid;
	if(human_readable) {	
		time_t tt = std::chrono::system_clock::to_time_t(u64_to_time(create_millis));
		tm utc_tm = *localtime(&tt);
		std::ostringstream str;
		str << std::setfill('0') 
			 << std::setw(2) << utc_tm.tm_hour << ":" 
			 << std::setw(2) << utc_tm.tm_min << ":" 
			 << std::setw(2) << utc_tm.tm_sec;
		json["time"]=str.str();
		str.str("");
		str << std::setw(2) << utc_tm.tm_mday << "/" 
			 << std::setw(2) << utc_tm.tm_mon+1 << "/" 
			 << std::setw(4) << utc_tm.tm_year+1900;
		json["date"]=str.str();
		}
	json["create_millis"]=(Json::UInt64)create_millis;
	json["receive_millis"]=(Json::UInt64)receive_millis;
	json["loc_file"]=loc_file;
	json["loc_line"]=loc_line;
	json["loc_method"]=loc_method;
	json["type"]=type;
	json["message"]=message;
	json["server_status"]=server_status;

	std::ostringstream str;
	str << json;

	return str.str();
	}

Snoop::AppEntry::AppEntry()
	: id(0), create_millis(0), receive_millis(0), pid(0), server_status(0), connected(false)
	{
	}

Snoop::AppEntry::AppEntry(const std::string& uuid_, uint64_t create_millis_, uint64_t receive_millis_, pid_t pid_, 
								  const std::string& ip_address_, const std::string& machine_id_, 
								  const std::string& app_name_,   const std::string& app_version_,
								  int server_status_)
	: uuid(uuid_), create_millis(create_millis_), receive_millis(receive_millis_), pid(pid_), ip_address(ip_address_),
	  machine_id(machine_id_), app_name(app_name_), app_version(app_version_), server_status(server_status_)
	{
	}

std::string Snoop::AppEntry::to_json(bool human_readable) const
	{
	std::ostringstream str;
	str << "{ \"id\": " << id  
		 << ", \"uuid\": \"" << uuid << "\"";
	if(human_readable) {	
		time_t tt = std::chrono::system_clock::to_time_t(u64_to_time(create_millis));
		tm utc_tm = *localtime(&tt);
		str << ", \"time\": \"" 
			 << std::setfill('0') 
			 << std::setw(2) << utc_tm.tm_hour << ":" 
			 << std::setw(2) << utc_tm.tm_min << ":" 
			 << std::setw(2) << utc_tm.tm_sec << "\""
			 << ", \"date\": \""
			 << std::setw(2) << utc_tm.tm_mday << "/" 
			 << std::setw(2) << utc_tm.tm_mon+1 << "/" 
			 << std::setw(4) << utc_tm.tm_year+1900
			 << "\"";
		}
	str << ", \"create_millis\": " << create_millis
		 << ", \"receive_millis\": " << receive_millis
		 << ", \"pid\": " << pid 
		 << ", \"ip_address\": \"" << ip_address << "\""
		 << ", \"machine_id\": \"" << machine_id << "\""
		 << ", \"app_name\": \"" << app_name << "\""
		 << ", \"app_version\": \"" << app_version << "\""
		 << ", \"server_status\": " << server_status
		 << ", \"connected\": " << connected
		 << "}";

	return str.str();
	}

void Snoop::LogEntry::from_json(const Json::Value& entry) 
	{
	// FIXME: if info is missing, catch the exception.
	log_id        = entry["log_id"].asInt();
	id            = entry["id"].asInt();
	uuid          = entry["uuid"].asString();
	create_millis = entry["create_millis"].asUInt64();
	loc_file      = entry["loc_file"].asString();
	loc_line      = entry["loc_line"].asInt();
	loc_method    = entry["loc_method"].asString();
	type          = entry["type"].asString();
	message       = entry["message"].asString();
	server_status = entry["server_status"].asInt();
	}

void Snoop::AppEntry::from_json(const Json::Value& entry)
	{
	// FIXME: if info is missing, catch the exception.
	id            = entry["id"].asInt();
	uuid          = entry["uuid"].asString();
	create_millis = entry["create_millis"].asUInt64();
	pid           = entry["pid"].asInt();
	ip_address    = entry["ip_address"].asString();
	machine_id    = entry["machine_id"].asString();
	app_name      = entry["app_name"].asString();
	app_version   = entry["app_version"].asString();
	server_status = entry["server_status"].asInt();
	}
