#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>
#include <OpenCLMocker/TypeValidation.hpp>

#include <CL/cl.h>

#include <chrono>
#include <vector>
#include <thread>

namespace OpenCL
{
	class Context;
	class Queue;

	class Event : public Object, public Retainable, private EventValidation
	{
	public:
		using Clock = std::chrono::high_resolution_clock;
		using TimePoint = Clock::time_point;

		Context* ctx;
		Queue* queue;

		Event(const TimePoint& start, const TimePoint& end)
			: queued(Clock::now())
			, start(start)
			, end(end)
		{
		}

		~Event();

		template <class TDuration>
		Event(const std::vector<cl_event>& events, const TDuration& duration)
			: queued(Clock::now())
			, start(ProduceStart(events))
			, end(start + duration)
		{
		}

		DefaultMove(Event);

		auto GetDuration() const { return end - start; }
		bool IsFinished() const { return Clock::now() > end; }
		const TimePoint& GetQueued() const { return queued; }
		const TimePoint& GetSubmitted() const { return start; }
		const TimePoint& GetStart() const { return start; }
		const TimePoint& GetEnd() const { return end; }
		const TimePoint& GetComplete() const { return end; }

		void Wait() const
		{
			const auto now = Clock::now();
			if (Validate(this) && end > now)
				std::this_thread::sleep_for(end - now);
		}

		static bool Validate(const Event* event) { return event != nullptr && event->Object::Validate() && event->EventValidation::Validate(); }

	private:
		TimePoint queued;
		TimePoint start;
		TimePoint end;

		static TimePoint ProduceStart(const std::vector<cl_event>& events)
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
