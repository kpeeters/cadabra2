
#pragma once

// ComputeThread is the base class which takes care of doing actual
// computations with the cells in a document. It handles talking to
// the server backend. It knows how to pass cells to the server and
// ask them to be executed. Results are reported back to the GUI by
// putting ActionBase objects onto its todo stack.

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/functional.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> WSClient;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

#include "DataCell.hh"

namespace cadabra {
	
	class GUIBase;
	class DocumentThread;
	
	class ComputeThread {
		public:
			// If the ComputeThread is constructed with a null pointer to the
			// gui, there will be no gui updates, just DTree updates.
			
			ComputeThread(GUIBase *, DocumentThread&);
			ComputeThread(const ComputeThread& )=delete; // You cannot copy this object
            ComputeThread(const ComputeThread&&);
			~ComputeThread();
			
			// Main entry point, which will connect to the server and
			// then start an event loop to handle communication with the
			// server. Only terminates when the connection drops, so run
			// your GUI on a different thread.

			void run(); 
			
			// In order to execute code on the server, call the following
			// from the GUI thread.  This method returns as soon as the
			// request has been put on the network queue.  The
			// ComputeThread will report the result of the computation by
			// adding actions to the DocumentThread owned pending_actions
			// stack, by calling queue_action.

			void execute_cell(const DataCell&);

		private:
			GUIBase        *gui;
			DocumentThread& docthread;

			// WebSocket++ things.
			WSClient wsclient;
			bool     connection_is_open;
			WSClient::connection_ptr    connection;
			websocketpp::connection_hdl our_connection_hdl;
			void init();
			void try_connect();
			void on_open(websocketpp::connection_hdl hdl);
			void on_fail(websocketpp::connection_hdl hdl);
			void on_close(websocketpp::connection_hdl hdl);
			void on_message(websocketpp::connection_hdl hdl, message_ptr msg);
	};

}
