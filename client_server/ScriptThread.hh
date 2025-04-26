
#pragma once

#include "websocket_server.hh"


namespace cadabra {

	/// \ingroup scripting
	///
   /// Class containing all functionality which allows users to
	/// control a running notebook client by sending it commands
	/// via a websocket port. This can be used to script its
	/// operations.
	///
	/// This runs as a separate thread, so it is neither on the
	/// main thread (GUI, DocumentThread) nor on the thread that
	/// handles communication with the cadabra-server.

	class DocumentThread;
	class GUIBase;
	
	class ScriptThread {
		public:
			ScriptThread(DocumentThread *, GUIBase *);
			~ScriptThread();

			ScriptThread(const ScriptThread&) = delete;
			
			void run();
			void terminate();

			uint16_t    get_local_port() const;
			std::string get_authentication_token() const;
			
		private:
			void on_message(websocket_server::id_type id, const std::string& msg,
								 const websocket_server::request_type& req, const std::string& ip_address);
			void on_open(websocket_server::id_type id);
			void on_close(websocket_server::id_type id);
			
			websocket_server  wserver;
			DocumentThread   *document;
			GUIBase          *gui;

			// Authentication token, needs to be sent along with any message.
			// Gets shown when we start up.
			mutable std::mutex url_mutex;
			std::string        authentication_token;
			uint16_t           local_port;
	};
	
}
