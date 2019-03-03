
#pragma once

#include "xeus/xinterpreter.hpp"
#include "Server.hh"

using xeus::xinterpreter;
using xeus::xjson;

namespace cadabra {

	/// \ingroup clientserver
	///
	/// A Jupyter kernel for Cadabra, which provides the Cadabra
	/// pre-processor to enable input as in the Gtk notebook frontend.
	/// Built using Xeus.

	class CadabraJupyter : public xinterpreter, public Server {
		public:

			CadabraJupyter();
			virtual ~CadabraJupyter() = default;

		public:
			virtual uint64_t send(const std::string& output, const std::string& msg_type, uint64_t parent_id, bool last) override;

			int current_counter;
			bool finished=true;

		private:

			void configure_impl() override;

			xjson execute_request_impl(int execution_counter,
			                           const std::string& code,
			                           bool silent,
			                           bool store_history,
			                           xjson user_expressions,
			                           bool allow_stdin) override;

			xjson complete_request_impl(const std::string& code,
			                            int cursor_pos) override;

			xjson inspect_request_impl(const std::string& code,
			                           int cursor_pos,
			                           int detail_level) override;

			xjson is_complete_request_impl(const std::string& code) override;

			xjson kernel_info_request_impl() override;

			void shutdown_request_impl() override;
		};
	}
