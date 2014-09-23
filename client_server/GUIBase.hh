
#pragma once

#include <deque>

#include "Client.hh"

// An abstract base class determining the interface which any GUI implementation 
// needs to implement. 

namespace cadabra {

	class GUIBase {
		public:
			// A GUI implementation only needs to know how to add, remove or update
			// cells. A stack of pending actions is kept and accessed both from the
         // GUI and the network Client.
			
			class GUIAction {
				public:
					enum class Type { ADD, REMOVE, UPDATE };

					GUIAction(Type, Client::iterator);

					Type             action;
					Client::iterator cell;
			};

			std::mutex                     gui_todo_mutex;
			std::deque<GUIBase::GUIAction> gui_todo_deque;

         // After the Client updates the todo deque, it will call new_todo_notification to
         // inform the GUI about this. Typically the GUI would be made to wake up and
         // then process any remaining items on the todo deque.

			virtual void new_todo_notification()=0;

			// Network status is propagated from the Client to the GUI using the following
         // (processing these do not require the GUI to read the DTree).

			virtual void on_connect()=0;
			virtual void on_disconnect()=0;
			virtual void on_network_error()=0;
	};

};
