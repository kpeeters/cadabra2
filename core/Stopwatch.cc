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

#include "Stopwatch.hh"
#include <assert.h>

#ifdef WIN32
#include "windows.h" // for GetSystemTime among other things

// credit to @Michaelangel007 from https://stackoverflow.com/questions/10905892/equivalent-of-gettimeday-for-windows#26085827 
int gettimeofday(struct Stopwatch_timeval * tp, struct Stopwatch_timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}
#endif // WIN32

Stopwatch::Stopwatch()
	: diffsec(0), diffusec(0), stopped_(true)
	{
   gettimeofday(&tv1,&tz);	// the time since the counter has been running, if stopped_=false
	}

void Stopwatch::reset()
	{
	diffsec=0;
	diffusec=0;
   gettimeofday(&tv1,&tz);	
	}

void Stopwatch::start()
	{
   gettimeofday(&tv1,&tz);	
	stopped_=false;
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

long Stopwatch::seconds() const
	{
	if(stopped_==false) checkpoint_();
	return diffsec;
	}

long Stopwatch::useconds() const
	{
	if(stopped_==false) checkpoint_();
	return diffusec;
	}

std::ostream& operator<<(std::ostream& os, const Stopwatch& mt)
	{
   os << mt.diffsec << " sec and " << mt.diffusec << " microsec";
	return os;
	}

