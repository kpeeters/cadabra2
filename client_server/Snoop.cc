
#include "Snoop.hh"

#include <iostream>
#include <string.h>
#include <regex>
#include <iostream>
#include <chrono>
#include <ctime>
#include <memory.h>
#ifndef _WIN32
  #ifndef _WIN64
     #include <sys/utsname.h>
  #endif
#endif
#include <time.h>
#ifdef _WIN32
    #define timegm _mkgmtime
#endif
#include <stdint.h>
#include <set>
#include "nlohmann/json.hpp"
#include "Stopwatch.hh"
#ifdef ENCRYPT_PASSWORDS
  #include <libscrypt.h>
#endif

std::string snoop_base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
std::string snoop_base64_decode(std::string const& encoded_string);

#include <signal.h>
#include <sys/types.h>
#if !defined(_WIN32) && !defined(_WIN64)
   #include <pwd.h>
   #include <unistd.h>
#else
   #include <windows.h>
   #include <io.h>
   #include <process.h>
   #include <direct.h>
#endif

#ifdef __APPLE__
   #include <pwd.h>
   #include "TargetConditionals.h"
#else
   #include <glibmm/miscutils.h>
#endif

   #define BOOST_SPIRIT_THREADSAFE
   #include <boost/signals2.hpp>
   #include <boost/property_tree/ptree.hpp>
   #include <boost/property_tree/json_parser.hpp>
   #include <boost/config.hpp>
#if !defined(TARGET_OS_IPHONE)
   #include <boost/program_options/detail/config_file.hpp>
   #include <boost/program_options/parsers.hpp>
#endif
   #include <boost/uuid/uuid.hpp>
   #include <boost/uuid/uuid_generators.hpp> // generators
   #include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#define SNOOPDEBUG(ln) 
//#define SNOOPDEBUG(ln) ln

using namespace snoop;

// Global instance.

snoop::Snoop snoop::log;
snoop::Flush snoop::flush;

using u64_millis = std::chrono::duration<uint64_t, std::milli>;
static std::chrono::time_point<std::chrono::system_clock, u64_millis> u64_to_time(uint64_t timestamp) {
    return std::chrono::time_point<std::chrono::system_clock, u64_millis>{u64_millis{timestamp}};
}

// Until we have widespread C++20 support, we will need to get
// the timezone offset using old-style code. This code returns
// the offset in minutes the same as Javascript's
// Date.getTimezoneOffset(); so if you are in zone GMT+2,
// it returns -120.

int local_utc_offset_minutes()
	{
	time_t t  = time ( NULL );
	struct tm * locg = localtime ( &t );
	struct tm locl;
	memcpy ( &locl, locg, sizeof ( struct tm ) );
	return -1 * (int)( timegm ( locg ) - mktime ( &locl ) ) / 60;
	}

std::string safestring(const unsigned char *c)
	{
	if(c==0) return "";
	else     return std::string((const char *)c);
	}

Snoop::Snoop()
   : sync_immediately_(false)
	, db(0)
	, payload_db(0)
	, auth_db(0)
	, insert_statement(0)
	, id_for_uuid_statement(0)
	, payload_insert_statement(0)
	, testq_statement(0)
	, connection_is_open(false)
	, connection_attempt_failed(false)
   {
   }

Snoop::ParseError::ParseError(const std::string& m)
	: std::logic_error(m)
	{
	}

void Snoop::init(const std::string& app_name, const std::string& app_version, std::string server, std::string dbname, std::string machine_id)
   {
	assert(app_name.size()>0);
	
	if(db==0) { // Only initialise if database has not been opened before
		this_app_.app_name=app_name;
		this_app_.app_version=app_version;
		this_app_.pid = getpid();
#if defined(_WIN32) || defined(_WIN64) 
		DWORD dwVersion = 0; 
		DWORD dwMajorVersion = 0;
		DWORD dwMinorVersion = 0; 
		DWORD dwBuild = 0;
		
		dwVersion = GetVersion();
		
		// Get the Windows version.
		
		dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
		dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
		
		// Get the build number.
		
		if (dwVersion < 0x80000000)              
			dwBuild = (DWORD)(HIWORD(dwVersion));
		
		this_app_.machine_id = "Windows "+std::to_string(dwMajorVersion)+"."+std::to_string(dwMinorVersion);
#else
		struct utsname buf;
		if(uname(&buf)==0) {
			this_app_.machine_id = std::string(buf.sysname)
				+", "+buf.nodename+", "+buf.release+", "+buf.version+", "+buf.machine;
#ifdef __linux__
			this_app_.machine_id += std::string(", ")+buf.domainname;
#endif
			}
#endif
        if(machine_id!="")
            this_app_.machine_id = machine_id; // override (used in Objective-C backend).
        
		this_app_.user_id = get_user_uuid(app_name);

		auto duration =  std::chrono::system_clock::now().time_since_epoch();
		this_app_.create_millis   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		this_app_.create_timezone = local_utc_offset_minutes();

		if(server.find("ws://")==std::string::npos && server.find("wss://")==std::string::npos)
			server_="wss://"+server;
		else
			server_=server;

		std::string payload_dbname, auth_dbname;
		if(dbname.size()==0) {
#if defined(_WIN32) || defined(_WIN64)
			// On Windoze, we store in 'user_data_dir' provided by Glib.
         std::string logdir = Glib::get_user_data_dir();
         mkdir(logdir.c_str());
#elif defined(TARGET_OS_IPHONE)
         // On iOS, we store in '~/Library/.log/'.
         std::string homedir=getenv("HOME");
         homedir+="/Library";
			std::string logdir = homedir+std::string("/.log");
			mkdir(logdir.c_str(), 0700);
#else
			// On Unix, we store in '~/.log/'.
			struct passwd *pw = getpwuid(getuid());
			const char *homedir = pw->pw_dir;
			std::string logdir = homedir+std::string("/.log");
			mkdir(logdir.c_str(), 0700);
#endif
			SNOOPDEBUG( std::cerr << "Snoop::init: storing databases in " << logdir << std::endl; )
			dbname=logdir+"/"+app_name+".db";
			payload_dbname=logdir+"/"+app_name+"_payload.db";
			auth_dbname=logdir+"/"+app_name+"_auth.db";
			}
		else {
			payload_dbname=dbname+"_payload.db";
			auth_dbname=dbname+"_auth.db";			
			dbname+=".db";
			}

		// Open the main database.
		int ret = sqlite3_open_v2(dbname.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if(ret) 
			throw std::logic_error("Snoop::init: Cannot open main snoop database");
		SNOOPDEBUG( std::cerr << "Snoop::init: main snoop database '" << dbname << "' open" << std::endl; );
		sqlite3_exec(db, "PRAGMA synchronous  = NORMAL", NULL, NULL, NULL);
		sqlite3_exec(db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
		sqlite3_exec(db, "PRAGMA busy_timeout = 5000", NULL, NULL, NULL);

		// Open the payload database.
		ret = sqlite3_open_v2(payload_dbname.c_str(), &payload_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if(ret) 
			throw std::logic_error("Snoop::init: Cannot open payload database");
		SNOOPDEBUG( std::cerr << "Snoop::init: payload database open" << std::endl; );
		sqlite3_exec(payload_db, "PRAGMA synchronous  = NORMAL", NULL, NULL, NULL);
		sqlite3_exec(payload_db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
		sqlite3_exec(payload_db, "PRAGMA busy_timeout = 5000", NULL, NULL, NULL);
		SNOOPDEBUG( std::cerr << "Snoop::init: payload database configured" << std::endl; );

		create_tables();
		SNOOPDEBUG( std::cerr << "Snoop::init: logging and payload tables created" << std::endl; );

		// Open the auth database.
		ret = sqlite3_open_v2(auth_dbname.c_str(), &auth_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
		if(ret) 
			throw std::logic_error("Snoop::init: Cannot open authentication database");
		SNOOPDEBUG( std::cerr << "Snoop::init: auth database open" << std::endl; );
		sqlite3_exec(auth_db, "PRAGMA synchronous  = NORMAL", NULL, NULL, NULL);
		sqlite3_exec(auth_db, "PRAGMA journal_mode = WAL", NULL, NULL, NULL);
		sqlite3_exec(auth_db, "PRAGMA busy_timeout = 5000", NULL, NULL, NULL);
		SNOOPDEBUG( std::cerr << "Snoop::init: auth database configured" << std::endl; );
		create_authentication_tables();
		SNOOPDEBUG( std::cerr << "Snoop::init: auth database tables created" << std::endl; );

		// Turn off synchronous writes as they seriously degrade performance (by orders of magnitude)
		// for our single-row inserts. See
		// http://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite?rq=1
		// for more options to speed things up.
		// The busy_timeout is to be compatible with litestream.
		// sqlite3_exec(db, "PRAGMA synchronous  = OFF", NULL, NULL, NULL);

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

std::string Snoop::get_user_uuid(const std::string& appname) 
	{
	std::string user_uuid="";

#ifdef TARGET_OS_IPHONE
	// On iOS, config files go in '~/Library/.config/'
	std::string configdir=getenv("HOME");
	configdir+="/Library/.config";
#else
	// on Unix/Windoze, Glib knows where to store config data.
	std::string configdir = Glib::get_user_config_dir();
#endif

	std::string configpath=configdir + std::string("/snoop/"+appname+".conf");
	std::ifstream config(configpath);
	bool need_to_write=true;
#ifndef TARGET_OS_IPHONE
	if(config) {
		std::set<std::string> options;
		options.insert("user");

		for(boost::program_options::detail::config_file_iterator i(config, options), e ; i != e; ++i) {
			// FIXME: http://stackoverflow.com/questions/24701547/how-to-parse-boolean-option-in-config-file
			if(i->string_key=="user") {
				user_uuid=i->value[0];
				need_to_write=false;
				}
			}
		}
#endif
	if(need_to_write) {
		// First time run; create config subdirectory for snoop.
		std::string configsubdir = configdir+std::string("/snoop");
#if defined(_WIN32) || defined(_WIN64)
		mkdir(configdir.c_str());
		mkdir(configsubdir.c_str());
#else
		mkdir(configdir.c_str(), 0700);
		mkdir(configsubdir.c_str(), 0700);
#endif
		
		std::ofstream config(configpath);
		if(config) {
			auto tmp = boost::uuids::random_generator()();
			std::ostringstream str;
			str << tmp;
			user_uuid = str.str();

			config << "user = " << user_uuid << std::endl;
			}
		else {
			SNOOPDEBUG( std::cerr << "Snoop: cannot write " << configpath << std::endl; )
			}
		}
	return user_uuid;
	}

void Snoop::set_sync_immediately(bool s)
	{
	sync_immediately_=s;
	}

void Snoop::create_tables()
	{
	assert(db!=0);
	assert(payload_db!=0);

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	char *errmsg;
	// Create the `runs` table.
	if(sqlite3_exec(db, "create table if not exists runs ("
						 "id              integer primary key autoincrement,"
						 "uuid            text,"
						 "create_millis   unsigned big int,"
						 "receive_millis  unsigned big int,"
						 "pid             int,"
						 "ip_address      text,"
						 "machine_id      text,"
						 "app_name        text,"
						 "app_version     text,"
						 "user_id         text,"
						 "server_status   int,"
                   "create_timezone int default -1);"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create table runs");
		}
	// Add the create_timezone column, ignore any errors.
	if(sqlite3_exec(db, "alter table runs add column create_timezone int default -1;"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		}

	// Create the `logs` table.
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
						 "server_status   int,"
						 "session_uuid    text,"
						 "create_timezone int default -1);"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create table logs");
		}
	// Add new session_uuid to existing table if we do not have it already.
	if(sqlite3_exec(db, "alter table logs add column session_uuid text;"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		}
	// Add the create_timezone column, ignore any errors.
	if(sqlite3_exec(db, "alter table logs add column create_timezone int default -1;"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		}

	// Create the `payload` table.
	if(sqlite3_exec(payload_db, "create table if not exists payload ("
						 "payload_id         integer primary key autoincrement,"
						 "client_payload_id  integer,"						 
						 "id                 integer,"  /* 'references login', but that's not possible across databases */
						 "create_millis      unsigned big int,"
						 "receive_millis     unsigned big int,"
						 "payload            text,"
						 "server_status      int,"
						 "create_timezone    int default -1);"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create table payload");
		}
	// Add the create_timezone column, ignore any errors.
	if(sqlite3_exec(payload_db, "alter table payload add column create_timezone int default -1;"
						 , NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		}
	
	if(sqlite3_exec(db, "create index if not exists logs_id_idx on logs(id);", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create index on logs.id: "+err);
		}
	if(sqlite3_exec(db, "create index if not exists logs_client_log_id_idx on logs(client_log_id);", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create index on logs.client_log_id: "+err);
		}
	if(sqlite3_exec(db, "create index if not exists logs_create_millis_idx on logs(create_millis);", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create index on logs.create_millis: "+err);
		}
	if(sqlite3_exec(db, "create index if not exists logs_type_idx on logs(type);", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create index on logs.type: "+err);
		}
	if(sqlite3_exec(db, "create index if not exists runs_uuid_idx on runs(uuid);", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create index on runs.uuid: "+err);
		}
	if(sqlite3_exec(db, "create index if not exists runs_machine_id_idx on runs(machine_id);", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create index on runs.machine_id: "+err);
		}
	if(sqlite3_exec(db, "create index if not exists runs_app_version_idx on runs(app_version);", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create index on runs.app_version: "+err);
		}
	if(sqlite3_exec(db, "create index if not exists runs_app_name_idx on runs(app_name);", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error("Failed to create index on runs.app_name: "+err);
		}
	}

void Snoop::create_authentication_tables()
	{
	// We need two tables: one for the username/salted-password
	// combo (together with some other user info, probably)
	// and one for authorisation tickets issued after a successful
	// login. 

	assert(auth_db!=0);

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	char *errmsg;
	if(sqlite3_exec(auth_db, "create table if not exists users ("
						 "id              integer primary key autoincrement,"
						 "username        text unique,"
	                "password        text,"
						 "enabled         int,"
						 "\"group\"           int,"
						 "email           text,"
						 "device          text);"
	                , NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err("Failed to create table users: ");
		err += std::string(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error(err);
		}
	if(sqlite3_exec(auth_db, "create table if not exists groups ("
						 "id              integer primary key autoincrement,"
						 "\"group\"       text unique,"
      	                "description        text,"
			"enabled            int);"
	                , NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err("Failed to create table groups: ");
		err += std::string(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error(err);
		}
	// The following table is fairly trivial right now but can be used
	// to add other information at a later stage. Mainly introduced
	// so we can handle multiple user_detail records for each user.
//	if(sqlite3_exec(auth_db, "create table if not exists user_details ("
//						 "id              integer primary key autoincrement,"
//						 "user_id         integer,"
//	                "device          text);"
//	                , NULL, NULL, &errmsg) != SQLITE_OK) {
//		sqlite3_free(errmsg);
//		throw std::logic_error("Failed to create table user_details");
//		}
	if(sqlite3_exec(auth_db, "create table if not exists tickets ("
						 "id              integer primary key autoincrement,"
						 "user_id         integer,"
						 "ticket_uuid     text,"
	                "valid           integer);"
	                , NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err("Failed to create table tickets: ");
		err += std::string(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error(err);
		}
	if(sqlite3_exec(auth_db, "create table if not exists auth_attempts ("
						 "id              integer primary key autoincrement,"
						 "time_millis     unsigned big int,"
						 "user_id         integer,"
						 "ticket_id       integer,"
	                "success         integer,"
						 "msg             text);"
	                , NULL, NULL, &errmsg) != SQLITE_OK) {
		std::string err("Failed to create table auth_attempts: ");
		err += std::string(errmsg);
		sqlite3_free(errmsg);
		throw std::logic_error(err);
		}
	}


void Snoop::obtain_uuid()
	{
	assert(db!=0);

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
//	ss << "select uuid from runs where pid=" << getpid() << " order by create_millis desc limit 1";
	ss << "select uuid from runs where pid=" << getpid() << " order by create_millis desc limit 1";
	
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
		auto tmp = boost::uuids::random_generator()();
		std::ostringstream str;
		str << tmp;
		this_app_.uuid = str.str();

		SNOOPDEBUG( std::cerr << "Snoop: registering run with uuid " << this_app_.uuid << std::endl; );
		bool stored = store_app_entry_without_lock(this_app_);
		if(!stored) {
  		   SNOOPDEBUG( std::cerr << "Snoop: uuid already registered" << std::endl; );
		   }
		else {
  		   SNOOPDEBUG( std::cerr << "Snoop: app run for this uuid stored" << std::endl; );
		   }
		}
	else {
	   SNOOPDEBUG( std::cerr << "Snoop: pid already has a uuid, not re-registering" << std::endl; );
	   }
	}

/// Get the app_version string for the last run on the given device.

std::string Snoop::last_seen_version(std::string machine_id)
	{
	assert(db!=0);

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select app_version from runs where machine_id=? order by id desc limit 1";

	std::string last_version="";
	int res = sqlite3_prepare(db, ss.str().c_str(), -1, &statement, NULL);
	if(res==SQLITE_OK) {
		sqlite3_bind_text(statement, 1, machine_id.c_str(), machine_id.size(), 0);
		int ret = sqlite3_step(statement);
		if(ret==SQLITE_ROW) {
			if(sqlite3_column_type(statement, 0)==SQLITE3_TEXT) 
				last_version=safestring(sqlite3_column_text(statement, 0));
			}
		}
	sqlite3_finalize(statement);

	return last_version;
	}

bool Snoop::authenticate(std::function<void (std::string, bool)> f, std::string user, std::string pass, bool always_reauth)
   {
   // Wait for the websocket client thread to have spun up.
	std::unique_lock<std::mutex> lock(connection_mutex);
	if(connection_is_open==false && connection_attempt_failed==false) {
		connection_cv.wait(lock);
		}

	authentication_callback=f;
	
   // If we have a ticket, setup check request and return true; 
   // If we do not have a ticket, and user and pass are not empty, setup login check, return false.

   // First check if we have a ticket from a previous session.

	std::string ticket_uuid;
	if(always_reauth==false)
		ticket_uuid=get_local_ticket();
	
	// Check ticket validity with server, or do password login.
	                     
	if(ticket_uuid.size()!=0 && user=="" && pass=="") { // have ticket and want to login without name/pass
		SNOOPDEBUG( std::cerr << "Have a ticket already, re-validating." << std::endl; );
	   std::ostringstream pack;
	   pack << "{ \"authenticate\": {"
	        << "     \"ticket_uuid\": \"" << ticket_uuid << "\"} } \n";
	   if(!connection_is_open) {
		   SNOOPDEBUG( std::cerr << "Connection not open, cannot verify ticket, allowed through." << std::endl; );
		   }
	   else {
		   wsclient.send(pack.str());
		   }
	   return true;
	   }
   else {
		ticket_uuid=""; // FIXME: should we also remove it from the local database?

		// Store the user in the local database.
		SNOOPDEBUG( std::cerr << "Snoop::authenticate: storing user '" << user
						<< "' (but not password) in local database." << std::endl; );
		if(!add_user(user, "", true)) {
			SNOOPDEBUG( std::cerr << "Snoop::authenticate: error storing user." << std::endl; );
			}

		// Test:
		std::string local_user_vrfy = local_user();
		SNOOPDEBUG( std::cerr << "Snoop::authenticate: local_user = " << local_user_vrfy << std::endl; );

		// Send authentication request to server, asking for ticket. Then exit, as
		// the ticket will come back async.
	   SNOOPDEBUG( std::cerr << "No ticket yet, requesting one with login/pass." << std::endl; );
	   std::ostringstream pack;
	   pack << "{ \"authenticate\": {"
	        << "     \"user\": \"" << user << "\", \"password\": \"" << pass << "\" } } \n";

	   if(!connection_is_open) {
		   SNOOPDEBUG( std::cerr << "No connection, cannot verify login credentials" << std::endl; );
		   // FIXME: need a 'false' which also shows that we have not been able to verify.
		   }
	   else {
		   wsclient.send(pack.str());
		   }
	   return false;
	   }
   }

void Snoop::set_error_handler(std::function<bool (std::string)> f)
	{
	error_callback=f;
	}

void Snoop::set_session_uuid(const std::string& s)
	{
	session_uuid_=s;
	}

std::string Snoop::get_local_ticket()
   {
	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	// Prepare the query for the ticket_uuid. This always
	// queries for the ticket with user number 0 (as the local
	// client storage does not store user details). 

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select ticket_uuid from tickets where user_id=0 and valid=1";
	int res = sqlite3_prepare(auth_db, ss.str().c_str(), -1, &statement, NULL);
	assert(res==SQLITE_OK);
	int ret = sqlite3_step(statement);
	std::string ticket_uuid;
	if(ret==SQLITE_ROW) {
		ticket_uuid=safestring(sqlite3_column_text(statement, 0));		
		}
	sqlite3_finalize(statement);

	return ticket_uuid;
   }

void Snoop::set_local_ticket(std::string ticket_uuid)
   {
   // First delete local ticket.
   
   std::ostringstream ss;
	sqlite3_stmt *statement=0;   
   ss << "delete from tickets";
   int res = sqlite3_prepare(auth_db, ss.str().c_str(), -1, &statement, NULL);
   if(res!=SQLITE_OK)
	   throw std::logic_error("Snoop::delete_local_ticket: sqlite3_prepare failed error "+std::to_string(res));
   
   res = sqlite3_step(statement);
   if(res!=SQLITE_DONE)
	   throw std::logic_error("Snoop::store_ticket: sqlite3_step failed error "+std::to_string(res));

   sqlite3_finalize(statement);
   
   // Now store.
   if(ticket_uuid.size()!=0)
	   store_ticket(ticket_uuid, 0, 1);
   }

Snoop::Ticket::Ticket()
	{
	ticket_id=-1;
	user_id=-1;
	valid=false;
	}

Snoop::Ticket Snoop::is_ticket_valid(std::string ticket_uuid) 
   {
	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	// Prepare the query for the ticket_uuid.

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "select valid, users.enabled, ifnull(groups.enabled,1) as groupsenabled, tickets.id, users.id from tickets join users on tickets.user_id=users.id left join groups on users.\"group\"=groups.id where ticket_uuid=?";
	int res = sqlite3_prepare(auth_db, ss.str().c_str(), -1, &statement, NULL);
	assert(res==SQLITE_OK);
	sqlite3_bind_text(statement, 1, ticket_uuid.c_str(), ticket_uuid.size(), 0);

	// Query database.

	Ticket tret;
	tret.ticket_uuid=ticket_uuid;
	int valid=0;
	int enabled=0;
	int groupsenabled=0;
	int ret = sqlite3_step(statement);
	if(ret==SQLITE_ROW) {
	  valid=sqlite3_column_int(statement, 0);
	  enabled=sqlite3_column_int(statement, 1);
	  groupsenabled=sqlite3_column_int(statement, 2);
	  tret.ticket_id=sqlite3_column_int(statement, 3);
	  tret.user_id=sqlite3_column_int(statement, 4);	  
	}
	sqlite3_finalize(statement);

	tret.valid = (valid==1 && enabled==1 && groupsenabled==1);
	
	return tret;
   }

int Snoop::store_ticket(std::string ticket_uuid, int user_id, bool valid)
   {
   assert(auth_db!=0);

   std::ostringstream ss;
	sqlite3_stmt *statement=0;   
   ss << "insert into tickets (user_id, ticket_uuid, valid) values (?, ?, ?)";
   int res = sqlite3_prepare(auth_db, ss.str().c_str(), -1, &statement, NULL);
   if(res!=SQLITE_OK)
	   throw std::logic_error("Snoop::store_ticket: sqlite3_prepare failed error "+std::to_string(res));
   
   sqlite3_bind_int(statement,  1, user_id);
   sqlite3_bind_text(statement, 2, ticket_uuid.c_str(), ticket_uuid.size(), 0);
   sqlite3_bind_int(statement,  3, valid?1:0);
   res = sqlite3_step(statement);
   if(res!=SQLITE_DONE)
	   throw std::logic_error("Snoop::store_ticket: sqlite3_step failed error "+std::to_string(res));

   sqlite3_finalize(statement);
	return sqlite3_last_insert_rowid(auth_db);	
   }

bool Snoop::store_app_entry(Snoop::AppEntry& app)
	{
	assert(db!=0);

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	return store_app_entry_without_lock(app);
	}

bool Snoop::store_app_entry_without_lock(Snoop::AppEntry& app)
	{
	// Do we already have a record with this uuid?
	sqlite3_stmt *testq=0;
	int testq_res = sqlite3_prepare(db, "select count(*) from runs where uuid=?", -1, &testq, NULL);
	if(testq_res!=SQLITE_OK) 
		throw std::logic_error("Snoop::store_app_entry_without_lock: failed to test for row presence");

	sqlite3_bind_text(testq, 1, app.uuid.c_str(), app.uuid.size(), 0);
	sqlite3_step(testq);
	int64_t num = sqlite3_column_int64(testq, 0);
	sqlite3_finalize(testq);
	if(num>0) 
		return false;
	
	// No entry yet, we need to store it.
	sqlite3_stmt *statement=0;
	int res = sqlite3_prepare(db, "insert into runs (uuid, create_millis, receive_millis, pid, ip_address, machine_id, "
									  "app_name, app_version, user_id, server_status, create_timezone) "
									  "values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
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
		sqlite3_bind_text(statement,   9, app.user_id.c_str(), app.user_id.size(), 0);
		sqlite3_bind_int(statement,   10, app.server_status);		
//		sqlite3_bind_text(statement,  11, app.uuid.c_str(), app.uuid.size(), 0);
		sqlite3_bind_int(statement,   11, app.create_timezone);

		sqlite3_step(statement);
		sqlite3_finalize(statement);

		app.id = sqlite3_last_insert_rowid(db);

		return true;
		}
	else {
		throw std::logic_error("Failed to prepare insertion");
		}
	}

bool Snoop::store_log_entry(Snoop::LogEntry& log_entry, bool avoid_server_duplicates)
	{
	assert(db!=0);
	
	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	if(false && avoid_server_duplicates) {
		// This can take 0.3s on a 15Gb database. We cannot afford that.

		// Do we already have a record with this client_log_id and id?
		if(testq_statement==0) {
			int testq_res = sqlite3_prepare(db, "select exists (select 1 from (select client_log_id from logs where id=?) where client_log_id=?)",
													  -1, &testq_statement, NULL);
			if(testq_res!=SQLITE_OK) 
				throw std::logic_error("Snoop::store_log_entry_without_lock: failed to prepare query for duplicate check");
			}

		std::cerr << log_entry.id << ", " << log_entry.client_log_id << std::endl;
		sqlite3_bind_int64(testq_statement, 1, log_entry.id);
		sqlite3_bind_int64(testq_statement, 2, log_entry.client_log_id);
		sqlite3_step(testq_statement);
		int64_t num = sqlite3_column_int64(testq_statement, 0);
		sqlite3_reset(testq_statement);
		if(num>0) 
			return false;

		}
	
	// Need to store this entry.

	auto duration =  std::chrono::system_clock::now().time_since_epoch();
	log_entry.receive_millis   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

	int res=SQLITE_OK;
	if(insert_statement==0) {
		res=sqlite3_prepare_v2(db, "insert into logs "
									  "(client_log_id, id, create_millis, receive_millis, loc_file, loc_line, loc_method, "
									  " type, message, server_status, session_uuid, create_timezone) "
									  "values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
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
		sqlite3_bind_text(insert_statement,  11, log_entry.session_uuid.c_str(), log_entry.session_uuid.size(), 0);		
		sqlite3_bind_int(insert_statement,   12, log_entry.create_timezone);

		sqlite3_step(insert_statement);

		log_entry.log_id = sqlite3_last_insert_rowid(db);
		sqlite3_reset(insert_statement);

		return true;
		}
	else {
		throw std::logic_error("Failed to insert log entry");
		}
	}

bool Snoop::store_auth_attempt_entry(int user_id, int ticket_id, int valid, std::string msg)
	{
	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	sqlite3_stmt *statement=0;
	int res = sqlite3_prepare(auth_db, "insert into auth_attempts (time_millis, user_id, ticket_id, success, msg) "
									  "values (?, ?, ?, ?, ?)",
									  -1, &statement, NULL);

	auto duration =  std::chrono::system_clock::now().time_since_epoch();
	uint64_t time_millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	
	if(res==SQLITE_OK) {
		sqlite3_bind_int64(statement,  1, time_millis);
		sqlite3_bind_int(statement,    2, user_id);
		sqlite3_bind_int(statement,    3, ticket_id);
		sqlite3_bind_int(statement,    4, valid);
		sqlite3_bind_text(statement,   5, msg.c_str(), msg.size(), 0);		
		
		sqlite3_step(statement);
		sqlite3_finalize(statement);

		return true;
		}
	else {
		throw std::logic_error("Failed to prepare insertion for auth_attempts");
		}
	}

bool Snoop::store_payload_entry(Snoop::PayLoad& payload)
	{
	assert(payload_db!=0);
	
	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	// Payload can be large, so we always first check if we have this entry already.
	sqlite3_stmt *testq=0;
	int testq_res = sqlite3_prepare(payload_db, "select count(*) from payload where client_payload_id=? and id=? and client_payload_id!=-1", -1, &testq, NULL);
	if(testq_res!=SQLITE_OK) 
		throw std::logic_error("Snoop::store_payload_entry_without_lock: failed to test for row presence");
	
	sqlite3_bind_int64(testq, 1, payload.client_payload_id);
	sqlite3_bind_int64(testq, 2, payload.id);
	sqlite3_step(testq);
	int64_t num = sqlite3_column_int64(testq, 0);
	sqlite3_finalize(testq);
	if(num>0) {
		SNOOPDEBUG( std::cerr << "Already have payload with client_payload_id=" << payload.client_payload_id << " and id=" << payload.id << std::endl; );
	  return false;
	}
	
	// Need to store this entry.

	SNOOPDEBUG( std::cerr << "Storing payload entry" << std::endl; );
	int res=SQLITE_OK;
	if(payload_insert_statement==0) {
		res=sqlite3_prepare_v2(payload_db, "insert into payload "
									  "(client_payload_id, id, create_millis, receive_millis, payload, server_status, create_timezone) "
									  "values (?, ?, ?, ?, ?, ?, ?)",
									  -1, &payload_insert_statement, NULL);
		}

	if(res==SQLITE_OK ) {
		sqlite3_bind_int(payload_insert_statement,    1, payload.client_payload_id);
		sqlite3_bind_int(payload_insert_statement,    2, payload.id);
		sqlite3_bind_int64(payload_insert_statement,  3, payload.create_millis);
		sqlite3_bind_int64(payload_insert_statement,  4, payload.receive_millis);
		sqlite3_bind_text(payload_insert_statement,   5, payload.payload.c_str(), payload.payload.size(), 0);
		sqlite3_bind_int(payload_insert_statement,    6, payload.server_status);
		sqlite3_bind_int(payload_insert_statement,    7, payload.create_timezone);		

		sqlite3_step(payload_insert_statement);

		payload.payload_id = sqlite3_last_insert_rowid(db);
		sqlite3_reset(payload_insert_statement);

		return true;
		}
	else {
		throw std::logic_error("Failed to insert payload entry");
		}

	}

void Snoop::start_websocket_client()
	{
	SNOOPDEBUG( std::cerr << "Snoop: attempting open" << std::endl; );
	{ 	std::unique_lock<std::mutex> lock(connection_mutex);
		connection_attempt_failed=false;
		//std::cerr << "Snoop: attempting open" << std::endl;
		}
	SNOOPDEBUG( std::cerr << "Snoop: got connection_mutex" << std::endl; );

//	wsclient.start_perpetual();

	// Start I/O thread. All actual connection work runs there,
	// and is also re-started there on connection failure/loss.
	wsclient_thread=std::thread([this]{
	     io_thread_run();
        SNOOPDEBUG( std::cerr << "Snoop: client stopped running, thread will end" << std::endl; );
    });
	}

void Snoop::try_connect()
   {
	// This code runs in the separate thread which handles I/O.

	wsclient.set_connect_handler(std::bind(&Snoop::on_client_open,    this));
	wsclient.set_fail_handler(std::bind(&Snoop::on_client_fail,       this, std::placeholders::_1));
	wsclient.set_close_handler(std::bind(&Snoop::on_client_close,     this));
	wsclient.set_message_handler(std::bind(&Snoop::on_client_message, this, std::placeholders::_1));

	std::string uri = server_;
	SNOOPDEBUG( std::cerr << "Snoop: uri = " << uri << std::endl; );

	// Attempt the connection. Failure is reported on the `on_fail` handler.
	wsclient.connect(uri);

	// FIXME: we need to retry the connection, and handle when the connection
	// is down!!!
   }

void Snoop::io_thread_run()
   {
	// This code runs in the separate thread which handles I/O.

	SNOOPDEBUG( std::cerr << "Snoop: I/O thread starting." << std::endl; );

	try_connect();
   wsclient.run();

   SNOOPDEBUG( std::cerr << "Snoop: I/O thread up and running." << std::endl; );
   }

void Snoop::sync_with_server(bool from_wsthread)
	{
	assert(server_.size()>0);

	if(!from_wsthread)
		if(!connection_is_open) 
			return;

	sync_runs_with_server(from_wsthread);
	sync_logs_with_server(from_wsthread);
	sync_payloads_with_server(from_wsthread);	
	}

void Snoop::sync_runs_with_server(bool from_wsthread)
	{
	if(!from_wsthread)
		if(!connection_is_open) 
			return;

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	// Create a JSON text with all locally stored entries which have
	// a server_status field negative or zero.

	sqlite3_stmt *statement=0;
	std::ostringstream ssc;
	std::ostringstream pack;
	pack << "{ \"run\": [";
	ssc << "select id, uuid, create_millis, receive_millis, pid, ip_address, machine_id, app_name, app_version, user_id, server_status, create_timezone "
		 << "from runs where server_status=0";

	int sres = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
	if(sres==SQLITE_OK) {
		bool first=true;
		bool go=true;
		while(go) {
			int ret = sqlite3_step(statement);
			switch(ret) {
				case SQLITE_BUSY: 
					throw std::logic_error("Unexpected SQLITE_BUSY in sync_runs_with_server");
					break;
				case SQLITE_ROW: {
					Snoop::AppEntry ae;
					// FIXME: isolate this in a separate function so we can fetch individual records more easily
					ae.id               = sqlite3_column_int(statement, 0);
					ae.uuid             = safestring(sqlite3_column_text(statement, 1));
					ae.create_millis    = sqlite3_column_int64(statement, 2);
					ae.receive_millis   = sqlite3_column_int64(statement, 3);
					ae.pid              = sqlite3_column_int(statement, 4);
					ae.ip_address       = safestring(sqlite3_column_text(statement, 5));
					ae.machine_id       = safestring(sqlite3_column_text(statement, 6));
					ae.app_name         = safestring(sqlite3_column_text(statement, 7));
					ae.app_version      = safestring(sqlite3_column_text(statement, 8));
					ae.user_id          = safestring(sqlite3_column_text(statement, 9));
					ae.server_status    = sqlite3_column_int(statement, 10);
					ae.create_timezone  = sqlite3_column_int(statement, 11);
					if(!first) pack << ", \n";
					else       first=false;
					pack << ae.to_json(false);
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
	ssc << "update runs set server_status=server_status-1 where server_status=0";
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
   SNOOPDEBUG( std::cerr << "Snoop::sync_logs_with_server: " << pack.str() << std::endl; )
	wsclient.send(pack.str());
	}

void Snoop::sync_logs_with_server(bool from_wsthread)
	{
	if(!from_wsthread)
		if(!connection_is_open) 
			return;

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	// Create a JSON text with all locally stored entries which have
	// a server_status field negative or zero.

	sqlite3_stmt *statement=0;
	std::ostringstream ssc;
	std::ostringstream pack;
	pack << "{ \"log\": [";
	ssc << "select log_id, client_log_id, id, create_millis, loc_file, loc_line, loc_method, type, message, server_status, session_uuid, create_timezone "
		 << "from logs where server_status=0";
	if(local_types.size()>0) {
		ssc << " and type not in (";
		bool first=true;
		for(const auto& lt: local_types) {
			if(first) first=false;
			else      ssc << ", ";
			// FIXME: sql injection!
			ssc << "'" << lt << "'";
			}
		ssc << ")";
		}

	int sres = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
	if(sres==SQLITE_OK) {
		bool first=true;
		bool go=true;
		while(go) {
			int ret = sqlite3_step(statement);
			switch(ret) {
				case SQLITE_BUSY: 
					throw std::logic_error("Unexpected SQLITE_BUSY in sync_runs_with_server");
					break;
				case SQLITE_ROW: {
					Snoop::LogEntry le;
					le.log_id           = sqlite3_column_int(statement, 0);
					le.client_log_id    = sqlite3_column_int(statement, 1);
					le.id               = sqlite3_column_int(statement, 2);
					le.uuid             = this_app_.uuid; // FIXME: this is wrong, we may still have log entries from a previous run!
					// See 'sync_payload_with_server' in Snoop.java for the query to handle this properly.
					le.create_millis    = sqlite3_column_int64(statement, 3);
					le.loc_file         = safestring(sqlite3_column_text(statement, 4));
					le.loc_line         = sqlite3_column_int(statement, 5);
					le.loc_method       = safestring(sqlite3_column_text(statement, 6));
					le.type             = safestring(sqlite3_column_text(statement, 7));
					le.message          = safestring(sqlite3_column_text(statement, 8));
					le.server_status    = sqlite3_column_int(statement, 9);
					le.session_uuid     = safestring(sqlite3_column_text(statement, 10));
					le.create_timezone  = sqlite3_column_int(statement, 11);
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
   SNOOPDEBUG( std::cerr << "Snoop::sync_logs_with_server: " << pack.str() << std::endl; )
	wsclient.send(pack.str());
	}

bool Snoop::is_connected() const
   {
   return connection_is_open;
   }

void Snoop::sync_payloads_with_server(bool from_wsthread)
	{
	if(!from_wsthread)
		if(!connection_is_open) 
			return;

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	// Create a JSON text with all locally stored entries which have
	// a server_status field negative or zero.

	// std::cerr << "Syncing payloads" << std::endl;
	
	sqlite3_stmt *statement=0;
	std::ostringstream ssc;
	std::ostringstream pack;
	pack << "{ \"payload\": [";
	ssc << "select payload_id, client_payload_id, id, create_millis, payload, server_status, create_timezone "
		 << "from payload where server_status=0";

	int sres = sqlite3_prepare(payload_db, ssc.str().c_str(), -1, &statement, NULL);
	if(sres==SQLITE_OK) {
		bool first=true;
		bool go=true;
		while(go) {
			int ret = sqlite3_step(statement);
			switch(ret) {
				case SQLITE_BUSY: 
					throw std::logic_error("Unexpected SQLITE_BUSY in sync_payloads_with_server");
					break;
				case SQLITE_ROW: {
					Snoop::PayLoad payload;
					payload.uuid             = this_app_.uuid; // FIXME: this is wrong, we may still have log entries from a previous run!
					payload.payload_id       = sqlite3_column_int(statement,   0);
					payload.client_payload_id= sqlite3_column_int(statement,   1);
					payload.id               = sqlite3_column_int(statement,   2);
					payload.create_millis    = sqlite3_column_int64(statement, 3);
					payload.payload          = safestring(sqlite3_column_text(statement, 4));
					payload.server_status    = sqlite3_column_int(statement,   5);
					payload.create_timezone  = sqlite3_column_int(statement,   6);					
					if(!first) pack << ", \n";
					else       first=false;
					pack << payload.to_json(false);
					break;
					}
				case SQLITE_DONE: {
					go=false;
					break;
					}
				}
			}
		}
	else {
		throw std::logic_error("Failed to prepare statement for payload select");
		}
	pack << "] }";
	sqlite3_finalize(statement);

	// Before we upload, decrease the server_status flag of all these
	// rows to indicate that we have started an attempt to get the info
	// to the server.

	ssc.str("");
	ssc << "update payload set server_status=server_status-1 where server_status=0";
	sres = sqlite3_prepare(payload_db, ssc.str().c_str(), -1, &statement, NULL);
	if(sres==SQLITE_OK) {
		sqlite3_step(statement);
		sqlite3_finalize(statement);
		}
	else {
		sqlite3_finalize(statement);
		return;
		}

	// std::cerr << "Syncing payloads almost done" << std::endl;


   // Upload to the server.

	wsclient.send(pack.str());
	}

std::vector<Snoop::AppEntry> Snoop::get_app_registrations(std::string uuid_filter)
	{
	Stopwatch sw;
	sw.start();
	
	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	sqlite3_stmt *statement=0;

	std::ostringstream ssc;
	ssc << "select id, uuid, create_millis, receive_millis, pid, ip_address, machine_id, "
		"app_name, app_version, user_id, server_status, create_timezone from runs";
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
					ae.user_id          = safestring(sqlite3_column_text(statement, 9));
					ae.server_status    = sqlite3_column_int(statement, 10);
					ae.create_timezone  = sqlite3_column_int(statement, 11);
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

	sw.stop();
	// This is now fast because we have added an index on `uuid`.
   //	std::cerr << "get_app_registrations: " << uuid_filter << sw << std::endl;
	
	return entries;
	}

void Snoop::on_client_open()
	{
	SNOOPDEBUG( std::cerr << "Snoop: connection to " << server_  << " open " << std::this_thread::get_id() << std::endl; );
	sync_with_server(true);
	std::unique_lock<std::mutex> lock(connection_mutex);
	connection_is_open=true;
	connection_attempt_failed=false;
	connection_cv.notify_all();
	SNOOPDEBUG( std::cerr << "Snoop: synced after open, ready to roll" << std::endl; );
	}

void Snoop::on_client_fail(const boost::beast::error_code& ec)
	{
	// This runs in the I/O thread.
	std::string error_reason = ec.message();
	
	SNOOPDEBUG( std::cerr << "Snoop: connection failed: " << error_reason << std::endl;	);
	// Clients may be waiting for the connection to open, but we may
	// never get to that stage. Signal them to move on.
	std::unique_lock<std::mutex> lock(connection_mutex);
	connection_attempt_failed=true;
	connection_cv.notify_all();

	// Run the callback
	bool retry=true;
	if(error_callback) {
		retry = error_callback(error_reason);
		if(authentication_callback)
			authentication_callback("", false);
		}
	
	// Attempt to re-open.
	if(retry && this_app_.app_name!="SnoopServer") {
	  SNOOPDEBUG( std::cerr << "Snoop: sleeping a little before attempting reconnect" << std::endl;	);
	  sleep(3);
	  SNOOPDEBUG( std::cerr << "Snoop: attempting to re-open connection" << std::endl; );
	  try_connect();
	  }
	}

void Snoop::on_client_close()
	{
	// Clients may be waiting for the connection to open, but we may
	// never get to that stage. Signal them to move on.
	  {
		 std::unique_lock<std::mutex> lock(connection_mutex);
		 connection_is_open=false;
		 connection_attempt_failed=true;
		 connection_cv.notify_all();
		 SNOOPDEBUG( std::cerr << "Snoop: connection closed" << std::endl; );
	  }

	// Attempt to re-open.
	if(this_app_.app_name!="SnoopServer") {
	  SNOOPDEBUG( std::cerr << "Snoop: attempting to re-open connection" << std::endl; );
	  try_connect();
	  }
	}

void Snoop::on_client_message(const std::string& msg)
	{
	nlohmann::json root;

	// std::cerr << msg->get_payload() << std::endl;
	try {
		root=nlohmann::json::parse(msg);
		}
	catch(nlohmann::json::exception& ex) {
		SNOOPDEBUG( std::cerr << "Snoop::on_client_message: Cannot parse LogEntry from JSON: " << ex.what() << std::endl; );
		return;
		}
	
	// Determine what type of message this is, and take corresponding action.

	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	try {
		
		if(root.count("log_stored")>0) {
			// Mark all log entries for which we have received confirmation from the 
			// server that they have been stored with 'server_status=1', to prevent
			// us from re-uploading them again. 

			SNOOPDEBUG( std::cerr << "server ack'ed storing log entries " << root["log_stored"] << std::endl; );
			
			sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

			sqlite3_stmt *statement=0;
			std::ostringstream ssc;
			ssc << "update logs set server_status=1 where log_id=?";
			int ret = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
			if(ret!=SQLITE_OK) 
				throw std::logic_error("Failed to prepare statement for on_client_message");
		
			const auto& entries=root["log_stored"];
			for(auto entry: entries) {
				sqlite3_bind_int(statement, 1, entry.get<int>() );
				sqlite3_step(statement);
				sqlite3_reset(statement);
				}
			sqlite3_finalize(statement);

			sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
			}

		if(root.count("app_stored")>0) {
			// Mark all app entries for which we have received confirmation from the 
			// server that they have been stored with 'server_status=1', to prevent
			// us from re-uploading them again. 

			SNOOPDEBUG( std::cerr << "server ack'ed storing run entries " << root["app_stored"] << std::endl; );

			sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

			sqlite3_stmt *statement=0;
			std::ostringstream ssc;
			ssc << "update runs set server_status=1 where id=?";
			int ret = sqlite3_prepare(db, ssc.str().c_str(), -1, &statement, NULL);
			if(ret!=SQLITE_OK) 
				throw std::logic_error("Failed to prepare statement for on_client_message");
		
			const auto& entries=root["app_stored"];
			for(auto entry: entries) {
				sqlite3_bind_int(statement, 1, entry.get<int>());
				sqlite3_step(statement);
				sqlite3_reset(statement);
				}
			sqlite3_finalize(statement);

			sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
			}
	
		if(root.count("payload_stored")>0) {
			// Mark all payload entries for which we have received confirmation from the 
			// server that they have been stored with 'server_status=1', to prevent
			// us from re-uploading them again. 

			sqlite3_exec(payload_db, "BEGIN TRANSACTION", NULL, NULL, NULL);

			sqlite3_stmt *statement=0;
			std::ostringstream ssc;
			ssc << "update payload set server_status=1 where id=?";
			int ret = sqlite3_prepare(payload_db, ssc.str().c_str(), -1, &statement, NULL);
			if(ret!=SQLITE_OK) 
				throw std::logic_error("Failed to prepare statement for on_client_message");
		
			const auto& entries=root["payload_stored"];
			for(auto entry: entries) {
				sqlite3_bind_int(statement, 1, entry.get<int>());
				sqlite3_step(statement);
				sqlite3_reset(statement);
				}
			sqlite3_finalize(statement);

			sqlite3_exec(payload_db, "END TRANSACTION", NULL, NULL, NULL);
			}
	
		if(root.count("authenticate")>0) {
			SNOOPDEBUG( std::cerr << "Received authentication response message from server." << std::endl; );
			const auto& auth=root["authenticate"];
			std::string ticket_uuid = auth.value("ticket_uuid", "");
			bool valid              = auth.value("valid", false);
			if(valid)
				set_local_ticket(ticket_uuid);
			else
				set_local_ticket("");
			authentication_callback(ticket_uuid, valid);
			authentication_callback=nullptr;
			}
		}
	catch(nlohmann::json::exception& ex) {
		SNOOPDEBUG( std::cerr << "Discarding message, JSON malformed: " << ex.what() << std::endl; );
		}
	}


Snoop::~Snoop()
   {
	// If a program runs for only a very short time, the connection to the logging 
	// server may not be open yet when we reach this destructor. In that case,
	// we will want to wait for the connection to open, so that we can do a final
	// sync before we terminate.

	// However, if the connection was attempted but failed (e.g. no server), we
	// can skip all that.

   // std::cerr << "|" << this_app_.app_name << "|" << std::endl;
	std::unique_lock<std::mutex> lock(connection_mutex);
	// std::cerr << "unlocked" << std::endl;
	
	if(this_app_.app_name!="" && this_app_.app_name!="SnoopServer") {
		if(connection_is_open==false && connection_attempt_failed==false) {
			connection_cv.wait(lock);
			}
		
		sync_with_server();
		
		if(db!=0) { // If the database is not open the wsclient won't be running either
			wsclient.stop();
			wsclient_thread.join();
			}
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
	if(payload_db!=0) {
		sqlite3_close(payload_db);
		payload_db=0;
		}
   }

void Snoop::set_local_type(const std::string& t)
   {
   local_types.insert(t);
   }

Snoop& Snoop::operator()(const std::string& type, std::string fl, int loc, std::string method) 
	{
	std::lock_guard<std::recursive_mutex> lock(call_mutex);

	assert(this_app_.app_name.size()>0);

	if(type!="") 
		this_log_.type=type;
	this_log_.loc_file=fl;
	this_log_.loc_line=loc;
	this_log_.loc_method=method;

	return *this;
	}

Snoop& Snoop::operator<<(const Flush&)
   {
    // SNOOPDEBUG(std::cerr << "================= Flush" << std::endl;)
	std::lock_guard<std::recursive_mutex> lock(call_mutex);
       
	assert(this_app_.app_name.size()>0);

	// Fill in the remaining fields of the LogEntry to be stored/sent.

	auto duration =  std::chrono::system_clock::now().time_since_epoch();
	auto millis   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	this_log_.create_millis = millis;
	this_log_.create_timezone = local_utc_offset_minutes();
	this_log_.message       = out_.str();
	this_log_.server_status = 0;
	this_log_.session_uuid  = session_uuid_;

	store_log_entry(this_log_, false);
	if(sync_immediately_)
		sync_logs_with_server();

   out_.str("");

	this_log_.loc_file="";
	this_log_.loc_line=-1;
	this_log_.loc_method="";
	this_log_.type="";
	this_log_.message="";

	return *this;
   }

Snoop& Snoop::payload(const std::vector<char>& data)
   {
	std::lock_guard<std::recursive_mutex> lock(call_mutex);

	assert(this_app_.app_name.size()>0);

	// Fill in the remaining fields of the PayLoad to be stored/sent.

	Snoop::PayLoad pl(data);
	auto duration =  std::chrono::system_clock::now().time_since_epoch();
	auto millis   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	pl.create_millis   = millis;
	pl.create_timezone = local_utc_offset_minutes();
	pl.server_status   = 0;

	store_payload_entry(pl);
	sync_payloads_with_server();

	return *this;
   }

Snoop::LogEntry::LogEntry()
	: log_id(0), client_log_id(0), id(0), create_millis(0), receive_millis(0), loc_line(0), server_status(0), create_timezone(-1)
	{
	}

Snoop::LogEntry::LogEntry(int log_id_, int client_log_id_, int c1, const std::string& c1b,
								  uint64_t c2, uint64_t c2b, const std::string& c3, int c4, const std::string& c5,
								  const std::string& c6, const std::string& c7, int c8, const std::string& c9, int create_timezone_)
	: log_id(log_id_), client_log_id(client_log_id_), id(c1), uuid(c1b), 
	  create_millis(c2), receive_millis(c2b), loc_file(c3), loc_line(c4), loc_method(c5), 
	  type(c6), message(c7), server_status(c8), session_uuid(c9), create_timezone(create_timezone_)
	{
	}

std::string Snoop::LogEntry::to_json(bool human_readable) const
	{
	nlohmann::json json;
	
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
	json["create_millis"]=create_millis;
	json["create_timezone"]=create_timezone;
	json["receive_millis"]=receive_millis;
	json["loc_file"]=loc_file;
	json["loc_line"]=loc_line;
	json["loc_method"]=loc_method;
	json["type"]=type;
	json["message"]=message;
	json["server_status"]=server_status;
	json["session_uuid"]=session_uuid;

	std::ostringstream str;
	str << json;

//	SNOOPDEBUG( std::cerr << "Snoop::LogEntry::to_json: " << str.str() << std::endl; )
	
	return str.str();
	}

Snoop::AppEntry::AppEntry()
	: id(0), create_millis(0), receive_millis(0), pid(0), server_status(0), connected(false), create_timezone(-1)
	{
	}

Snoop::AppEntry::AppEntry(const std::string& uuid_, uint64_t create_millis_, uint64_t receive_millis_, uint64_t pid_, 
								  const std::string& ip_address_, const std::string& machine_id_, 
								  const std::string& app_name_,   const std::string& app_version_,
								  const std::string& user_id_,
								  int server_status_, int create_timezone_)
	: uuid(uuid_), create_millis(create_millis_), receive_millis(receive_millis_), pid(pid_), ip_address(ip_address_),
	  machine_id(machine_id_), app_name(app_name_), app_version(app_version_), 
	  user_id(user_id_), server_status(server_status_), create_timezone(create_timezone_)
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
		 << ", \"create_timezone\": " << create_timezone
		 << ", \"receive_millis\": " << receive_millis
		 << ", \"pid\": " << pid 
		 << ", \"ip_address\": \"" << ip_address << "\""
		 << ", \"machine_id\": \"" << machine_id << "\""
		 << ", \"app_name\": \"" << app_name << "\""
		 << ", \"app_version\": \"" << app_version << "\""
		 << ", \"user_id\": \"" << user_id << "\""
		 << ", \"server_status\": " << server_status
		 << ", \"connected\": " << connected
		 << "}";

	return str.str();
	}

Snoop::PayLoad::PayLoad()
	{
	client_payload_id=-1;
	auto duration =  std::chrono::system_clock::now().time_since_epoch();
	receive_millis   = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	}

Snoop::PayLoad::PayLoad(const std::vector<char>& data)
	: PayLoad()
	{
	payload=snoop_base64_encode(reinterpret_cast<const unsigned char*>(data.data()), data.size());
	}

std::string Snoop::PayLoad::to_json(bool) const
	{
	nlohmann::json json;

	json["payload_id"]=payload_id;
	json["id"]=id;
	json["create_millis"]=create_millis;
	json["create_timezone"]=create_timezone;
	json["receive_millis"]=receive_millis;
	json["payload"]=payload;
	json["server_status"]=server_status;
	json["uuid"]=uuid;
	
	std::ostringstream str;
	str << json;

	return str.str();
	}

void Snoop::LogEntry::from_json(const nlohmann::json& entry) 
	{
	// std::cerr << "logentry: " << entry << std::endl;

	if(entry.count("log_id")==0 ||
		entry.count("id")==0 ||
		entry.count("uuid")==0 ||
		entry.count("type")==0 ||
		entry.count("message")==0) {
		throw ParseError("Incomplete log entry");
		}
													  
	try {
		log_id          = entry["log_id"].get<int>();
		id              = entry["id"].get<int>();
		uuid            = entry["uuid"].get<std::string>();
		if(entry.count("create_millis")>0)
			create_millis   = entry["create_millis"].get<uint64_t>();
		loc_file        = entry.value("loc_file", "");
		loc_line        = entry.value("loc_line", 0);
		loc_method      = entry.value("loc_method", "");
		type            = entry["type"].get<std::string>();
		message         = entry.value("message", "");
		server_status   = entry.value("server_status", 0);
		session_uuid    = entry.value("session_uuid", "");
		create_timezone = entry.value("create_timezone", -1);
		}
	catch(nlohmann::json::exception& ex) {
		SNOOPDEBUG( std::cerr << "Snoop::LogEntry::from_json: " << ex.what() << std::endl; );		
		}

	// std::cerr << "LogEntry parsed" << std::endl;
	}

void Snoop::AppEntry::from_json(const nlohmann::json& entry)
	{
	// std::cerr << entry.dump() << std::endl;
	
	try {
		id              = entry["id"].get<int>();
		uuid            = entry["uuid"].get<std::string>();
		create_millis   = entry["create_millis"].get<uint64_t>();
		if(entry.count("pid")>0)
			pid             = entry["pid"].get<uint64_t>();
		if(entry.count("ip_address")>0) 
			ip_address      = entry["ip_address"].get<std::string>();
		
		machine_id      = entry["machine_id"].get<std::string>();
		app_name        = entry["app_name"].get<std::string>();
		app_version     = entry["app_version"].get<std::string>();
		if(entry.count("user_id")>0) 
			user_id         = entry["user_id"].get<std::string>();
		if(entry.count("server_status")>0)
			server_status   = entry["server_status"].get<int>();
		
		create_timezone = entry.value("create_timezone", -1);
		}
	catch(nlohmann::json::exception& ex) {
		SNOOPDEBUG( std::cerr << "Snoop::AppEntry::from_json: " << ex.what() << std::endl; );		
		}

	// std::cerr << "AppEntry parsed" << std::endl;
	}

void Snoop::PayLoad::from_json(const nlohmann::json& entry)
	{
	try {
		payload_id      = entry["payload_id"].get<int>();
		id              = entry["id"].get<int>();
		create_millis   = entry["create_millis"].get<uint64_t>();
		payload         = entry["payload"].get<std::string>();
		server_status   = entry["server_status"].get<int>();
		create_timezone = entry.value("create_timezone", -1);
		}
	catch(nlohmann::json::exception& ex) {
		SNOOPDEBUG( std::cerr << "Snoop::LogEntry::from_json: " << ex.what() << std::endl; );		
		}
	}

bool Snoop::add_user(std::string user, std::string password, bool single)
   {
	std::lock_guard<std::recursive_mutex> lock(sqlite_mutex);

	assert(auth_db!=0);

#ifdef ENCRYPT_PASSWORDS
	char outbuf[SCRYPT_MCF_LEN+1];
	// std::cerr << SCRYPT_N << ", " << SCRYPT_r << ", " << SCRYPT_p << ", " << SCRYPT_MCF_LEN << std::endl;
   libscrypt_hash(outbuf, password.c_str(), SCRYPT_N, SCRYPT_r, SCRYPT_p);
#else
	const char *outbuf = password.c_str();
#endif
	
	// If `single`, flush the database.
	if(single) {
		int res = sqlite3_exec(auth_db, "delete from users", NULL, NULL, NULL);
		if(res!=SQLITE_OK) {
			SNOOPDEBUG( std::cerr << "Snoop::add_user: problem erasing users table." << std::endl; )
			}
		}
	
	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	ss << "insert into users (username, password) values (?,?)";
	int res = sqlite3_prepare(auth_db, ss.str().c_str(), -1, &statement, NULL);
	assert(res==SQLITE_OK);
	sqlite3_bind_text(statement, 1, user.c_str(), user.size(), 0);
	sqlite3_bind_text(statement, 2, outbuf, strlen(outbuf), 0);		
	res=sqlite3_step(statement);
	sqlite3_finalize(statement);

	if(res!=SQLITE_DONE) {
		SNOOPDEBUG( std::cerr << "Snoop::add_user: error " << sqlite3_errmsg(db) << std::endl; )
		return false;
		}
	
   return true;
   }

std::string Snoop::local_user() const
	{
	std::string name="";

	sqlite3_stmt *statement=0;
	std::ostringstream ss;
	int res = sqlite3_prepare(auth_db, "select username from users where username!=\"\"", -1, &statement, NULL);
	if(res!=SQLITE_OK)
		return "";
	
	int ret = sqlite3_step(statement);
	if(ret==SQLITE_ROW) {
		name = safestring(sqlite3_column_text(statement, 0));
		SNOOPDEBUG( std::cerr << "Snoop::init: localUser = " << name << std::endl; );		
		}
	else {
		SNOOPDEBUG( std::cerr << "Snoop::init: no localUser set (yet)" << std::endl; );		
		}
	
	sqlite3_finalize(statement);
	return name;
	}


static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string snoop_base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}
std::string snoop_base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

