
// A test client to do some basic manipulation of a notebook and
// some I/O with a cadabra server.

#include "Client.hh"
#include <thread>
#include <system_error>

class MyClient : public cadabra::Client {
	public:
		MyClient();

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

MyClient client;
UI       ui;


MyClient::MyClient()
	: Client(0)
	{
	}

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
	int i;
	std::cin >> i;

	cadabra::Client::iterator it=client.dtree().begin();
	auto cell = std::make_shared<cadabra::Client::DataCell>();
	auto ac = std::make_shared<cadabra::Client::ActionAddCell>(cell, it, cadabra::Client::ActionAddCell::Position::child);

	try {
		std::cout << "calling perform" << std::endl;
		client.perform(ac);
		} catch(std::error_code& ex) {
		std::cout << ex.message() << std::endl;
		}
	std::cout << "perform called" << std::endl;

	sleep(10);
	}

int main(int, char **)
	{
	//	client.init();
	std::cout << "client connected" << std::endl;

	try {
		// Spawn two threads.
		std::thread client_thread(&MyClient::run, std::ref(client));
		std::thread ui_thread(&UI::run, ui);

		// Wait for all threads to finish.
		client_thread.join();
		ui_thread.join();
		} catch(std::error_code& ex) {
		std::cout << ex.message() << std::endl;
		}

	}
