#pragma once

#include <OpenCLMocker/Object.hpp>

#include <OpenCLMocker/MapToCl.hpp>
#include <OpenCLMocker/MemFlags.hpp>
#include <OpenCLMocker/Retainable.hpp>

#include <CL/cl.h>

#include <atomic>
#include <memory>

namespace OpenCL
{
	struct Context;

	class Buffer : public Object, public Retainable, private BufferValidation
	{
	public:
		std::unique_ptr<char[]> gpuMemory = nullptr;
		std::unique_ptr<char[]> hostMemory = nullptr;

		OpenCL::Context* ctx = nullptr;
		char* start = nullptr;
		char* hostPtr = nullptr;
		std::size_t size = 0;

		Buffer(Context* context, MemFlags flags, size_t size, void* host_ptr);

		const MemFlags& GetMemFlags() const { return flags; }

		static bool Validate(const Buffer* buffer) { return buffer != nullptr && buffer->Object::Validate() && buffer->BufferValidation::Validate(); }

		void Dump(const std::string& operation);

	private:
		MemFlags flags;
		std::atomic<std::size_t> dumpIndex = 0;
	};
}

MapToCl(OpenCL::Buffer, cl_mem)
