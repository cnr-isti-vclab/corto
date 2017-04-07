#ifndef NX_TIMER_H
#define NX_TIMER_H

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace nx {

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
	int stop() {
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

#endif // NX_TIMER_H
