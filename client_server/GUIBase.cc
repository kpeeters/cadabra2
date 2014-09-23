
#include "GUIBase.hh"

using namespace cadabra;

GUIBase::GUIAction::GUIAction(Type ac, Client::iterator it)
	: action(ac), cell(it)
	{
	}
