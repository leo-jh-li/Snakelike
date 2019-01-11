#include <windows.h>
#include <chrono>
#include <ctime>

int clamp(int n, int lower, int upper) {
	return max(lower, min(n, upper));
}

int GetDifferenceInSeconds(std::chrono::system_clock::time_point startTime, std::chrono::system_clock::time_point endTime) {
	std::chrono::duration<double> gameSeconds = endTime - startTime;
	return (int)gameSeconds.count();
}