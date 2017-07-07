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

#ifndef WIN32
	extern "C" {
		#include <sys/time.h>
		#include <unistd.h>
	}
#else
	//#include <winsock2.h>
	// MSVC defines this in winsock2.h but it's a heavyweight include
	typedef struct Stopwatch_timeval {
		long tv_sec;
		long tv_usec;
	} Stopwatch_timeval;
	typedef struct Stopwatch_timezone {
		// don't actually use this I guess
		long dummy;
	} Stopwatch_timezone;
#endif !WIN32

extern "C" {
#include <signal.h>
}

#include <iostream>

class Stopwatch {
	public:
		Stopwatch();

		void reset();
		void start();
		void stop();
		long seconds() const;
		long useconds() const;
		bool stopped() const;

		friend std::ostream& operator<<(std::ostream&, const Stopwatch&);

	private:
		void checkpoint_() const;
#ifndef WIN32
		mutable struct timeval  tv1, tv2;
		mutable struct timezone tz;
#else
		mutable struct Stopwatch_timeval  tv1, tv2;
		mutable struct Stopwatch_timezone tz;
#endif // !WIN32
		mutable long diffsec, diffusec;
		bool stopped_;
};

std::ostream& operator<<(std::ostream&, const Stopwatch&);

#endif
