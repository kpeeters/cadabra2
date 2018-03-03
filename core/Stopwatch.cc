/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2018  Kasper Peeters <kasper.peeters@aei.mpg.de>

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

#include <ratio>
#include "Stopwatch.hh"

Stopwatch::Stopwatch()
   : elapsed_(0), stopped_(true)
	{
	}

void Stopwatch::reset()
	{
	elapsed_=0;
	start_=clock::now();
	}

void Stopwatch::start()
	{
	stopped_=false;
	start_=clock::now();
	}

void Stopwatch::stop()
	{
	stopped_=true;
	checkpoint_();
	}

bool Stopwatch::stopped() const
	{
	return stopped_;
	}

void Stopwatch::checkpoint_() const
	{
	using namespace std::chrono;
	clock::time_point stop_ = clock::now();
	elapsed_ += duration_cast<duration<long, std::micro>>(stop_ - start_).count();
	start_ = stop_;
	}

long Stopwatch::seconds() const
	{
	if(!stopped_) checkpoint_();
	return elapsed_ / s_to_us;
	}

long Stopwatch::useconds() const
	{
	if(!stopped_) checkpoint_();
	return elapsed_ % s_to_us;
	}

std::ostream& operator<<(std::ostream& lhs, const Stopwatch& rhs)
	{
	return lhs << rhs.seconds() << "s and " << rhs.useconds() << "us";
	}

