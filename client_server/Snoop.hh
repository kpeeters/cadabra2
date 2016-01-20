
/*

   Snoop v1.0 
   Copyright (C) 2015  Kasper Peeters
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
#include <json/json.h>

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
						 const std::string& local_file, std::string server="");

			/// Operator to initialise a logging entry with the type of
			/// the log message as well as (optionally) the file, line
			/// number and method.

			Snoop& operator()(const std::string& type, std::string fl="", int loc=-1, std::string method="");

			/// Generic operator to log an object to the log message being constructed.

			template<class T>
			Snoop& operator<<(const T& obj) {
			   out_ <<(obj);
			   return *this;
			}
         
			/// Flush the log entry to disk/server.

         Snoop& operator<<(const Flush&);

			/// Set to sync with server after every log line.

			void set_sync_immediately(bool);

			/// Ensure that the local database is synchronised with the
			/// server (this sends multiple app or log entries in one
			/// websocket message). Leave the bool argument at its
			/// default argument under all normal circumstances.

			void sync_with_server(bool from_wsthread=false);
			
			/// As above, but only for app entries. 

			void sync_apps_with_server(bool from_wsthread=false);
			
			/// As above, but only for log entries. 

			void sync_logs_with_server(bool from_wsthread=false);
			

			/// C++ representation of an app entry.

			class AppEntry {
				public:
					AppEntry();
					AppEntry(const std::string& uuid_, uint64_t create_millis_, uint64_t receive_millis_, pid_t pid_, 
								const std::string& ip_address_, const std::string& machine_id_, 
								const std::string& app_name_,   const std::string& app_version_,
								int server_status_);

					std::string to_json(bool human_readable) const;
					void        from_json(const Json::Value&);

					int         id;
					std::string uuid;
					uint64_t    create_millis;
					uint64_t    receive_millis;
					pid_t       pid;
					std::string ip_address;
					std::string machine_id;
					std::string app_name;
					std::string app_version;
					int         server_status; // 1: synced, 0 and negative: number of attempts at syncing made
					bool        connected;
			};

			/// C++ representation of a log entry.

			class LogEntry {
				public:
					LogEntry();
					LogEntry(int log_id_, int client_log_id_, int id_, const std::string&, 
								uint64_t, uint64_t, const std::string&, int, const std::string&, 
								const std::string& , const std::string&, int status);

					std::string to_json(bool human_readable) const;
					void        from_json(const Json::Value&);
					
					int         log_id;
					int         client_log_id;
					int         id;
					std::string uuid;
					uint64_t    create_millis;
					uint64_t    receive_millis;
					std::string loc_file;
					int         loc_line;
					std::string loc_method;
					std::string type;
					std::string message;
					int         server_status; // 1: synced, 0 and negative: number of attempts at syncing made
			};

      protected:			
			std::ostringstream out_;
			bool               sync_immediately_;

			SnoopImpl *impl;
			friend SnoopImpl;

   };

	extern Snoop log;

	const char info[] ="info";
	const char warn[] ="warning";
	const char error[]="error";
	const char fatal[]="fatal";
	const char email[]="email";
}

#define LOC __FILE__, __LINE__, __func__

