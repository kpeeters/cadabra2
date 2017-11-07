#include <stdexcept>
#include <iostream>

#include "TeXEngine.hh"

using namespace cadabra;

int main()
	{
    try {
	    TeXEngine te;
        te.set_geometry(1000);
        te.set_font_size(100);
        te.set_scale(1.0);

	    te.checkin("$e = mc^2$", "", "");
	    te.checkin("$\\int_{-\\infty}^{\\infty} e^{-ax^2} {\\rm d}x = \\sqrt{\\frac{\\pi}{a}}$", "", "");

	    te.convert_all();
        std::cout << "Test passed!! " << std::endl;
        }
    catch(std::exception& ex) {
        std::cout << "Test failed!! " << ex.what() << std::endl;
        return -1;
        }
    return 0;
	}
