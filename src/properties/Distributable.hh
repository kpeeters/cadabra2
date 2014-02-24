
#pragma once

class Distributable : virtual public  property {
	public:
		virtual ~Distributable() {};
		virtual std::string name() const;
};

