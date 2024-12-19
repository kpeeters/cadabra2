#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <functional>
#include <memory>
#include <string>
#include <queue>
#include <regex>

class websocket_client {
	public:
		// Callback handlers
		using message_handler = std::function<void(const std::string&)>;
		using connect_handler = std::function<void()>;
		using close_handler = std::function<void()>;
		using fail_handler = std::function<void(const boost::beast::error_code&)>;

		websocket_client();
		~websocket_client();

		// No copying
		websocket_client(const websocket_client&) = delete;
		websocket_client& operator=(const websocket_client&) = delete;

		// Set handlers (all optional)
		void set_message_handler(message_handler h);
		void set_connect_handler(connect_handler h);
		void set_close_handler(close_handler h);
		void set_fail_handler(fail_handler h);

		// Async operations (all return immediately)
		void connect(const std::string& uri);  // ws:// or wss://
		void send(const std::string& message);
		void close();
    
		void run();
		void stop();

	private:
		void on_resolve(const boost::beast::error_code& ec, 
							 boost::asio::ip::tcp::resolver::results_type results);
		void on_connect(const boost::beast::error_code& ec);
		void on_ssl_handshake(const boost::beast::error_code& ec);
		void on_handshake(const boost::beast::error_code& ec);
		void on_write(const boost::beast::error_code& ec, std::size_t bytes_transferred);
		void on_read(const boost::beast::error_code& ec, std::size_t bytes_transferred);
		void on_close(const boost::beast::error_code& ec);
		void do_read();
		void do_write();
		void fail(const boost::beast::error_code& ec);

		// State
		boost::asio::io_context        ioc_;
		boost::asio::ssl::context      ssl_ctx_;
		boost::asio::ip::tcp::resolver resolver_;
		std::unique_ptr<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>> wss_stream_;
		std::unique_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws_stream_;
		boost::beast::flat_buffer buffer_;
		bool is_ssl_;
		std::string host_, port_, path_;

		// Handlers
		message_handler message_handler_;
		connect_handler connect_handler_;
		close_handler   close_handler_;
		fail_handler    fail_handler_;

		// Message queue.
		struct queued_message {
				std::string data;
				std::shared_ptr<boost::beast::flat_buffer> buffer;
		};
		std::queue<queued_message> message_queue_;

		bool writing_{false};
};


class Uri {
	public:
		Uri(const std::string& uri);

		std::string to_string() const;

		std::string protocol;
		std::string host;
		std::string port;
		std::string path;

};
