#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/Context.hpp>
#include <OpenCLMocker/Device.hpp>
#include <OpenCLMocker/Event.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>
#include <OpenCLMocker/TypeValidation.hpp>

#include <memory>
#include <vector>

namespace OpenCL
{
	class Queue : public Object, public Retainable, private QueueValidation
	{
	public:
		Context* ctx = nullptr;
		Device* device = nullptr;
		bool outOfOrderExecutionMode = false;
		bool profilingEnabled = false;

		Queue() = default;

		void RegisterEvent(Event* ev)
		{
			events.emplace_back(ev);
			ev->queue = this;
			ev->ctx = ctx;
		}

		void UnregisterEvent(Event* ev) { events.erase(std::find(events.begin(), events.end(), ev)); }

		void Wait() const
		{
			for (auto& ev : events)
				ev->Wait();
		}

		static bool Validate(const Queue* queue) { return queue != nullptr && queue->Object::Validate() && queue->QueueValidation::Validate(); }

	private:
		std::vector<Event*> events;
	};
}

MapToCl(OpenCL::Queue, cl_command_queue)
