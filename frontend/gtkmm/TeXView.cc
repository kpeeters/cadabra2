
#include "TeXView.hh"
#include "GtkCompat.hh"
#include <iostream>
#include <cairomm/context.h>
#include <regex>
#include <algorithm>

// MicroTeX
#include "cairo/graphic_cairo.h"

using namespace cadabra;

TeXView::TeXView(TeXEngine& eng, DTree::iterator it, int hmargin)
	:
#ifdef USE_GTK4
	  TeXViewBase(cdb::ORIENT_VERTICAL),
#endif
	  content(0)
	, datacell(it)
	, vbox(cdb::ORIENT_VERTICAL, 10)
	, hbox(cdb::ORIENT_HORIZONTAL, hmargin)
	, image(this)
	, engine(eng)
	, is_error(false)
	{
	// Still need to checkin even when using MicroTeX, otherwise
	// all requests will be empty.
	content = engine.checkin(datacell->textbuf, "", "");
	
// 	add(rbox);
// 	rbox.add(vbox);
// 	rbox.set_reveal_child(false);
// 	rbox.set_transition_duration(1000);
// 	rbox.set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_CROSSFADE); //SLIDE_DOWN);

	vbox.set_margin_top(10);
	vbox.set_margin_bottom(0);
//	vbox.pack_start(hbox, true, 0);
//	image._text_size = text_size();
//	hbox.pack_start(image, true, hmargin);


//	vbox.pack_start(hbox, true, 0);
	image._text_size = text_size();
	cdb::add_child(*this, image);

#ifndef USE_GTK4
	// GTK4 has no event masks; controllers subscribe to what they need.
	add_events( Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK );
#endif
	set_can_focus(true);

	click_gesture = cdb::add_click_gesture(*this);
	click_gesture->set_button(0);   // react to any button; we dispatch ourselves
	click_gesture->signal_pressed().connect(sigc::mem_fun(*this, &TeXView::on_button_pressed));
	click_gesture->signal_released().connect(sigc::mem_fun(*this, &TeXView::on_button_released));

	context_menu.append(item_copy_as_latex);
	item_copy_as_latex.set_label("Copy as LaTeX");
	context_menu.show_all();
	item_copy_as_latex.signal_activate().connect(sigc::mem_fun(*this, &TeXView::on_copy_as_latex));
	}

TeXView::~TeXView()
	{
	engine.checkout(content);
	}

void TeXView::on_copy_as_latex() const
	{
	signal_on_copy_as_latex.emit();
	}

void TeXView::set_is_error(bool is_error_)
	{
	is_error=is_error_;
	}

float TeXView::text_size() const
	{
	// The device pixel ratio scale is applied in the ::on_draw method,
	// so we do not scale the font size by that here. But we do need
	// to scale by the text scaling factor of the system, which is
	// available in engine.get_scale().

	// FIXME: we used to multiply with engine.get_scale() but that seems
	// wrong.
	
	float ret = 28.0f/12.0f*engine.get_font_size()*engine.get_scale()/engine.get_device_scale()/1.7f;
//	std::cerr << "engine.font_size = " << engine.get_font_size() << ", .scale = " << engine.get_scale()
//				 << ", text_size = " << ret << std::endl;
	return ret;
	}

void TeXView::on_show()
	{
	// std::cerr << "**** show called" << std::endl;
	convert();
	TeXViewBase::on_show();
	}

Gtk::SizeRequestMode TeXView::TeXArea::get_request_mode_vfunc() const
	{
	return cdb::SIZE_REQUEST_HEIGHT_FOR_WIDTH;
	}

/// The MicroTeX height-for-width computation, shared between gtkmm-3's
/// get_preferred_height_for_width_vfunc and GTK4's measure_vfunc.

void TeXView::TeXArea::measure_height_for_width(int width,
															  int& minimum_height, int& natural_height) const
	{
	if(width==1) {
		minimum_height=9999;
		natural_height=9999;
		// std::cerr << "**** skipped bogus width" << std::endl;
		}
	else {
		// If we are given a width which is not the window_width - 20, force the
		// width. This should not happen, but alas, it does.
		// std::cerr << width << " > " << owner->window_width - 20 << "?" << std::endl;
		if(owner->window_width != 0) {
			if(width > owner->window_width - 20)
				width = owner->window_width - 20;
			}
		int remember = rendering_width;
		// Never go negative: MicroTeX cannot lay out to it, and it would
		// propagate into set_size_request() further down.
		rendering_width = std::max(0, width - 2*padding_x);
		layout_latex();
		if(_render!=nullptr) {
			minimum_height = _render->getHeight() + 2*padding_y;
			natural_height = _render->getHeight() + 2*padding_y;
			//		std::cerr << "**** computed for width " << width << " height as " << natural_height << std::endl;
			}
		if(rendering_width==9999)
			rendering_width = remember;
		}
	// std::cerr << "**** asked height for width " << width << ", replied " << minimum_height << std::endl;
	}

/// The re-layout that follows a new allocation, shared between gtkmm-3's
/// on_size_allocate and GTK4's size_allocate_vfunc.

void TeXView::TeXArea::relayout_for_allocation(int width, int height)
	{
	if(width != rendering_width + 2*padding_x) {
//		std::cerr << "**** need to rerender" << std::endl;
		rendering_width = std::max(0, width - 2*padding_x);
		layout_latex();
		}
	// Some truly undocumented behaviour is happening when messing with
	// size allocation in gtk-3.0. In any case, if we need a new height,
	// we need to request a new size as below.
	if(_render==nullptr) return;
	int extra = (int) (padding_y * 2);
	int my_height = _render->getHeight() + extra;
	if(height != my_height) {
		// std::cerr << "*** need new height " << my_height << std::endl;
		set_size_request(rendering_width, my_height);
		queue_resize();
		// queue_draw();
		}
	}

int TeXView::TeXArea::need_height(int width) const
	{
	int h1=0, h2=0;
	measure_height_for_width(width, h1, h2);
	return h1;
	}

#ifdef USE_GTK4

void TeXView::TeXArea::measure_vfunc(Gtk::Orientation orientation, int for_size,
												 int& minimum, int& natural,
												 int& minimum_baseline, int& natural_baseline) const
	{
	minimum_baseline = -1;
	natural_baseline = -1;

	if(orientation==cdb::ORIENT_VERTICAL) {
		// GTK4 passes -1 for an unconstrained width; gtkmm-3 used 1 as its
		// sentinel for a bogus width, so map onto that.
		const int width = (for_size < 0) ? 1 : for_size;
		measure_height_for_width(width, minimum, natural);
		return;
		}

	// FIXME: the gtkmm-3 path additionally forces its own allocation width to
	// owner->window_width - 20 in on_size_allocate. A GTK4 widget cannot
	// override the size its parent gives it, so that would have to be reported
	// from here (HORIZONTAL) instead. Falling through to the base class for now.
	Gtk::DrawingArea::measure_vfunc(orientation, for_size, minimum, natural,
											  minimum_baseline, natural_baseline);
	}

void TeXView::TeXArea::size_allocate_vfunc(int width, int height, int baseline)
	{
	Gtk::DrawingArea::size_allocate_vfunc(width, height, baseline);
	relayout_for_allocation(width, height);
	}

#else

void TeXView::TeXArea::get_preferred_height_for_width_vfunc(int width,
																				int& minimum_height, int& natural_height) const
	{
	measure_height_for_width(width, minimum_height, natural_height);
	}

void TeXView::TeXArea::get_preferred_width_for_height_vfunc(int height,
																				int& minimum_width, int& natural_width) const
	{
	Gtk::Widget::get_preferred_width_for_height_vfunc(height, minimum_width, natural_width);
	return;

//	if(height==0) {
//		minimum_width = 99999;
//		natural_width = 99999;
//		}
//	else {
//		minimum_width = rendering_width + 2 * padding_x;
//		natural_width = rendering_width + 2 * padding_x;
//		}
//	std::cerr << "**** asked width for height " << height << ", replied " << minimum_width << std::endl;
	}

void TeXView::TeXArea::on_size_allocate(Gtk::Allocation& allocation)
	{
	if(owner->window_width != 0) {
		// std::cerr << "setting texview width = " << owner->window_width - 20 << std::endl;
		allocation.set_width(owner->window_width - 20);
		// allocation.set_height(need_height(owner->window_width - 20));
		}
	set_allocation(allocation);
	// std::cerr << "**** offered allocation " << allocation.get_width()
	// 			 << " x " << allocation.get_height() << std::endl;

	relayout_for_allocation(allocation.get_width(), allocation.get_height());
	}

#endif

void TeXView::convert()
	{
//	std::cerr << "*** convert called" << std::endl;
	try {
		// The `content` variable is the TeXRequest; MicroTeX only needs the
		// LaTeX string out of it.
		image.update_image(content);
		}
	catch(TeXEngine::TeXException& ex) {
		tex_error.emit(ex.what());
		}
	}


void TeXView::dim(bool d)
	{
	if(d) image.set_opacity(0.3);
	else  image.set_opacity(1.0);
	}

void TeXView::on_button_released(int /* n_press */, double /* x */, double /* y */)
	{
	if(click_gesture->get_current_button() == 1) {
		show_hide_requested.emit(datacell);
		click_gesture->set_state(cdb::SEQ_CLAIMED);
		}
	}

void TeXView::on_button_pressed(int n_press, double /* x */, double /* y */)
	{
	// Only a single (not double/triple) press, matching the old
	// event->type==GDK_BUTTON_PRESS test.
	if(n_press != 1) return;

	if(click_gesture->get_current_button() == 3) {
		// nullptr means 'the event currently being processed'.
		context_menu.popup_at_pointer(nullptr);
		click_gesture->set_state(cdb::SEQ_CLAIMED);
		}
	else {
		select_requested.emit();
		}
	}

void TeXView::update_image()
	{
	float new_size = text_size();
	if(image._text_size != new_size) {
		image._text_size = new_size;
		image.layout_latex();
		}
	image.update_image(content);
	}

void TeXView::TeXArea::update_image(std::shared_ptr<TeXEngine::TeXRequest> content)
	{
	if(content) {
		if(content->latex()!=unfixed)
			set_latex(content->latex());
		}
	}

void TeXView::TeXArea::layout_latex() const
	{
	if(_render) {
		delete _render;
		_render = nullptr;
		}

//	std::cerr << "running layout with text_size = " << _text_size << std::endl;
	try {
		// std::cerr << "fixed = " << fixed << std::endl;
		_render = microtex::MicroTeX::parse(
			fixed, //microtex::utf82wide(fixed),
			rendering_width,
			_text_size,
			_text_size / 3.f,
			0xff424242);
		}
	catch(std::exception& err) {
		// std::cerr << "MicroTeX::parse: exception parsing input (" << fixed << "): " << err.what() << std::endl;
		owner->tex_error.emit(err.what());

		// Instead put an error message in the output cell.
		std::cerr << err.what() << std::endl;
		std::cerr << fixed << std::endl;
		_render = microtex::MicroTeX::parse(
			"\\text{\\textcolor{red}{\\LaTeX error, probably mismatching brackets.}}", //microtex::utf82wide(fixed),
			rendering_width,
			_text_size,
			_text_size / 3.f,
			0xff424242);
		}
	}

guint32 color_from_rgba(Gdk::RGBA color)
	{
	return
		(guint8)(color.get_alpha()*255)<<24|
		(guint8)(color.get_red()*255)<<16|
		(guint8)(color.get_green()*255)<<8|
		(guint8)(color.get_blue()*255);
	}

#ifndef USE_GTK4
bool TeXView::TeXArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	return draw_impl(cr);
	}
#else
void TeXView::TeXArea::draw(const Cairo::RefPtr<Cairo::Context>& cr, int /* width */, int /* height */)
	{
	draw_impl(cr);
	}
#endif

bool TeXView::TeXArea::draw_impl(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	auto style_context = get_style_context();
	style_context->render_background(cr, 0, 0, get_width(), get_height());
	Gdk::RGBA fg_colour;
	// Theme colour names can be found at
	// https://stackoverflow.com/questions/35045469/default-gtk-css-color-scheme
	if(owner->is_error)
		style_context->lookup_color("error_color", fg_colour);
	else
		style_context->lookup_color("theme_text_color", fg_colour);

	if (_render == nullptr) return true;

	microtex::Graphics2D_cairo g2(cr->cobj());
	_render->setForeground(color_from_rgba(fg_colour));
	_render->draw(g2, padding_x, padding_y);

	return true;
	}

std::string TeXView::TeXArea::verbatimise(const std::string& inp) const
	{
	std::string line;
	line = std::regex_replace(inp,  std::regex(R"(\\)"), "\\,\\backslash\\! ");
	line = std::regex_replace(line, std::regex(R"(\{)"), "\\{");
	line = std::regex_replace(line, std::regex(R"(\})"), "\\}");					
	line = std::regex_replace(line, std::regex(R"(_)"), "\\_");
	line = std::regex_replace(line, std::regex(R"(\~)"), "\\widetilde{~}");
	return line;
	}

void TeXView::TeXArea::set_latex(const std::string& latex)
	{
	//std::cout << "**** fixing latex " << latex << std::endl;
	std::regex discretionary(R"(\\discretionary\{\}\{\}\{\})");
	std::regex begin_dmath(R"(\\begin\{dmath\*\})");
	std::regex end_dmath(R"(\\end\{dmath\*\})");

	unfixed = latex;
	if(latex.find(R"(\begin{dmath*})")==0) {
		// math mode
		std::regex spacenewline(R"(\\\\\[.*\])");
		fixed = std::regex_replace(latex, begin_dmath, "");
		fixed = std::regex_replace(fixed, end_dmath, "");
		fixed = std::regex_replace(fixed, discretionary, "\\-{}");
		fixed = std::regex_replace(fixed, spacenewline, "\\\\");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\+)"),
											" + ");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\linebreak\[[0-9]\])"),
											"");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\square)"),
											"□");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\bigO)"),
											"\\mathcal{O}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\dot\{\\dot\{)"),
											"\\ddot{{");
		fixed = std::regex_replace(fixed, std::regex(R"(\{\}_)"), "{\\,\\!}_");
		fixed = std::regex_replace(fixed, std::regex(R"(\{\}\^)"), "{\\,\\!}^");
		fixed = "\\text{$\\displaystyle{}"+fixed+"$}";
		}
	else {
		// text mode

		// In a verbatim block, replace all newlines with explicit '\\'.
		std::string line;
		std::istringstream ss(latex);
		bool inverbatim=false;
		std::string beg("\\begin{verbatim}");
		std::string end("\\end{verbatim}");
		fixed="";
		while(std::getline(ss, line)) {
			size_t pos=line.find(beg);
			if(pos!=std::string::npos) {
				inverbatim=true;
				if(pos!=0)
					fixed += line.substr(0, pos);
				fixed += "\\texttt{";
				size_t endpos = pos+beg.size(); 
				if(line.size() > endpos)
					line = line.substr(endpos);
				else
					line = "";
				}
			pos = line.find(end);
			if(pos!=std::string::npos) {
				if(pos>0)
					line = line.substr(0, pos);
				else
					line = "";
				}
			if(line.size()>0) {
				if(inverbatim) {
					line = verbatimise(line);
					fixed += line+"\\\\";
					}
				else {
					fixed += line+"\n";
					}
				}
			if(pos!=std::string::npos) {
				inverbatim=false;
				fixed += "}";
				}
			}

		// Now the rest.
		fixed = std::regex_replace(fixed,
											std::regex("\n"),
											" ");
		fixed = std::regex_replace(fixed, begin_dmath, "");
		fixed = std::regex_replace(fixed, end_dmath, "");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\color)"),
											"\\textcolor");
		fixed = std::regex_replace(fixed, discretionary, "\\-{}");
		fixed = std::regex_replace(fixed, std::regex(R"(\\textbackslash)"), "\\backslash");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\section\*\{([^\}]*)\}[ ]*)"),
											"\\text{\\Large\\textbf{$1}}\\\\\\vspace{2.5ex}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\subsection\*\{([^\}]*)\}[ ]*)"),
											"\\text{\\large\\textbf{$1}}\\\\\\vspace{2.0ex}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\package\{([^\}]*)\}\{([^\}]*)\})"),
											"\\text{\\large\\texttt{$1}}\\\\\\vspace{2.5ex}\\text{\\textit{$2}}\\\\");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\algorithm\{([^\}]*)\}\{([^\}]*)\}[ ]*)"),
											"\\text{\\large\\texttt{$1}}\\\\\\vspace{2.5ex}\\text{\\textit{$2}}\\\\\\vspace{2.5ex}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\available\{([^\}]*)\}[ ]*)"),
											"{\\!\\small available since: $1}\\\\\\vspace{3.5ex}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\property\{([^\}]*)\}\{([^\}]*)\}[ ]*)"),
											"\\text{\\large\\texttt{$1}}\\\\\\vspace{2.5ex}\\text{\\textit{$2}}\\\\\\vspace{2.5ex}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\url\{([^\}]*)\})"),
											"\\texttt{$1}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\href\{([^\}]*)\}\{([^\}]*)\})"),
											"\\texttt{$2}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\algo\{([^\}]*)\})"),
											"\\texttt{$1}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\prop\{([^\}]*)\})"),
											"\\texttt{$1}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\emph\{([^\}]*)\})"),
											"\\textit{$1}");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\begin\{equation\*?\})"),
											"$");
		fixed = std::regex_replace(fixed,
											std::regex(R"(\\end\{equation\*?\})"),
											"$");

		// Finally, the inline verbatim.
		auto verb_regex = std::regex(R"(\\verb\|([^\|]*)\|)");
		std::sregex_iterator it(fixed.begin(), fixed.end(), verb_regex);
		std::sregex_iterator endit;
		size_t last_pos=0;
		std::string result;
		for (; it != endit; ++it) {
			result += fixed.substr(last_pos, it->position() - last_pos);
			std::string transformed = verbatimise(it->str(1));
			result += "\\texttt{" + transformed + "}";
			last_pos = it->position() + it->length();
			}
		if(last_pos < fixed.length()) 
			result += fixed.substr(last_pos);
		fixed=result;
		
		fixed = "\\text{"+fixed+"}";
		}

   //	fixed = "\\text{$A_{m n}$}";
	// std::cout << "**** fixed to " << fixed << std::endl;
	}

TeXView::TeXArea::TeXArea(TeXView *owner_)
	: rendering_width(1)
	, owner(owner_)
	, _render(nullptr), _text_size(5.f)
	, padding_x(15*1.7), padding_y(10)
	{
	set_hexpand(true);
	set_name("TeXArea");
#ifdef USE_GTK4
	// GTK4 removed the on_draw vfunc in favour of a draw callback.
	set_draw_func(sigc::mem_fun(*this, &TeXArea::draw));
#endif
	cdb::set_accessible_name(*this, "TeXArea");
	cdb::set_accessible_role(*this, cdb::AccessibleRole::math);
	set_focus_on_click(true);
	}

TeXView::TeXArea::~TeXArea()
	{
	if(_render)
		delete _render;
	}

