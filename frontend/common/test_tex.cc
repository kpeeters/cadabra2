
#include "TeXEngine.hh"

using namespace cadabra;

int main()
	{
	TeXEngine te;

	te.checkin("$e = mc^2$", "", "");
	te.checkin("$\\int_{-\\infty}^{\\infty} e^{-ax^2} {\\rm d}x = \\sqrt{\\frac{\\pi}{a}}$", "", "");

	te.convert_all();
	}
