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

#include <chrono>
#include <iosfwd>
#include <iostream>

/// \class Stopwatch
/// \ingroup Client-Server
///
/// The Stopwach class provides a simple interace to allow
/// timing function calls etc... It is possible to stop
/// the stopwatch for an indefinite amount of time without
/// losing the current elapsed time period, allowing it
/// to be used to calculate the actual amount of time 
/// a function spends performing a task without taking 
/// into account e.g. time spent being blocked by a mutex.
/// The class is exported to python in the cadabra2 
/// module as Stopwatch.
///
///
/// Example C++ usage:
///
/// #include <iostream>
/// #include <vector>
/// #include "Stopwatch.hh"
///
/// void do_other_stuff() { std::cout << "Doing other stuff\n"; }
///
/// int main()
/// {
///   const int n = 1000;
///   std::vector<int> v;
///   Stopwatch s;
///
///   s.start();
///   for (unsigned int i = 0; i < n; ++i)
///     v.push_back(i);
///   s.stop();
///   do_other_stuff();
///   s.start();
///   for (unsigned int i = n; i > 0; --i)
///     v.push_back(i);
///   s.stop();
///   std::cout << "Total time spent pushing to vector and not spent doing other stuff: " << s << '\n';
///
///   s.reset();
///   s.start();
///   do_other_stuff();
///   s.stop();
///   std::cout << "Doing other stuff takes " << s.useconds() << "us\n"; //Assuming do_other_stuff() takes < 1s
///   return 0;
/// }
///
///
/// Example python usage:
///
/// from cadabra2 import Stopwatch
///
/// def do_other_stuff():
///   print("Doing other stuff")
///
/// n = 1000
/// v = []
/// s = Stopwatch()
///
/// s.start()
/// for i in range(n):
///   v += [i]
/// s.stop()
/// do_other_stuff()
/// s.start()
/// for i in range(n, 0, -1):
///   v += [i]
/// s.stop()
/// print("Total time spent pushing to vector and not spent doing other stuff: {}".format(s))
///
/// s.reset()
/// s.start()
/// do_other_stuff()
/// s.stop()
/// print("Doing other stuff takes {}us".format(s.useconds())) #Assuming do_other_stuff() takes < 1s


class Stopwatch {
	public:
		Stopwatch();

      typedef std::chrono::steady_clock clock;

      /// Reset to no-time-elapsed.
		void reset();
      /// Continue timing (does not reset).
		void start();
      /// Stop timing.
		void stop();
      /// Number of seconds elapsed.
		long seconds() const;
      /// Number of micro-seconds elapsed (needs to be added to 'seconds').
		long useconds() const;
      /// Is the stopwatch currently timing?
		bool stopped() const;

		friend std::ostream& operator<<(std::ostream&, const Stopwatch&);

	private:
		void checkpoint_() const;

      mutable clock::time_point start_;
      mutable long              elapsed_;
		bool                      stopped_;

      static const long s_to_us = 1000000L;
};

/// Print human-readable representation of the time elapsed.
std::ostream& operator<<(std::ostream&, const Stopwatch&);

#endif
