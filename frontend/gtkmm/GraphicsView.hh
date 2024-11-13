
#pragma once

#include <filament/FilamentAPI.h>
#include <filament/Engine.h>
#include <filament/SwapChain.h>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Skybox.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <utils/EntityManager.h>

#include <gtkmm/box.h>
#include <gtkmm/glarea.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/drawingarea.h>

namespace cadabra {

	/// \ingroup frontend
	///
	/// A widget to display OpenGL (or perhaps one day Metal) content.

	class GraphicsView : public Gtk::EventBox {
		public:
			GraphicsView(filament::Engine *, filament::Renderer *);
			virtual ~GraphicsView();

			virtual bool on_motion_notify_event(GdkEventMotion *event) override;
			virtual bool on_button_press_event(GdkEventButton *event) override;
			virtual bool on_button_release_event(GdkEventButton *event) override;

			/// Update the data visualised.
			bool update(long timestamp_millis);
			
			/// Set the content of this 3d view to be the gltf (json).
			void set_gltf(const std::string&);
			
			class GLView :
				public Gtk::DrawingArea  {
//				public Gtk::GLArea  {
				public:
					GLView(filament::Engine *, filament::Renderer *);
					virtual ~GLView();
					
					//virtual bool on_render (const Glib::RefPtr< Gdk::GLContext > &context);
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

					void set_gltf(const std::string&);

					int angle=0;
				private:
					bool need_setup_on_first_render;
					
					void first_render();
					void setup_camera();
					bool load(const std::string& gltf_data);

					std::string gltf_str;
					
					// Filament things. The engine and renderer are owned
					// by the NotebookWindow and passed in on creation of
					// GraphicsView. The swapchain, on the other hand, is
					// per-view, so we are responsible for it.
					filament::Engine       *engine=0;
					filament::Renderer     *renderer=0;
					filament::SwapChain    *swapChain=0;

					// Test stuff for filament.
					filament::VertexBuffer  *vb=0;
					filament::IndexBuffer   *ib=0;
					filament::Material      *mat=0;
					filament::Camera        *cam=0;
					filament::Scene         *scene=0;
					filament::Skybox        *skybox=0;
					filament::View          *view=0;
					utils::Entity            camera;
					utils::Entity            renderable;
					utils::Entity            mainlight, spotlight;
					filament::Texture       *ambient_texture=0;
					filament::IndirectLight *ambient_light=0;

					float zoom;

			};
			
		private:
			Gtk::VBox   vbox;
			GLView      glview;

			bool   sizing;
			double prev_x, prev_y;
			int    height_at_press, width_at_press;

		};

	};
