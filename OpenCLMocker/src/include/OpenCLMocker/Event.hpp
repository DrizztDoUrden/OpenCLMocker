#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/MapToCl.hpp>

#include <CL/cl.h>

#include <chrono>
#include <vector>
#include <thread>

namespace OpenCL
{
    class Event
    {
        ForbidCopy(Event);

    public:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = Clock::time_point;

        Event(const TimePoint& start, const TimePoint& end)
            : start(start)
            , end(end)
        {
        }

        template <class TDuration>
        Event(const std::vector<cl_event>& events, const TDuration& duration)
            : start(ProduceStart(events))
            , end(start + duration)
        {
        }

        auto GetDuration() const { return end - start; }
        bool IsFinished() const { return Clock::now() > end; }
        void Wait() const { std::this_thread::sleep_for(end - Clock::now()); }
        const TimePoint& GetStart() const { return start; }
        const TimePoint& GetEnd() const { return end; }

    private:
        TimePoint start;
        TimePoint end;

        static TimePoint ProduceStart(const std::vector<cl_event> & events)
        {
            auto start = Event::Clock::now();

            for (const auto rawOther : events)
            {
                const auto& other = *reinterpret_cast<Event*>(rawOther);
                if (start > other.GetEnd())
                    start = other.GetEnd();
            }

            return start;
        }
    };
}

MapToCl(OpenCL::Event, cl_event)
