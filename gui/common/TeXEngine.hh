
#pragma once

#include <stdexcept>
#include <vector>
#include <set>

/// TeXEngine is a singleton which is used to convert LaTeX strings
/// into Gdk::Pixbuf objects. This is a two-stage process: you first
/// 'check in' a string into the system, in exchange for a pointer to 
/// a TeXRequest object. When you are ready to retrieve the image,
/// call 'get_pixbuf'. 
///
/// If you need to generate images for more than one string, simply
/// check them all in and then call 'convert_all' before retrieving
/// the pixbufs. This requires only one round-trip through
/// latex/dvipng.

class TeXEngine {
	public:
		class TeXException : public std::logic_error {
			public:
				TeXException(const std::string&);
		};

		class TeXRequest {
			public:
				TeXRequest();
				friend class TeXEngine;
			private:
				std::string                latex_string;
				std::string                start_wrap, end_wrap;
				bool                       needs_generating;
				std::vector<unsigned char> image;
				unsigned                   width, height;
		};
		
		TeXEngine();
		~TeXEngine();

		/// Set the width and font size for all images to be generated.
		void set_geometry(int horizontal_pixels);
		void set_font_size(int font_size);
		std::vector<std::string> latex_packages;

		/// All checkin/checkout conversion routines.
		TeXRequest                *checkin(const std::string&,
													  const std::string& startwrap, const std::string& endwrap);
		TeXRequest                *modify(TeXRequest *, const std::string&);
		void                       convert_all();
		void                       checkout(TeXRequest *);
		
	private:		
		static double millimeter_per_inch;

		std::set<TeXRequest *> requests;

		int                    horizontal_pixels_;
		int                    font_size_;

		void erase_file(const std::string&) const;
		void convert_one(TeXRequest*);
		void convert_set(std::set<TeXRequest *>&);

		std::string handle_latex_errors(const std::string&) const;
};
