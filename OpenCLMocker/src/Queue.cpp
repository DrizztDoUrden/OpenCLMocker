#include <OpenCLMocker/Queue.hpp>

#include <OpenCLMocker/Event.hpp>
#include <OpenCLMocker/Kernel.hpp>

namespace OpenCL
{

	void Queue::EnqueueNDRangeKernel(const Kernel& kernel, const std::vector<size_t>& global_work_offset, const std::vector<size_t>& global_work_size, const std::vector<size_t>& local_work_size, const std::vector<cl_event>& event_wait_list, cl_event* ev)
	{
		auto mockEvent = std::make_unique<Event>(
			event_wait_list,
			std::chrono::nanoseconds(3000 + rand() % 3000));

		RegisterEvent(mockEvent.get());

		if (ev != nullptr)
			*ev = MakeHandle(std::move(mockEvent));
	}

}