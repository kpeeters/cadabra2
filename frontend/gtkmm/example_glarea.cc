/* Open GL Area
 *
 * Gtk::GLArea is a widget that allows custom drawing using OpenGL calls.
 *
 */

#include <iostream>
#include <string>
#include <vector>

#include <gtkmm.h>
#include <giomm/resource.h>
#include <epoxy/gl.h>

#include <pybind11/embed.h>
namespace py = pybind11;

using std::cerr;
using std::endl;
using std::string;

enum {
  X_AXIS,
  Y_AXIS,
  Z_AXIS,

  N_AXIS
};

static const GLfloat vertex_data[] = {
  0.f,   0.5f,   0.f, 1.f,
  0.5f, -0.366f, 0.f, 1.f,
 -0.5f, -0.366f, 0.f, 1.f,
};

class Example_GLArea : public Gtk::Window
{
public:
  Example_GLArea();
  ~Example_GLArea() override;

protected:
  // The m_GLContext must be deleted after m_GLArea, therefore declared before m_GLArea.
  Glib::RefPtr<Gdk::GLContext> m_GLContext;
  Gtk::Box m_VBox {Gtk::ORIENTATION_VERTICAL, false};
  Gtk::GLArea m_GLArea;
  Gtk::Box m_Controls {Gtk::ORIENTATION_VERTICAL, false};
  Gtk::Button m_Button {"Quit"};

  GLuint m_Vao {0};
  GLuint m_Buffer {0};
  GLuint m_Program {0};
  GLuint m_Mvp {0};

  py::scoped_interpreter guard;
		
  std::vector<float> m_RotationAngles;

  void on_axis_value_change(int axis, const Glib::RefPtr<Gtk::Adjustment>& adj);

  void realize();
  void unrealize();
  bool render(const Glib::RefPtr<Gdk::GLContext>& context);

  Gtk::Box* create_axis_slider_box(int axis);
  void init_buffers();
  void init_shaders(const std::string& vertex_path, const std::string& fragment_path);
  void draw_triangle();
		void first_render();
};

Gtk::Window* do_glarea()
{
  return new Example_GLArea();
}

Example_GLArea::Example_GLArea() : m_RotationAngles(N_AXIS, 0.0f)
{
  set_title("GL Area");
  set_default_size(400, 600);
  set_border_width(12);

  m_VBox.set_spacing(6);
  add(m_VBox);

  m_GLArea.set_hexpand(true);
  m_GLArea.set_vexpand(true);
  m_GLArea.set_auto_render(true);
  m_GLArea.set_required_version(3, 3);
  m_GLArea.set_has_depth_buffer(true);
  m_VBox.add(m_GLArea);

  // Connect gl area signals
  // Important that the realize signal calls our handler to clean up
  // GL resources _after_ the default unrealize handler is called (the "true")
  m_GLArea.signal_realize().connect(sigc::mem_fun(*this, &Example_GLArea::realize), true);
  // Important that the unrealize signal calls our handler to clean up
  // GL resources _before_ the default unrealize handler is called (the "false")
  m_GLArea.signal_unrealize().connect(sigc::mem_fun(*this, &Example_GLArea::unrealize), false);
  m_GLArea.signal_render().connect(sigc::mem_fun(*this, &Example_GLArea::render));

  m_VBox.add(m_Controls);
  m_Controls.set_hexpand(true);

  for(int i = 0 ; i < N_AXIS ; ++i)
  {
    auto sliderBox = create_axis_slider_box(i);
    m_Controls.add(*sliderBox);
  }

  m_Button.set_hexpand(true);
  m_VBox.add(m_Button);
  // Connect clicked to close of window
  m_Button.signal_clicked().connect(sigc::mem_fun(*this, &Gtk::Window::close));

  show_all();
}

Example_GLArea::~Example_GLArea()
{
}

void Example_GLArea::first_render()
	{
    py::exec(R"(
import moderngl
import struct
ctx = moderngl.create_context(require=330)
print(ctx)
prog = ctx.program(
    vertex_shader = """
        #version 330
        in vec2 vert;
        
        void main() {
            gl_Position = vec4(vert, 0.0, 1.0);
        }
    """,
    fragment_shader = """
        #version 330
        out vec4 color;
        
        void main() {
            color = vec4(0.3, 0.5, 1.0, 1.0);
        }
    """
)
print(prog)

vbo = ctx.buffer(struct.pack(
    "6f",
    0.0, 0.8,
    -0.6, -0.8,
    0.6, -0.8,
))
print(vbo)
vao = ctx.simple_vertex_array(prog, vbo, "vert")
)");
	}


void Example_GLArea::on_axis_value_change(int axis, const Glib::RefPtr<Gtk::Adjustment>& adj)
{
  m_RotationAngles[axis] = adj->get_value();
  m_GLArea.queue_draw();
}

void Example_GLArea::realize()
{
  m_GLArea.make_current();
  try
  {
    m_GLArea.throw_if_error();
    init_buffers();

    const bool use_es = m_GLArea.get_context()->get_use_es();
    const std::string vertex_path = use_es ? "glarea-gles.vs.glsl" : "glarea-gl.vs.glsl";
    const std::string fragment_path = use_es ? "glarea-gles.fs.glsl" : "glarea-gl.fs.glsl";
    init_shaders(vertex_path, fragment_path);

    // GtkGLArea can drop its reference on the GLContext before the unrealize signal
    // is emitted. We get our own reference and keep it until Example_GLArea::unrealize()
    // has been called, thus making sure the context is not prematurely deleted.
    // https://gitlab.gnome.org/GNOME/gtkmm/issues/63
    // m_GLContext is unnecessary if https://gitlab.gnome.org/GNOME/gtk/issues/2309
    // is fixed as in gtk4.
    m_GLContext = m_GLArea.get_context();
  }
  catch(const Gdk::GLError& gle)
  {
    cerr << "An error occured making the context current during realize:" << endl;
    cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
  }
}

void Example_GLArea::unrealize()
{
  // Does nothing if GtkGLArea has dropped its reference on the GLContext.
  m_GLArea.make_current();

  if (m_GLContext)
    // Necessary if m_GLArea.make_current() does nothing.
    m_GLContext->make_current();

  try
  {
    m_GLArea.throw_if_error();

    // Delete buffers and program
    glDeleteBuffers(1, &m_Buffer);
    glDeleteProgram(m_Program);
  }
  catch(const Gdk::GLError& gle)
  {
    cerr << "An error occured making the context current during unrealize" << endl;
    cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
  }
}

bool Example_GLArea::render(const Glib::RefPtr<Gdk::GLContext>& context)
	{
	static bool first_call=true;
	
	m_GLArea.throw_if_error();
	
// 	 try {
// 		 if(first_call) {
// 			 first_call=false;
// 			 first_render();
// 			 }
//     py::exec(R"(
// print("rendering")
// vao.render()
// )");
// catch(py::error_already_set &e) {
// 	std::cerr << "Python exception: " << e.what() << std::endl;
// 	}
// return true;
	
	try {
		m_GLArea.throw_if_error();
		glClearColor(0.5, 0.5, 0.5, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		draw_triangle();
		
		glFlush();
		
		return true;
		}
	catch(const Gdk::GLError& gle) {
		cerr << "An error occurred in the render callback of the GLArea" << endl;
		cerr << gle.domain() << "-" << gle.code() << "-" << gle.what() << endl;
		return false;
		}
	}

Gtk::Box* Example_GLArea::create_axis_slider_box(int axis)
{
  auto box = Gtk::manage(new Gtk::Box{Gtk::ORIENTATION_HORIZONTAL, false});

  const char* text;

  switch(axis)
  {
    case X_AXIS:
    {
      text = "X axis";
      break;
    }
    case Y_AXIS:
    {
      text = "Y axis";
      break;
    }
    case Z_AXIS:
    {
      text = "Z axis";
      break;
    }
    default:
    {
      g_assert_not_reached();
    }
  }

  auto label = Gtk::manage(new Gtk::Label{text});
  box->add(*label);
  label->show();

  auto adj = Gtk::Adjustment::create(0.0, 0.0, 360.0, 1.0, 12.0, 0.0);

  adj->signal_value_changed().connect(
    sigc::bind(sigc::mem_fun(*this, &Example_GLArea::on_axis_value_change), axis, adj)
                                      );
  auto slider = Gtk::manage(new Gtk::Scale{adj, Gtk::ORIENTATION_HORIZONTAL});
  box->add(*slider);
  slider->set_hexpand(true);
  slider->show();

  box->show();

  return box;
}

void Example_GLArea::init_buffers()
{
  glGenVertexArrays(1, &m_Vao);
  glBindVertexArray(m_Vao);

  glGenBuffers(1, &m_Buffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static GLuint create_shader(int type, const char *src)
{
  auto shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  int status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(status == GL_FALSE)
  {
    int log_len;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

    string log_space(log_len+1, ' ');
    glGetShaderInfoLog(shader, log_len, nullptr, (GLchar*)log_space.c_str());

    cerr << "Compile failure in " <<
      (type == GL_VERTEX_SHADER ? "vertex" : "fragment") <<
      " shader: " << log_space << endl;

    glDeleteShader(shader);

    return 0;
  }

  return shader;
}

void Example_GLArea::init_shaders(const std::string& vertex_path, const std::string& fragment_path)
{
  auto vshader_bytes = Gio::Resource::lookup_data_global(vertex_path);
  if(!vshader_bytes)
  {
    cerr << "Failed fetching vertex shader resource" << endl;
    m_Program = 0;
    return;
  }
  gsize vshader_size {vshader_bytes->get_size()};
  auto vertex = create_shader(GL_VERTEX_SHADER,
                         (const char*)vshader_bytes->get_data(vshader_size));

  if(vertex == 0)
  {
    m_Program = 0;
    return;
  }

  auto fshader_bytes = Gio::Resource::lookup_data_global(fragment_path);
  if(!fshader_bytes)
  {
    cerr << "Failed fetching fragment shader resource" << endl;
    glDeleteShader(vertex);
    m_Program = 0;
    return;
  }
  gsize fshader_size {fshader_bytes->get_size()};
  auto fragment = create_shader(GL_FRAGMENT_SHADER,
                           (const char*)fshader_bytes->get_data(fshader_size));

  if(fragment == 0)
  {
    glDeleteShader(vertex);
    m_Program = 0;
    return;
  }

  m_Program = glCreateProgram();
  glAttachShader(m_Program, vertex);
  glAttachShader(m_Program, fragment);

  glLinkProgram(m_Program);

  int status;
  glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
  if(status == GL_FALSE)
  {
    int log_len;
    glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &log_len);

    string log_space(log_len+1, ' ');
    glGetProgramInfoLog(m_Program, log_len, nullptr, (GLchar*)log_space.c_str());

    cerr << "Linking failure: " << log_space << endl;

    glDeleteProgram(m_Program);
    m_Program = 0;
  }
  else
  {
    /* Get the location of the "mvp" uniform */
    m_Mvp = glGetUniformLocation(m_Program, "mvp");

    glDetachShader(m_Program, vertex);
    glDetachShader(m_Program, fragment);
  }
  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

static void compute_mvp(float *res,
                        float phi,
                        float theta,
                        float psi)
{
  float x       {phi * ((float)G_PI / 180.f)};
  float y       {theta * ((float)G_PI / 180.f)};
  float z       {psi * ((float)G_PI / 180.f)};
  float c1      {cosf (x)};
  float s1      {sinf (x)};
  float c2      {cosf (y)};
  float s2      {sinf (y)};
  float c3      {cosf (z)};
  float s3      {sinf (z)};
  float c3c2    {c3 * c2};
  float s3c1    {s3 * c1};
  float c3s2s1  {c3 * s2 * s1};
  float s3s1    {s3 * s1};
  float c3s2c1  {c3 * s2 * c1};
  float s3c2    {s3 * c2};
  float c3c1    {c3 * c1};
  float s3s2s1  {s3 * s2 * s1};
  float c3s1    {c3 * s1};
  float s3s2c1  {s3 * s2 * c1};
  float c2s1    {c2 * s1};
  float c2c1    {c2 * c1};

  /* apply all three rotations using the three matrices:
   *
   * ⎡  c3 s3 0 ⎤ ⎡ c2  0 -s2 ⎤ ⎡ 1   0  0 ⎤
   * ⎢ -s3 c3 0 ⎥ ⎢  0  1   0 ⎥ ⎢ 0  c1 s1 ⎥
   * ⎣   0  0 1 ⎦ ⎣ s2  0  c2 ⎦ ⎣ 0 -s1 c1 ⎦
   */
  res[0] = c3c2;  res[4] = s3c1 + c3s2s1;  res[8] = s3s1 - c3s2c1; res[12] = 0.f;
  res[1] = -s3c2; res[5] = c3c1 - s3s2s1;  res[9] = c3s1 + s3s2c1; res[13] = 0.f;
  res[2] = s2;    res[6] = -c2s1;         res[10] = c2c1;          res[14] = 0.f;
  res[3] = 0.f;   res[7] = 0.f;           res[11] = 0.f;           res[15] = 1.f;
}

void Example_GLArea::draw_triangle()
{
  float mvp[16];

  compute_mvp(mvp,
              m_RotationAngles[X_AXIS],
              m_RotationAngles[Y_AXIS],
              m_RotationAngles[Z_AXIS]);

  glUseProgram(m_Program);

  glUniformMatrix4fv(m_Mvp, 1, GL_FALSE, &mvp[0]);

  glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);
}

/* main.cc
 *
 * Copyright (C) 2001-2002 The gtkmm Development Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <gtkmm/application.h>

int main(int argc, char* argv[])
{
  Glib::RefPtr<Gtk::Application> app =
    Gtk::Application::create(argc, argv);

  Example_GLArea window;

  return app->run(window);
}
