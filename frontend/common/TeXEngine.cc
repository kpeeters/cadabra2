
#include "TeXEngine.hh"

#include <boost/filesystem.hpp>
#include "exec-stream.h"
#include "lodepng.h"
#include <fstream>
#include <sstream>

//#define DEBUG

using namespace cadabra;


double TeXEngine::millimeter_per_inch = 25.4;

TeXEngine::TeXException::TeXException(const std::string& str)
	: std::logic_error(str)
	{
	}

unsigned TeXEngine::TeXRequest::width() const
	{
	return width_;
	}

unsigned TeXEngine::TeXRequest::height() const
	{
	return height_;
	}

const std::vector<unsigned char>& TeXEngine::TeXRequest::image() const
	{
	return image_;
	}

void TeXEngine::erase_file(const std::string& nm) const
	{
	boost::filesystem::remove(nm);
	}

std::string TeXEngine::handle_latex_errors(const std::string& result) const
	{
	std::string::size_type pos=result.find("! LaTeX Error");
	if(pos != std::string::npos) {
		 return "LaTeX error, probably a missing style file. See the output below.\n\n" +result;
		 }
	
	pos=result.find("! TeX capacity exceeded");
	if(pos != std::string::npos) {
		 return "Output cell too large (TeX capacity exceeded), output suppressed.";
		 }
	
	pos=result.find("! Double superscript.");
	if(pos != std::string::npos) {
		return "Internal typesetting error: double superscript. Please report a bug.\n\n" + result;
		}
	pos=result.find("! Double subscript.");
	if(pos != std::string::npos) {
		return "Internal typesetting error: double subscript. Please report a bug.\n\n" + result;
		}
	
	pos=result.find("! Package breqn Error: ");
	if(pos != std::string::npos) {
		return "Typesetting error (breqn.sty related). Please report a bug.\n\n" + result;
		}
	
	pos=result.find("! Undefined control sequence");
	if(pos != std::string::npos) {
		 std::string::size_type undefpos=result.find("\n", pos+30);
		 if(undefpos==std::string::npos) 
			  return "Undefined control sequence (failed to parse LaTeX output).";
		 std::string::size_type backslashpos=result.rfind("\\", undefpos);
		 if(backslashpos==std::string::npos || backslashpos < 2) 
			  return "Undefined control sequence (failed to parse LaTeX output).";

		 std::string undefd=result.substr(backslashpos-1,undefpos-pos-30);
		 return "Undefined control sequence:\n\n" +undefd+"\nNote that all symbols which you use in cadabra have to be valid LaTeX expressions. If they are not, you can still use the LaTeXForm property to make them print correctly; see the manual for more information.";
		 }

	return "";
	}

TeXEngine::~TeXEngine()
	{
	}

TeXEngine::TeXEngine()
	: horizontal_pixels_(800), font_size_(12), scale_(1.0)
	{
	}

void TeXEngine::set_geometry(int horpix)
	{
	if(horizontal_pixels_!=horpix) {
		// flag all requests as requiring an update
		std::set<std::shared_ptr<TeXRequest> >::iterator reqit=requests.begin();
		while(reqit!=requests.end()) {
			(*reqit)->needs_generating=true;
			++reqit;
			}
		}
	horizontal_pixels_=horpix;
	}

void TeXEngine::set_font_size(int fontsize)
	{
	if(font_size_!=fontsize) {
		// flag all requests as requiring an update
		std::set<std::shared_ptr<TeXRequest> >::iterator reqit=requests.begin();
		while(reqit!=requests.end()) {
			(*reqit)->needs_generating=true;
			++reqit;
			}
		}
	font_size_=fontsize;
	}

void TeXEngine::set_scale(double scale)
	{
	if(scale_!=scale) {
		// flag all requests as requiring an update
		std::set<std::shared_ptr<TeXRequest> >::iterator reqit=requests.begin();
		while(reqit!=requests.end()) {
			(*reqit)->needs_generating=true;
			++reqit;
			}
		}
	scale_=scale;
	}

TeXEngine::TeXRequest::TeXRequest()
	: needs_generating(true)
	{
	}

std::shared_ptr<TeXEngine::TeXRequest> TeXEngine::checkin(const std::string& txt,
																			 const std::string& startwrap, const std::string& endwrap)
	{
	auto req = std::make_shared<TeXRequest>();
	req->latex_string=txt;
	req->start_wrap=startwrap;
	req->end_wrap=endwrap;
	req->needs_generating=true;
	requests.insert(req);
	return req;
	}

void TeXEngine::checkout(std::shared_ptr<TeXRequest> req)
	{
	std::set<std::shared_ptr<TeXRequest> >::iterator it=requests.find(req);
	assert(it!=requests.end());
	requests.erase(it);
	}

void TeXEngine::checkout_all()
	{
	requests.clear();
	}

void TeXEngine::invalidate_all()
	{
	auto it=requests.begin();
	while(it!=requests.end()) {
		(*it)->needs_generating=true;
		++it;
		}
	}

std::shared_ptr<TeXEngine::TeXRequest> TeXEngine::modify(std::shared_ptr<TeXRequest> req, const std::string& txt)
	{
	req->latex_string=txt;
	req->needs_generating=true;
	return req;
	}

void TeXEngine::convert_all()
	{
	size_t need_generating=0;
	auto it=requests.begin();
	while(it!=requests.end()) {
		if((*it)->needs_generating)
			++need_generating;
		++it;
		}

	if(need_generating!=0) {
//		std::cerr << "cadabra-client: running TeX on " << requests.size() << " requests" << std::endl;
		convert_set(requests);
		}
	}

void TeXEngine::convert_one(std::shared_ptr<TeXRequest> req)
	{
	std::set<std::shared_ptr<TeXRequest> > reqset;
	reqset.insert(req);
	convert_set(reqset);
	}

void TeXEngine::convert_set(std::set<std::shared_ptr<TeXRequest> >& reqs)
	{
	// We now follow
	// 
	// https://www.securecoding.cert.org/confluence/display/seccode/FI039-C.+Create+temporary+files+securely
	// 
	// for temporary files.

	char olddir[1024];
	if(getcwd(olddir, 1023)==NULL)
		 olddir[0]=0;
	if(chdir("/tmp")==-1)
		throw TeXException("Failed to chdir to /tmp.");

	char templ[]="/tmp/cdbXXXXXX";

	// The size in mm or inches which we use will in the end determine how large
	// the font will come out. 
	//
	// For given horizontal size, we stretch this up to the full window
	// width using horizontal_pixels/(horizontal_size/millimeter_per_inch) dots per inch.
	// The appropriate horizontal size in mm is determined by trial and error, 
	// and of course scales with the number of horizontal pixels.
	
	// Note: the number here has no effect on the size in pixels of the generated 
	// PDF. That is set with the -D parameter of dvipng.

	const double horizontal_mm=horizontal_pixels_*(12.0/font_size_)/3.94/scale_;
//#ifdef DEBUG
//	std::cerr << "tex_it: font_size " << font_size << std::endl
//				 << "        pixels    " << horizontal_pixels_ << std::endl
//				 << "        mm        " << horizontal_mm << std::endl;
//#endif

	//(int)(millimeter_per_inch*horizontal_pixels/100.0); //140;
	const double vertical_mm=10*horizontal_mm;


	// Write each string in the set of requests into a buffer, separating
	// them by a page eject.

	std::ostringstream total;
	int fd = mkstemp(templ);
	if(fd == -1) 
		 throw TeXException("Failed to create temporary file in /tmp.");

	total << "\\documentclass[11pt]{article}\n"
			<< "\\usepackage[dvips,verbose,voffset=0pt,hoffset=0pt,textwidth="
			<< horizontal_mm << "mm,textheight="
			<< vertical_mm << "mm]{geometry}\n"
			<< "\\usepackage{inconsolata}\n"
			<< "\\usepackage{amsmath}\n"
			<< "\\usepackage{color}\\usepackage{amssymb}\n"
			<< "\\usepackage[parfill]{parskip}\n\\usepackage{tableaux}\n";

	for(size_t i=0; i<latex_packages.size(); ++i)
		total << "\\usepackage{" << latex_packages[i] << "}\n";

	total	<< "\\def\\specialcolon{\\mathrel{\\mathop{:}}\\hspace{-.5em}}\n"
			<< "\\renewcommand{\\bar}[1]{\\overline{#1}}\n"
			<< "\\begin{document}\n\\pagestyle{empty}\n"
			<< "\\renewcommand{\\arraystretch}{1.2}\n";

	std::set<std::shared_ptr<TeXRequest> >::iterator reqit=reqs.begin();
	while(reqit!=reqs.end()) {
		if((*reqit)->needs_generating) {
			if((*reqit)->latex_string.size()>100000)
				total << "Expression too long, output suppressed.\n";
			else {
				if((*reqit)->start_wrap.size()>0) 
					total << (*reqit)->start_wrap;
				total << (*reqit)->latex_string;
				if((*reqit)->end_wrap.size()>0)
					total << "\n" << (*reqit)->end_wrap;
				else total << "\n";
				}
			total << "\\eject\n";
			}
		++reqit;
		}
	total << "\\end{document}\n";


	// Now write the 'total' buffer to the .tex file

	ssize_t start=0;
	do {
		ssize_t written=write(fd, &(total.str().c_str()[start]), total.str().size()-start);
		if(written>=0)
			start+=written;
		else {
			if(errno != EINTR) {
				close(fd);
				throw TeXException("Failed to write LaTeX temporary file.");
				}
			} 
		} while(start<static_cast<ssize_t>(total.str().size()));
	close(fd);
#ifdef DEBUG
	std::cerr  << templ << std::endl;
	std::cerr << "---\n" << total.str() << "\n---" << std::endl;
#endif

	std::string nf=std::string(templ)+".tex";
	rename(templ, nf.c_str());

#ifdef __CYGWIN__
	// MikTeX does not see /tmp, it needs \cygwin\tmp
	nf="\\cygwin"+nf;
	pcrecpp::RE("/").GlobalReplace("\\\\", &nf);
#endif

	// Run LaTeX on the .tex file.
	exec_stream_t latex_proc;
	std::string result;
	try {
		latex_proc.start("latex", "--interaction nonstopmode "+nf);
 		std::string line; 
		while( std::getline( latex_proc.out(), line ).good() ) 
			result+=line+"\n";

		erase_file(std::string(templ)+".tex");
//		std::cout << "TeX file in " << std::string(templ)+".tex" << std::endl;
		erase_file(std::string(templ)+".aux");
		erase_file(std::string(templ)+".log");
#ifdef DEBUG		
		std::cerr << result << std::endl;
#endif

		std::string err=handle_latex_errors(result);

		if(err.size()>0) {
			reqit=reqs.begin();
			while(reqit!=reqs.end()) 
				(*reqit++)->needs_generating=false;
			 erase_file(std::string(templ)+".dvi");
			 if(chdir(olddir)==-1)
				 throw TeXException(err+" (and cannot chdir back to original "+olddir+")");
			 throw TeXException(err); 
			 }
		}
	catch(std::logic_error& err) {
		erase_file(std::string(templ)+".tex");
		erase_file(std::string(templ)+".dvi");
		erase_file(std::string(templ)+".aux");
		erase_file(std::string(templ)+".log");
		
		std::string latex_err=handle_latex_errors(result);
		reqit=reqs.begin();
		while(reqit!=reqs.end()) 
			(*reqit++)->needs_generating=false;

		if(latex_err.size()>0) {
			 if(chdir(olddir)==-1)
				 throw TeXException(latex_err+" (and cannot chdir back to original "+olddir+")");
			 throw TeXException(latex_err); 
			 }

		// Even if we cannot find an explicit error in the output, we have to terminate
		// since LaTeX has thrown an exception.
		if(chdir(olddir)==-1)
			throw TeXException("Cannot start LaTeX, is it installed? (and cannot chdir back to original)");
		throw TeXException("Cannot start LaTeX, is it installed?");
		}

	// Convert the entire dvi file to png files.
	//
	std::ostringstream resspec;
	resspec << horizontal_pixels_/(1.0*horizontal_mm)*millimeter_per_inch; 
	exec_stream_t dvipng_proc;
//	dvipng_proc << "-T" << "tight" << "-bg" << "Transparent"; // << "-fg";
//	rgbspec << "\"rgb "
//			  << foreground_colour.get_red()/65536.0 << " "
//			  << foreground_colour.get_green()/65536.0 << " "
//			  << foreground_colour.get_blue()/65536.0 << "\"";
//	dvipng_proc << rgbspec.str();
//	dvipng_proc << "-D";
//	resspec << horizontal_pixels_/(1.0*horizontal_mm)*millimeter_per_inch;
//	dvipng_proc << resspec.str() << std::string(templ)+".dvi";

	try {
		dvipng_proc.start("dvipng", "-T tight -bg Transparent -D "+resspec.str()+" "+std::string(templ)+".dvi");
		std::string s, result;
		while( std::getline( dvipng_proc.out(), s ).good() ) {
			result+=s;
			}		
#ifdef DEBUG
	std::cerr << result << std::endl;
#endif
		}
	catch(std::logic_error& ex) {
		// Erase all dvi and png files and put empty pixbufs into the TeXRequests.
		erase_file(std::string(templ)+".dvi");
		reqit=reqs.begin();
		int pagenum=1;
		while(reqit!=reqs.end()) {
			if((*reqit)->needs_generating) {
				std::ostringstream pngname;
				pngname << std::string(templ) << pagenum << ".png";
				erase_file(pngname.str());
				(*reqit)->image_.clear();
				(*reqit)->needs_generating=true;
				++pagenum;
				}
			++reqit;
			}
		if(chdir(olddir)==-1)
			throw TeXException(
				std::string("Cannot run dvipng, is it installed? (and cannot chdir back to original)\n\n")+ex.what());
		throw TeXException(std::string("Cannot run dvipng, is it installed?\n\n")+ex.what());
		}

	erase_file(std::string(templ)+".dvi");

	// Conversion completed successfully, now convert all resulting PNG files to Pixbuf images.

	reqit=reqs.begin();
	int pagenum=1;
	while(reqit!=reqs.end()) {
		if((*reqit)->needs_generating) {
			std::ostringstream pngname;
			pngname << std::string(templ) << pagenum << ".png";
			std::ifstream tst(pngname.str().c_str());
			if(tst.good()) {
				(*reqit)->image_.clear();
				unsigned error = lodepng::decode((*reqit)->image_, (*reqit)->width_, 
															(*reqit)->height_, pngname.str());
				if(error!=0)
					throw TeXException("PNG conversion error");
				(*reqit)->needs_generating=false;
				erase_file(pngname.str());
				}
			++pagenum;
			}
		++reqit;
		}

	if(chdir(olddir)==-1)
		throw TeXException("Failed to chdir back to " +std::string(olddir)+".");
	}

