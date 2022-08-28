#include <OpenCLMocker/Event.hpp>

#include <OpenCLMocker/Queue.hpp>

namespace OpenCL
{

	Event::~Event()
	{
		queue->UnregisterEvent(this);
	}

}
