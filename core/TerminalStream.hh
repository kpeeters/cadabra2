#pragma once

#include "Kernel.hh"
#include <ostream>

namespace cadabra {

	class TerminalStream {
		public:
			TerminalStream(const Kernel&, std::ostream&);

			TerminalStream& operator<<(const Ex&);
			TerminalStream& operator<<(std::shared_ptr<Ex>);

			template<class T>
			TerminalStream& operator<<(const T& obj)
				{
				out_ << obj;
				return *this;
				}

			TerminalStream& operator <<(std::ostream& (*os)(std::ostream&))
				{
				out_ << os;
				return *this;
				}
		private:
			const Kernel& kernel;
			std::ostream& out_;
		};


	}
