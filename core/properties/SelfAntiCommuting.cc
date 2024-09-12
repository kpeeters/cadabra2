
#include "properties/SelfAntiCommuting.hh"

using namespace cadabra;

SelfAntiCommuting::~SelfAntiCommuting()
	{
	}

std::string SelfAntiCommuting::name() const
	{
	return "SelfAntiCommuting";
	}

int SelfAntiCommuting::sign() const
	{
	return -1;
	}
