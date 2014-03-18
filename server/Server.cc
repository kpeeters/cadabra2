
#include <boost/python.hpp>
#include <zmq.hpp>

Server::Server(const std::string& socket)
	{
	socket_name=socket;
	}

void Server::run() 
	{
	zmq::context_t context(1);
	zmq::socket_t  socket(context, ZMQ_REP);
	socket.bind(socket_name);

	while(true) {
		zmq::message_t request;
		socket.recv (&request);
		std::cout << "Received Hello" << std::endl;

		zmq::message_t reply (5);
		memcpy ((void *) reply.data (), "World", 5);
		socket.send (reply);
		}	
	}
