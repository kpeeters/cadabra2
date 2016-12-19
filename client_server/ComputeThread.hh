
#pragma once

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/functional.hpp>
#include <thread>

typedef websocketpp::client<websocketpp::config::asio_client> WSClient;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

#include "DataCell.hh"

namespace cadabra {
	
	class GUIBase;
	class DocumentThread;

	/// \ingroup clientserver
	///
	/// Base class which talks to the server and sends Action objects back to the 
	/// DocumentThread.
	///
   /// ComputeThread is the base class which takes care of doing actual
   /// computations with the cells in a document. It handles talking to
   /// the server backend. It knows how to pass cells to the server and
   /// ask them to be executed. Results are reported back to the GUI by
   /// putting ActionBase objects onto its todo stack. ComputeThread never
	/// directly modifies the document tree.
	
	class ComputeThread {
		public:
			/// If the ComputeThread is constructed with a null pointer to the
			/// gui, there will be no gui updates, just DTree updates.
			
			ComputeThread();
			ComputeThread(const ComputeThread& )=delete; // You cannot copy this object
			~ComputeThread();

			/// Determine the objects that this compute thread should be 
			/// talking to.
			void set_master(GUIBase *, DocumentThread *);
			
			/// Main entry point, which will connect to the server and
			/// then start an event loop to handle communication with the
			/// server. Only terminates when the connection drops, so run
			/// your GUI on a different thread.

			void run(); 
			
			/// In order to execute code on the server, call the
			/// following from the GUI thread.  This method returns as
			/// soon as the request has been put on the network queue. If
			/// no communication with the server is necessary, this
			/// returns immediately. The ComputeThread will report the
			/// result of the computation/processing by adding actions to
			/// the DocumentThread owned pending_actions stack, by
			/// calling queue_action. It will never modify the cell
			/// directly, and will also never modify any other cells in
			/// the document tree.

			void execute_cell(DTree::iterator);

			/// Stop the current cell execution on the server and remove
			/// all other cells from the run queue as well.

			void stop();

			/// Restart the kernel.

			void restart_kernel();

			// Determine if there are still cells running on the server.
			// FIXME: this does not guarantee thread-safety but at the moment
			// is only used for updating status bars etc.
			// FIXME: can be moved to DocumentThread.

			int number_of_cells_executing(void) const;

			/// Terminate the compute thread, in preparation for shutting
			/// down the client altogether.

			void terminate();
			
		private:
			GUIBase        *gui;
			DocumentThread *docthread;

			// For debugging purposes, we keep record of the gui thread id,
			// so that we can flag when code runs on the wrong thread.
			// Gets initialised in the ComputeThread constructor.
			std::thread::id  gui_thread_id; 

			// Keeping track of cells which are running on the server, in
			// a form which allows us to look them up quickly based only
			// on the id (which is all that the server knows about). 

			std::map<DataCell::id_t, DTree::iterator> running_cells;

			// WebSocket++ things.
			WSClient wsclient;
			bool     connection_is_open, restarting_kernel;
			WSClient::connection_ptr    connection;
			websocketpp::connection_hdl our_connection_hdl;
			void init();
			void try_connect();
			void try_spawn_server();
			void on_open(websocketpp::connection_hdl hdl);
			void on_fail(websocketpp::connection_hdl hdl);
			void on_close(websocketpp::connection_hdl hdl);
			void on_message(websocketpp::connection_hdl hdl, message_ptr msg);

			DTree::iterator find_cell_by_id(DataCell::id_t, bool remove);
			
			/// Set all cells to be non-running (e.g. after a kernel failure) and
			/// report the status of each cell to the GUI.
			void all_cells_nonrunning();


			// Self-started server
			pid_t           server_pid;
			int             server_stdout, server_stderr;
			unsigned short  port;
	};

}
