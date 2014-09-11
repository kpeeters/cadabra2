
// A test client to do some basic manipulation of a notebook and
// some I/O with a cadabra server.

#include "Client.hh"

class MyClient : public cadabra::Client {
	public:
		
};

int main(int, char **)
	{
	MyClient client;

	client.run();
	}
