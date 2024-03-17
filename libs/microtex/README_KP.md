MicroTeX for Cadabra
--------------------

Build this with

    mkdir build
	cd build
	cmake -DGRAPHICS_DEBUG=OFF -DHAVE_LOG=OFF ..
	make LaTeX
	
This will generate the libLaTeX.a library for Cairo, which is all we need.	


# Internals

The first entry point is `tex::LaTeX::parse`, which creates a
`TeXRenderBuilder` and runs the `build` method on that, giving it its
`Formula` (defined in `core/formula.h`).

Line splitting (or more appropriately: hbox splitting) is done in the
`core.cpp` function `BoxSplitter::split`. Once this gets called on an
hbox, you will see debug output about this. This only seems to get 
called in `render.cpp`.

NEXT: check that we indeed get there, and figure out the difference between
split and non-split cases.
