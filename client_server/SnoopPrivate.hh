
#pragma once

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/functional.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> WebsocketClient;
typedef websocketpp::config::asio_client::message_type::ptr   message_ptr;

namespace snoop {
	
	class SnoopImpl {
		public:
			SnoopImpl(Snoop *);
			~SnoopImpl();

         Snoop& operator<<(const Flush&);

			/// Initialise the logging stream. Should be called once at
         /// program startup, but can be called multiple times without
         /// causing problems.

			void init(const std::string& app_name, const std::string& app_version, 
						 const std::string& local_file, std::string server="");

			/// Operator to initialise a logging entry with the type of
			/// the log message as well as (optionally) the file, line
			/// number and method.

			Snoop& operator()(const std::string& type, std::string fl="", int loc=-1, std::string method="");

			/// Ensure that the local database is synchronised with the
			/// server (this sends multiple app or log entries in one
			/// websocket message). Leave the bool argument at its
			/// default argument under all normal circumstances.

			void sync_with_server(bool from_wsthread=false);
			
			/// As above, but only for app entries. 

			void sync_apps_with_server(bool from_wsthread=false);
			
			/// As above, but only for log entries. 

			void sync_logs_with_server(bool from_wsthread=false);

			Snoop             *snoop_;

			sqlite3           *db;
			
			Snoop::AppEntry    this_app_;
			Snoop::LogEntry    this_log_;
			std::string        server_;

			/// Start the websocket client. This tries to connect to the server and then
         /// waits in a separate thread until the server sends us something (typically
         /// in response to something the main thread makes by calling wsclient.send).

			void start_websocket_client();

			/// Ensure that the required tables are present in the
			/// database file.

			void create_tables();

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

			void store_log_entry(Snoop::LogEntry&);

			/// Return a vector of all aps registered in the database. If
			/// the uuid filter is non-empty, will filter on the given
			/// uuid.

			std::vector<Snoop::AppEntry> get_app_registrations(std::string uuid_filter="");

			/// Prepared statements (only need to prepare these once).
			
			sqlite3_stmt *insert_statement, *id_for_uuid_statement;
			std::mutex    sqlite_mutex;

			/// Websocket client to talk to a remote logging server.
			
			WebsocketClient                 wsclient;
			std::thread                     wsclient_thread;
			bool                            connection_is_open;
			WebsocketClient::connection_ptr connection;
			websocketpp::connection_hdl     our_connection_hdl;
			
			void on_client_open(websocketpp::connection_hdl hdl);
			void on_client_fail(websocketpp::connection_hdl hdl);
			void on_client_close(websocketpp::connection_hdl hdl);
			void on_client_message(websocketpp::connection_hdl hdl, message_ptr msg);
	};
}
