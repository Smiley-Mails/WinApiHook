#pragma once

#ifndef EXECUTION_TIMER_H
#define EXECUTION_TIMER_H

#include <windows.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <type_traits>

template<class Resolution = std::chrono::milliseconds>

class ExecutionTimer {
public:
	using Clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
		std::chrono::high_resolution_clock,
		std::chrono::steady_clock>;
private:
	const Clock::time_point mStart = Clock::now();

public:
	ExecutionTimer() = default;
	~ExecutionTimer() {
		//const auto end = Clock::now();
		//std::ostringstream strStream;
		//strStream << "Destructor Elapsed: "
		//	<< std::chrono::duration_cast<Resolution>(end - mStart).count()
		//	<< std::endl;
		//std::cout << strStream.str() << std::endl;
	}

	inline void stop() {
		const auto end = Clock::now();
		std::ostringstream strStream;
		strStream << "Stop Elapsed: "
			<< std::chrono::duration_cast<Resolution>(end - mStart).count()
			<< std::endl;
		std::cout << strStream.str() << std::endl;
	}
	
	inline void ElapsedTime() {
		SYSTEMTIME lt;
		GetLocalTime(&lt);
		const auto end = Clock::now();
		TCHAR szTimeBuff[MAX_PATH];
		wsprintf(szTimeBuff, L"%02d:%02d:%02d Exec Timer: \"Stop Elapsed %d µs\" \r\n", \
			lt.wHour, lt.wMinute, lt.wSecond, \
			std::chrono::duration_cast<Resolution>(end - mStart).count());
		OutputDebugString(szTimeBuff);
	}
}; // ExecutionTimer

#endif // EXECUTION_TIMER_H