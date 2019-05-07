#pragma once

#include <OpenCLMocker/Context.hpp>
#include <OpenCLMocker/Device.hpp>
#include <OpenCLMocker/Event.hpp>
#include <OpenCLMocker/ForbidCopy.hpp>
#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/Retainable.hpp>

#include <memory>
#include <vector>

namespace OpenCL
{
	class Queue : public Retainable
    {
        ForbidCopy(Queue);

    public:
        Context* ctx = nullptr;
        Device* device = nullptr;

        Queue() = default;

        void RegisterEvent(Event* ev) { events.emplace_back(ev); }

        void Wait() const
        {
            for (auto& ev : events)
                ev->Wait();
        }

    private:
        std::vector<std::unique_ptr<Event>> events;
    };
}

MapToCl(OpenCL::Queue, cl_command_queue)
