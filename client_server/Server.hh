
#pragma once

#include <string>
#include <set>
#include <deque>
#include <signal.h>
#include <boost/uuid/uuid.hpp>
#include <future>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include "nlohmann/json.hpp"
#include "websocket_server.hh"

#include "Stopwatch.hh"

/// \ingroup clientserver
///
/// Object representing a Cadabra server, capable of receiving messages
/// on a websocket, running Python code, and sending output back to the
/// client.
///
/// Contains the logic to intercept raw Python output but also provides
/// functionality to the Python side which enables clients to send
/// various objects in JSON encoded form. See Server::on_block_finished
/// for the format of these messages.
///
/// Every block is run on the same Python scope. They run sequentially,
/// one at a time, on a thread separate from the websocket++ main loop.
/// When the Python code finishes (or when it is interrupted), this thread
/// locks the socket_mutex and calls on_block_finished().

class Server {
	public:
		Server();
		Server(const Server&)=delete;
		Server(const std::string& socket);
		virtual ~Server();

		/// The only user-visible part: just instantiate a server object and
		/// start it with run(). This will not return until the server has
		/// been shut down. Picks a random port when port==0. If
		/// `exit_on_disconnect==false`, keep the server alive on the
		/// same port and with the same authentication token when
		/// the connection drops (so you can reconnect).

		void run(int port=0, bool exit_on_disconnect=true);


		/// Python output catching. We implement this in a C++ class
		/// because we want to be able to catch each line of output
		/// separately, and perhaps add additional functionality to send
		/// out-of-band messages to the client later.

		class CatchOutput {
			public:
				CatchOutput();
				CatchOutput(const CatchOutput&);

				void        write(const std::string& txt);
				void        clear();
				std::string str() const;
			private:
				std::string collect;
			};

		CatchOutput catchOut, catchErr;

		Stopwatch server_stopwatch;
		Stopwatch sympy_stopwatch;

		/// Raw code to send a string (which must be JSON formatted) as
		/// a message to the client. Handles communication of the result
		/// back to the client in JSON format. This is always of the
		/// form
		///
		///      { "header":   { "parent_id":     "...",
		///                      "parent_origin": "client" | "server",
		///                      "cell_id":       "...",
		///                      "cell_origin":   "client" | "server"
		///                    },
		///        "content":  { "output":  "...",
		///                      "width":   int (optional),
		///                      "height":  int (optional)
      ///                    },
		///        "msg_type": "..."
		///      }
		///
		/// msg_type can be "output", "latex", "image_png" and so on,
		/// corresponding to the possible values of DataCell::CellType.
		///
		/// Returns the serial number of the new cell sent.

		virtual uint64_t         send(const std::string& output, const std::string& msg_type,
												uint64_t parent_id=0, uint64_t cell_id=0, bool last_in_sequence=false);

		void                     send_progress_update(const std::string& msg, int n, int total);
		void                     send_json(const std::string&);

		bool                     handles(const std::string& otype) const;
		std::string              architecture() const;

		/// Thread entry point for the code that waits for blocks to
		/// appear on the block queue, and executes them in turn.
		/// In practice we run this on the main thread.
		void wait_for_job();

		/// Thread entry point for code that sets up and runs the
		/// websocket listener.
		void wait_for_websocket();

	protected:
		void init();

		// WebSocket++ dependent parts below.
		void on_message(websocket_server::id_type id, const std::string& msg,
							 const websocket_server::request_type& req, const std::string& ip_address);
		void on_open(websocket_server::id_type id);
		void on_close(websocket_server::id_type id);
		websocket_server wserver;

		// Connection tracking.  There can be multiple connections to
		// the server, but they all have access to the same Python
		// scope. With multiple connections, one can inspect the Python
		// stack from a different client (e.g. for debugging purposes).
		// All connections share the same authentication token.

		class Connection {
			public:
				Connection();

				websocket_server::id_type ws_id;
				boost::uuids::uuid        uuid;
			};
		typedef std::map<websocket_server::id_type, Connection> ConnectionMap;
		ConnectionMap connections;
		
		// Authentication token, needs to be sent along with any message.
		// Gets set when the server announces its port.
		std::string  authentication_token;

		// Mutex to be able to use the websocket layer from both the
		// main loop and the python-running thread.
		std::mutex ws_mutex;


		// Basics for the working thread that processes blocks.
		std::thread             runner;
		std::mutex              block_available_mutex;
		std::condition_variable block_available;
		bool                    exit_on_disconnect;
		int                     run_on_port;
		unsigned long           main_thread_id;

		// Data and connection info for a single block of code.
		class Block {
			public:
				Block();
				Block(websocket_server::id_type, const std::string&, uint64_t id, const std::string& msg_type);
				
				websocket_server::id_type   ws_id; // FIXME: decouple from websocket?
				std::string                 msg_type;
				std::string                 input;
				std::string                 output;
				std::string                 error;
				uint64_t                    cell_id;
				std::set<std::string>       variables;
				std::set<std::string>       remove_variable_assignments;

				// When a cell is re-run on variable change, we re-use the output cells of the
				// previous run. The IDs of these cells are sent to us by the frontend. We
				// store them here, and then pop them off the front for each call to `send`.

				std::deque<uint64_t>        reuse_output_cell_ids; 

				// Response message, partially filled in when the
				// request comes in.

				nlohmann::json              response;
			};
		std::queue<Block>           block_queue;
		Block                       current_block;
		websocket_server::id_type   current_ws_id;
		uint64_t                    current_id;    // id of the block given to us by the client.

		// Run a piece of Python code. This is called from a separate
		// thread constructed by on_message().
		std::string              run_string(const std::string&,
														bool handle_output=true,
														bool extract_variables=false,
														std::set<std::string> remove_variable_assignments=std::set<std::string>());

		std::set<std::string> run_string_variables;

		/// Called by the run_block() thread upon completion of the
		/// task. This will send any output generated by printing directly
		/// to stdout or stderr from Python (so, output not generated by
		/// using the 'display' function). Indicates to the client that
		/// this block has finished executing. Will send an empty string
		/// if there has been no output 'print'ed.

		virtual void             on_block_finished(Block);
		virtual void             on_block_error(Block);
		virtual void             on_kernel_fault(Block);

		/// Halt the currently running block and prevent execution of any
		/// further blocks that may still be on the queue.

		void                     stop_block();
		bool                     started;
		std::future<std::string> job;

		/// Takes a JSON encoded message and performs the required action to process it.
		/// Where applicable these messages are compatible with IPython's message types,
		/// http://ipython.org/ipython-doc/dev/development/messaging.html

		void dispatch_message(websocket_server::id_type, const std::string& json_string);

		// Python global info.
		pybind11::scoped_interpreter guard;
		pybind11::module             main_module;
		pybind11::object             main_namespace;
	};
