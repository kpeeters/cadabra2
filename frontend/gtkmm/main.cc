
#include "Client.hh"

class CadabraGtk : public cadabra::Client {
	public:
		virtual void on_connect() {};
		virtual void on_disconnect() {};
		virtual void on_network_error() {};
		virtual void on_progress() {};

		virtual void before_tree_change(cadabra::Client::ActionBase&) {};
		virtual void after_tree_change(cadabra::Client::ActionBase&) {};
};

int main(int argc, char **argv)
	{
	CadabraGtk cdb;

	std::thread client_thread(&CadabraGtk::run, std::ref(cdb));

	client_thread.join();
	}
