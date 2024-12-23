#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>
#include <memory>
#include <string>
#include <queue>
#include <optional>
#include <unordered_map>
#include <iostream>
#include <pthread.h>

class websocket_server {
	public:
		using id_type = std::size_t;
		using request_type  = boost::beast::http::request<boost::beast::http::string_body>;
		using response_type = boost::beast::http::response<boost::beast::http::string_body>;
		
		using message_handler = std::function<void(id_type, const std::string&, const request_type&, const std::string& ip_address)>;
		using connect_handler = std::function<void(id_type)>;
		using disconnect_handler = std::function<void(id_type)>;
		using http_handler = std::function<void(request_type&, response_type&)>;

		websocket_server() = default;
		explicit websocket_server(uint16_t port);
		~websocket_server();

		websocket_server(const websocket_server&) = delete;
		websocket_server& operator=(const websocket_server&) = delete;

		// Change the port on which to listen.
		void listen(uint16_t port);

		void set_message_handler(message_handler h);
		void set_connect_handler(connect_handler h);
		void set_disconnect_handler(disconnect_handler h);
		void set_http_handler(http_handler h);

		void run();
		void stop();
		void send(id_type id, const std::string& message);
		void close(id_type id);

		uint16_t get_local_port() const;
		
	private:
		// The connection class handles all actual communication. There
		// is one instance for each connection. Connections are
		// identified by the single `id_type` stored as `id`.
		
		class connection : public std::enable_shared_from_this<connection> {
			public:
				connection(boost::asio::io_context& ioc, websocket_server& server, id_type id);

				void start();
				void send(const std::string& message);
				void close();

				friend websocket_server;

			private:
				void on_read_request(boost::beast::error_code ec, std::size_t);
				void on_websocket_accept(boost::beast::error_code ec);
				void do_read_websocket();
				void on_read_websocket(boost::beast::error_code ec, std::size_t);
				void do_write();
				void on_write(boost::beast::error_code ec, std::size_t);
				void on_close(boost::beast::error_code ec);
				void handle_http_request();

				boost::asio::ip::tcp::socket   socket_;
				std::optional<boost::beast::websocket::stream<boost::asio::ip::tcp::socket&>> ws_stream_;
				boost::beast::flat_buffer      buffer_;
				websocket_server::request_type http_request_;
				websocket_server&              server_;

				id_type id_;
				class queued_message {
					public:
						queued_message() {
						std::cerr << "Thread " << pthread_self() << " queued_message constructor " << (void*)this << std::endl;
						}
						~queued_message() {
						std::cerr << "Thread " << pthread_self() << " queued_message destructor " << (void*)this << std::endl;
						}
						
						std::string data;
						std::shared_ptr<boost::beast::flat_buffer> buffer;
						int seq;
				};
				std::queue<queued_message> message_queue_;
				bool writing_{false};
				bool is_websocket_{false};
		};

		void do_accept();
		void remove_connection(id_type id);

		boost::asio::io_context                                  ioc_;
		std::optional<boost::asio::ip::tcp::acceptor>            acceptor_;
		std::unordered_map<id_type, std::shared_ptr<connection>> connections_;
		id_type                                                  next_connection_id_{0};

		message_handler    message_handler_;
		connect_handler    connect_handler_;
		disconnect_handler disconnect_handler_;
		http_handler       http_handler_;
};

