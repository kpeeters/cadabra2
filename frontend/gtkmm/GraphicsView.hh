
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/glarea.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/drawingarea.h>

#include <filament/Engine.h>
#include <filament/SwapChain.h>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <utils/EntityManager.h>

namespace cadabra {

	/// \ingroup frontend
	///
	/// A widget to display OpenGL (or perhaps one day Metal) content.

	class GraphicsView : public Gtk::EventBox {
		public:
			GraphicsView(filament::Engine *);
			virtual ~GraphicsView();

			virtual bool on_motion_notify_event(GdkEventMotion *event) override;
			virtual bool on_button_press_event(GdkEventButton *event) override;
			virtual bool on_button_release_event(GdkEventButton *event) override;

			class GLView : public Gtk::DrawingArea /* public Gtk::GLArea */ {
				public:
					GLView(filament::Engine *);
					// virtual bool on_render (const Glib::RefPtr< Gdk::GLContext > &context);
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

				private:
					void first_render();

					// Filament things. The engine is owned by the NotebookWindow and passed
					// in on creation of GraphicsView. The swapchain, on the other hand, is
					// per-view, so we are responsible for it.
					filament::Engine        *engine=0;
					filament::SwapChain     *swapChain=0;

					// Test stuff for filament.
					filament::VertexBuffer* vb;
					filament::IndexBuffer*  ib;
					filament::Material*     mat;
					filament::Camera*       cam;
					filament::Scene*        scene;
					filament::View*         view;
					utils::Entity           camera;
					utils::Entity           renderable;
					filament::Renderer     *renderer;
			};
			
		private:
			Gtk::VBox   vbox;
			GLView      glview;

			bool   sizing;
			double prev_x, prev_y;
			int    height_at_press, width_at_press;

		};

	};
