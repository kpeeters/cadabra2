#include "websocket_server.hh"
#include <iostream>

websocket_server::connection::connection(boost::asio::io_context& ioc, websocket_server& server, id_type id)
	: socket_(ioc)
	, server_(server)
	, id_(id)
	{
	}

void websocket_server::connection::start()
	{
	// First read as HTTP
	boost::beast::http::async_read(
		socket_,
		buffer_,
		http_request_,
		[self = shared_from_this()](
			boost::beast::error_code ec, std::size_t bytes_transferred) {
		self->on_read_request(ec, bytes_transferred);
		});
	}

void websocket_server::connection::on_read_request(boost::beast::error_code ec, std::size_t)
	{
	if (ec) {
		server_.remove_connection(id_);
		return;
		}

	if (boost::beast::websocket::is_upgrade(http_request_)) {
		// Handle as WebSocket
		is_websocket_ = true;
		ws_stream_.emplace(socket_);
		ws_stream_->async_accept(
			http_request_,
			[self = shared_from_this()](boost::beast::error_code ec) {
			self->on_websocket_accept(ec);
			});
		}
	else {
		// Handle as HTTP
		handle_http_request();
		}
	}

void websocket_server::connection::handle_http_request()
	{
	// Create response that lives through the async operation
	auto response = std::make_shared<boost::beast::http::response<boost::beast::http::string_body>>();
   
	if (server_.http_handler_) {
		server_.http_handler_(http_request_, *response);
		}
	else {
		response->result(boost::beast::http::status::not_found);
		response->version(http_request_.version());
		response->set(boost::beast::http::field::server, "Beast");
		response->set(boost::beast::http::field::content_type, "text/plain");
		response->body() = "404 Not Found\r\n";
		}
	
	response->prepare_payload();
   
	boost::beast::http::async_write(
		socket_,
		*response,
		[self = shared_from_this(), response](  // Keep response alive in lambda
			boost::beast::error_code ec, std::size_t) {
		if (ec) {
			self->server_.remove_connection(self->id_);
			return;
			}
		// HTTP is done, close the connection
		boost::beast::error_code sec;
		self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, sec);
		self->server_.remove_connection(self->id_);
		});
	}

void websocket_server::connection::on_websocket_accept(boost::beast::error_code ec)
	{
	if (ec) {
		server_.remove_connection(id_);
		return;
		}
	
	if (server_.connect_handler_) {
		server_.connect_handler_(id_);
		}
	
	do_read_websocket();
	}

void websocket_server::connection::do_read_websocket()
	{
	ws_stream_->async_read(
		buffer_,
		[self = shared_from_this()](
			boost::beast::error_code ec, std::size_t bytes_transferred) {
		self->on_read_websocket(ec, bytes_transferred);
		});
	}

void websocket_server::connection::on_read_websocket(boost::beast::error_code ec, std::size_t)
	{
	if (ec) {
		server_.remove_connection(id_);
		return;
		}
	
	if (server_.message_handler_) {
		server_.message_handler_(id_,
										 boost::beast::buffers_to_string(buffer_.data()),
										 http_request_,
										 socket_.remote_endpoint().address().to_string());
		}
	
	buffer_.consume(buffer_.size());
	do_read_websocket();
	}

void websocket_server::connection::send(const std::string& message)
	{
	if (!is_websocket_) return;
   
	message_queue_.push(message);
   
	if (!writing_) {
		do_write();
		}
	}

void websocket_server::connection::do_write()
	{
	if (message_queue_.empty() || !is_websocket_) {
		writing_ = false;
		return;
		}
	
	writing_ = true;
	auto msg = message_queue_.front();
	message_queue_.pop();
	
	ws_stream_->async_write(
		boost::asio::buffer(msg),
		[self = shared_from_this()](
			boost::beast::error_code ec, std::size_t bytes_transferred) {
		self->on_write(ec, bytes_transferred);
		});
	}

void websocket_server::connection::close()
	{
	if (!is_websocket_) {
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		server_.remove_connection(id_);
		return;
		}
	
	ws_stream_->async_close(
		boost::beast::websocket::close_code::normal,
		[self = shared_from_this()](boost::beast::error_code ec) {
		self->on_close(ec);
		});
	}

void websocket_server::connection::on_write(boost::beast::error_code ec, std::size_t)
	{
	if (ec) {
		server_.remove_connection(id_);
		return;
		}
	
	do_write();
	}

void websocket_server::connection::on_close(boost::beast::error_code ec)
	{
	server_.remove_connection(id_);
	}

websocket_server::websocket_server(uint16_t port)
	{
	listen(port);
	}

websocket_server::~websocket_server()
	{
	stop();
	}

void websocket_server::listen(uint16_t port)
	{
	boost::asio::ip::tcp::endpoint endpoint{boost::asio::ip::tcp::v4(), port};
   
	acceptor_.emplace(ioc_);
	acceptor_->open(endpoint.protocol());
	acceptor_->set_option(boost::asio::socket_base::reuse_address(true));
	acceptor_->bind(endpoint);
	acceptor_->listen(64 /* backlog */);
	
	do_accept();
	}

void websocket_server::set_message_handler(message_handler h)
	{
	message_handler_ = std::move(h);
	}

void websocket_server::set_connect_handler(connect_handler h)
	{
	connect_handler_ = std::move(h);
	}

void websocket_server::set_disconnect_handler(disconnect_handler h)
	{
	disconnect_handler_ = std::move(h);
	}

void websocket_server::set_http_handler(http_handler h)
	{
	http_handler_ = std::move(h);
	}

void websocket_server::do_accept()
	{
	if(!acceptor_) return;
	
	acceptor_->async_accept(
		[this](boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
		if (!ec) {
			std::cerr << "websocket::server::do_accept: opening connection." << std::endl;
			auto id = next_connection_id_++;
			auto conn = std::make_shared<connection>(ioc_, *this, id);
			socket.set_option(boost::asio::ip::tcp::no_delay(true));
			conn->socket_ = std::move(socket);
			connections_[id] = conn;
			conn->start();
			}
		else {
			std::cerr << "websocket::server::do_accept: error on accept, " << ec << std::endl;
			}

		// restart for the next connection
		do_accept();
		});
	}

void websocket_server::send(id_type id, const std::string& message)
	{
	if (auto it = connections_.find(id); it != connections_.end()) {
		it->second->send(message);
		}
	}

void websocket_server::close(id_type id)
	{
	if (auto it = connections_.find(id); it != connections_.end()) {
		it->second->close();
		}
	}

void websocket_server::remove_connection(id_type id)
	{
	if (disconnect_handler_) {
		disconnect_handler_(id);
		}
	connections_.erase(id);
	}

void websocket_server::run()
	{
	ioc_.run();
	}

void websocket_server::stop()
	{
	boost::beast::error_code ec;
	if(acceptor_)
		acceptor_->close(ec);
	
	for (auto& [_, conn] : connections_) {
		conn->close();
		}
	
	connections_.clear();
	ioc_.stop();
	}
