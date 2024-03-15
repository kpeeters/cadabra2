MicroTeX for Cadabra
--------------------

Build this with

    mkdir build
	cd build
	cmake -DGRAPHICS_DEBUG=OFF -DHAVE_LOG=OFF ..
	make LaTeX
	
This will generate the libLaTeX.a library for Cairo, which is all we need.	
