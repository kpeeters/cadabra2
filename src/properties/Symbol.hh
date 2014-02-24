
#pragma once

class Symbol : public property {
	public:
		virtual std::string name() const;

		static const Symbol *get(exptree::iterator, bool ignore_parent_rel=false);
};
