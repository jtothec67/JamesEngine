#pragma once

#include <chrono>
#include <iostream>

class Timer
{
public:
    Timer() { }
    ~Timer() { }

    void Start(); // Set start time
    float Stop(); // Get time passed and stop the timer
    float GetElapsedSeconds(); // Get time passed without stopping the timer
    float GetElapsedMilliseconds(); // Get time passed in milliseconds without stopping the timer

    bool IsRunning() { return mRunning; }

private:
    std::chrono::steady_clock::time_point mStart;
    std::chrono::steady_clock::time_point mEnd;
    std::chrono::steady_clock::time_point mTempTimeStamp;

    bool mRunning = false;
};

class ScopedTimer
{
public:
	ScopedTimer(const char* _name) : mName(_name)
	{
		mTimer.Start();
	}
	~ScopedTimer()
	{
		float elapsed = mTimer.GetElapsedMilliseconds();
		std::cout << "ScopedTimer [" << mName << "] took " << elapsed << " milliseconds.\n";
	}
private:
	const char* mName;
	Timer mTimer;
};