#include "Thread.hpp"

void Thread::start()
{
    isRunning = true;
    stopFlag = false;
    thread = std::thread{&Thread::threadFunction, this};
}

void Thread::stop()
{
    stopFlag = true;
    thread.join();
    isRunning = false;
}
