
#pragma once

#include <string>
#include <mutex>

#include "tree.hh"
#include "json/json.h"

namespace cadabra {

	/// \ingroup files
	///
	/// DataCells are the basic building blocks for a document. They
	/// represent visual units in the notebook interface. They
	/// are stored in a tree inside the client, and can be transmitted
	/// over the wire between server and client in JSON format (see the
	/// documentation of the cadabra::JSON_serialise and cadabra::JSON_deserialise
	/// methods below for details on this representation). The notebook
	/// user interface reads these cells and construct corresponding
	/// graphical output for them.
	///
	/// The cadabra.display method of the cadabra python library knows
	/// how to turn various Python objects into the corresponding JSON
	/// representation of a DataCell.

	class DataCell {
		public:

			/// Cells are labelled with the data type of its contents, which is
			/// stored in a textural representation but may need processing for
			/// proper display.

			enum class CellType {
				document,   ///< head node, only one per document
				python,     ///< input : editor cell for code in python
				latex,      ///< input : editor cell for code in latex
				output,     ///< output: cell showing python stdout, verbatim
				verbatim,   ///< output: cell showing other verbatim output
				latex_view, ///< output: cell showing LaTeX text formatted using LaTeX
				input_form, ///< output: cell containing input form of preceding output cell
				image_png,  ///< output: cell showing a base64 encoded PNG image
				error,      ///< output: cell showing LaTeX text for errors
				// section
				};

			/// Each cell is identified by a serial number 'id' which is used
			/// to keep track of it across network calls, and a bool indicating
			/// whether the client or the server has created this cell.

			class id_t {
				public:
					id_t();

					uint64_t  id;
					bool      created_by_client;

					bool operator<(const id_t& other) const;
				};

			/// Standard constructor, generates a new unique id for this DataCell.

			DataCell(CellType t=CellType::python, const std::string& str="", bool hidden=false);

			/// Initialise a cell with an already determined id (it is the caller's responsibility
			/// to ensure that this id does not clash with any other DataCell's id).

			DataCell(id_t, CellType t=CellType::python, const std::string& str="", bool hidden=false);

			/// Copy constructor; preserves all information including id.

			DataCell(const DataCell&);

			CellType                      cell_type;

			/// Textual representation of the cell content. For e.g. latex cells it is a bit of a
			/// waste to store this representation both in the input and in the output cell.
			/// However, this gives us the flexibility to do manipulations on the input (e.g.
			/// resolving equation references) before feeding it to LaTeX.

			std::string                   textbuf;

			/// Flag indicating whether this cell should be hidden from
			/// view. The GUI should have a way to bring the cells back
			/// into view, typically by clicking on the output cell
			/// corresponding to the input cell.

			bool                          hidden;
			bool                          sensitive;

			/// Indicator whether this cell is currently being evaluated by the server.
			/// Currently only has a meaning for cells of type 'python'.
			/// This flag is set/reset using the ActionSetRunStatus action.

			bool                          running;

			id_t                          id() const;

		private:

			static std::mutex             serial_mutex;
			id_t                          serial_number;
			static uint64_t               max_serial_number; // on the client, server keeps track separately.

		};

	typedef tree<DataCell> DTree;

	/// Serialise a document into .cj format, which is a JSON version of
	/// the document tree.

	std::string JSON_serialise(const DTree&);
	void        JSON_recurse(const DTree&, DTree::iterator, Json::Value&);

	/// Load a document from .cj format, i.e. the inverse of the above.

	void        JSON_deserialise(const std::string&, DTree&);
	void        JSON_in_recurse(DTree& doc, DTree::iterator loc, const Json::Value& cells);

	/// Export a document to a single self-contained HTML file containing inline CSS.

	std::string export_as_HTML(const DTree& doc, bool for_embedding=false, bool
	                           strip_code=false, std::string title="");
	void        HTML_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str,
	                         const std::string& preamble_string,
	                         bool for_embedding=false, bool strip_code=false, std::string title="");

	/// Convert various LaTeX constructions to HTML-with-Mathjax, e.g. \\section{...},
	/// \\begin{verbatim}...\\end{verbatim}, \\verb.

	std::string latex_to_html(const std::string&);

	/// Export a document to a single self-contained LaTeX file, with the exception of
	/// images which get saved as separate numbered files.

	std::string export_as_LaTeX(const DTree& doc, const std::string& image_file_base);
	void        LaTeX_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str,
	                          const std::string& preamble_string, const std::string& image_file_base,
	                          int& image_num);

	/// Export a document to a python-like file (converting text cells to comments
	/// and python cells to python code, dropping output cells).

	std::string export_as_python(const DTree& doc);
	void        python_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str);

	/// Replace all occurrences of a substring in the original string.
	// std::string replace_all(std::string, const std::string& old, const std::string& nw);
	}
