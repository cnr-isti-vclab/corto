/*
Corto

Copyright(C) 2017 - Federico Ponchio
ISTI - Italian National Research Council - Visual Computing Lab

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  You should have received 
a copy of the GNU General Public License along with Corto.
If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CRT_TIMER_H
#define CRT_TIMER_H

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace crt {

class Timer {
public:

#ifdef _WIN32
	Timer() {
		QueryPerformanceFrequency(&frequency);
		start();
	}

	void start() {
		QueryPerformanceCounter(&t_start);
	}
	void stop() {
		QueryPerformanceCounter(&t_end);
	}
	int64_t elapsed() {
		stop();
		int64_t ms = (t_end.QuadPart - t_start.QuadPart)*1000/frequency.QuadPart;
		start();
		return ms;
	}

private:
	LARGE_INTEGER frequency;
	LARGE_INTEGER t_start;
	LARGE_INTEGER t_end;


#else
	Timer() { start(); }

	void start() {
		gettimeofday(&t_start, NULL);
	}
	void stop() {
		gettimeofday(&t_end, NULL);
	}
	int64_t elapsed() {
		stop();
		int64_t ms = (t_end.tv_sec - t_start.tv_sec)*1000L + (t_end.tv_usec - t_start.tv_usec)/1000L;
		start();
		return ms;
	}
	private:
		timeval t_start;
		timeval t_end;
#endif
};

} //namespace

#endif // CRT_TIMER_H
