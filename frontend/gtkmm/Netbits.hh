
#pragma once

#include "Client.hh"

// A network client which knows how to propagate the Client callback
// functions to the NotebookWindow class so that the Gtk interface
// gets updated appropriately.
// This class runs in its own thread, calls methods in NotebookWindow
// to queue updates, and then calls on the dispatcher to make that
// NotebookWindow thread wake up.

namespace cadabra {
	class NotebookWindow;
};

class Netbits : public cadabra::Client {
	public:
		Netbits(cadabra::NotebookWindow&);

		virtual void on_connect();
		virtual void on_disconnect() {};
		virtual void on_network_error() {};
		virtual void on_progress() {};

		virtual void before_tree_change(cadabra::Client::ActionBase&) {};
		virtual void after_tree_change(cadabra::Client::ActionBase&) {};
		
	private:
		cadabra::NotebookWindow& nbw;
};
