
#pragma once

class order : virtual public algorithm, virtual public locate {
	public:
		order(Kernel&, exptree&, bool anticomm);

		virtual bool     can_apply(iterator);

	protected:
		result_t doit(iterator&, bool sign);
};

