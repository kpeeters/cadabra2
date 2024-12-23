#include "websocket_client.hh"
#include <iostream>

websocket_client::websocket_client()
	: ssl_ctx_(boost::asio::ssl::context::tlsv12_client)
	, resolver_(ioc_)
	, is_ssl_(false)
	{
	ssl_ctx_.set_default_verify_paths();
	ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
	}

websocket_client::~websocket_client()
	{
   boost::beast::error_code ec;  // ignored errors
	
	if (ws_stream_) {
		ws_stream_->close(boost::beast::websocket::close_code::normal, ec);
		boost::beast::get_lowest_layer(*ws_stream_).shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		}
	
	if (wss_stream_) {
		wss_stream_->close(boost::beast::websocket::close_code::normal, ec);
		wss_stream_->next_layer().shutdown(ec);
		boost::beast::get_lowest_layer(*wss_stream_).shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		}
	
	ioc_.stop();
	}

void websocket_client::set_message_handler(message_handler h) { message_handler_ = std::move(h); }
void websocket_client::set_connect_handler(connect_handler h) { connect_handler_ = std::move(h); }
void websocket_client::set_close_handler(close_handler h) { close_handler_ = std::move(h); }
void websocket_client::set_fail_handler(fail_handler h) { fail_handler_ = std::move(h); }

void websocket_client::connect(const std::string& uri_string)
	{
	// Parse URI (basic)
	Uri uri(uri_string);
	is_ssl_ = uri.protocol=="wss";
	host_   = uri.host;
	port_   = uri.port.empty() ? (is_ssl_ ? "443" : "80") : uri.port;
	path_   = uri.path;
	
	// Create appropriate stream
	if (is_ssl_) {
		wss_stream_ = std::make_unique<boost::beast::websocket::stream<
			boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>>(ioc_, ssl_ctx_);
		wss_stream_->binary(false);  // Set to text mode
		wss_stream_->auto_fragment(false);  // Don't fragment messages
		wss_stream_->read_message_max(64 * 1024 * 1024);  // 64MB max message size
		}
	else {
		ws_stream_ = std::make_unique<boost::beast::websocket::stream<
			boost::asio::ip::tcp::socket>>(ioc_);
		ws_stream_->binary(false);  // Set to text mode
		ws_stream_->auto_fragment(false);  // Don't fragment messages
		ws_stream_->read_message_max(64 * 1024 * 1024);  // 64MB max message size
		}
	
	// Start the connection process
	resolver_.async_resolve(host_, port_,
									[this](const boost::beast::error_code& ec,
											 boost::asio::ip::tcp::resolver::results_type results) {
									on_resolve(ec, results);
									});
	}

void websocket_client::on_resolve(const boost::beast::error_code& ec,
											 boost::asio::ip::tcp::resolver::results_type results)
	{
	if (ec) return fail(ec);
	
	if (is_ssl_) {
		boost::asio::async_connect(
			wss_stream_->next_layer().next_layer(),
			results,
			[this](const boost::beast::error_code& ec,
					 const boost::asio::ip::tcp::endpoint& /*endpoint*/) {
			on_connect(ec);
			});
		}
	else {
		boost::asio::async_connect(
			ws_stream_->next_layer(),
			results,
			[this](const boost::beast::error_code& ec,
					 const boost::asio::ip::tcp::endpoint& /*endpoint*/) {
			on_connect(ec);
			});
		}
	}

void websocket_client::on_connect(const boost::beast::error_code& ec)
	{
	if (ec) return fail(ec);
	
	if (is_ssl_) {
		wss_stream_->next_layer().async_handshake(
			boost::asio::ssl::stream_base::client,
			[this](const boost::beast::error_code& ec) {
			on_ssl_handshake(ec);
			});
		}
	else {
		ws_stream_->async_handshake(host_, path_,
											 [this](const boost::beast::error_code& ec) {
											 on_handshake(ec);
											 });
		}
	}

void websocket_client::on_ssl_handshake(const boost::beast::error_code& ec)
	{
	if (ec) return fail(ec);
	
	wss_stream_->async_handshake(host_, path_,
										  [this](const boost::beast::error_code& ec) {
										  on_handshake(ec);
										  });
	}

void websocket_client::on_handshake(const boost::beast::error_code& ec)
	{
	if (ec) return fail(ec);
	
	if (connect_handler_) {
		connect_handler_();
		}
	
	do_read();
	}



void websocket_client::send(const std::string& message)
	{
	// Beast does not allow us to run two `async_write` at the
	// same time; we have to wait for the completion handler
	// to be called.
	
	// Create a new buffer for this message
	queued_message msg;
	msg.data   = message;
	msg.buffer = std::make_shared<boost::beast::flat_buffer>();
	boost::beast::ostream(*msg.buffer) << msg.data;
	
	message_queue_.push(msg);
	if (!writing_)
		do_write();
	}

void websocket_client::on_write(const boost::beast::error_code& ec, std::size_t /* bytes_transferred */)
	{
	if(ec) {
		if(fail_handler_) {
			fail_handler_(ec);
			}
		return;
		}
	
	// Remove the message and the associated beast buffer.
	message_queue_.pop();  
	
	// Write next message, if any.
	do_write();
	}

void websocket_client::do_write()
	{
	if (message_queue_.empty()) {
		writing_ = false;
		return;
		}
	
	writing_ = true;
	auto& msg = message_queue_.front();
   
	if (is_ssl_) {
		wss_stream_->async_write(
			msg.buffer->data(),
                [this](boost::beast::error_code ec, std::size_t bytes_transferred) {
					 on_write(ec, bytes_transferred);
                });
		}
	else {
		ws_stream_->async_write(
			msg.buffer->data(),
			[this](boost::beast::error_code ec, std::size_t bytes_transferred) {
			on_write(ec, bytes_transferred);
			});
		}
	}

void websocket_client::do_read()
	{
	if (is_ssl_) {
		wss_stream_->async_read(
			buffer_,
			[this](const boost::beast::error_code& ec, std::size_t bytes) {
			on_read(ec, bytes);
			});
		}
	else {
		ws_stream_->async_read(
			buffer_,
			[this](const boost::beast::error_code& ec, std::size_t bytes) {
			on_read(ec, bytes);
			});
		}
	}

void websocket_client::on_read(const boost::beast::error_code& ec, std::size_t /* bytes_transferred */)
	{
	if (ec) return fail(ec);
	
	if (message_handler_) {
		message_handler_(boost::beast::buffers_to_string(buffer_.data()));
		}
	
	buffer_.consume(buffer_.size());
	do_read();
	}

void websocket_client::close()
	{
	if (is_ssl_) {
		wss_stream_->async_close(
			boost::beast::websocket::close_code::normal,
			[this](const boost::beast::error_code& ec) {
			on_close(ec);
			});
		}
	else {
		ws_stream_->async_close(
			boost::beast::websocket::close_code::normal,
			[this](const boost::beast::error_code& ec) {
			on_close(ec);
			});
		}
	}

void websocket_client::on_close(const boost::beast::error_code& ec)
	{
	if (ec) return fail(ec);
	
	if (close_handler_) {
		close_handler_();
		}
	}

void websocket_client::fail(const boost::beast::error_code& ec)
	{
	if (fail_handler_) {
		fail_handler_(ec);
		}
	}

void websocket_client::run()
	{
	ioc_.run();
	}

void websocket_client::stop()
	{
	ioc_.stop();
	}


Uri::Uri(const std::string& uri)
	{
	path = "/";  // default path
	std::regex pattern("^([^:]+)://([^/:]+)(?::(\\d+))?(/.*)?");
	std::smatch matches;
   
	if (std::regex_match(uri, matches, pattern)) {
		protocol = matches[1];
		host = matches[2];
		if (matches[3].matched) port = matches[3];
		if (matches[4].matched) path = matches[4];
		}
	}

std::string Uri::to_string() const
	{
	std::string result = protocol + "://" + host;
	if (!port.empty()) result += ":" + port;
	result += path;
	return result;
	}
