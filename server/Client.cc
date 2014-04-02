
#include <zmq.hpp>
#include <string>
#include <iostream>

Client::Client()
	{
	add_cell();
	}

int CadabraClient::get_fd()
	{
	// http://stackoverflow.com/questions/6452131/how-to-use-zeromq-in-an-gtk-qt-clutter-application
	int fd;
	size_t sizeof_fd = sizeof(fd);
	if(zmq_getsockopt(socket, ZMQ_FD, &fd, &sizeof_fd))
      perror("retrieving zmq fd");
	}

void CadabraClient::run()
	{
	zmq::context_t context (1);
	zmq::socket_t socket (context, ZMQ_REQ);
	
	std::cout << "Connecting to hello world server…" << std::endl;
	socket.connect ("tcp://localhost:5555");
	
	//  Do 10 requests, waiting each time for a response
	for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
		zmq::message_t request (6);
		memcpy ((void *) request.data (), "Hello", 5);
		std::cout << "Sending Hello " << request_nbr << "…" << std::endl;
		socket.send (request);
		
		//  Get the reply.
		zmq::message_t reply;
		socket.recv (&reply);
		std::cout << "Received World " << request_nbr << std::endl;
		}
	return 0;
	}
