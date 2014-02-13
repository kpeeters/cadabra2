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

#include "stopwatch.hh"
#include <assert.h>

stopwatch::stopwatch()
	: diffsec(0), diffusec(0), stopped_(true)
	{
   gettimeofday(&tv1,&tz);	// the time since the counter has been running, if stopped_=false
	}

void stopwatch::reset()
	{
	diffsec=0;
	diffusec=0;
   gettimeofday(&tv1,&tz);	
	}

void stopwatch::start()
	{
   gettimeofday(&tv1,&tz);	
	stopped_=false;
	}

void stopwatch::stop()
	{
	stopped_=true;
	checkpoint_();
	}

bool stopwatch::stopped() const
	{
	return stopped_;
	}

void stopwatch::checkpoint_() const
	{
   gettimeofday(&tv2,&tz);
   diffsec  += tv2.tv_sec-tv1.tv_sec;
   diffusec += tv2.tv_usec-tv1.tv_usec;
	tv1=tv2;
   if(diffusec < 0) {
		diffsec--;
		diffusec+=1000000L;
		}
	if(diffusec>1000000L) {
		diffsec+=diffusec/1000000L;
		diffusec%=1000000L;
		}
	}

long stopwatch::seconds() const
	{
	if(stopped_==false) checkpoint_();
	return diffsec;
	}

long stopwatch::useconds() const
	{
	if(stopped_==false) checkpoint_();
	return diffusec;
	}

std::ostream& operator<<(std::ostream& os, const stopwatch& mt)
	{
   os << mt.diffsec << " sec and " << mt.diffusec << " microsec";
	return os;
	}

