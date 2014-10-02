
#pragma once

#include <string>
#include <mutex>

#include "tree.hh"

namespace cadabra {

	// DataCells are the basic building blocks for a document. They are stored 
	// in a tree inside the client. A user interface should read these cells
	// and construct corresponding graphical output for them.
	
	class DataCell {
		public:
			enum class CellType { input, output, comment, texcomment, tex, error };
			
			DataCell(CellType t=CellType::input, const std::string& str="", bool texhidden=false);
			DataCell(const DataCell&);
			
			CellType                      cell_type;
			std::string                   textbuf;
			std::string                   cdbbuf;       // c_output only: the output in cadabra input format
			bool                          tex_hidden;   // c_tex only
			bool                          sensitive;
			bool                          running;
			
			// Each cell is identified by a serial number 'id' which is used
			// to keep track of it across network calls.

			long                          id() const;
			
		private:

			static std::mutex             serial_mutex;
			long                          serial_number;
			static long                   max_serial_number;
	};

	typedef tree<DataCell> DTree;
	
}
