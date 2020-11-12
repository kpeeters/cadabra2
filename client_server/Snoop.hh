
/*

   Snoop
   Copyright (C) 2015-2020  Kasper Peeters
   Available under the terms of the GPL v3.

   Snoop is a lightweight logging library which stores its log entries in
	a local SQLite database or on a remote server.

 */

#pragma once

#include <string>
#include <sstream>
#include <sqlite3.h>
#include <stdint.h>
#include <mutex>
#include "nlohmann/json.hpp"
#include <thread>
#include <set>
#ifdef SNOOP_SSL
  #include <websocketpp/config/asio_client.hpp>
#else
  #include <websocketpp/config/asio_no_tls_client.hpp>
#endif
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/functional.hpp>

#ifndef _MSC_VER
  #include <unistd.h>
#endif

#ifdef SNOOP_SSL
   typedef websocketpp::client<websocketpp::config::asio_tls_client> WebsocketClient;
#else
   typedef websocketpp::client<websocketpp::config::asio_client> WebsocketClient;
#endif
typedef websocketpp::config::asio_client::message_type::ptr   message_ptr;

std::string safestring(const unsigned char *c);

namespace snoop {

	class SnoopImpl;
   class Flush {};
   extern Flush flush;

	/// Logging class with functionality to send log information to a
	/// remote server using a websocket connection.

   class Snoop {
      public:
         Snoop();
         ~Snoop();

			/// Initialise the logging stream. Should be called once at
         /// program startup, but can be called multiple times without
         /// causing problems.

			void init(const std::string& app_name, const std::string& app_version, 
                      std::string server="", std::string local_log_file="", std::string machine_id="");

			/// Get a string which uniquely identifies the current user. This is
			/// stored in ~/.config/snoop/appname.conf, and in the 'user_id' field
			/// in each LogEntry. Note that this is different from the 'uuid' field,
			/// which will change from one run to the next.

			std::string get_user_uuid(const std::string& app_name);

			/// Operator to initialise a logging entry with the type of
			/// the log message as well as (optionally) the file, line
			/// number and method.

			Snoop& operator()(const std::string& type="", std::string fl="", int loc=-1, std::string method="");

         /// Determine the 'type' field of records which should not be
         /// sent to the remote logging server. Can be called multiple times.

         void set_local_type(const std::string& type);

         /// Generic operator to log an object to the log message being constructed.

			template<class T>
			Snoop& operator<<(const T& obj) {
			   out_ <<(obj);
			   return *this;
			}

			/// Log payload data.

			Snoop& payload(const std::vector<char>&);
			
			/// Flush the log entry to disk/server.

         Snoop& operator<<(const Flush&);

			/// Set to sync with server after every log line.

			void set_sync_immediately(bool);

			/// Ensure that the local database is synchronised with the
			/// server (this sends multiple app or log entries in one
			/// websocket message). Leave the bool argument at its
			/// default argument under all normal circumstances.

			void sync_with_server(bool from_wsthread=false);
			
			/// As above, but only for run entries. 

			void sync_runs_with_server(bool from_wsthread=false);
			
			/// As above, but only for log entries. 

			void sync_logs_with_server(bool from_wsthread=false);
			
			/// As above, but only for payload data. 

			void sync_payloads_with_server(bool from_wsthread=false);

         /// Are we connected to the log server?

         bool is_connected() const;         

         /// Return version of last run seen on given device.
         
         std::string last_seen_version(std::string machine_id);

         /// Authentication logic; passes ticket or credentials
         /// to server, and registers callback function for when
         /// the response comes back.

         bool authenticate(std::function<void (std::string, bool)>, std::string user="", std::string pass="");
       
         /// Get status of a given authentication ticket.

			class Ticket {
				public:
					Ticket();
					int         ticket_id;
					int         user_id;
					std::string ticket_uuid;
					bool        valid;
			};
         Ticket is_ticket_valid(std::string ticket_uuid);

         /// C++ representation of a run entry.

			class AppEntry {
				public:
					AppEntry();
					AppEntry(const std::string& uuid_, uint64_t create_millis_, uint64_t receive_millis_, uint64_t pid_, 
								const std::string& ip_address_, const std::string& machine_id_, 
								const std::string& app_name_,   const std::string& app_version_,
								const std::string& user_id_,
								int server_status_, int create_timezone);

					std::string to_json(bool human_readable) const;
					void        from_json(const nlohmann::json&);

					int         id;
					std::string uuid;
					uint64_t    create_millis;
					uint64_t    receive_millis;
					uint64_t    pid;
					std::string ip_address;
					std::string machine_id;
					std::string app_name;
					std::string app_version;
					std::string user_id;
					int         server_status; // 1: synced, 0 and negative: number of attempts at syncing made
					bool        connected;
					int         create_timezone;					
			};

			/// C++ representation of a log entry.

			class LogEntry {
				public:
					LogEntry();
					LogEntry(int log_id_, int client_log_id_, int id_, const std::string&, 
								uint64_t, uint64_t, const std::string&, int, const std::string&, 
								const std::string& , const std::string&, int status, const std::string&,
								int create_timezone);

					std::string to_json(bool human_readable) const;
					void        from_json(const nlohmann::json&);
					
					int         log_id;
					int         client_log_id;
					int         id;
					std::string uuid;              // this goes on the wire, but is not stored on disk.
					uint64_t    create_millis;
					uint64_t    receive_millis;
					std::string loc_file;
					int         loc_line;
					std::string loc_method;
					std::string type;
					std::string message;
					int         server_status;     // 1: synced, 0 and negative: number of attempts at syncing made
					std::string session_uuid;
					int         create_timezone;
			};

			/// C++ representation of a payload entry.

			class PayLoad {
				public:
					PayLoad();
					PayLoad(const std::vector<char>& data);

					std::string to_json(bool human_readable) const;
					void        from_json(const nlohmann::json&);

					int         payload_id;
					int         client_payload_id;
					int         id;
					std::string uuid;              // this goes on the wire, but is not stored on disk.
					uint64_t    create_millis;
					uint64_t    receive_millis;
					std::string payload;
					int         server_status;     // 1: synced, 0 and negative: number of attempts at syncing made
					int         create_timezone;
			};
       
          /// Client-side fetching of ticket.
       
          std::string get_local_ticket();
       

      protected:
			/// Start the websocket client. This tries to connect to the server and then
         /// waits in a separate thread until the server sends us something (typically
         /// in response to something the main thread makes by calling wsclient.send).

			void start_websocket_client();

			/// Ensure that the required tables are present in the
			/// database file.

			void create_tables();

	      /// Ensure that the required authentication tables are present
	      /// in the authentication database. Only used on the server.

	      void create_authentication_tables();

			/// Obtain a uuid by finding the last AppEntry stored in the
			/// local database. Will attempt to re-turn a previously
			/// generated uuid but will do so only if one is stored for
			/// the current pid; if no entry with the current pid is
			/// stored then a new one will always be generated.

			void obtain_uuid();

			/// Store an app entry in the database. Will update the 'id'
			/// field in the AppEntry.

			bool store_app_entry(Snoop::AppEntry&);
			bool store_app_entry_without_lock(Snoop::AppEntry&);

			/// Store a log entry in the local database. Generates its
			/// own receive_millis field (the one given gets
			/// overwritten). Will update the 'id' field in the LogEntry.
			/// Returns 'true' if the entry was stored, or 'false' if an
			/// entry with this client_log_id was already present (except
			/// when it is 0).

			bool store_log_entry(Snoop::LogEntry&, bool avoid_server_duplicates);

			/// Store payload data in the local database.

			bool store_payload_entry(Snoop::PayLoad&);

			/// Store an attempt to login into the authentication database.
			
			bool store_auth_attempt_entry(int user_id, int ticket_id, int valid, std::string msg);
			
			/// Return a vector of all aps registered in the database. If
			/// the uuid filter is non-empty, will filter on the given
			/// uuid.

			std::vector<Snoop::AppEntry> get_app_registrations(std::string uuid_filter="");

         /// Store an authentication ticket in the database.
         
	      int store_ticket(std::string ticket_uuid, int user_id, bool valid);

         /// Client-side storing of ticket (simpler than store_ticket above).
         /// If ticket is empty, only deletes current ticket.

         void set_local_ticket(std::string ticket_uuid);
         
         /// Variables
         
         bool          sync_immediately_;
	      sqlite3      *db, *payload_db, *auth_db;
			sqlite3_stmt *insert_statement, *id_for_uuid_statement, *payload_insert_statement;
			std::recursive_mutex    sqlite_mutex;

      private:
			/// Websocket client to talk to a remote logging server.
			
			WebsocketClient                 wsclient;
			std::thread                     wsclient_thread;
			std::mutex                      connection_mutex;
			std::condition_variable         connection_cv;
			bool                            connection_is_open, connection_attempt_failed;
			WebsocketClient::connection_ptr connection;
			websocketpp::connection_hdl     our_connection_hdl;
			
			void on_client_open(websocketpp::connection_hdl hdl);
			void on_client_fail(websocketpp::connection_hdl hdl);
			void on_client_close(websocketpp::connection_hdl hdl);
			void on_client_message(websocketpp::connection_hdl hdl, message_ptr msg);

			std::ostringstream out_;
			
			Snoop::AppEntry    this_app_;
			Snoop::LogEntry    this_log_;
			std::string        server_;

         std::recursive_mutex   call_mutex;
			bool         secure;

	      std::set<std::string> local_types;
//	      std::set<std::string> ...
         std::function<void (std::string, bool)> authentication_callback;

   };

	extern Snoop log;

	const char info[] ="info";
	const char warn[] ="warning";
	const char error[]="error";
	const char fatal[]="fatal";
	const char email[]="email";
}

// set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst
//  ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

#define LOC __FILE__, __LINE__, __func__

