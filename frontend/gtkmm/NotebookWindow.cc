
#include <iostream>
#include "Actions.hh"
#include "Functional.hh"
#include "Cadabra.hh"
#include "Config.hh"
#include "InstallPrefix.hh"
#include "NotebookWindow.hh"
#include "DataCell.hh"

#include <gtkmm/box.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/radioaction.h>
#include <gtkmm/scrollbar.h>
#include <giomm/actiongroup.h>
#include <fstream>
#include <glib/gstdio.h>

#include <gtkmm/entry.h>
#if GTKMM_MINOR_VERSION < 10
#include <gtkmm/main.h>
#endif
#include "Snoop.hh"
#include "ChooseColoursDialog.hh"
#include "SelectFileDialog.hh"
#include "process.hpp"
#include <internal/string_tools.h>

// For MicroTeX
#include "cairo/graphic_cairo.h"
#include "microtex.h"
#include <pangomm/init.h>

using namespace cadabra;

// #define DEBUG 1

NotebookWindow::NotebookWindow(Cadabra *c, bool ro)
	: Gtk::ApplicationWindow()
	, DocumentThread(this)
	, current_cell(doc.end())
	, cdbapp(c)
	, search_case_insensitive("Case insensitive", true)
	, console(sigc::mem_fun(this, &NotebookWindow::interactive_execute))
	, current_canvas(0)
	, kernel_spinner_status(false)
	, progress_frac(0)
	, status_line(-1)
	, status_col(-1)
	, title_prefix("Cadabra: ")
	, modified(false)
	, read_only(ro)
	, crash_window_hidden(true)
	, last_configure_width(0)
	, follow_cell(doc.end())
	, tex_running(false), tex_need_width(0)
	, last_find_location(doc.end(), std::string::npos)
	, is_configured(false)

	{
	// Connect the dispatcher.
	dispatcher.connect(sigc::mem_fun(*this, &NotebookWindow::process_todo_queue));
	dispatch_refresh.connect(sigc::mem_fun(*this, &NotebookWindow::refresh_after_tex_engine_run));
	dispatch_tex_error.connect(sigc::mem_fun(*this, &NotebookWindow::handle_thread_tex_error));
	dispatch_update_status.connect(sigc::mem_fun(*this, &NotebookWindow::update_status));

	// Set the window icon.
	set_icon_name("cadabra2-gtk");

	// Query high-dpi settings. For all systems we can probe the
	// HiDPI scale, and for some window managers we also probe the
	// text scale factor.
	auto display = Gdk::Display::get_default();
	auto screen = Gdk::Screen::get_default();
	scale = screen->get_monitor_scale_factor(0);
	display_scale = scale;
//	std::cerr << "Monitor scale factor = " << display_scale << std::endl;
#ifndef __APPLE__
	// FIXME: this does not work for ssh sessions with ForwardX11.
	const char *ds = std::getenv("DESKTOP_SESSION");
	if(ds) {
		settings = Gio::Settings::create((strcmp(ds, "cinnamon") == 0) ? "org.cinnamon.desktop.interface" : "org.gnome.desktop.interface");
		scale *= settings->get_double("text-scaling-factor");
		}
#endif

	engine.set_scale(scale, display_scale);
	engine.set_font_size(12+(prefs.font_step*2));

	// For MicroTeX
//	Pango::init(); Gtk::Main supposedly calls this for us.
	std::vector<std::string> font_paths;
//	font_paths.push_back(install_prefix()+"/share/cadabra2/microtex/newcm/");	
	font_paths.push_back(install_prefix()+"/share/cadabra2/microtex/lm-math/");	
//	font_paths.push_back(install_prefix()+"/share/cadabra2/microtex/xits/");
//	font_paths.push_back(install_prefix()+"/share/cadabra2/microtex/cm/");	

	// Need to re-convert fonts setting the correct

//	for(const std::string& path: font_paths)
//		std::cerr << "Will search " << path << " for fonts." << std::endl;
	
	microtex::MicroTeX::setDefaultMathFont("LatinModernMath-Regular");
	auto auto_init = microtex::InitFontSenseAuto();
//	std::cerr << "microtex::InitFontSenseAuto done" << std::endl;
	auto_init.search_paths=font_paths;
	microtex::Init init = auto_init;
	microtex::MicroTeX::init(init);
	microtex::MicroTeX::setDefaultMathFont("LatinModernMath-Regular");

   // Add all the CMR fonts (as one family). See the url below for
	// a list of all fonts with their standard names:
	//
	// https://usemodify.com/fonts/computer-modern/
	
	const microtex::FontSrcFile font_rm
		= microtex::FontSrcFile(install_prefix()+"/share/cadabra2/microtex/cm/cmunrm.clm1",
										install_prefix()+"/share/cadabra2/microtex/cm/cmunrm.otf");
	const microtex::FontSrcFile font_tt
		= microtex::FontSrcFile(install_prefix()+"/share/cadabra2/microtex/cm/cmuntt.clm1",
										install_prefix()+"/share/cadabra2/microtex/cm/cmuntt.otf");
	const microtex::FontSrcFile font_tt_bf
		= microtex::FontSrcFile(install_prefix()+"/share/cadabra2/microtex/cm/cmuntb.clm1",
										install_prefix()+"/share/cadabra2/microtex/cm/cmuntb.otf");
	const microtex::FontSrcFile font_bf
		= microtex::FontSrcFile(install_prefix()+"/share/cadabra2/microtex/cm/cmunbx.clm1",
										install_prefix()+"/share/cadabra2/microtex/cm/cmunbx.otf");
	const microtex::FontSrcFile font_it
		= microtex::FontSrcFile(install_prefix()+"/share/cadabra2/microtex/cm/cmunti.clm1",
										install_prefix()+"/share/cadabra2/microtex/cm/cmunti.otf");
	const microtex::FontSrcFile font_it_bf
		= microtex::FontSrcFile(install_prefix()+"/share/cadabra2/microtex/cm/cmunbi.clm1",
										install_prefix()+"/share/cadabra2/microtex/cm/cmunbi.otf");

	microtex::MicroTeX::addFont(font_rm,    "Computer Modern");
	microtex::MicroTeX::addFont(font_tt,    "Computer Modern");
	microtex::MicroTeX::addFont(font_tt_bf, "Computer Modern"); // FIXME: incorrect
	microtex::MicroTeX::addFont(font_bf,    "Computer Modern");
	microtex::MicroTeX::addFont(font_it,    "Computer Modern");
	microtex::MicroTeX::addFont(font_it_bf, "Computer Modern");
	
	microtex::MicroTeX::setDefaultMainFont("Computer Modern");
	microtex::PlatformFactory::registerFactory("gtk", std::make_unique<microtex::PlatformFactory_cairo>());
	microtex::PlatformFactory::activate("gtk");

	// std::cerr << "MicroTeX::hasGlyphPathRender = " << microtex::MicroTeX::hasGlyphPathRender() << std::endl;
	// std::cerr << "Math fonts:" << std::endl;
	// for(const auto& n: microtex::MicroTeX::mathFontNames()) {
	// 	std::cerr << "   " << n << std::endl;
	// 	}
	// std::cerr << "Main fonts:" << std::endl;
	// for(const auto& n: microtex::MicroTeX::mainFontFamilies()) {
	// 	std::cerr << "   " << n << std::endl;
	// 	}
	
#ifndef __APPLE__
	if(ds) {
		settings->signal_changed().connect(
		   sigc::mem_fun(*this, &NotebookWindow::on_text_scaling_factor_changed));
		}
#endif


	// Setup styling. Note that 'margin-left' and so on do not work; you need
	// to use 'padding'. However, 'padding-top' fails because it does not make the
	// widget larger enough... So we still do that with set_margin_top(...).
	css_provider = Gtk::CssProvider::create();
	// padding-left: 20px; does not work on some versions of gtk, so we use margin in CodeInput
	// We use CSS selectors for old-style and new-style (post 3.20) simultaneously.
	// Run program with 'GTK_DEBUG=interactive' environment variable and press Ctrl-Shift-D
	// to inspect.
	load_css();

	//	auto screen = Gdk::Screen::get_default();
	//	std::cerr << "cadabra-client: scale = " << screen->get_monitor_scale_factor(0) << std::endl;
	Gtk::StyleContext::add_provider_for_screen(screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	// Setup menu.
	actiongroup = Gio::SimpleActionGroup::create();

	// File menu actions.
	actiongroup->add_action( "New",                   sigc::mem_fun(*this, &NotebookWindow::on_file_new) );
	actiongroup->add_action( "Open",                  sigc::mem_fun(*this, &NotebookWindow::on_file_open) );
	actiongroup->add_action( "Close",                 sigc::mem_fun(*this, &NotebookWindow::on_file_close) );
	actiongroup->add_action( "Save",                  sigc::mem_fun(*this, &NotebookWindow::on_file_save) );
	actiongroup->add_action( "SaveAs",                sigc::mem_fun(*this, &NotebookWindow::on_file_save_as) );
	actiongroup->add_action( "ExportAsJupyter",       sigc::mem_fun(*this, &NotebookWindow::on_file_save_as_jupyter) );
	actiongroup->add_action( "ExportHtml",            sigc::mem_fun(*this, &NotebookWindow::on_file_export_html) );
	actiongroup->add_action( "ExportHtmlSegment",     sigc::mem_fun(*this, &NotebookWindow::on_file_export_html_segment) );
	actiongroup->add_action( "ExportLaTeX",           sigc::mem_fun(*this, &NotebookWindow::on_file_export_latex) );
	actiongroup->add_action( "ExportPython",          sigc::mem_fun(*this, &NotebookWindow::on_file_export_python) );
	actiongroup->add_action( "Quit",                  sigc::mem_fun(*this, &NotebookWindow::on_file_quit) );

	// Edit menu actions for undo/redo; enabling/disabling of the above
	// is done in update_title.
	action_undo = Gio::SimpleAction::create("EditUndo");
	action_redo = Gio::SimpleAction::create("EditRedo");	
	action_undo->signal_activate().connect( sigc::mem_fun(*this, &NotebookWindow::on_edit_undo) );
	action_redo->signal_activate().connect( sigc::mem_fun(*this, &NotebookWindow::on_edit_redo) );
	actiongroup->add_action( action_undo );
	actiongroup->add_action( action_redo );

	// Edit menu actions for copy/paste.
	action_copy = Gio::SimpleAction::create("EditCopy");
	action_copy->signal_activate().connect( sigc::mem_fun(*this, &NotebookWindow::on_edit_copy) );
	action_copy->set_enabled(false);
	actiongroup->add_action( action_copy );
	actiongroup->add_action( "EditPaste",             sigc::mem_fun(*this, &NotebookWindow::on_edit_paste) );
	actiongroup->add_action( "EditInsertAbove",       sigc::mem_fun(*this, &NotebookWindow::on_edit_insert_above) );
	actiongroup->add_action( "EditInsertBelow",       sigc::mem_fun(*this, &NotebookWindow::on_edit_insert_below) );
	actiongroup->add_action( "EditDelete",            sigc::mem_fun(*this, &NotebookWindow::on_edit_delete) );
	actiongroup->add_action( "EditSplit",             sigc::mem_fun(*this, &NotebookWindow::on_edit_split) );
	actiongroup->add_action( "EditFind",              sigc::mem_fun(*this, &NotebookWindow::on_edit_find) );
	actiongroup->add_action( "EditFindNext",          sigc::mem_fun(*this, &NotebookWindow::on_search_text_changed) );
	actiongroup->add_action( "EditMakeCellTeX",       sigc::mem_fun(*this, &NotebookWindow::on_edit_cell_is_latex) );
	actiongroup->add_action( "EditMakeCellPython",    sigc::mem_fun(*this, &NotebookWindow::on_edit_cell_is_python) );	
	actiongroup->add_action( "EditIgnoreCellOnImport",sigc::mem_fun(*this, &NotebookWindow::on_ignore_cell_on_import) );	
	action_auto_close_latex = Gio::SimpleAction::create_bool("AutoCloseLaTeX", prefs.auto_close_latex);
	action_auto_close_latex->signal_activate().connect( sigc::mem_fun(*this, &NotebookWindow::on_prefs_auto_close_latex) );
	actiongroup->add_action(action_auto_close_latex);

	// View menu actions.
	actiongroup->add_action( "ViewSplit",             sigc::mem_fun(*this, &NotebookWindow::on_view_split) );
	action_view_close = Gio::SimpleAction::create("ViewClose");
	action_view_close->signal_activate().connect( sigc::mem_fun(*this, &NotebookWindow::on_view_close) );	
	actiongroup->add_action( action_view_close );
	
	action_fontsize  = actiongroup->add_action_radio_integer( "FontSize",
																				 sigc::mem_fun(*this, &NotebookWindow::on_prefs_font_size),
																				 prefs.font_step );	
	action_highlight = actiongroup->add_action_radio_integer( "HighlightSyntax",
																				 sigc::mem_fun(*this, &NotebookWindow::on_prefs_highlight_syntax),
																				 prefs.highlight );	
	action_microtex = actiongroup->add_action_radio_integer( "MicroTeX",
																				 sigc::mem_fun(*this, &NotebookWindow::on_prefs_microtex),
																				 prefs.microtex );	
	actiongroup->add_action( "HighlightChoose",       sigc::mem_fun(*this, &NotebookWindow::on_prefs_choose_colours) );		
	actiongroup->add_action( "ViewUseDefaultSettings",sigc::mem_fun(*this, &NotebookWindow::on_prefs_use_defaults) );		

	// Evaluate menu actions.
	actiongroup->add_action( "EvaluateCell",          sigc::mem_fun(*this, &NotebookWindow::on_run_cell) );
	actiongroup->add_action( "EvaluateAll",           sigc::mem_fun(*this, &NotebookWindow::on_run_runall) );		
	actiongroup->add_action( "EvaluateToCursor",      sigc::mem_fun(*this, &NotebookWindow::on_run_runtocursor) );		
	action_stop = actiongroup->add_action( "EvaluateStop",  sigc::mem_fun(*this, &NotebookWindow::on_run_stop) );

	// Kernel menu actions.
	actiongroup->add_action( "KernelRestart",         sigc::mem_fun(*this, &NotebookWindow::on_kernel_restart) );			
	
	// Tools menu actions.
	actiongroup->add_action( "CompareFile",           sigc::mem_fun(*this, &NotebookWindow::compare_to_file) );
	actiongroup->add_action( "CompareGitLatest",      sigc::mem_fun(*this, &NotebookWindow::compare_git_latest) );
	actiongroup->add_action( "CompareGitChoose",      sigc::mem_fun(*this, &NotebookWindow::compare_git_choose) );
	actiongroup->add_action( "CompareGitSpecific",    sigc::mem_fun(*this, &NotebookWindow::compare_git_specific) );
	actiongroup->add_action( "CompareGitSelect",      sigc::mem_fun(*this, &NotebookWindow::select_git_path) );
	action_console = actiongroup->add_action_radio_integer( "Console", sigc::mem_fun(*this, &NotebookWindow::on_prefs_set_cv), 0 );
	actiongroup->add_action( "ToolsOptions",          sigc::mem_fun(*this, &NotebookWindow::on_tools_options) );
	actiongroup->add_action( "ToolsClearCache",       sigc::mem_fun(*this, &NotebookWindow::on_tools_clear_cache) );	
	
   // Help menu actions.
	actiongroup->add_action( "HelpAbout",             sigc::mem_fun(*this, &NotebookWindow::on_help_about) );
	actiongroup->add_action( "HelpContext",           sigc::mem_fun(*this, &NotebookWindow::on_help) );
	action_register = actiongroup->add_action( "HelpRegister",  sigc::mem_fun(*this, &NotebookWindow::on_help_register) );
	action_register->set_enabled(!prefs.is_registered);

	insert_action_group("cdb",  actiongroup);

	// Set shortcuts for actions.
	cdbapp->set_accel_for_action("cdb.New",                    "<Primary>N");
	cdbapp->set_accel_for_action("cdb.Open",                   "<Primary>O");
	cdbapp->set_accel_for_action("cdb.Close",                  "<Primary>W");
	cdbapp->set_accel_for_action("cdb.Save",                   "<Primary>S");
	cdbapp->set_accel_for_action("cdb.Quit",                   "<Primary>Q");
	cdbapp->set_accel_for_action("cdb.EditUndo",               "<Primary>Z");
	cdbapp->set_accel_for_action("cdb.EditRedo",               "<Primary>Y");
	cdbapp->set_accel_for_action("cdb.EditCopy",               "<Primary>C");
	cdbapp->set_accel_for_action("cdb.EditPaste",              "<Primary>V");
	cdbapp->set_accel_for_action("cdb.EditInsertAbove",        "<alt>Up");
	cdbapp->set_accel_for_action("cdb.EditInsertBelow",        "<alt>Down");
	cdbapp->set_accel_for_action("cdb.EditDelete",             "<ctrl>Delete");
	cdbapp->set_accel_for_action("cdb.EditFind",               "<control>F");
	cdbapp->set_accel_for_action("cdb.EditFindNext",           "<control>G");
	cdbapp->set_accel_for_action("cdb.EditMakeCellTeX",        "<control><shift>L");
	cdbapp->set_accel_for_action("cdb.EditMakeCellPython",     "<control><shift>P");
	cdbapp->set_accel_for_action("cdb.EditIgnoreCellOnImport", "<control><shift>I");
	cdbapp->set_accel_for_action("cdb.EditSplit",              "<control>Return");
	cdbapp->set_accel_for_action("cdb.EvaluateCell",           "<shift>Return");
	cdbapp->set_accel_for_action("cdb.KernelRestart",          "<control><alt>R");
	cdbapp->set_accel_for_action("cdb.HelpContext",            "F1");

	// Menu and toolbar layout.
	uimanager = Gtk::Builder::create();
	Glib::ustring ui_info="<interface></interface>";

	if(!read_only) {
		ui_info =
	   "<interface>"
	   "  <menu id='MenuBar'>"
		"    <submenu>"
	   "      <attribute name='label'>File</attribute>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>New</attribute>"
		"          <attribute name='action'>cdb.New</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Open</attribute>"
		"          <attribute name='action'>cdb.Open</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Close</attribute>"
		"          <attribute name='action'>cdb.Close</attribute>"
		"        </item>"
		"      </section>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Save</attribute>"
		"          <attribute name='action'>cdb.Save</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Save as...</attribute>"
		"          <attribute name='action'>cdb.SaveAs</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Export as Jupyter notebook</attribute>"
		"          <attribute name='action'>cdb.ExportAsJupyter</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Export as HTML</attribute>"
		"          <attribute name='action'>cdb.ExportHtml</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Export as HTML segment</attribute>"
		"          <attribute name='action'>cdb.ExportHtmlSegment</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Export as LaTex</attribute>"
		"          <attribute name='action'>cdb.ExportLaTeX</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Export as Python</attribute>"
		"          <attribute name='action'>cdb.ExportPython</attribute>"
		"        </item>"
		"      </section>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Quit</attribute>"
		"          <attribute name='action'>cdb.Quit</attribute>"
		"        </item>"
		"      </section>"
		"    </submenu>"
		"    <submenu>"
	   "      <attribute name='label'>Cell</attribute>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Insert above</attribute>"
		"          <attribute name='action'>cdb.EditInsertAbove</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Insert below</attribute>"
		"          <attribute name='action'>cdb.EditInsertBelow</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Delete cell</attribute>"
		"          <attribute name='action'>cdb.EditDelete</attribute>"
		"        </item>"
		"      </section>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Split cell</attribute>"
		"          <attribute name='action'>cdb.EditSplit</attribute>"
		"        </item>"
		"      </section>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Make cell LaTeX</attribute>"
		"          <attribute name='action'>cdb.EditMakeCellTeX</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Make cell Python</attribute>"
		"          <attribute name='action'>cdb.EditMakeCellPython</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Ignore cell on import</attribute>"
		"          <attribute name='action'>cdb.EditIgnoreCellOnImport</attribute>"
		"        </item>"
		"      </section>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Auto-close LaTeX cells</attribute>"
		"          <attribute name='action'>cdb.AutoCloseLaTeX</attribute>"
		"        </item>"
		"      </section>"
		"    </submenu>"
		"    <submenu>"
	   "      <attribute name='label'>Edit</attribute>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Undo</attribute>"
		"          <attribute name='action'>cdb.EditUndo</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Redo</attribute>"
		"          <attribute name='action'>cdb.EditRedo</attribute>"
		"        </item>"
		"      </section>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Copy</attribute>"
		"          <attribute name='action'>cdb.EditCopy</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Paste</attribute>"
		"          <attribute name='action'>cdb.EditPaste</attribute>"
		"        </item>"
		"      </section>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Find</attribute>"
		"          <attribute name='action'>cdb.EditFind</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Find next</attribute>"
		"          <attribute name='action'>cdb.EditFindNext</attribute>"
		"        </item>"
		"      </section>"
		"    </submenu>"
		"    <submenu>"
	   "      <attribute name='label'>View</attribute>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Split view</attribute>"
		"          <attribute name='action'>cdb.ViewSplit</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Close view</attribute>"
		"          <attribute name='action'>cdb.ViewClose</attribute>"
		"        </item>"
		"      </section>"
		"      <submenu>"
		"        <attribute name='label'>Font size</attribute>"
		"        <section>"
	   "           <item>"
		"              <attribute name='label'>Small</attribute>"
		"              <attribute name='action'>cdb.FontSize</attribute>"
		"              <attribute name='target' type='i'>-1</attribute>"
		"           </item>"
	   "           <item>"
		"              <attribute name='label'>Medium (default)</attribute>"
		"              <attribute name='action'>cdb.FontSize</attribute>"
		"              <attribute name='target' type='i'>0</attribute>"
		"           </item>"
	   "           <item>"
		"              <attribute name='label'>Large</attribute>"
		"              <attribute name='action'>cdb.FontSize</attribute>"
		"              <attribute name='target' type='i'>2</attribute>"
		"           </item>"
	   "           <item>"
		"              <attribute name='label'>Extra large</attribute>"
		"              <attribute name='action'>cdb.FontSize</attribute>"
		"              <attribute name='target' type='i'>4</attribute>"
		"           </item>"
		"        </section>"
		"      </submenu>"
		"      <submenu>"
		"        <attribute name='label'>Highlighting</attribute>"
		"        <section>"
	   "           <item>"
		"              <attribute name='label'>Highlighting off</attribute>"
		"              <attribute name='action'>cdb.HighlightSyntax</attribute>"
		"              <attribute name='target' type='i'>0</attribute>"
		"           </item>"
	   "           <item>"
		"              <attribute name='label'>Highlighting on</attribute>"
		"              <attribute name='action'>cdb.HighlightSyntax</attribute>"
		"              <attribute name='target' type='i'>1</attribute>"
		"           </item>"
	   "           <item>"
		"              <attribute name='label'>Choose colours</attribute>"
		"              <attribute name='action'>cdb.HighlightChoose</attribute>"
		"           </item>"
		"        </section>"
			"      </submenu>";
		const char *appdir = getenv("APPDIR");
		if(!appdir) {
			ui_info+=
				"      <submenu>"
				"        <attribute name='label'>Maths rendering</attribute>"
				"        <section>"
				"           <item>"
				"              <attribute name='label'>LaTeX (external)</attribute>"
				"              <attribute name='action'>cdb.MicroTeX</attribute>"
				"              <attribute name='target' type='i'>0</attribute>"
				"           </item>"
				"           <item>"
				"              <attribute name='label'>MicroTeX (internal)</attribute>"
				"              <attribute name='action'>cdb.MicroTeX</attribute>"
				"              <attribute name='target' type='i'>1</attribute>"
				"           </item>"
				"        </section>"
				"      </submenu>";
			}
	   ui_info +=
		"      <item>"
		"         <attribute name='label'>Use default settings</attribute>"
		"         <attribute name='action'>cdb.ViewUseDefaultSettings</attribute>"
		"      </item>"
		"    </submenu>"
		"    <submenu id='MenuEvaluate'>"
	   "      <attribute name='label'>Evaluate</attribute>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Evaluate cell</attribute>"
		"          <attribute name='action'>cdb.EvaluateCell</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Evaluate all</attribute>"
		"          <attribute name='action'>cdb.EvaluateAll</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Evaluate to cursor</attribute>"
		"          <attribute name='action'>cdb.EvaluateToCursor</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Stop evaluation</attribute>"
		"          <attribute name='action'>cdb.EvaluateStop</attribute>"
		"        </item>"
		"      </section>"
		"    </submenu>"
		"    <submenu>"
	   "      <attribute name='label'>Kernel</attribute>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Restart kernel</attribute>"
		"          <attribute name='action'>cdb.KernelRestart</attribute>"
		"        </item>"
		"      </section>"
		"    </submenu>"
		"    <submenu>"
	   "      <attribute name='label'>Tools</attribute>"
		"      <submenu>"
		"        <attribute name='label'>Compare</attribute>"
		"        <section>"
	   "           <item>"
		"             <attribute name='label'>With other file</attribute>"
		"             <attribute name='action'>cdb.CompareFile</attribute>"
		"           </item>"
	   "           <item>"
		"             <attribute name='label'>Last commit</attribute>"
		"             <attribute name='action'>cdb.CompareGitLatest</attribute>"
		"           </item>"
	   "           <item>"
		"             <attribute name='label'>Select commit from list</attribute>"
		"             <attribute name='action'>cdb.CompareGitChoose</attribute>"
		"           </item>"
	   "           <item>"
		"             <attribute name='label'>Manually enter commit hash</attribute>"
		"             <attribute name='action'>cdb.CompareGitSpecific</attribute>"
		"           </item>"
	   "           <item>"
		"             <attribute name='label'>Select GIT binary</attribute>"
		"             <attribute name='action'>cdb.CompareGitSelect</attribute>"
		"           </item>"
		"        </section>"
		"      </submenu>"
		"      <submenu>"
		"        <attribute name='label'>Console</attribute>"
		"        <section>"
	   "           <item>"
		"              <attribute name='label'>Hide</attribute>"
		"              <attribute name='action'>cdb.Console</attribute>"
		"              <attribute name='target' type='i'>0</attribute>"
		"           </item>"
	   "           <item>"
		"              <attribute name='label'>Dock (horizontal)</attribute>"
		"              <attribute name='action'>cdb.Console</attribute>"
		"              <attribute name='target' type='i'>1</attribute>"
		"           </item>"
	   "           <item>"
		"              <attribute name='label'>Dock (vertical)</attribute>"
		"              <attribute name='action'>cdb.Console</attribute>"
		"              <attribute name='target' type='i'>2</attribute>"
		"           </item>"
	   "           <item>"
		"              <attribute name='label'>Floating</attribute>"
		"              <attribute name='action'>cdb.Console</attribute>"
		"              <attribute name='target' type='i'>3</attribute>"
		"           </item>"
		"        </section>"
		"      </submenu>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>Options</attribute>"
		"          <attribute name='action'>cdb.ToolsOptions</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Clear package cache</attribute>"
		"          <attribute name='action'>cdb.ToolsClearCache</attribute>"
		"        </item>"
		"      </section>"
		"    </submenu>"
		"    <submenu>"
	   "      <attribute name='label'>Help</attribute>"
		"      <section>"
	   "        <item>"
		"          <attribute name='label'>About Cadabra</attribute>"
		"          <attribute name='action'>cdb.HelpAbout</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Contextual help</attribute>"
		"          <attribute name='action'>cdb.HelpContext</attribute>"
		"        </item>"
	   "        <item>"
		"          <attribute name='label'>Register</attribute>"
		"          <attribute name='action'>cdb.HelpRegister</attribute>"
		"        </item>"
		"      </section>"
		"    </submenu>"
		"  </menu>"
	   "</interface>";
		};

	uimanager->add_from_string(ui_info);

	// Main box structure dividing the window.
	add(topbox);
	Glib::RefPtr<Glib::Object> menubar_obj = uimanager->get_object("MenuBar");
	Glib::RefPtr<Gio::Menu> gmenu = Glib::RefPtr<Gio::Menu>::cast_dynamic(menubar_obj);
	Gtk::MenuBar* pMenuBar = Gtk::make_managed<Gtk::MenuBar>(gmenu);
	topbox.pack_start(*pMenuBar, Gtk::PACK_SHRINK);

	auto get_icon = [this](const std::string& nm) {
		Glib::RefPtr<Gio::File> ff = Gio::File::create_for_path(nm);
		Glib::RefPtr<Gio::FileInputStream> str = ff->read();
		Glib::RefPtr<Gdk::Pixbuf> pb = Gdk::Pixbuf::create_from_stream_at_scale(str, 50/scale, 50/scale, true);
		return pb;
		};
	
	// Setup the toolbar and buttons in it.
	if(!read_only) {
		toolbar.set_size_request(-1, 70/display_scale);
		tool_stop.add(*Gtk::make_managed<Gtk::Image>(
							  get_icon(install_prefix()+"/share/cadabra2/cdb-icons/cdb-cancel.svg")));
		tool_run.add(*Gtk::make_managed<Gtk::Image>(
							 get_icon(install_prefix()+"/share/cadabra2/cdb-icons/cdb-run.svg")));
		tool_restart.add(*Gtk::make_managed<Gtk::Image>(
								  get_icon(install_prefix()+"/share/cadabra2/cdb-icons/cdb-restart.svg")));
		tool_open.add(*Gtk::make_managed<Gtk::Image>(
							  get_icon(install_prefix()+"/share/cadabra2/cdb-icons/cdb-open.svg")));
		tool_save.add(*Gtk::make_managed<Gtk::Image>(
							  get_icon(install_prefix()+"/share/cadabra2/cdb-icons/cdb-save.svg")));
		tool_save_as.add(*Gtk::make_managed<Gtk::Image>(
								  get_icon(install_prefix()+"/share/cadabra2/cdb-icons/cdb-save-as.svg")));
		tool_stop.set_size_request(70/display_scale, 70/display_scale);
		tool_run.set_size_request(70/display_scale, 70/display_scale);
		tool_restart.set_size_request(70/display_scale, 70/display_scale);
		tool_open.set_size_request(70/display_scale, 70/display_scale);
		tool_save.set_size_request(70/display_scale, 70/display_scale);
		tool_save_as.set_size_request(70/display_scale, 70/display_scale);
		tool_run.set_tooltip_text("Execute all cells");
		tool_stop.set_tooltip_text("Stop execution");
		tool_restart.set_tooltip_text("Restart kernel");
		tool_open.set_tooltip_text("Open notebook...");
		tool_save.set_tooltip_text("Save notebook");
		tool_save_as.set_tooltip_text("Save notebook as...");
	
		topbox.pack_start(toolbar, Gtk::PACK_SHRINK);
		toolbar.pack_start(tool_open, Gtk::PACK_SHRINK);
		toolbar.pack_start(tool_save, Gtk::PACK_SHRINK);
		toolbar.pack_start(tool_save_as, Gtk::PACK_SHRINK);
		toolbar.pack_start(tool_run, Gtk::PACK_SHRINK);
		toolbar.pack_start(tool_stop, Gtk::PACK_SHRINK);
		toolbar.pack_start(tool_restart, Gtk::PACK_SHRINK);
		toolbar.pack_start(top_label);
		toolbar.pack_end(kernel_spinner, Gtk::PACK_SHRINK);
		kernel_spinner.set_size_request(50/display_scale, 50/display_scale);
		kernel_spinner.set_margin_end(10/display_scale);
		}
	
	// Normally we would use 'set_action_name' to associate the
	// buttons to actions already defined, but gtkmm-3.24 has
	// a TODO item "derive from and implement Actionable". So
	// we re-do what we did for actions.
	tool_open.signal_clicked().connect(sigc::mem_fun(*this, &NotebookWindow::on_file_open));
	tool_save.signal_clicked().connect(sigc::mem_fun(*this, &NotebookWindow::on_file_save));
	tool_save_as.signal_clicked().connect(sigc::mem_fun(*this, &NotebookWindow::on_file_save_as));
	tool_run.signal_clicked().connect(sigc::mem_fun(*this, &NotebookWindow::on_run_runall));
	tool_stop.signal_clicked().connect(sigc::mem_fun(*this, &NotebookWindow::on_run_stop));
	tool_restart.signal_clicked().connect(sigc::mem_fun(*this, &NotebookWindow::on_kernel_restart));
	
//	
//	Gtk::Widget *toolbar=0;
//	uimanager->get_widget("/ToolBar", toolbar);
//	topbox.pack_start(*toolbar, Gtk::PACK_SHRINK);
	topbox.pack_start(supermainbox, true, true);
	topbox.pack_start(statusbarbox, false, false);
	supermainbox.pack_start(dragbox, true, true);
	dragbox.add1(mainbox);
	mainbox.pack_start(searchbar, false, false);
	mainbox.set_name("mainbox");
	searchbar.add(search_hbox);
//	searchbar.set_halign(Gtk::ALIGN_START);
	search_hbox.pack_start(searchentry, Gtk::PACK_EXPAND_WIDGET, 10);
	searchentry.set_size_request(200, -1);
	search_hbox.pack_start(search_case_insensitive, Gtk::PACK_SHRINK, 10);
	search_case_insensitive.set_active(true);

	// Status bar
	kernel_label.set_text("Server: not connected");
	statusbarbox.set_size_request(-1,50/display_scale);
	statusbarbox.set_homogeneous(false);
	statusbarbox.pack_start(status_label, false, false, 0);
	statusbarbox.pack_start(kernel_label, false, false, 0);
	statusbarbox.pack_start(progressbar, true, true, 0);
	status_label.set_size_request(300,-1);
	kernel_label.set_size_request(300,-1);
	// status_label.set_halign(Gtk::Align::ALIGN_START);
	// kernel_label.set_halign(Gtk::Align::ALIGN_START);
	status_label.set_xalign(0);
	kernel_label.set_xalign(0);
	progressbar.set_text("Idle");
	progressbar.set_show_text(true);

   //	auto context = kernel_spinner.get_style_context();
   //	context->add_class("spinner");
	statusbarbox.set_name("statusbar");

	searchentry.signal_search_changed().connect(sigc::mem_fun(*this, &NotebookWindow::on_search_text_changed));

	// The three main widgets
	//	mainbox.pack_start(buttonbox, Gtk::PACK_SHRINK, 0);

	// We always have at least one canvas.
	canvasses.push_back(manage( new NotebookCanvas() ));
	mainbox.pack_start(*canvasses[0], Gtk::PACK_EXPAND_WIDGET, 0);

	// FIXME: need to do this for every canvas.
	canvasses[0]->scroll.signal_size_allocate().connect(
	   sigc::mem_fun(*this, &NotebookWindow::on_scroll_size_allocate));
	//	canvasses[0]->scroll.get_vadjustment()->signal_value_changed().connect(
	//		sigc::mem_fun(*this, &NotebookWindow::on_vscroll_changed));
	canvasses[0]->scroll.get_vscrollbar()->signal_change_value().connect(
	   sigc::mem_fun(*this, &NotebookWindow::on_vscroll_changed));
	//	canvasses[0]->ebox.signal_button_press_event().connect(
	//		sigc::mem_fun(*this, &NotebookWindow::on_mouse_wheel));

	canvasses[0]->scroll.signal_scroll_event().connect(
	   sigc::mem_fun(*this, &NotebookWindow::on_scroll), false);


	// Window size and title, and ready to go.
	if(!ro) {
		set_default_size(screen->get_width()/2, screen->get_height()*0.8);
		}
	else {
		set_default_size(screen->get_width()/3, screen->get_height()*0.6);
		}
	// FIXME: the subtraction for the margin and scrollbar made below
	// is estimated but should be computed.
	//	engine.set_geometry(screen->get_width()/2 - 2*30);
	update_title();
	show_all();
	kernel_spinner.hide();
	if(read_only) {
		statusbarbox.hide();
		progressbar.hide();
//		toolbar->hide();
		}
	else {
		// Buttons
		set_stop_sensitive(false);
		}

	new_document();
	}

NotebookWindow::~NotebookWindow()
	{
	}

void NotebookWindow::load_css()
	{
	// std::cerr << "(re)loading css" << std::endl;
	std::string text_colour = prefs.highlight ? "black" : "blue";
	Glib::ustring data = "";
	data += "scrolledwindow { background-color: @theme_base_color; }\n";
	data += "textview text { color: "+text_colour+"; background-color: @theme_base_color; -GtkWidget-cursor-aspect-ratio: 0.2; }\n";
	data += "textview *:focus { background-color: #eee; }\n";
	data += ".view text selection { color: #fff; background-color: #888; }\n";
	data += "textview.error { background: transparent; -GtkWidget-cursor-aspect-ratio: 0.2; color: @theme_fg_color; }\n";
	data += "#ImageView { transition-property: padding, background-color; transition-duration: 1s; }\n";
	data += "#CodeInput { font-family: monospace; font-size: "+std::to_string((100.0+(prefs.font_step*10.0)))+"%; }\n";
	data += "#Console   { padding: 2px; }\n";
	data += "#mainbox * { transition-property: opacity; transition-duration: 0.5s; }\n";
	data += "#mainbox:disabled * { opacity: 0.85; }\n";
	data += "label { margin-left: 4px; }\n";
	data += "#TeXArea:selected { background-color: #eee; }\n";
	data += "spinner { background: none; opacity: 1; -gtk-icon-source: -gtk-icontheme(\"process-working-symbolic\"); }\n";
	data += "spinner:checked { opacity: 1; animation: spin 1s linear infinite; }\n";

	// Some of the css properties defined in gtk-cadabra.css are overridden, and so are included
	// here to force them to be used
#ifdef _MSC_VER
	data += R"(
		.titlebar {
			margin: 0;
			padding: 0;
			box-shadow: inset 0 0 1px #ddd;
			background-image: none;
			background-color: #209020;
			color: white;
		}

		.titlebar:backdrop {
			background-color: #367d36;
		}

		.titlebar label:backdrop {
			color: white;
		}

		.titlebar .titlebutton {
			margin: 0px 0px 0px -8px;
			padding: 0px 8px;
		}

		dialog .titlebar,
		dialog .titlebar:backdrop {
			background-color: #666;
		}

		tooltip {
			background-color: transparent;
		}

		toolbar button {
			padding: 0px;
			margin-right: 5px;
		}
)";
#endif

	//	data += "scrolledwindow { kinetic-scrolling: false; }\n";

	if(!css_provider->load_from_data(data)) {
		std::cerr << "Cannot parse internal CSS." << std::endl;
		throw std::logic_error("Failed to parse widget CSS information.");
		}
	}

bool NotebookWindow::on_delete_event(GdkEventAny* event)
	{
	if (quit_safeguard(true)) {
		prefs.save();
		return Gtk::Window::on_delete_event(event);
		}
	else
		return true;
	}

void NotebookWindow::on_prefs_set_cv(int npos)
	{
	action_console->set_state(Glib::Variant<int>::create(npos));
	Console::Position pos=static_cast<Console::Position>(npos);
	// Unparent from whatever we're currently a child of
	console.hide();
	if (console.get_parent() == &dragbox) {
		dragbox.remove(console);
		}
	else if (console.get_parent() == console_win.get_content_area()) {
		console_win.get_content_area()->remove(console);
		console_win.hide();
		}

	// Reparent onto something new
	if (pos == Console::Position::DockedH) {
		dragbox.set_orientation(Gtk::Orientation::ORIENTATION_HORIZONTAL);
		dragbox.set_position(dragbox.get_allocation().get_width() / 2);
		dragbox.add2(console);
		console.show();
		}
	else if (pos == Console::Position::DockedV) {
		dragbox.set_orientation(Gtk::Orientation::ORIENTATION_VERTICAL);
		dragbox.set_position(dragbox.get_allocation().get_height() / 2);
		dragbox.add2(console);
		console.set_size_request(-1, 200);
		console.show();
		}
	else if (pos == Console::Position::Floating) {
		console_win.set_transient_for(*this);
		console_win.set_resizable(true);
		console_win.set_default_size(900, 300);
		console_win.set_title("Interactive Console");
		console_win.get_content_area()->add(console);
		// Reflect hidden state in menu when the floating
		// console is closed.
		console_win.signal_response().connect( [this](int) {
			this->on_prefs_set_cv(0);
			});
		console_win.show_all();
		}
	}

void NotebookWindow::resize_codeinput_texview_all(int width)
	{
	// Inform CodeInput and TeXView widgets about the size; they need to
   // have a maximum width equal to the window width (minus scrollbar),
	// except when they contain Python code (which does not word-wrap).
	auto it=doc.begin();
	// std::cerr << "----" << std::endl;
	while(it!=doc.end()) {
		resize_codeinput_texview(it, width);
		++it;
		}
	}

void NotebookWindow::resize_codeinput_texview(DTree::iterator it, int width)
	{
	// std::cerr << "resizing cell to " << width << std::endl;
	if(it->cell_type==DataCell::CellType::python || it->cell_type==DataCell::CellType::latex) {
		for(unsigned int i=0; i<canvasses.size(); ++i) {
			auto fnd = canvasses[i]->visualcells.find(&(*it));
			if(fnd != canvasses[i]->visualcells.end()) {
				CodeInput *code_input = fnd->second.inbox;
				//	code_input->edit.set_size_request(width, -1);  seems unnecessary
				Gtk::Allocation allocation = code_input->edit.get_allocation();
				allocation.set_width(width);
				code_input->edit.size_allocate(allocation);
				code_input->edit.queue_draw();
				code_input->edit.window_width = width;
				code_input->update_buffer();
				}
			}
		}
	else if(it->cell_type==DataCell::CellType::latex_view) {
		for(unsigned int i=0; i<canvasses.size(); ++i) {
			auto fnd = canvasses[i]->visualcells.find(&(*it));
			if(fnd != canvasses[i]->visualcells.end()) {
				TeXView *tex_view = fnd->second.outbox;
				tex_view->window_width = width;

				Gtk::Allocation allocation = tex_view->image.get_allocation();
				allocation.set_width(width - 20);
				int need_height = tex_view->image.need_height(width - 20);
				// std::cerr << "Need height " << need_height << std::endl;
				allocation.set_height(need_height+100);
				tex_view->image.size_allocate(allocation);
				
				tex_view->queue_draw();
				Glib::signal_timeout().connect_once([tex_view]()
						{
						tex_view->queue_resize();
						}, 50);
				}
			}
		}
	}

bool NotebookWindow::on_configure_event(GdkEventConfigure *cfg)
	{
	// std::cerr << "cadabra-client: on_configure_event " << cfg->width << " x " << cfg->height << std::endl;
	is_configured=true;

	bool ret=Gtk::Window::on_configure_event(cfg);

	if(cfg->width != last_configure_width) {
		if(prefs.microtex) {
			resize_codeinput_texview_all(cfg->width);
			last_configure_width = cfg->width;
			}
		else {
			// Re-run latex.
			std::lock_guard<std::recursive_mutex> guard(tex_need_width_mutex);
			if(tex_running) {
				tex_need_width = cfg->width;
				}
			else {
				tex_need_width = cfg->width;
				// std::cerr << "Attempting to run" << std::endl;
			tex_run_async();
			// std::cerr << "OK, running" << std::endl;
				}
			last_configure_width = cfg->width;
			}
		}

	return ret;
	}

void NotebookWindow::refresh_after_tex_engine_run()
	{
	for(unsigned int i=0; i<canvasses.size(); ++i)
		canvasses[i]->refresh_all();
	}

void NotebookWindow::handle_thread_tex_error()
	{
	on_tex_error(tex_error_string, doc.end());
	// std::cerr << "clicked away" << std::endl;
	tex_error_string="";
	}

bool NotebookWindow::on_unhandled_error(const std::exception& err)
{
	Gtk::MessageDialog md(*this, err.what(), false, Gtk::MessageType::MESSAGE_ERROR, Gtk::ButtonsType::BUTTONS_OK, true);
	md.run();
	return true;
}

void NotebookWindow::set_title_prefix(const std::string& pf)
	{
	title_prefix=pf;
	}

void NotebookWindow::update_title()
	{
	if(name.size()>0) {
		if(modified)
			set_title(title_prefix+name+"*");
		else
			set_title(title_prefix+name);
		}
	else {
		if(modified)
			set_title("Cadabra*");
		else
			set_title("Cadabra");
		}
	
	// std::cerr << "undo_stack.size() = " << undo_stack.size() << std::endl;
	// std::cerr << "redo_stack.size() = " << redo_stack.size() << std::endl;
	action_undo->set_enabled( undo_stack.size() > 0 );
	action_redo->set_enabled( redo_stack.size() > 0 );
	action_view_close->set_enabled( canvasses.size() > 1 );
	}

void NotebookWindow::set_statusbar_message(const std::string& message, int line, int col)
	{
		{
		std::lock_guard<std::mutex> guard(status_mutex);
		status_string = message;
		status_line = line;
		status_col = col;
		}
	dispatch_update_status.emit();
	}

void NotebookWindow::set_stop_sensitive(bool s)
	{
	// Disable menu items and toolbar items by simply disabling
	// the action.
	
	action_stop->set_enabled(s);
	}

void NotebookWindow::process_data()
	{
	dispatcher.emit();
	}


void NotebookWindow::on_connect()
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = "connected";
	progress_string = "Idle";
	dispatcher.emit();
	dispatch_update_status.emit();
	console.initialize();
	// prefs.python_path might end in a backslash which will raise an EOF syntax error, so we add a
	// semicolon to the end of it and then remove the (empty) last element of the resulting list
	if (!trim(prefs.python_path).empty())
		console.send_input("sys.path = r'''" + escape_backslashes(prefs.python_path) + ";'''.split(';')[:-1] + sys.path");
	if (!name.empty()) {
		console.send_input("sys.path.insert(0, '''" + escape_backslashes(name.substr(0, name.find_last_of("\\/"))) + "''')");
		}
	}

void NotebookWindow::on_disconnect(const std::string& reason)
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = reason;
	dispatcher.emit();
	}

void NotebookWindow::on_network_error()
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = "cannot reach server, retrying...";
	dispatcher.emit();
	}

void NotebookWindow::on_kernel_runstatus(bool running)
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_spinner_status=running;
	dispatcher.emit();
	}

void NotebookWindow::tex_run_async()
	{
	if(prefs.microtex==false) {
		if(tex_error_string!="")
			return;
		
		// Run TeX on a separate thread; it'll take a while and
		// we don't want to block the UI thread.
		// FIXME: join properly when exiting the program, otherwise
		// we get a 'terminate called without active exception' message.
		auto tex_code = [this]() {
			try {
				for(;;) {
					{  std::lock_guard<std::recursive_mutex> guard(tex_need_width_mutex);
						engine.invalidate_all();
						engine.set_geometry(tex_need_width-2*30);
						}
					
					engine.convert_all();
					dispatch_refresh.emit();
					
					{  std::lock_guard<std::recursive_mutex> guard(tex_need_width_mutex);
						if(tex_need_width-2*30 == engine.get_geometry()) {
							tex_running=false;
							break;
							}
						}
					}
				}
			catch(TeXEngine::TeXException& ex) {
				// Display the error in the main thread.
				std::lock_guard<std::recursive_mutex> guard(tex_need_width_mutex);
				tex_error_string=ex.what();
				tex_running=false;
				dispatch_refresh.emit();
				dispatch_tex_error.emit();
				}
			};
		
		std::lock_guard<std::recursive_mutex> guard(tex_need_width_mutex);
		tex_running=true;
		
		// Join any thread which may still exist.
		if(tex_thread)
			tex_thread->join();
		
		tex_thread = std::make_unique<std::thread>( tex_code );
		}
	else {
		dispatch_refresh.emit();
		}
	}

void NotebookWindow::process_todo_queue()
	{
	static bool running=false;

	// Prevent from re-entering this from the process_action_queue entered below.
	if(running) return;
	running=true;

	// Update the status/kernel messages into the corresponding widgets.
		{
		std::lock_guard<std::mutex> guard(status_mutex);
		kernel_label.set_text("Server: " + kernel_string);
		if(compute)
			mainbox.set_sensitive(compute->kernel_is_connected());

		if(kernel_spinner_status) {
			kernel_spinner.show();
			kernel_spinner.start();
			}
		else {
			kernel_spinner.stop();
			kernel_spinner.hide();
			}
		}

	// Perform any ActionBase actions.
	process_action_queue();

	// Before we pop up any dialogs, enable queue processing again, otherwise
	// subsequent calls to process_todo_queue will get postponed until a
	// dispatcher.emit() is called when the dialog is closed.
	running=false;

	if(crash_window_hidden && kernel_string=="not connected") {
		crash_window_hidden=false;
		Gtk::MessageDialog md("Kernel crashed", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
		md.set_transient_for(*this);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		md.set_secondary_text("The kernel crashed unexpectedly, and has been restarted. You will need to re-run all cells.");
		md.signal_response().connect(sigc::mem_fun(*this, &NotebookWindow::on_crash_window_closed));
		md.run();

		nlohmann::json req;
		req["msg_type"] = "exit";
		req["header"]["from_server"] = true;
		on_interactive_output(req);
		}

	}

void NotebookWindow::on_crash_window_closed(int)
	{
	crash_window_hidden=true;
	}

bool NotebookWindow::on_key_press_event(GdkEventKey* event)
	{
	bool is_ctrl_up    = event->keyval==GDK_KEY_Up   && (event->state&Gdk::CONTROL_MASK);
	bool is_ctrl_down  = event->keyval==GDK_KEY_Down && (event->state&Gdk::CONTROL_MASK);
	bool is_ctrl_home  = event->keyval==GDK_KEY_Home && (event->state&Gdk::CONTROL_MASK);
	bool is_ctrl_end   = event->keyval==GDK_KEY_End  && (event->state&Gdk::CONTROL_MASK);
	bool is_pagedown   = event->keyval==GDK_KEY_Page_Down;
	bool is_pageup     = event->keyval==GDK_KEY_Page_Up;

	bool have_current_cell = current_cell != doc.end();

	if(is_ctrl_up && have_current_cell) {
		std::shared_ptr<ActionBase> actionpos =
		   std::make_shared<ActionPositionCursor>(current_cell->id(), ActionPositionCursor::Position::previous);
		queue_action(actionpos);
		process_todo_queue();
		return true;
		}
	else if(is_ctrl_down && have_current_cell) {
		std::shared_ptr<ActionBase> actionpos =
		   std::make_shared<ActionPositionCursor>(current_cell->id(), ActionPositionCursor::Position::next);
		queue_action(actionpos);
		process_todo_queue();
		return true;
		}
	else if(is_ctrl_home) {
//		std::shared_ptr<ActionBase> actionpos =
//		   std::make_shared<ActionPositionCursor>(current_cell->id(), ActionPositionCursor::Position::in);
//		queue_action(actionpos);
//		process_todo_queue();
		Glib::RefPtr<Gtk::Adjustment> va=canvasses[current_canvas]->scroll.get_vadjustment();
		va->set_value( va->get_lower() );
		return true;
		}
	else if(is_ctrl_end) {
//		std::shared_ptr<ActionBase> actionpos =
//		   std::make_shared<ActionPositionCursor>(current_cell->id(), ActionPositionCursor::Position::in);
//		queue_action(actionpos);
//		process_todo_queue();
		Glib::RefPtr<Gtk::Adjustment> va=canvasses[current_canvas]->scroll.get_vadjustment();
		va->set_value( va->get_upper() );
		return true;
		}
	else if(is_pageup) {
		Glib::RefPtr<Gtk::Adjustment> va=canvasses[current_canvas]->scroll.get_vadjustment();
		va->set_value( va->get_value()-va->get_page_increment() );
		return true;
		}
	else if(is_pagedown) {
		Glib::RefPtr<Gtk::Adjustment> va=canvasses[current_canvas]->scroll.get_vadjustment();
		va->set_value( va->get_value()+va->get_page_increment() );
		return true;
		}
	else {
		return Gtk::Window::on_key_press_event(event);
		}
	}

void NotebookWindow::add_cell(const DTree& tr, DTree::iterator it, bool visible)
	{
	// std::cerr << "Add cell for " << it->id().id << std::endl;
	// Add a visual cell corresponding to this document cell in
	// every canvas.

	if(compute!=0)
		set_stop_sensitive( compute->number_of_cells_executing()>0 );

	Glib::RefPtr<Gtk::TextBuffer>          global_buffer;
	std::shared_ptr<TeXEngine::TeXRequest> global_texrequest;

	for(unsigned int i=0; i<canvasses.size(); ++i) {

		// If this data cell already has a representation in the current canvas
		// we can continue to the next canvas. However, we need to set the global
		// buffer from existing cells.

		if(canvasses[i]->visualcells.find(&(*it))!=canvasses[i]->visualcells.end()) {
#ifdef DEBUG
			std::cerr << "found a visualcell for cell " << &(*it) << " in canvas " << i << std::endl;
#endif
			if(i==0 && (it->cell_type==DataCell::CellType::python || it->cell_type==DataCell::CellType::latex)) {
				global_buffer = canvasses[i]->visualcells[&(*it)].inbox->buffer;
				}
			continue;
			}

		// Create a visual cell of the appropriate type.

		VisualCell newcell;
		Gtk::Widget *w=0;
		switch(it->cell_type) {
			case DataCell::CellType::document:
				newcell.document = manage( new Gtk::VBox() );
				w=newcell.document;
				break;

			case DataCell::CellType::output:
			case DataCell::CellType::error:
			case DataCell::CellType::verbatim:
				// std::cerr << "creating verbatim cell for " << it->textbuf << std::endl;
			case DataCell::CellType::latex_view: {
				// FIXME: would be good to share the input and output of TeXView too.
				// Right now nothing is shared...
				newcell.outbox = manage( new TeXView(engine, it, prefs.microtex) );
				newcell.outbox->window_width = last_configure_width;
				// std::cerr << "Add widget " << newcell.outbox << " for cell " << it->id().id << std::endl;
				newcell.outbox->tex_error.connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::on_tex_error), it ) );

				newcell.outbox->show_hide_requested.connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_toggle_visibility), i ) );

#if GTKMM_MINOR_VERSION>=10
				to_reveal.push_back(&newcell.outbox->rbox);
#endif

				w=newcell.outbox;
				newcell.outbox->signal_button_press_event().connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::handle_outbox_select), it ) );
				break;
				}
			case DataCell::CellType::python:
			case DataCell::CellType::latex: {
				CodeInput *ci;
				// Ensure that all CodeInput cells share the same text buffer.
				if(i==0) {
					ci = new CodeInput(it, it->textbuf, scale/display_scale, prefs,
											 canvasses[i]->scroll.get_vadjustment() );
					global_buffer=ci->buffer;
					}
				else ci = new CodeInput(it, global_buffer, scale/display_scale, prefs,
												canvasses[i]->scroll.get_vadjustment());

				using namespace std::placeholders;
				ci->relay_cursor_pos(std::bind(&NotebookWindow::set_statusbar_message, this, "", _1, _2));
				if(read_only)
					ci->edit.set_editable(false);
				ci->get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

				ci->edit.content_changed.connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_changed), i ) );
				ci->edit.content_insert.connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_insert), i ) );
				ci->edit.content_erase.connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_erase), i ) );

				ci->edit.content_execute.connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_execute), i, true ) );
				ci->edit.complete_request.connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_complete_request), i ) );
				ci->edit.cell_got_focus.connect(
				   sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_got_focus), i ) );

				newcell.inbox = manage( ci );
				w=newcell.inbox;

				break;
				}
			case DataCell::CellType::image_png: {
				// FIXME: horribly memory inefficient
				ImageView *iv=new ImageView(scale/display_scale);

				iv->set_image_from_base64(it->textbuf);
				newcell.imagebox = manage( iv );
				w=newcell.imagebox;
				break;
				}
			case DataCell::CellType::image_svg: {
				// FIXME: horribly memory inefficient
				ImageView *iv=new ImageView(scale/display_scale);

				iv->set_image_from_svg(it->textbuf);
				newcell.imagebox = manage( iv );
				w=newcell.imagebox;
				break;
				}
			case DataCell::CellType::input_form:
				// This cell is there only for cutnpaste functionality; do not display.
				break;

			default:
				throw std::logic_error("Unimplemented datacell type");
			}


		if(w!=0) {
			canvasses[i]->visualcells[&(*it)]=newcell;
			resize_codeinput_texview(it, last_configure_width);

			// Document cells are easy; just add. They have no parent in the DTree.

			if(it->cell_type==DataCell::CellType::document) {
				canvasses[i]->scroll.add(*w);
				w->show_all(); // FIXME: if you drop this, the whole document remains invisible
				continue;
				}


			// Figure out where to store this new VisualCell in the GUI widget
			// tree by exploring the DTree near the new DataCell.
			// First determine the parent cell and the corresponding Gtk::Box
			// so that we can determine where to pack_start this cell. At this
			// stage, all cells have parents.

			DTree::iterator parent = DTree::parent(it);
			assert(tr.is_valid(parent));

			VisualCell& parent_visual = canvasses[i]->visualcells[&(*parent)];
			Gtk::VBox *parentbox=0;
			int offset=0;
			if(parent->cell_type==DataCell::CellType::document)
				parentbox=parent_visual.document;
			else {
				// FIXME: Since we are adding children of input cells to the vbox in which
				// the exp_input_tv widget is the 0th cell, we have to offset. Would be
				// cleaner to have a separate 'children' vbox in CodeInput (or in fact
				// every widget that can potentially contain children).
				offset=1;
				parentbox=parent_visual.inbox;
				}

			//		std::cout << "adding cell to canvas " << i << std::endl;
			parentbox->pack_start(*w, false, false);
			unsigned int index    =tr.index(it)+offset;
			unsigned int index_gui=parentbox->get_children().size()-1;
			//		std::cout << "is index " << index << " vs " << index_gui << std::endl;
			if(index!=index_gui) {
				//			std::cout << "need to re-order" << std::endl;
				parentbox->reorder_child(*w, index);
				}
			if(visible) {
				w->show_all();
				w->show_now();
				}
			}

		}

	// Connect
	Glib::signal_idle().connect(sigc::mem_fun(*this, &NotebookWindow::idle_handler));

	//	if(current_cell!=doc.end())
	//		setup_focus_after_allocate(it);
	}

void NotebookWindow::on_interactive_output(const nlohmann::json& msg)
	{
	console.signal_message(msg);
	}

void NotebookWindow::set_progress(const std::string& msg, int cur_step, int total_steps)
	{
		{
		std::lock_guard<std::mutex> guard(status_mutex);
		if (total_steps == 0) {
			progress_string = msg;
			progress_frac = 0.0;
			}
		else {
			progress_frac = (double)cur_step / total_steps;
			progress_string = msg + " (" + std::to_string(cur_step) + "/" + std::to_string(total_steps) + ")";
			}
		}
	dispatch_update_status.emit();
	}

void NotebookWindow::update_status()
	{
	// This should only be called from dispatch_update_status!

	// Update status and progress, kernel status is taken card of in process_action_queue
	std::lock_guard<std::mutex> guard(status_mutex);
	progressbar.set_text(progress_string);
	progressbar.set_fraction(progress_frac);
	
	std::string pos = " Line: " + std::to_string(status_line)
		+ "   Col: " + std::to_string(status_col);
	if (status_string == "")
		status_label.set_text(pos);
	else
		status_label.set_text(pos + "   |   " + status_string);
 	}

void NotebookWindow::remove_cell(const DTree& doc, DTree::iterator it)
	{
	// Remember: this member should only remove the visual cell; the
	// document tree will be updated by the ActionRemove that led to this
	// member being called. However, we should ensure that any references
	// to the visual cell are removed as well; in particular, if
	// current_cell is pointing to this cell, we need to unset it.

	// Can only remove cells which have a parent (i.e. not the
	// top-level document cell).

	if(current_cell==it)
		current_cell=doc.end();

	// Ensure we will not continue searching in this cell.
	if(last_find_location.first==it) {
		last_find_location.first=doc.end();
		last_find_location.second=std::string::npos;
		}

	DTree::iterator parent = DTree::parent(it);
	assert(doc.is_valid(parent));

	for(unsigned int i=0; i<canvasses.size(); ++i) {
		VisualCell& parent_visual = canvasses[i]->visualcells[&(*parent)];
		Gtk::VBox *parentbox=0;
		if(it->cell_type==DataCell::CellType::document)
			parentbox=parent_visual.document;
		else
			parentbox=parent_visual.inbox;


		auto fnd = canvasses[i]->visualcells.find(&(*it));
		// It is possible that there is no visual cell corresponding to this
		// DTree cell (happens for instance when deleting a cell, undoing,
		// then re-evaluating: the DTree still has the output cells, while
		// they have gone from the visual tree).
		if(fnd==canvasses[i]->visualcells.end()) {
			std::cerr << "No visual cell for " << it->id().id << std::endl;
			continue;
			}

		VisualCell& actual = fnd->second;//canvasses[i]->visualcells[&(*it)];

		// The pointers are all in a union, and Gtkmm does not care
		// about the precise type, so we just remove imagebox, knowing
		// that it may actually be an inbox or outbox.
		// std::cerr << "Removing " << actual.imagebox << std::endl;
		parentbox->remove(*actual.imagebox);

		// The above does not delete the Gtk widget, despite having been
		// wrapped in manage at construction. So we have to delete it
		// ourselves. Fortunately the container does not try to delete
		// it again in its destructor.
		delete actual.imagebox;

		// The above removes the entire Gtk tree corresponding to the cell at
		// 'it' and the subtree below it. In order to remove all these from
		// the visualcells map, we need to walk the tree explicitly.
		do_subtree(doc, it, [this, i](DTree::iterator rm) {
#ifdef DEBUG
			std::cerr << "removing cell " << &(*rm) << " " << rm->textbuf << "\n from canvas " << i << std::endl;
#endif
			auto fnd = canvasses[i]->visualcells.find(&(*rm));
			if(fnd!=canvasses[i]->visualcells.end()) {
				canvasses[i]->visualcells.erase(fnd);
				}
			else {
#ifdef DEBUG
				std::cerr << "no visualcell for that one" << std::endl;
#endif
				}
			return rm;
			});
		}
	modified=true;
	update_title();
	}

void NotebookWindow::remove_all_cells()
	{
	// Simply removing the document cell should do the trick.
	for(unsigned int i=0; i<canvasses.size(); ++i) {
		canvasses[i]->scroll.remove();
		canvasses[i]->visualcells.clear();
		}
	engine.checkout_all();
	update_title();
	}

void NotebookWindow::update_cell(const DTree&, DTree::iterator it)
	{
	// We just do a redraw for now, but this may require more work later.
	disable_stacks=true;

	for(unsigned int i=0; i<canvasses.size(); ++i) {
		VisualCell& vc = canvasses[i]->visualcells[&(*it)];
		if(it->cell_type==DataCell::CellType::python || it->cell_type==DataCell::CellType::latex) {
			vc.inbox->update_buffer();
			vc.inbox->queue_draw();
			}
		else if(it->cell_type==DataCell::CellType::latex_view || it->cell_type==DataCell::CellType::verbatim
				  || it->cell_type==DataCell::CellType::output) {
			vc.outbox->image.set_latex(it->textbuf);
			vc.outbox->image.layout_latex();
			vc.outbox->queue_draw();
			}
		else if(it->cell_type==DataCell::CellType::image_png) {
			vc.imagebox->set_image_from_base64(it->textbuf);
			vc.imagebox->queue_draw();
			}
		else if(it->cell_type==DataCell::CellType::image_svg) {
			vc.imagebox->set_image_from_svg(it->textbuf);
			vc.imagebox->queue_draw();			
			}
		}

	disable_stacks=false;
	update_title();
	}

void NotebookWindow::position_cursor(const DTree&, DTree::iterator it, int pos)
	{
	//	if(it==doc.end()) return;
	// std::cerr << "cadabra-client: positioning cursor at cell " << it->textbuf << std::endl;
	set_stop_sensitive( compute->number_of_cells_executing()>0 );

	if(canvasses[current_canvas]->visualcells.find(&(*it))==canvasses[current_canvas]->visualcells.end()) {
		std::cerr << "cadabra-client: Cannot find cell to position cursor." << std::endl;
		return;
		}

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];

	//	Gtk::Allocation alloc=target.inbox->get_allocation();
	target.inbox->edit.grab_focus();

	if(pos>=0) {
		auto cursor=target.inbox->edit.get_buffer()->begin();
		cursor.forward_chars(pos);
		target.inbox->edit.get_buffer()->place_cursor(cursor);
		}

	current_cell=it;
	update_title();
	}

void NotebookWindow::select_range(const DTree&, DTree::iterator it, int start, int len)
	{
	if(it->cell_type!=DataCell::CellType::python && it->cell_type!=DataCell::CellType::latex) {
		std::cerr << "Warning: select_range called on cell which is not python or latex." << std::endl;
		return;
		}

	if(canvasses[current_canvas]->visualcells.find(&(*it))==canvasses[current_canvas]->visualcells.end()) {
		std::cerr << "cadabra-client: Cannot find cell to select range." << std::endl;
		return;
		}

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];

	auto start_it=target.inbox->edit.get_buffer()->begin();
	start_it.forward_chars(start);
	auto end_it=start_it;
	end_it.forward_chars(len);
	// std::cerr << start << ", " << len << std::endl;
	target.inbox->edit.get_buffer()->select_range(start_it, end_it);
	}

size_t NotebookWindow::get_cursor_position(const DTree&, DTree::iterator it)
	{
	if(canvasses[current_canvas]->visualcells.find(&(*it))==canvasses[current_canvas]->visualcells.end()) {
		std::cerr << "cadabra-client: Cannot find cell to retrieve cursor position for." << std::endl;
		return -1;
		}

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];
	size_t offset = target.inbox->buffer->get_insert()->get_iter().get_offset();

	return offset;
	}

void NotebookWindow::scroll_current_cell_into_view()
	{
	if(current_cell==doc.end()) return;
	scroll_cell_into_view(current_cell);
	}

void NotebookWindow::scroll_cell_into_view(DTree::iterator cell)
	{
//	std::cerr << "-----" << std::endl;
//	std::cerr << "cell content to show: " << cell->textbuf << std::endl;

	if(current_canvas>=(int)canvasses.size()) return;

	if(canvasses[current_canvas]->visualcells.find(&(*cell))==canvasses[current_canvas]->visualcells.end()) return;

	const VisualCell& focusbox = canvasses[current_canvas]->visualcells[&(*cell)];

	if(focusbox.inbox==0) return;

	Gtk::Allocation               al;
	if(cell->cell_type==DataCell::CellType::python ||
		cell->cell_type==DataCell::CellType::latex)
		al=focusbox.inbox->edit.get_allocation();
	else if(cell->cell_type==DataCell::CellType::latex_view ||
			  cell->cell_type==DataCell::CellType::output ||
			  cell->cell_type==DataCell::CellType::verbatim)
		al=focusbox.outbox->get_allocation();
	else if(cell->cell_type==DataCell::CellType::image_png)
		al=focusbox.imagebox->get_allocation();
	else if(cell->cell_type==DataCell::CellType::image_svg)
		al=focusbox.imagebox->get_allocation();
	else
		return;

	Glib::RefPtr<Gtk::Adjustment> va=canvasses[current_canvas]->scroll.get_vadjustment();

	double upper_visible=va->get_value();
	double page_size    =va->get_page_size();
	double lower_visible=va->get_value()+page_size;


	// When we get called, the busybox has its allocation done and size set. However,
	// the edit box below still has its old position (but its correct height). So we
	// should make sure that busybox.y+busybox.height+editbox.height is at the bottom
	// of the scrollbox.
//	std::cerr << "viewport = " << upper_visible << " - " << lower_visible << std::endl;
//	std::cerr << "cell to show = " << al.get_y() << " height " << al.get_height() << std::endl;

	double should_be_visible = al.get_y()+al.get_height()+10;
	double shift = should_be_visible - lower_visible;
//	std::cerr << "position " << should_be_visible << " should be visible" << std::endl;
//	std::cerr << "shift = " << shift << std::endl;
	if(shift>0 || (-shift)>va->get_page_size()) {
		va->set_value( upper_visible + shift);
		}
	}

bool NotebookWindow::on_vscroll_changed(Gtk::ScrollType, double )
	{
	//	std::cerr << "vscroll changed " << std::endl;
	// FIXME: does not catch scroll wheel events.
	follow_cell=doc.end();
	return false;
	}

//bool NotebookWindow::on_mouse_wheel(GdkEventButton *b)
//	{
//	if(b->button==2)
//		follow_cell=doc.end();
//	return false;
//	}

bool NotebookWindow::on_scroll(GdkEventScroll *)
	{
	// We get here when the user rolls the scroll wheel inside the canvas;
	// need to disable auto-follow.

	follow_cell=doc.end();
	return false;
	}

void NotebookWindow::on_scroll_size_allocate(Gtk::Allocation& )
	{
	// The auto-scroll logic is as follows. Whenever a cell is ran (by
	// user pressing shift-enter only, not by full run), we set to
	// auto-scroll as soon as output for that cell appears. If multiple
	// cells are sent to the queue, we follow output for the last one
	// sent. Any scrollbar event stops auto-scrolling. Perhaps only
	// auto-restart when scrolling to the bottom of the notebook,
	// though that is not of much extra use. Could have a 'follow
	// current' mode when running an entire notebook, which is again
	// stopped by scrollbar event.
	if(follow_cell!=doc.end()) {
		//		std::cerr << "  scroll" << std::endl;
		scroll_current_cell_into_view();
		}
	}

bool NotebookWindow::cell_toggle_visibility(DTree::iterator it, int )
	{
	// Find the parent node. If that one is a latex cell, toggle visibility of
	// the CodeInput widget (but not anything else in its vbox).

	auto parent=DTree::parent(it);
	if(parent->cell_type==DataCell::CellType::latex) {
		// FIXME: we are not allowed to do this directly, all should go through
		// actions.
		parent->hidden = !parent->hidden;
		for(unsigned int i=0; i<canvasses.size(); ++i) {
			auto vis = canvasses[i]->visualcells.find(&(*parent));
			if(vis==canvasses[i]->visualcells.end()) {
				throw std::logic_error("Cannot find visual cell.");
				}
			else {
				if(parent->hidden) {
//					Glib::signal_timeout().connect_once([it, this]()
//							{
//							resize_codeinput_texview(it, get_width());
////							w->queue_resize();							
//							}, 50);
					(*vis).second.inbox->edit.hide();
//					auto vis2 = canvasses[i]->visualcells.find(&(*it));
//					if(vis2==canvasses[i]->visualcells.end()) {
//						TeXView *w = (*vis2).second.outbox;
//						Glib::signal_timeout().connect_once([w]()
//								{
//								w->queue_resize();							
//								}, 30);
//						}
					}
				else {
					CodeInput *w = (*vis).second.inbox;
					w->edit.show_all();
					// The very first time a widget is toggled to visible,
					// the TextView will have a height which is incorrect
					// (looks like it computes the maximal height if the
					// width would be minimal). The hack below makes that
					// problem go away. Similar to the hack used for
					// hide followed by immediate show.
					Glib::signal_timeout().connect_once([w]()
							{
							w->edit.queue_resize();							
							}, 30);
					}
				}
			}
		}

	return false;
	}

bool NotebookWindow::cell_content_changed(DTree::iterator, int /* i */)
	{
	if(disable_stacks) return false;

	modified=true;
	unselect_output_cell();
	update_title();

	return false;
	}

bool NotebookWindow::cell_content_insert(const std::string& content, int pos, DTree::iterator it, int)
	{
	if(disable_stacks) return false;

	unselect_output_cell();
	std::shared_ptr<ActionBase> action = std::make_shared<ActionInsertText>(it->id(), pos, content);
	queue_action(action);
	process_todo_queue();

	return false;
	}

bool NotebookWindow::cell_content_erase(int start, int end, DTree::iterator it, int )
	{
	if(disable_stacks) return false;

	unselect_output_cell();
	//std::cerr << "cell_content_erase" << std::endl;
	std::shared_ptr<ActionBase> action = std::make_shared<ActionEraseText>(it->id(), start, end);
	queue_action(action);
	process_todo_queue();

	return false;
	}

void NotebookWindow::dim_output_cells(DTree::iterator it)
	{
	// Dim the corresponding output cell, if any.
	auto ch=doc.begin(it);
	while(ch!=doc.end(it)) {
		if(ch->cell_type==DataCell::CellType::latex_view) {
			for(unsigned int i=0; i<canvasses.size(); ++i) {
				auto vc = canvasses[i]->visualcells.find(&(*ch));
				if(vc!=canvasses[i]->visualcells.end())
					vc->second.outbox->dim(true);
				}
			}
		++ch;
		}

	}

bool NotebookWindow::cell_got_focus(DTree::iterator it, int canvas_number)
	{
//	std::cerr << "focus on cell " << &(*it) << " canvas " << canvas_number << std::endl;
	current_cell=it;
	current_canvas=canvas_number;

	unselect_output_cell(); // cell_got_focus is an input cell, so output cells should not be selected anymore.

	return false;
	}

void NotebookWindow::interactive_execute()
	{
	uint64_t id = 0;
	std::string input = console.grab_input(id);
	compute->execute_interactive(id, input);
	}

bool NotebookWindow::cell_complete_request(DTree::iterator it, int pos, int canvas_number)
	{
	if (!prefs.tab_completion)
		return false;

	int cnum=0;
	if(undo_stack.size()>0) {
		auto cmp = std::dynamic_pointer_cast<ActionCompleteText>(undo_stack.top());
		if(cmp) {
			// std::cerr << "Undoing last completion" << std::endl;
			cmp->revert(*this, *gui);
			pos-=cmp->length();
			cnum=cmp->alternative()+1;
			undo_stack.pop();
			}
		}
	return compute->complete(it, pos, cnum);
	}

bool NotebookWindow::cell_content_execute(DTree::iterator it, int canvas_number, bool )
	{
	// This callback runs on the GUI thread. The cell pointed to by 'it' is
	// guaranteed to be valid.

	// First ensure that this cell is not already running, otherwise all hell
	// will break loose when we try to double-remove the existing output cell etc.

	if(it->running) {
		return true;
		}

	// Ensure this cell is not empty either.

	if(it->textbuf.size()==0) return true;

	current_canvas=canvas_number;

	// Remove child nodes, if any.
	// FIXME: Does it make more sense to do this only after the
	// execution result comes back from the server?

	DTree::sibling_iterator sib=doc.begin(it);
	dim_output_cells(it);
	while(sib!=doc.end(it)) {
		// std::cout << "cadabra-client: scheduling output cell for removal: " << sib->id().id << std::endl;
		std::shared_ptr<ActionBase> action = std::make_shared<ActionRemoveCell>(sib->id());
		queue_action(action);
		++sib;
		}

	// Execute the cell.
	set_stop_sensitive(true);
	follow_cell=it;
	// std::cerr << "Executing cell " << it->id().id << std::endl;

	// If this is a LaTeX input cell, and auto-close is turned on, close
	// the input cell. Make sure to also feed that into the document
	// itself!
	if(it->cell_type==DataCell::CellType::latex) {
		if(prefs.auto_close_latex) {
			for(unsigned int i=0; i<canvasses.size(); ++i) {
				auto vis = canvasses[i]->visualcells.find(&(*it));
				if(vis==canvasses[i]->visualcells.end()) {
					throw std::logic_error("Cannot find visual cell.");
					}
				else {
					(*vis).second.inbox->edit.hide();
					it->hidden=true;
//					resize_codeinput_texview(it, last_configure_width);
					}
				}
			}
		}

	// Execute the cell. Make sure this comes after the hiding logic above.
	compute->execute_cell(it);

	return true;
	}

bool NotebookWindow::on_tex_error(const std::string& str, DTree::iterator it)
	{
	// Re-open the input cell if it was hidden, so we can fix the error.
	DTree::iterator pit = doc.parent(it);
	for(unsigned int i=0; i<canvasses.size(); ++i) {
		auto vis = canvasses[i]->visualcells.find(&(*pit));
		if(vis==canvasses[i]->visualcells.end()) {
			}
		else {
			Gtk::Widget& w = (*vis).second.inbox->edit;
			// Probably a bug: if we call show() without the delay below, we can have
			// the hide() and show() happen very quickly after each other, which does not
			// work (widget stays hidden).
			Glib::signal_timeout().connect_once([&w]() { w.show_all(); }, 50);

			pit->hidden=false;
			}
		}

	if(!prefs.microtex) {
		// Show the error in a dialog.
		Gtk::MessageDialog md("Generic TeX error", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
		md.set_resizable(true);
		//	Gtk::Button ok(Gtk::Stock::OK);
		md.set_transient_for(*this);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		auto box = md.get_message_area();
		//	md.add_button(Gtk::Stock::OK, 1);
		Gtk::ScrolledWindow sw;
		Gtk::TextView tv;
		auto buffer = tv.get_buffer();
		buffer->set_text(str);
		//	auto iter = buffer->get_iter_at_offset(0);
		//	buffer->insert(iter, str);
		tv.set_editable(false);
		box->add(sw);
		sw.add(tv);
		auto context = tv.get_style_context();
		context->add_class("error");
		auto screen = Gdk::Screen::get_default();
		sw.set_size_request(screen->get_width()/4, screen->get_width()/4);
		sw.show_all();
		md.run();
		}
	
	return true;
	}

void NotebookWindow::on_file_new()
	{
	if(quit_safeguard(false)) {
		doc.clear();
		remove_all_cells();
		new_document();
		compute->restart_kernel();
		position_cursor(doc, doc.begin(doc.begin()), -1);
		name="";
		update_title();
		}
	}

void NotebookWindow::on_file_close()
	{
	if(quit_safeguard(true))
		hide();
	}

Glib::RefPtr<Gtk::FileFilter> create_filter(const Glib::ustring& name, const Glib::ustring& pattern)
{
	auto filter = Gtk::FileFilter::create();
	filter->set_name(name);
	filter->add_pattern(pattern);
	return filter;
}

void NotebookWindow::on_file_open()
	{
	if(quit_safeguard(false)==false)
		return;

	Gtk::FileChooserDialog dialog("Please choose a Cadabra notebook (.cnb file) to open",
	                              Gtk::FILE_CHOOSER_ACTION_OPEN);

	dialog.set_transient_for(*this);
	dialog.add_filter(create_filter("Cadabra notebooks (*.cnb)", "*.cnb"));
	dialog.add_filter(create_filter("All files", "*"));
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			name = dialog.get_filename();
			snoop::log("open") << "menu" << snoop::flush;
			std::ifstream file(name);
			std::string content, line;

			while(std::getline(file, line))
				content+=line;

			compute->restart_kernel();
			load_file(content);
			break;
			}
		}
	}

void NotebookWindow::set_name(const std::string& n)
	{
	name=n;
	update_title();
	}

void NotebookWindow::load_file(const std::string& notebook_contents)
	{
	mainbox.set_sensitive(false);

	load_from_string(notebook_contents);

	mainbox.show_all();
	modified=false;
	update_title();

	Glib::signal_idle().connect( sigc::mem_fun(*this, &NotebookWindow::on_first_redraw) );
	}

bool NotebookWindow::on_first_redraw()
	{
	mainbox.set_sensitive(true);
	return false;
	}

void NotebookWindow::on_file_save()
	{
	// check if name known, otherwise call save_as
	if(name.size()>0) {
		size_t dotpos = name.rfind(".");
		if(dotpos==std::string::npos)
			name += ".cnb";
		std::string res=save(name);
		if(res.size()>0) {
			Gtk::MessageDialog md("Error saving notebook "+name);
			md.set_transient_for(*this);
			md.set_secondary_text(res);
			md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
			md.run();
			}
		else {
			modified=false;
			update_title();
			}
		}
	else on_file_save_as();
	}

void NotebookWindow::on_file_save_as()
	{
	Gtk::FileChooserDialog dialog("Please choose a file name to save this notebook",
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_do_overwrite_confirmation(true);
	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
	case(Gtk::RESPONSE_OK): {
			std::string old_name = name;
			name = dialog.get_filename();
			size_t dotpos = name.rfind(".");
			if(dotpos==std::string::npos)
				name += ".cnb";
			std::string res=save(name);
			if(res.size()>0) {
				Gtk::MessageDialog md("Error saving notebook "+name);
				md.set_transient_for(*this);
				md.set_secondary_text(res);
				md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
				md.run();
				}
			else {
				modified=false;
				update_title();
				console.send_input("sys.path.insert(0, r'''" + escape_backslashes(name.substr(0, name.find_last_of("\\/"))) + "''')");
				}
			break;
			}
		}
	}

void NotebookWindow::on_file_save_as_jupyter()
	{
	Gtk::FileChooserDialog dialog("Please choose a file name to export as Jupyter notebook",
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_do_overwrite_confirmation(true);
	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string ipynb_name = dialog.get_filename();
			std::string out = JSON_serialise(doc);
			nlohmann::json json=nlohmann::json::parse(out);
			auto ipynb = cnb2ipynb(json);
			std::ofstream file(ipynb_name);
			file << ipynb.dump(3) << std::endl;

//			if(res.size()>0) {
//				Gtk::MessageDialog md("Error saving Jupyter notebook "+name);
//				md.set_transient_for(*this);
//				md.set_secondary_text(res);
//				md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
//				md.run();
//				}
		break;
		}
		}
	}

void NotebookWindow::on_file_export_html()
	{
	Gtk::FileChooserDialog dialog("Please enter a file name for the HTML document",
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string name = dialog.get_filename();
			std::ofstream temp(name);
			temp << export_as_HTML(doc);
			}
		}
	}

void NotebookWindow::on_file_export_latex()
	{
	Gtk::FileChooserDialog dialog("Please enter a file name for the LaTeX document",
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string name = dialog.get_filename();
			std::ofstream temp(name);
			std::size_t dotpos = name.rfind('.');
			std::string base = name.substr(0, dotpos);
			// std::cerr << base << std::endl;
			temp << export_as_LaTeX(doc, base);
			}
		}
	}

void NotebookWindow::on_file_export_python()
	{
	Gtk::FileChooserDialog dialog("Please enter a file name for the Python/Cadabra document",
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string name = dialog.get_filename();
			std::ofstream temp(name);
			temp << export_as_python(doc);
			}
		}
	}

void NotebookWindow::on_file_export_html_segment()
	{
	// FIXME: merge with on_file_export_html.
	Gtk::FileChooserDialog dialog("Please enter a file name for the HTML segment",
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string name = dialog.get_filename();
			std::ofstream temp(name);
			temp << export_as_HTML(doc, true);
			}
		}
	}

// FIXME: this logic can go into DocumentThread to be system independent.

std::string NotebookWindow::save(const std::string& fn) const
	{
	// Make a backup first, just in case things go wrong.
	std::ifstream old(fn.c_str());
	std::ofstream temp(std::string(fn+"~").c_str());

	if(old) { // only backup if there is something to backup
		if(temp) {
			std::string ln;
			while(std::getline(old, ln)) {
				temp << ln << "\n";
				if(!temp) return "Error writing backup file";
				}
			}
		else {
			return "Failed to create backup file";
			}
		}

	std::string out = JSON_serialise(doc);
	std::ofstream file(fn);
	file << out << std::endl;
	return "";
	}

bool NotebookWindow::quit_safeguard(bool quit)
	{
	if(modified) {
		std::string mes;
		if(quit) {
			if(name.size()>0) mes="Save changes to "+name+" before closing?";
			else              mes="Save changes before closing?";
			}
		else {
			if(name.size()>0) mes="Save changes to "+name+" before continuing?";
			else              mes="Save changes before continuing?";
			}
		Gtk::MessageDialog md(mes, false, Gtk::MESSAGE_WARNING,
		                      Gtk::BUTTONS_NONE, true);
		md.set_transient_for(*this);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		md.add_button("Save before continuing",1);
		md.add_button("Cancel",2);
		if(quit)
			md.add_button("No need to save, quit now",3);
		else
			md.add_button("No need to save", 3);
		int action=md.run();
		switch(action) {
			case 1:
				on_file_save();
				return true;
			case 2:
				break;
			case 3:
				return true;
			}
		}
	else return true;

	return false;
	}

void NotebookWindow::on_file_quit()
	{
	// FIXME: this needs to not just close the current window, but also all
	// other ones.
	if (quit_safeguard(true))
		hide();
	}

void NotebookWindow::on_edit_undo(const Glib::VariantBase&)
	{
	undo();
	}

void NotebookWindow::on_edit_redo(const Glib::VariantBase&)
	{
	redo();
	}

void NotebookWindow::on_edit_copy(const Glib::VariantBase&)
	{
	if(selected_cell!=doc.end()) {
		Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get(GDK_SELECTION_CLIPBOARD);
		on_outbox_copy(clipboard, selected_cell);
		}
	if(current_cell!=doc.end()) {
//		std::cerr << "copy called for non-outbox cell" << std::endl;
		// FIXME: handle other cell types.
		}
	}

void NotebookWindow::on_edit_paste()
	{
	if(current_cell!=doc.end()) {
		// std::cerr << "paste: " << std::endl;
      Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get(GDK_SELECTION_CLIPBOARD);
      clipboard->request_targets( [clipboard, this](const std::vector<Glib::ustring>& targets) {
			// Figure out which formats we can request.
			bool have_cadabra=false, have_utf8_string=false, have_text=false;
			for(const auto& t: targets) {
				// std::cerr << t << std::endl;
				if(t=="cadabra")
					have_cadabra=true;
				else if(t=="UTF8_STRING")
					have_utf8_string=true;
				else if(t=="TEXT")
					have_text=true;
				}
			// Now insert.
			std::string target;
			if(have_cadabra)   target="cadabra";
			else if(have_utf8_string) target="UTF8_STRING";
			else if(have_text)        target="TEXT";
			else return;

			// std::cerr << "requesting target " << target << std::endl;
			clipboard->request_contents(target, [this](const Gtk::SelectionData& data) {
				std::string content = data.get_data_as_string();
				auto vis = canvasses[current_canvas]->visualcells.find(&(*current_cell));
				if(vis!=canvasses[current_canvas]->visualcells.end()) {
					// std::cerr << "inserting " << content << std::endl;
					CodeInput *inbox = (*vis).second.inbox;
					inbox->edit.get_buffer()->insert_at_cursor(content);
					}
//				else {
//					std::cerr << "cannot find cell to insert into" << std::endl;
//					}
				});
			});
		}
	}

void NotebookWindow::on_edit_insert_above()
	{
	if(current_cell==doc.end()) return;

	DataCell newcell(DataCell::CellType::python, "");
	std::shared_ptr<ActionBase> action =
	   std::make_shared<ActionAddCell>(newcell, current_cell->id(), ActionAddCell::Position::before);
	queue_action(action);
	if (prefs.move_into_new_cell) {
		std::shared_ptr<ActionBase> action2 =
			std::make_shared<ActionPositionCursor>(newcell.id(), ActionPositionCursor::Position::in);
		queue_action(action2);
	}
	process_data();
	}

void NotebookWindow::on_edit_insert_below()
	{
	if(current_cell==doc.end()) return;

	DataCell newcell(DataCell::CellType::python, "");
	std::shared_ptr<ActionBase> action =
	   std::make_shared<ActionAddCell>(newcell, current_cell->id(), ActionAddCell::Position::after);
	queue_action(action);
	if (prefs.move_into_new_cell) {
		std::shared_ptr<ActionBase> action2 =
			std::make_shared<ActionPositionCursor>(newcell.id(), ActionPositionCursor::Position::in);
		queue_action(action2);
	}
	process_data();
	}

void NotebookWindow::on_edit_delete()
	{
	if(current_cell==doc.end()) return;

	if(current_cell->running) return; // we are still expecting results, don't delete

	DTree::sibling_iterator nxt=doc.next_sibling(current_cell);
	if(current_cell->textbuf=="" && doc.is_valid(nxt)==false) return; // Do not delete last cell if it is empty.

	std::shared_ptr<ActionBase> action =
	   std::make_shared<ActionPositionCursor>(current_cell->id(), ActionPositionCursor::Position::next);
	queue_action(action);
	std::shared_ptr<ActionBase> action2 =
	   std::make_shared<ActionRemoveCell>(current_cell->id());
	queue_action(action2);
	process_data();
	}

void NotebookWindow::on_edit_find()
	{
	searchbar.set_search_mode(true);
	searchbar.set_show_close_button(true);
	searchentry.grab_focus();
	search_result.set_text("");
	}

void NotebookWindow::on_search_text_changed()
	{
	const auto text = searchentry.get_text();
	if(text.size()==0) return;

	auto start = std::make_pair<DTree::iterator, size_t>(doc.begin(), 0);
	if(last_find_location.second!=std::string::npos) {
		select_range(doc, last_find_location.first, 0, 0);
		start = last_find_location;
		if(last_find_string.size()==text.size())
			++start.second;
		}
	else {
		}

	auto res = find_string(start.first, start.second, text, search_case_insensitive.get_active());
	last_find_location=res;
	last_find_string=text;
	if(res.second!=std::string::npos) {
		if(res.first->cell_type!=DataCell::CellType::python &&
			res.first->cell_type!=DataCell::CellType::latex) {
			search_result.set_text("Found.");
			scroll_cell_into_view(res.first);
			}
		else {
			current_cell=res.first;
			search_result.set_text("Found.");
			scroll_current_cell_into_view();
			select_range(doc, res.first, res.second, text.size());
			}
		}
	else {
		last_find_string="";
		search_result.set_text("Not found, try again to start from start.");
		}
	}

void NotebookWindow::on_edit_split()
	{
	std::shared_ptr<ActionBase> action = std::make_shared<ActionSplitCell>(current_cell->id());
	queue_action(action);
	process_data();
	}

void NotebookWindow::on_edit_cell_is_python()
	{
	if(current_cell==doc.end()) return;

	if(current_cell->cell_type==DataCell::CellType::latex) {
		current_cell->cell_type = DataCell::CellType::python;
		update_cell(doc, current_cell);
		}
	refresh_highlighting();
	}

void NotebookWindow::on_ignore_cell_on_import()
	{
	if(current_cell==doc.end()) return;

	current_cell->ignore_on_import= !(current_cell->ignore_on_import);
	update_cell(doc, current_cell);
	refresh_highlighting();
	}

void NotebookWindow::on_edit_cell_is_latex()
	{
	if(current_cell==doc.end()) return;

	if(current_cell->cell_type==DataCell::CellType::python) {
		current_cell->cell_type = DataCell::CellType::latex;
		update_cell(doc, current_cell);
		}
	refresh_highlighting();
	}

void NotebookWindow::on_view_split()
	{
	canvasses.push_back(new NotebookCanvas());
	// Add the new canvas into the bottom pane of the last visible canvas.
	canvasses[canvasses.size()-2]->pack2(*canvasses.back(), true, true);
	build_visual_representation();
	canvasses.back()->show_all();
	canvasses[canvasses.size()-2]->set_position(canvasses[canvasses.size()-2]->get_height()/2.0);
	}

void NotebookWindow::on_view_close(const Glib::VariantBase&)
	{
	// FIXME: this always removes the last canvas, not the current one.
	if(canvasses.size()>1) {
		canvasses[canvasses.size()-2]->remove(*canvasses.back());
		NotebookCanvas *oldcanvas = canvasses.back();
		canvasses.pop_back();
		delete oldcanvas;
		}
	}

void NotebookWindow::on_run_cell()
	{
	if(read_only) return;

	// This is actually handled by the CodeInput widget, which ensures that the
	// DTree is up to date and then calls execute.

	VisualCell& actual = canvasses[current_canvas]->visualcells[&(*current_cell)];
	actual.inbox->edit.shift_enter_pressed();

	//	cell_content_execute(current_cell, current_canvas);
	}

void NotebookWindow::on_run_runall()
	{
	// FIXME: move to DocumentThread

	DTree::sibling_iterator sib=doc.begin(doc.begin());
	while(sib!=doc.end(doc.begin())) {
		if(sib->cell_type==DataCell::CellType::python)
			cell_content_execute(DTree::iterator(sib), current_canvas, false);
		++sib;
		}
	}

void NotebookWindow::on_run_runtocursor()
	{
	// FIXME: move to DocumentThread

	DTree::sibling_iterator sib=doc.begin(doc.begin());
	while(sib!=current_cell && sib!=doc.end(doc.begin())) {
		if(sib->cell_type==DataCell::CellType::python)
			cell_content_execute(DTree::iterator(sib), current_canvas, false);
		++sib;
		}
	}

void NotebookWindow::on_run_stop()
	{
	compute->stop();
	}

void NotebookWindow::on_kernel_restart()
	{
	// FIXME: add warnings

	compute->restart_kernel();

		{
		std::lock_guard<std::mutex> guard(status_mutex);
		progress_string = "Idle";
		progress_frac = 0.0;
		}

	dispatch_update_status.emit();
	}

void NotebookWindow::on_help() const
	{
	if(current_cell==doc.end()) return;
	if(current_cell->cell_type!=DataCell::CellType::python) return;

	// Figure out the keyword under the cursor.
	VisualCell& actual = canvasses[current_canvas]->visualcells[&(*current_cell)];
	std::string before, after;
	actual.inbox->slice_cell(before, after);

	help_t help_type;
	std::string help_topic;
	help_type_and_topic(before, after, help_type, help_topic);

	snoop::log("help") << help_topic << snoop::flush;

	bool ret=false;
	std::string pref = cadabra::install_prefix()+"/share/cadabra2/manual/";
	if(help_type==help_t::algorithm)
		ret=cdbapp->open_help(pref+"algorithms/"+help_topic+".cnb", help_topic);
	if(help_type==help_t::property)
		ret=cdbapp->open_help(pref+"properties/"+help_topic+".cnb", help_topic);

	if(!ret) {
		Gtk::MessageDialog md("No help available", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
		std::cerr << "NotebookWindow::on_help: did not find help in " << pref << std::endl;
		//		md.set_transient_for(*this);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		md.set_secondary_text("No help available for '"+help_topic+"'.\nNot all algorithms and properties have manual pages yet, sorry.");
		md.run();
		}
	}

void NotebookWindow::set_compute_thread(ComputeThread* cthread)
	{
	DocumentThread::set_compute_thread(cthread);
	}

void NotebookWindow::on_help_about()
	{
	Glib::RefPtr<Gdk::Pixbuf> logo=Gdk::Pixbuf::create_from_file(cadabra::install_prefix()+"/share/cadabra2/images/cadabra2-gtk.png");

	Gtk::AboutDialog about;
	about.set_transient_for(*this);
	about.set_program_name("Cadabra");
	about.set_comments("A field-theory motivated approach to computer algebra");
	about.set_version(std::string("Version ")+CADABRA_VERSION_SEM
	                  +" (build "+CADABRA_VERSION_BUILD+" dated "+CADABRA_VERSION_DATE+")");
	std::vector<Glib::ustring> authors;
	authors.push_back("Kasper Peeters");
	about.set_authors(authors);
	about.set_copyright(std::string("\xC2\xA9 ")+COPYRIGHT_YEARS+std::string(" Kasper Peeters"));
	about.set_license_type(Gtk::License::LICENSE_GPL_3_0);
	about.set_website("http://cadabra.science");
	about.set_website_label("cadabra.science");
	about.set_logo(logo);
	std::vector<Glib::ustring> special;
	special.push_back("José M. Martín-García (for the xPerm canonicalisation code)");
	special.push_back("Dominic Price (for many additions, e.g. the conversion to pybind and the meld algorithm)");
	special.push_back("Connor Behan (for various improvements related to index-free algorithms)");
	special.push_back("James Allen (for writing much of the factoring code)");
	special.push_back("NanoMichael (for the MicroTeX rendering library)");
	special.push_back("Daniel Butter (substitute rule cache and other improvements)");
	special.push_back("Software Sustainability Institute");
	special.push_back("Institute of Advanced Study (for a Christopherson/Knott fellowship)");
	about.add_credit_section("Special thanks", special);
	about.run();
	}

void NotebookWindow::on_help_register()
	{
	Gtk::Dialog md("Welcome to Cadabra!", *this, Gtk::MESSAGE_WARNING);
	md.set_transient_for(*this);
	md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	Gtk::Box *box = md.get_content_area();
	Gtk::Label txt;
	txt.set_markup("<span font_size=\"large\" font_weight=\"bold\">Welcome to Cadabra!</span>\n\nWriting this software takes an incredible amount of spare time,\nand it is extremely difficult to get funding for its development.\n\nPlease show your support by registering your email address,\nso I can convince the bean-counters that this software is of interest.\n\nI will only use this address to count users and to email you,\nroughly once every half a year, with a bit of news about Cadabra.\n\nMany thanks for your support!\n\nKasper Peeters, <a href=\"mailto:info@cadabra.science\">info@cadabra.science</a>");
	txt.set_line_wrap();
	txt.set_margin_top(10);
	txt.set_margin_start(10);
	txt.set_margin_end(10);
	txt.set_margin_bottom(10);
	box->pack_start(txt, Gtk::PACK_EXPAND_WIDGET);

	Gtk::Grid grid;
	grid.set_column_homogeneous(false);
	grid.set_hexpand(true);
	grid.set_margin_start(10);
	grid.set_margin_end(10);
	box->pack_start(grid, Gtk::PACK_EXPAND_WIDGET);

	Gtk::Label name_label("Name:");
	Gtk::Entry name;
//	name_label.set_alignment(0, 0.5);
	name.set_hexpand(true);
	grid.attach(name_label, 0, 0, 1, 1);
	grid.attach(name, 1, 0, 1, 1);
	Gtk::Label email_label("Email address:");
//	email_label.set_alignment(0, 0.5);
	Gtk::Entry email;
	email.set_hexpand(true);
	grid.attach(email_label, 0, 1, 1, 1);
	grid.attach(email, 1, 1, 1, 1);
	Gtk::Label affiliation_label("Affiliation:");
	Gtk::Entry affiliation;
//	affiliation_label.set_alignment(0, 0.5);
	affiliation.set_hexpand(true);
	grid.attach(affiliation_label, 0, 2, 1, 1);
	grid.attach(affiliation, 1, 2, 1, 1);

	Gtk::HBox hbox;
	box->pack_end(hbox, Gtk::PACK_SHRINK);
	Gtk::Button reg("Register my support"), nothanks("I prefer to stay anonymous"), alreadyset("I am already registered");
	hbox.pack_end(reg, Gtk::PACK_SHRINK, 10);
	hbox.pack_start(nothanks, Gtk::PACK_SHRINK, 10);
	hbox.pack_start(alreadyset, Gtk::PACK_SHRINK, 10);
	reg.signal_clicked().connect([&]() {
		set_user_details(name.get_text(), email.get_text(), affiliation.get_text());
		prefs.is_anonymous = false;
		prefs.is_registered = true;
		md.hide();
		});
	nothanks.signal_clicked().connect([&]() {
		prefs.is_anonymous = true;
		prefs.is_registered = false;
		md.hide();
		});
	alreadyset.signal_clicked().connect([&]() {
		prefs.is_registered = true;
		prefs.is_anonymous = false;
		md.hide();
		});
	box->show_all();
	md.run();
	action_register->set_enabled(!prefs.is_registered);
	}

void NotebookWindow::on_text_scaling_factor_changed(const std::string& key)
	{
#ifdef DEBUG
	std::cerr << key << std::endl;
#endif
	if(key=="text-scaling-factor" || key=="scaling-factor") {
		auto screen = Gdk::Screen::get_default();
		scale   = settings->get_double("text-scaling-factor");
		scale  *= screen->get_monitor_scale_factor(0);
		std::cout << "cadabra-client: total scaling-factor = " << scale << std::endl;
		engine.set_scale(scale, screen->get_monitor_scale_factor(0));
		engine.invalidate_all();
		tex_run_async();

		auto it=canvasses.begin();
		while(it!=canvasses.end()) {
			(*it)->refresh_all();
			++it;
			}
		}
	}

void NotebookWindow::select_git_path()
	{
	SelectFileDialog dialog("Select git executable", *this, true);
	dialog.set_transient_for(*this);
	dialog.set_text(prefs.git_path);

	if (dialog.run() == Gtk::RESPONSE_OK)
		prefs.git_path = dialog.get_text();
	}

void NotebookWindow::compare_to_file()
	{
	std::string filename;

	SelectFileDialog dialog("Select file to compare", *this, true);
	dialog.set_transient_for(*this);
	if (dialog.run() == Gtk::RESPONSE_OK)
		filename = dialog.get_text();
	else
		return;

	std::ifstream a(filename);
	if (!a.is_open()) {
		Gtk::MessageDialog error_dialog(*this, "The file '" + filename + "' could not be opened for reading", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		error_dialog.set_title("Could not open file");
		error_dialog.run();
		return;

		std::stringstream b;
		b << JSON_serialise(doc);

		diffviewer = std::make_unique<DiffViewer>(a, b, *this);
		//diffviewer->set_transient_for(*this);
		diffviewer->run_noblock();
		}
	}

std::string NotebookWindow::run_git_command(const std::string& args)
	{
	using namespace TinyProcessLib;

	auto split_pos = name.find_last_of("\\/");
	if (split_pos == std::string::npos)
		throw std::runtime_error("File must be in a valid git directory");
	std::string path = name.substr(0, split_pos);
	std::string output;

	Process git(
	   prefs.git_path + " " + args,
	   path,
	[&output](const char* bytes, size_t n) {
		output += std::string(bytes, n);
		},
	[&output](const char* bytes, size_t n) {
		output += std::string(bytes, n);
		}
	);

	if (git.get_exit_status())
		throw std::runtime_error(output);
	else
		return output;
	}

void NotebookWindow::compare_git(const std::string& commit_hash)
	{
	auto split_pos = name.find_last_of("\\/");
	if (split_pos == std::string::npos || split_pos == name.size() - 1)
		throw std::runtime_error("A valid file must be open");

	auto prefix = trim(run_git_command("rev-parse --show-prefix"));
	auto contents = run_git_command("show " + commit_hash + ":" + prefix + name.substr(split_pos + 1));

	std::stringstream a, b;
	a << contents;
	b << JSON_serialise(doc);

	diffviewer = std::make_unique<DiffViewer>(a, b, *this);
	//diffviewer->set_transient_for(*this);
	diffviewer->run_noblock();
	}

void NotebookWindow::compare_git_latest()
	{
	try {
		// Get latest commit hash
		auto commit = run_git_command("log --pretty=format:%h -n 1");
		compare_git(trim(commit));
		}
	catch (const std::exception& ex) {
		Gtk::MessageDialog error_dialog(ex.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		error_dialog.set_transient_for(*this);
		error_dialog.set_title("Git error");
		error_dialog.run();
		}
	}

class GitChooseModelColumns : public Gtk::TreeModel::ColumnRecord {
	public:
		GitChooseModelColumns()
			{
			add(commit_hash);
			add(author);
			add(timestamp);
			add(description);
			}

		Gtk::TreeModelColumn<std::string> commit_hash;
		Gtk::TreeModelColumn<std::string> author;
		Gtk::TreeModelColumn<std::string> timestamp;
		Gtk::TreeModelColumn<std::string> description;
	};


void NotebookWindow::compare_git_choose()
	{
	try {
		std::string commit_hash;
		std::string max_entries = "15";
		auto commits = string_to_vec(run_git_command("log --pretty=format:%h -n " + max_entries));
		auto authors = string_to_vec(run_git_command("log --pretty=format:%an -n " + max_entries));
		auto times = string_to_vec(run_git_command("log --pretty=format:%ar -n " + max_entries));
		auto descriptions = string_to_vec(run_git_command("log --pretty=format:%s -n " + max_entries));

		Gtk::TreeView tree_view;
		GitChooseModelColumns columns;
		Glib::RefPtr<Gtk::ListStore> list_store = Gtk::ListStore::create(columns);

		for (size_t i = 0; i < commits.size(); ++i) {
			Gtk::TreeModel::Row row = *(list_store->append());
			row[columns.commit_hash] = commits[i];
			row[columns.author] = authors[i];
			row[columns.timestamp] = times[i];
			row[columns.description] = descriptions[i];
			}

		tree_view.set_model(list_store);
		tree_view.append_column("Commit Hash", columns.commit_hash);
		tree_view.append_column("Author", columns.author);
		tree_view.append_column("Commit Time", columns.timestamp);
		tree_view.append_column("Description", columns.description);

		Gtk::ScrolledWindow scrolled_window;
		scrolled_window.add(tree_view);
		scrolled_window.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		scrolled_window.set_min_content_height(400);

		Gtk::Dialog select_dialog("Select a commit to compare", *this, true);
		select_dialog.set_transient_for(*this);
		select_dialog.get_content_area()->add(scrolled_window);
		select_dialog.add_button("Compare", Gtk::RESPONSE_OK);
		select_dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
		select_dialog.show_all();

		if (select_dialog.run() == Gtk::RESPONSE_OK)
			commit_hash = tree_view.get_selection()->get_selected()->get_value(columns.commit_hash);
		else
			return;

		compare_git(trim(commit_hash));
		}
	catch (const std::exception& ex) {
		Gtk::MessageDialog error_dialog(ex.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		error_dialog.set_transient_for(*this);
		error_dialog.set_title("Git error");
		error_dialog.run();
		}
	}

void NotebookWindow::compare_git_specific()
	{
	try {
		std::string commit_hash;
		Gtk::Entry entry;
		Gtk::Dialog entry_dialog("Enter hash of commit to compare against", *this, true);
		entry_dialog.set_transient_for(*this);
		entry_dialog.get_content_area()->add(entry);
		entry_dialog.add_button("Compare", Gtk::RESPONSE_OK);
		entry_dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
		entry_dialog.show_all();

		if (entry_dialog.run() == Gtk::RESPONSE_OK)
			commit_hash = entry.get_text();
		else
			return;

		compare_git(trim(commit_hash));
		}
	catch (const std::exception& ex) {
		Gtk::MessageDialog error_dialog(ex.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		error_dialog.set_transient_for(*this);
		error_dialog.set_title("Git error");
		error_dialog.run();
		}
	}

void NotebookWindow::on_prefs_auto_close_latex(const Glib::VariantBase& vb)
	{
	auto state_variant = action_auto_close_latex->get_state_variant(); //.get_bool();
	bool state = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(state_variant).get();
	
	if(prefs.auto_close_latex == !state) return;

	prefs.auto_close_latex = !state;
	action_auto_close_latex->set_state(Glib::Variant<bool>::create(prefs.auto_close_latex));
	prefs.save();
	}

void NotebookWindow::on_prefs_font_size(int num)
	{
	if(prefs.font_step==num) return;

	prefs.font_step=num;
	action_fontsize->set_state(Glib::Variant<int>::create(num));
	prefs.save();
	
	engine.set_font_size(12+(num*2));
	load_css();

	if(is_configured) {
		// std::cerr << "cadabra-client: re-running TeX to change font size" << std::endl;
		// No point in running TeX on all cells if we have not yet had an
		// on_configure_event signal; that will come after us and then we will
		// have to run all again.
		engine.invalidate_all();
		tex_run_async();

		//for(auto& canvas: canvasses) {
		//	for(auto& visualcell: canvas->visualcells) {
		//		if(visualcell.first->cell_type==DataCell::CellType::python ||
		//		      visualcell.first->cell_type==DataCell::CellType::latex) {
		//			visualcell.second.inbox->set_font_size(num);
		//			}
		//		}
		//	}

		for(unsigned int i=0; i<canvasses.size(); ++i)
			canvasses[i]->refresh_all();
		}
	}

void NotebookWindow::on_prefs_highlight_syntax(bool on)
	{
	if (prefs.highlight == on) return;
	prefs.highlight = on;
	prefs.save();
	action_highlight->set_state(Glib::Variant<int>::create(on));
	
	for (auto& canvas : canvasses) {
		for (auto& visualcell : canvas->visualcells) {
			// Need to be careful to only try and do this on latex or
			// python cells to avoid an exception being raised when
			// trying to edit an immutable cell type
			switch (visualcell.first->cell_type) {
				// Fallthrough
				case DataCell::CellType::python:
				case DataCell::CellType::latex:
					if(on) {
						load_css();
						visualcell.second.inbox->enable_highlighting(visualcell.first->cell_type, prefs);
						}
					else {
						load_css();
						visualcell.second.inbox->disable_highlighting();
						}
					break;
				default:
					break;
				}
			}
		}
	}

void NotebookWindow::on_prefs_microtex(bool on)
	{
	if (prefs.microtex == on) return;
	prefs.microtex = on;
	prefs.save();
	action_microtex->set_state(Glib::Variant<int>::create(on));

	Gtk::MessageDialog md("Restart Cadabra to make the rendering setting take effect",
								 false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
	md.set_resizable(false);
	md.set_transient_for(*this);
	md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	md.run();
	
// 	for (auto& canvas : canvasses) {
// 		for (auto& visualcell : canvas->visualcells) {
// 			// Need to be careful to only try and do this on latex or
// 			// python cells to avoid an exception being raised when
// 			// trying to edit an immutable cell type
// 			switch (visualcell.first->cell_type) {
// 				// Fallthrough
// 				case DataCell::CellType::python:
// 				case DataCell::CellType::latex:
// 					visualcell.second.outbox->set_use_microtex(prefs.microtex);
// 					break;
// 				default:
// 					break;
// 				}
// 			}
// 		}
	}

void NotebookWindow::on_prefs_choose_colours()
	{
	cadabra::ChooseColoursDialog dialog(prefs, *this);
	dialog.run();
	prefs.save();
	}

void NotebookWindow::on_prefs_use_defaults()
	{
	Gtk::MessageDialog sure("Use Default settings?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
	sure.set_transient_for(*this);
	sure.set_secondary_text("This will effect font size and colour schemes.");
	int response = sure.run();
	if (response == Gtk::RESPONSE_YES) {
		prefs = Prefs(true);
		for (auto& action : default_actions)
			action->activate();
		refresh_highlighting();
		action_register->set_enabled(!prefs.is_registered);
		prefs.save();
		}
	}

void NotebookWindow::on_tools_options()
{
	// Create the dialog box
	Gtk::Dialog options("Options", false);
	options.set_transient_for(*this);
	options.get_content_area()->property_margin() = 5;
	options.add_button("Apply", Gtk::RESPONSE_APPLY);
	options.add_button("Cancel", Gtk::RESPONSE_CANCEL);

	// Helper functions for making option widgets
	auto add_label = [&options](const char* text, Gtk::Align alignment) {
		auto label = std::make_unique<Gtk::Label>();
		label->set_markup(text);
		label->set_halign(alignment);
		options.get_content_area()->add(*label);
		return label;
	};
	auto add_sep = [&options]() {
		auto sep = std::make_unique<Gtk::Separator>();
		sep->property_margin() = 5;
		options.get_content_area()->add(*sep);
		return sep;
	};
	auto add_checkbox = [&options](const char* label, bool active) {
		auto cb = std::make_unique<Gtk::CheckButton>(label);
		cb->set_active(active);
		options.get_content_area()->add(*cb);
		return cb;
	};
	auto add_entry = [&options](const Glib::ustring& val) {
		auto entry = std::make_unique<Gtk::Entry>();
		entry->set_text(val);
		options.get_content_area()->add(*entry);
		return entry;
	};

	// Add widgets & display
	auto py_opts = add_label("<b>Cadabra/Python Options</b>", Gtk::ALIGN_CENTER);
	auto pypath_label = add_label("Default Python Path (semicolon separated):", Gtk::ALIGN_START);
	auto pypath = add_entry(prefs.python_path);
	auto sep1 = add_sep();
	auto gui_opts = add_label("<b>GUI Options</b>", Gtk::ALIGN_CENTER);
	auto auto_move = add_checkbox("Automatically move into a created cell", prefs.move_into_new_cell);
	auto tab_completion = add_checkbox("Show possible completions with TAB", prefs.tab_completion);
	auto sep2 = add_sep();
	options.show_all();
	int res = options.run();

	// Options menu closed, run any updates
	if (res == Gtk::RESPONSE_APPLY) {
		if (prefs.python_path != pypath->get_text()) {
			prefs.python_path = pypath->get_text();
			Gtk::MessageDialog md(*this, "Changes to python path won't take effect until kernel is restarted; restart now?", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
			md.set_title("Python Path Changed");
			md.set_modal();
			md.set_position(Gtk::WindowPosition::WIN_POS_CENTER);
			int res = md.run();
			if (res == Gtk::RESPONSE_YES) {
				on_kernel_restart();
			}
		}
		prefs.move_into_new_cell = auto_move->get_active();
		prefs.tab_completion = tab_completion->get_active();
	}

}

bool remove_recursive(const gchar* path)
{
	// If filename is a file then we have reached the end of the recursion tree
	if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
		return g_remove(path) == 0;
	bool success = true;
	auto dir = g_dir_open(path, 0, NULL);
	for (const gchar* child = g_dir_read_name(dir); child != NULL; child = g_dir_read_name(dir)) {
		gchar* fullpath = g_strconcat(path, G_DIR_SEPARATOR_S, child, NULL);
		success = remove_recursive(fullpath);
		g_free(fullpath);
		if (!success)
			break;
	}
	g_dir_close(dir);
	if (!success)
		return false;

	return g_remove(path) == 0;
}

void NotebookWindow::on_tools_clear_cache()
{
	Gtk::MessageDialog prog("Removing cached library files, please wait...", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
	prog.set_transient_for(*this);
	prog.set_title("Clearing cache");
	prog.show_all();

	auto config_dir = Glib::ustring(g_get_user_config_dir()) + "/cadabra_packages";
	bool success = remove_recursive(config_dir.c_str());

	prog.set_message(success
		? "Cache cleared!"
		: "Failed to remove cached library files. You can manually remove them by deleting the cadabra_packages folder in your config directory"
	);

	prog.run();
}

void NotebookWindow::refresh_highlighting()
	{
	if (prefs.highlight) {
		on_prefs_highlight_syntax(false);
		on_prefs_highlight_syntax(true);
		}
	}

bool NotebookWindow::idle_handler()
	{
#if GTKMM_MINOR_VERSION>=10
	for(auto& reveal: to_reveal) {
		reveal->set_reveal_child(true);
		}
	to_reveal.clear();
#endif
	return false; // disconnect
	}

void NotebookWindow::unselect_output_cell()
	{
	if(selected_cell==doc.end()) return;

	for(unsigned int i=0; i<canvasses.size(); ++i) {
		if(canvasses[i]->visualcells.find(&(*selected_cell))!=canvasses[i]->visualcells.end()) {
			auto& outbox = canvasses[i]->visualcells[&(*selected_cell)].outbox;
			outbox->image.set_state_flags(Gtk::STATE_FLAG_NORMAL);
			outbox->queue_draw();
			}
		}
	selected_cell=doc.end();
	action_copy->set_enabled(false);
	}

bool NotebookWindow::handle_outbox_select(GdkEventButton *, DTree::iterator it)
	{
	// std::cerr << "handle_outbox_select " << it->textbuf << std::endl;
	unselect_output_cell();

	// Colour the background of the selected cell, in all canvasses.
	for(int i=0; i<(int)canvasses.size(); ++i) {
		if(canvasses[i]->visualcells.find(&(*it))!=canvasses[i]->visualcells.end()) {
			auto& outbox = canvasses[i]->visualcells[&(*it)].outbox;
			outbox->image.set_state_flags(Gtk::STATE_FLAG_SELECTED);
			outbox->queue_draw();
			if(i==current_canvas) {
				outbox->grab_focus();
				}
			}
		}
	selected_cell=it;
	action_copy->set_enabled(true);

	// We now require that the user ctrl-c's this before copying to the clipboard.
	// Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get(GDK_SELECTION_PRIMARY);
	// on_outbox_copy(clipboard, selected_cell);
	return true;
	}

void NotebookWindow::on_outbox_copy(Glib::RefPtr<Gtk::Clipboard> refClipboard, DTree::iterator it)
	{
	std::string cpystring=(*it).textbuf;

	// Find the child cell which contains the input_form data.
	auto sib=doc.begin(it);
//	std::cerr << "Finding children of " << it->textbuf << std::endl;
	while(sib!=doc.end(it)) {
		if(sib->cell_type==DataCell::CellType::input_form) {
			clipboard_cdb = sib->textbuf;
//			std::cerr << "found input form " << clipboard_cdb << std::endl;
			break;
			}
		++sib;
		}

	// Setup clipboard handling
	clipboard_txt = cpystring;
	std::vector<Gtk::TargetEntry> listTargets;
	if(clipboard_cdb.size()>0) {
//		std::cerr << "let them know we have cadabra format" << std::endl;
		listTargets.push_back( Gtk::TargetEntry("cadabra") );
		}
	listTargets.push_back( Gtk::TargetEntry("UTF8_STRING") );
	listTargets.push_back( Gtk::TargetEntry("TEXT") );
	refClipboard->set( listTargets,
	                   sigc::mem_fun(this, &NotebookWindow::on_clipboard_get),
	                   sigc::mem_fun(this, &NotebookWindow::on_clipboard_clear) );
	}

void NotebookWindow::on_clipboard_get(Gtk::SelectionData& selection_data, guint )
	{
	const Glib::ustring target = selection_data.get_target();

	if(target == "cadabra") {
//		std::cerr << "received request for target cadabra: " << clipboard_cdb << std::endl;
		selection_data.set("cadabra", clipboard_cdb);
		}
	else if(target == "UTF8_STRING" || target=="TEXT") {
		selection_data.set_text(clipboard_txt);
		}
	}

void NotebookWindow::on_clipboard_clear()
	{
	}
