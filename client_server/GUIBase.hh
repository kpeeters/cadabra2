
#pragma once

#include "Client.hh"

// An abstract base class determining the interface which any GUI implementation 
// needs to implement. 

namespace cadabra {

	class GUIBase {
		public:
			// These functions all have corresponding actions defined in Client.hh.

			virtual void add_cell(cadabra::Client::iterator before)=0;
			virtual void remove_cell(cadabra::Client::iterator cell)=0;
	};

};
