
#include "Netbits.hh"
#include "NotebookWindow.hh"

Netbits::Netbits(cadabra::NotebookWindow& w)
	: nbw(w)
	{
	}

void Netbits::on_connect() 
	{
	std::cout << "connected" << std::endl;
	nbw.on_connect();
	}
