#include <OpenCLMocker/Buffer.hpp>

#include <OpenCLMocker/Config.hpp>
#include <OpenCLMocker/Context.hpp>
#include <OpenCLMocker/Exception.hpp>

#include <atomic>
#include <fstream>
#include <string>
#include <cstring>

namespace OpenCL
{

	Buffer::Buffer(MemFlags flags_)
		: flags(flags_)
	{

	}

	Buffer::Buffer(Context* context, MemFlags flags_, size_t size_, void* host_ptr)
		: ctx(context)
		, flags(std::move(flags_))
		, size(size_)
	{
		if (!Context::Validate(ctx))
			throw Exception(CL_INVALID_CONTEXT);

		// MIOpen reilies on missing this check
		//if (size == 0)
		//	throw Exception(CL_INVALID_BUFFER_SIZE);

		if (!flags.Validate())
			throw Exception(CL_INVALID_VALUE, "Invalid buffer creation flags: " + std::to_string(flags.GetValue()) + ".");

		if (flags.GetKernelAccessFlags() == 0)
			flags |= CL_MEM_READ_WRITE;

		const auto hasHostFlags = flags.HasAnyFlags(CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR);

		if (host_ptr == nullptr && hasHostFlags ||
			host_ptr != nullptr && !hasHostFlags)
			throw Exception(CL_INVALID_HOST_PTR);

		if (host_ptr != nullptr)
			hostPtr = reinterpret_cast<char*>(host_ptr);

		if (flags.HasFlags(CL_MEM_USE_HOST_PTR))
		{
			start = hostPtr;
		}
		else
		{
			gpuMemory = std::make_unique<char[]>(size);
			start = gpuMemory.get();

			if (flags.HasFlags(CL_MEM_COPY_HOST_PTR))
				std::memcpy(start, hostPtr, size);
		}

		if (flags.HasAnyFlags(CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR))
			Dump("create");
	}

	void Buffer::Dump(const std::string& operation)
	{
		const auto& root = Config::GetInstance().dumpBuffersRoot;
		const auto& filter = Config::GetInstance().dumpBuffersOpFilter;

		auto opId = std::string{};
		std::getline(std::istringstream{operation}, opId, '-');

		if (!root.has_value() || !filter.empty() && std::find(filter.begin(), filter.end(), opId) == filter.end())
			return;

		static std::atomic<std::size_t> globalIndex = 0;
		const auto curGlobalIndex = globalIndex++;
		const auto index = dumpIndex++;
		const auto thisStr = std::to_string(reinterpret_cast<std::ptrdiff_t>(this));
		const auto filename = globalIndex + "_" + thisStr + "_" + std::to_string(index) + "_" + operation + ".buffer";
		const auto path = *root / filename;

		std::filesystem::create_directories(*root);

		auto file = std::ofstream{path};
		file.write(gpuMemory.get(), size);
	}

}
