
#pragma once

class order : virtual public algorithm, virtual public locate {
	public:
		order(Kernel&, exptree&, bool anticommuting);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	protected:
		result_t doit(iterator&, bool sign);

		bool anticomm;
};

