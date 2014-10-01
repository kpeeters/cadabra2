
#pragma once

#include <string>
#include "tree.hh"

namespace cadabra {

	// DataCells are the basic building blocks for a document. They are stored 
	// in a tree inside the client. A user interface should read these cells
	// and construct corresponding graphical output for them.
	
	class DataCell {
		public:
			enum class CellType { input, output, comment, texcomment, tex, error };
			
			DataCell(CellType t=CellType::input, const std::string& str="", bool texhidden=false);
			
			CellType                      cell_type;
			std::string                   textbuf;
			std::string                   cdbbuf;       // c_output only: the output in cadabra input format
			bool                          tex_hidden;   // c_tex only
			bool                          sensitive;
			bool                          running;
			
		private:
			// Cells can be locked against modification, in particular deletion,
			// so that iterators pointing to it remain valid.
			bool                          locked;
	};

	typedef tree<DataCell> DTree;
	
}
