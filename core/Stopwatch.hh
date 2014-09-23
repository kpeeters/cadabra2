/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2011  Kasper Peeters <kasper.peeters@aei.mpg.de>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#ifndef stopwatch_hh__
#define stopwatch_hh__

extern "C" {
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
}

#include <iostream>

class stopwatch {
	public:
		stopwatch();

		void reset();
		void start();
		void stop();
		long seconds() const;
		long useconds() const;
		bool stopped() const;

		friend std::ostream& operator<<(std::ostream&, const stopwatch&);
	private:
		void checkpoint_() const;
		mutable struct timeval  tv1,tv2; 
		mutable struct timezone tz;
		mutable long diffsec, diffusec;
		bool stopped_;
};

std::ostream& operator<<(std::ostream&, const stopwatch&);

#endif
