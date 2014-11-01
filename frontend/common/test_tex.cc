
#include "TeXEngine.hh"

using namespace cadabra;

int main()
	{
	TeXEngine te;

	TeXEngine::TeXRequest *req1=te.checkin("$e = mc^2$", "", "");
	TeXEngine::TeXRequest *req2=te.checkin("$\\int_{-\\infty}^{\\infty} e^{-ax^2} {\\rm d}x = \\sqrt{\\frac{\\pi}{a}}$", "", "");

	te.convert_all();
	}
