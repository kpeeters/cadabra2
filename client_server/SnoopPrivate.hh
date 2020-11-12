
#pragma once

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/functional.hpp>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <set>

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
         /// causing problems. The 'local_database_name' should be a full
			/// path but without extension; the main database name will be this
			/// path with '.sql' added, while the payload database will have
			/// '_payload.sql' at the end.

			void init(const std::string& app_name, const std::string& app_version, 
						 std::string server="", std::string local_database_name="");

			/// Get a string which uniquely identifies the current user. This is
			/// stored in ~/.config/snoop/appname.conf.

			std::string get_user_uuid(const std::string& app_name);

			/// Log payload data.
			Snoop& payload(const std::vector<char>&);

         /// Operator to initialise a logging entry with the type of
			/// the log message as well as (optionally) the file, line
			/// number and method. Thread-safe: can be called from
			/// different threads at the same time.

			Snoop& operator()(const std::string& type, std::string fl="", int loc=-1, std::string method="");

         void set_local_type(const std::string& type);

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

         bool is_ticket_valid(std::string ticket_uuid);

         /// Get status of given user/pass combo. This queries the
         /// underlying authentication database table. If the login
         /// is valid, a ticket will be inserted in the database and
         /// returned. An empty string is returned if login is invalid.

         std::string ticket_for_login(std::string user, std::string password);
         
         /// Add a user/password combo to the user database.

         bool add_user(std::string user, std::string password);

	      Snoop             *snoop_;

	      sqlite3           *db, *payload_db, *auth_db;
			
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
			
			/// Return a vector of all aps registered in the database. If
			/// the uuid filter is non-empty, will filter on the given
			/// uuid.

			std::vector<Snoop::AppEntry> get_app_registrations(std::string uuid_filter="");

			/// Prepared statements (only need to prepare these once).
			
			sqlite3_stmt *insert_statement, *id_for_uuid_statement, *payload_insert_statement;
			std::mutex    sqlite_mutex;

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

		private:
			std::mutex   call_mutex;
			bool         secure;

	      std::set<std::string> local_types;
//	      std::set<std::string> ...
         std::function<void (std::string, bool)> authentication_callback;

	      void store_ticket(std::string ticket_uuid, int user_id, bool valid);
	};
}
