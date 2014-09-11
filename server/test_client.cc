
// A test client to do some basic manipulation of a notebook and
// some I/O with a cadabra server.

#include "Client.hh"
#include <thread>

class MyClient : public cadabra::Client {
	public:
		void on_connect();
		void on_disconnect();
		void on_network_error();
		void on_progress();
		void before_tree_change(ActionBase&);
		void after_tree_change(ActionBase&);
};

class UI {
	public:
		void run();
};

void MyClient::on_connect() 
	{
	std::cout << "connected to server" << std::endl;
	}

void MyClient::on_disconnect() 
	{
	std::cout << "disconnected from server" << std::endl;
	}

void MyClient::on_network_error()
	{
	std::cout << "network error" << std::endl;
	}

void MyClient::on_progress() 
	{
	}

void MyClient::before_tree_change(ActionBase& ab)
	{
	}

void MyClient::after_tree_change(ActionBase& ab)
	{
	}

void UI::run() 
	{
	sleep(10);
	}

int main(int, char **)
	{
	MyClient client;
	UI       ui;

	// Spawn two threads.
	std::thread client_thread(&MyClient::run, client);
	std::thread ui_thread(&UI::run, ui);

	// Wait for all threads to finish.
	client_thread.join();
	ui_thread.join();
	}
