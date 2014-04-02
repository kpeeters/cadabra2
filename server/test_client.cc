
// A test client to do some basic manipulation of a notebook and
// some I/O with a cadabra server.

#include "Client.hh"

int main(int, char **)
	{
	cadabra::Client client;

	cadabra::Client::ActionAddCell action(doc.begin());
	client.perform(action);

	
	}
