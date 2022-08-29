#include <OpenCLMocker/Buffer.hpp>
#include <OpenCLMocker/BufferType.hpp>
#include <OpenCLMocker/Context.hpp>
#include <OpenCLMocker/Device.hpp>
#include <OpenCLMocker/Enums.hpp>
#include <OpenCLMocker/Event.hpp>
#include <OpenCLMocker/Exception.hpp>
#include <OpenCLMocker/Kernel.hpp>
#include <OpenCLMocker/MemFlags.hpp>
#include <OpenCLMocker/Platform.hpp>
#include <OpenCLMocker/Program.hpp>
#include <OpenCLMocker/Queue.hpp>

#include <CL/cl.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <concepts>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>
#include <thread>

using namespace OpenCL;

namespace OpenCL
{
#ifdef OPENCL_EXTENSIVE_LOGGING
	constexpr bool ExtensiveLogging = true;
#else
	constexpr bool ExtensiveLogging = false;
#endif
}

namespace
{
	template <class TElement>
	bool FillArrayProperty(const TElement* arr, std::size_t size, size_t param_value_size, void* param_value, size_t* param_value_size_ret, const char* description = "")
	{
		const auto memory = size * sizeof(TElement);
		const auto fullDescription = strlen(description) > 0
			? std::string{" ("} + description + ")"
			: description;

		if (param_value != nullptr && param_value_size != 0)
		{
			if (ExtensiveLogging)
				std::cerr << "CL Mocker" << fullDescription << ": Writing " << memory << " bytes to 0x" << std::ios::hex << reinterpret_cast<std::ptrdiff_t>(param_value) << std::ios::dec << "." << std::endl;

			if (param_value_size < memory)
			{
				std::cerr << "CL Mocker" << fullDescription << ": Not enough memory to store value: " << param_value_size << " < " << memory << "." << std::endl;
				return false;
			}
			std::memcpy(param_value, arr, memory);
		}

		if (param_value_size_ret != nullptr)
			*param_value_size_ret = memory;

		return true;
	}

	template <class TValue>
	bool FillProperty(TValue value, size_t param_value_size, void* param_value, size_t* param_value_size_ret, const char* description = "")
	{
		const auto fullDescription = strlen(description) > 0
			? std::string{" ("} + description + ")"
			: description;

		if (param_value != nullptr && param_value_size != 0)
		{
			if (ExtensiveLogging)
				std::cerr << "CL Mocker" << fullDescription << ": Writing " << sizeof(TValue) << " bytes to 0x" << std::ios::hex << reinterpret_cast<std::ptrdiff_t>(param_value) << std::ios::dec << ": " << value << std::endl;

			if (param_value_size < sizeof(TValue))
			{
				std::cerr << "CL Mocker" << fullDescription << ": Not enough memory to store value: " << param_value_size << " < " << sizeof(TValue) << "." << std::endl;
				return false;
			}

			*reinterpret_cast<TValue*>(param_value) = value;
		}

		if (param_value_size_ret != nullptr)
			*param_value_size_ret = sizeof(TValue);

		return true;
	}

	bool FillStringProperty(const std::string& value, size_t param_value_size, void* param_value, size_t* param_value_size_ret, const char* description = "")
	{
		const auto fullDescription = strlen(description) > 0
			? std::string{" ("} + description + ")"
			: description;

		const auto memory = (value.length() + 1) * sizeof(char);

		if (param_value != nullptr && param_value_size != 0)
		{
			if (ExtensiveLogging)
				std::cerr << "CL Mocker" << fullDescription << ": Writing " << param_value_size << " bytes to 0x" << std::ios::hex << reinterpret_cast<std::ptrdiff_t>(param_value) << std::ios::dec << ": " << value << std::endl;

			if (param_value_size < memory)
			{
				std::cerr << "CL Mocker" << fullDescription << ": Not enough memory to store value: " << param_value_size << " < " << memory << ": " << value << std::endl;
				return false;
			}

			std::strcpy(reinterpret_cast<char*>(param_value), value.c_str());
		}

		if (param_value_size_ret != nullptr)
			*param_value_size_ret = memory;

		return true;
	}

	template<class TProperty>
	void IterateOverProperties(const TProperty* properties, const std::function<void(cl_int name, const TProperty& value)>& handler)
	{
		auto curPtr = properties;

		while (curPtr != nullptr && *curPtr != 0)
		{
			const auto& name = reinterpret_cast<const cl_int&>(*curPtr);

			if (++curPtr == nullptr)
				throw Exception{CL_INVALID_PROPERTY};

			handler(name, *curPtr);
			++curPtr;
		}
	}

	Platform* GetPlatformByDeviceType(cl_device_type device_type)
	{
		for (auto& platform : Platform::Get())
			for (auto& device : platform->devices)
				if (device.type == device_type)
					return platform;

		return nullptr;
	}
}

template <class TRet>
TRet Try(cl_int* errcode_ret, const Context* context, const TRet& defaultRet, const std::function<TRet()>& worker)
{
#if OPENCL_CATCH_EXCEPTIONS
	const auto callback = (context != nullptr && context->errorCallback)
		? context->errorCallback
		: [](const std::string& errorInfo, const std::vector<uint8_t>& /*miscData*/)
	{
		std::cerr << errorInfo << std::endl;
	};

	try
	{
#endif
		if (context != nullptr && !Context::Validate(context))
			throw Exception{CL_INVALID_CONTEXT};

		auto ret = worker();
		if (errcode_ret != nullptr)
			*errcode_ret = CL_SUCCESS;
		return ret;
#if OPENCL_CATCH_EXCEPTIONS
	}
	catch (const Exception& ex)
	{
		callback("Exception: " + ex.GetDescription() + "(status: " + std::to_string(ex.GetStatus()) + ")", ex.GetMiscData());
		if (errcode_ret != nullptr)
			*errcode_ret = ex.GetStatus();
		return defaultRet;
	}
	catch (const std::bad_alloc& ex)
	{
		callback(std::string{"Allocation failure: "} + ex.what(), {});
		if (errcode_ret != nullptr)
			*errcode_ret = CL_OUT_OF_HOST_MEMORY;
		return defaultRet;
	}
	catch (const std::exception& ex)
	{
		callback(std::string{"Std exception: "} + ex.what(), {});
		if (errcode_ret != nullptr)
			*errcode_ret = -1;
		return defaultRet;
	}
	catch (...)
	{
		callback("Unknown error.", {});
		if (errcode_ret != nullptr)
			*errcode_ret = -1;
		return defaultRet;
	}
#endif
}

template <class TRet, class TCtxSource>
	requires (!std::same_as<TCtxSource, std::nullptr_t> && !std::is_pointer_v<TCtxSource>)
TRet Try(cl_int* errcode_ret, const TCtxSource& ctxSource, const TRet& defaultRet, const std::function<TRet()>& worker)
{
	if (!TCtxSource::Validate(&ctxSource) || !Context::Validate(ctxSource.ctx))
	{
		if (errcode_ret != nullptr)
			*errcode_ret = CL_INVALID_CONTEXT;
		return defaultRet;
	}

	return Try(errcode_ret, ctxSource.ctx, defaultRet, worker);
}

class Void
{
};

cl_int Try(const Context* context, const std::function<void()>& worker)
{
	cl_int ret = CL_SUCCESS;
	Try<Void>(&ret, context, {}, [&worker]() { worker(); return Void{}; });
	return ret;
}

template <class TCtxSource>
cl_int Try(const TCtxSource& ctxSource, const std::function<void()>& worker)
{
	cl_int ret = CL_SUCCESS;
	Try<Void>(&ret, ctxSource, {}, [&worker]() { worker(); return Void{}; });
	return ret;
}

cl_int CL_API_CALL clGetPlatformIDs(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms) CL_API_SUFFIX__VERSION_1_0
{
	return Try(nullptr, [&]()
		{
			auto& existingPlatforms = Platform::Get();

			if (num_platforms != nullptr)
				*num_platforms = existingPlatforms.size();

			if (platforms != nullptr && num_entries > 0)
				for (auto i = 0; i < std::min(num_entries, static_cast<cl_uint>(existingPlatforms.size())); ++i)
					platforms[i] = MapType(existingPlatforms[i]);
		});
}

cl_int CL_API_CALL clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try(nullptr, [&]()
		{
			const auto& mockPlatform = MapType(platform);

			if (!Platform::Validate(&mockPlatform))
				throw Exception{CL_INVALID_PLATFORM};

			switch (param_name)
			{
			case CL_PLATFORM_NAME:
				if (!FillStringProperty(mockPlatform.name, param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PLATFORM_VENDOR:
				if (!FillStringProperty(mockPlatform.vendor, param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PLATFORM_PROFILE:
				if (!FillStringProperty(mockPlatform.profile, param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PLATFORM_VERSION:
				if (!FillStringProperty(mockPlatform.version, param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			default:
				std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
				throw Exception{CL_INVALID_VALUE};
			}
		});
}

cl_int CL_API_CALL clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices) CL_API_SUFFIX__VERSION_1_0
{
	return Try(nullptr, [&]()
		{
			auto& mockPlatform = MapType(platform);

			if (!Platform::Validate(&mockPlatform))
				throw Exception{CL_INVALID_PLATFORM};

			if (num_devices != nullptr)
				*num_devices = mockPlatform.devices.size();

			if (devices != nullptr && num_entries > 0)
				for (auto i = 0; i < std::min<cl_uint>(num_entries, mockPlatform.devices.size()); i++)
					devices[i] = MapType(mockPlatform.devices[i]);
		});
}

void IterateOverContextProperties(Context& ctx, const cl_context_properties* properties)
{
	const auto propertyHandler = [&ctx](cl_int name, const cl_context_properties& value)
	{
		switch (name)
		{
		case CL_CONTEXT_PLATFORM:
		{
			auto& mockPlatform = MapType(reinterpret_cast<cl_platform_id>(value));
			if (!Platform::Validate(&mockPlatform))
				throw Exception{CL_INVALID_PLATFORM};
			ctx.platform = &mockPlatform;
			return;
		}
		case CL_CONTEXT_INTEROP_USER_SYNC:
		{
			ctx.interopUserSync = *reinterpret_cast<const cl_bool*>(value);
			return;
		}
		default:
			throw Exception{CL_INVALID_PROPERTY};
		}
	};

	IterateOverProperties<cl_context_properties>(properties, propertyHandler);
}

cl_context CL_API_CALL clCreateContext(const cl_context_properties* properties, cl_uint num_devices, const cl_device_id* devices, void (CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*), void* user_data, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try<cl_context>(errcode_ret, nullptr, nullptr, [&]()
		{
			if (num_devices <= 0 || devices == nullptr || pfn_notify == nullptr && user_data != nullptr)
				throw Exception{CL_INVALID_VALUE};

			auto ctx = Context{};

			IterateOverContextProperties(ctx, properties);

			for (auto i = 0; i < num_devices; ++i)
			{
				auto& device = MapType(devices[i]);

				if (!Device::Validate(&device))
					throw Exception{CL_INVALID_DEVICE};

				ctx.devices.push_back(&device);
			}

			if (pfn_notify != nullptr)
				ctx.errorCallback = [=](const std::string& errorInfo, const std::vector<uint8_t>& privateInfo)
			{
				pfn_notify(errorInfo.c_str(), privateInfo.data(), privateInfo.size(), user_data);
			};

			if (ctx.platform == nullptr)
				ctx.platform = ctx.devices.front()->GetPlatform();

			return MapType(new Context{std::move(ctx)});
		});
}

cl_context CL_API_CALL clCreateContextFromType(const cl_context_properties* properties, cl_device_type device_type, void (CL_CALLBACK* pfn_notify)(const char*, const void*, size_t, void*), void* user_data, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try<cl_context>(errcode_ret, nullptr, nullptr, [&]()
		{
			if (pfn_notify == nullptr && user_data != nullptr)
				throw Exception{CL_INVALID_VALUE};

			auto ctx = Context{};

			IterateOverContextProperties(ctx, properties);

			if (pfn_notify != nullptr)
				ctx.errorCallback = [=](const std::string& errorInfo, const std::vector<uint8_t>& privateInfo)
			{
				pfn_notify(errorInfo.c_str(), privateInfo.data(), privateInfo.size(), user_data);
			};

			if (ctx.platform == nullptr)
				ctx.platform = GetPlatformByDeviceType(device_type);

			for (auto& device : ctx.platform->devices)
				ctx.devices.push_back(&device);

			return MapType(new Context{std::move(ctx)});
		});
}

cl_int CL_API_CALL clRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
	return Try(&MapType(context), [&]()
		{
			auto& ctx = MapType(context);

			if (!Context::Validate(&ctx))
				throw Exception{CL_INVALID_CONTEXT};

			ctx.Retain();
		});
}

cl_int CL_API_CALL clGetContextInfo(cl_context context, cl_context_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try(&MapType(context), [&]()
		{
			const auto& ctx = MapType(context);

			if (!Context::Validate(&ctx))
				throw Exception{CL_INVALID_CONTEXT};

			switch (param_name)
			{
			case CL_CONTEXT_REFERENCE_COUNT:
				if (!FillProperty(static_cast<cl_uint>(ctx.GetReferenceCount()), param_value_size, param_value, param_value_size_ret, "clGetContextInfo(CL_CONTEXT_REFERENCE_COUNT)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_CONTEXT_PLATFORM:
				if (!FillProperty(MapType(ctx.platform), param_value_size, param_value, param_value_size_ret, "clGetContextInfo(CL_CONTEXT_PLATFORM)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_CONTEXT_NUM_DEVICES:
				if (!FillProperty(static_cast<cl_uint>(ctx.devices.size()), param_value_size, param_value, param_value_size_ret, "clGetContextInfo(CL_CONTEXT_NUM_DEVICES)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_CONTEXT_DEVICES:
				if (!FillArrayProperty(ctx.devices.data(), ctx.devices.size(), param_value_size, param_value, param_value_size_ret, "clGetContextInfo(CL_CONTEXT_DEVICES)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			default:
				std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
				throw Exception{CL_INVALID_VALUE};
			}
		});
}

cl_int CL_API_CALL clGetDeviceInfo(cl_device_id device, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try(nullptr, [&]()
		{
			const auto& dev = MapType(device);

			if (!Device::Validate(&dev))
				throw Exception{CL_INVALID_DEVICE};

			switch (param_name)
			{
			case CL_DEVICE_NAME:
				if (!FillStringProperty(dev.name, param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_VERSION:
				if (!FillStringProperty(dev.version, param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DRIVER_VERSION:
				if (!FillStringProperty(dev.driver, param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
				if (!FillProperty(std::numeric_limits<cl_ulong>::max(), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_MAX_MEM_ALLOC_SIZE)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_GLOBAL_MEM_SIZE:
				if (!FillProperty(std::numeric_limits<cl_ulong>::max(), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_GLOBAL_MEM_SIZE)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_LOCAL_MEM_SIZE:
				if (!FillProperty(std::numeric_limits<cl_ulong>::max(), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_LOCAL_MEM_SIZE)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_MAX_WORK_GROUP_SIZE:
				if (!FillProperty(std::numeric_limits<size_t>::max(), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_MAX_CLOCK_FREQUENCY:
				if (!FillProperty(std::numeric_limits<cl_uint>::max(), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_MAX_CLOCK_FREQUENCY)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_MAX_COMPUTE_UNITS:
				if (!FillProperty(static_cast<cl_uint>(64), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_MAX_COMPUTE_UNITS)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_VENDOR_ID:
				if (!FillProperty(static_cast<cl_uint>(0x1002), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_VENDOR_ID)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_AVAILABLE:
				if (!FillProperty(static_cast<cl_bool>(true), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_AVAILABLE)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_DEVICE_PLATFORM:
				if (!FillProperty(MapType(dev.GetPlatform()), param_value_size, param_value, param_value_size_ret, "clGetDeviceInfo(CL_DEVICE_PLATFORM)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			default:
				std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
				throw Exception{CL_INVALID_VALUE};
				return;
			}
		});
}

void IterateOverQueueProperties(Queue& queue, const cl_queue_properties* properties)
{
	const auto propertyHandler = [&queue](cl_int name, const cl_queue_properties& value)
	{
		switch (name)
		{
		case CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE:
			queue.outOfOrderExecutionMode = *reinterpret_cast<const cl_bool*>(value);
			return;
		case CL_QUEUE_PROFILING_ENABLE:
			queue.profilingEnabled = *reinterpret_cast<const cl_bool*>(value);
			return;
		default:
			throw Exception{CL_INVALID_VALUE};
			return;
		}
	};

	IterateOverProperties<cl_queue_properties>(properties, propertyHandler);
}

cl_command_queue CL_API_CALL clCreateCommandQueue(cl_context context, cl_device_id device, cl_command_queue_properties /* properties */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_2_0
{
	return Try<cl_command_queue>(errcode_ret, &MapType(context), nullptr, [&]()
		{
			auto queue = Queue{};

			queue.ctx = &MapType(context);
			queue.device = &MapType(device);

			if (!Context::Validate(queue.ctx))
				throw Exception{CL_INVALID_CONTEXT};
			if (!Device::Validate(queue.device))
				throw Exception{CL_INVALID_DEVICE};

			if (errcode_ret != nullptr)
				*errcode_ret = CL_SUCCESS;

			return MapType(new Queue{std::move(queue)});
		});
}

cl_command_queue CL_API_CALL clCreateCommandQueueWithProperties(cl_context context, cl_device_id device, const cl_queue_properties* properties, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_2_0
{
	return Try<cl_command_queue>(errcode_ret, &MapType(context), nullptr, [&]()
		{
			auto queue = Queue{};

			queue.ctx = &MapType(context);
			queue.device = &MapType(device);

			if (!Context::Validate(queue.ctx))
				throw Exception{CL_INVALID_CONTEXT};
			if (!Device::Validate(queue.device))
				throw Exception{CL_INVALID_DEVICE};

			IterateOverQueueProperties(queue, properties);

			if (errcode_ret != nullptr)
				*errcode_ret = CL_SUCCESS;

			return MapType(new Queue{std::move(queue)});
		});
}

cl_int CL_API_CALL clRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(command_queue), [&]()
		{
			auto& queue = MapType(command_queue);

			if (!Queue::Validate(&queue))
				throw Exception{CL_INVALID_COMMAND_QUEUE};

			queue.Retain();
		});
}

cl_int CL_API_CALL clFlush(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(command_queue), [&]()
		{
			auto& queue = MapType(command_queue);

			if (!Queue::Validate(&queue))
				throw Exception{CL_INVALID_COMMAND_QUEUE};
		});
}

cl_int CL_API_CALL clFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(command_queue), [&]()
		{
			auto& queue = MapType(command_queue);

			if (!Queue::Validate(&queue))
				throw Exception{CL_INVALID_COMMAND_QUEUE};

			queue.Wait();
		});
}

cl_mem CL_API_CALL clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try<cl_mem>(errcode_ret, &MapType(context), nullptr, [&]()
		{
			return MapType(new Buffer{&MapType(context), MemFlags{flags}, size, host_ptr});
		});
}

cl_mem CL_API_CALL clCreateSubBuffer(cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type, const void* buffer_create_info, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
	return Try<cl_mem>(errcode_ret, MapType(buffer), nullptr, [&]()
		{
			auto& parent = MapType(buffer);

			if (!Buffer::Validate(&parent))
				throw Exception{CL_INVALID_MEM_OBJECT};

			auto flags_ = MemFlags{flags};

			if (!flags_.Validate() ||
				parent.GetMemFlags().HasFlags(CL_MEM_READ_ONLY) && flags_.HasAnyFlags(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY) ||
				parent.GetMemFlags().HasFlags(CL_MEM_WRITE_ONLY) && flags_.HasAnyFlags(CL_MEM_READ_WRITE | CL_MEM_READ_ONLY) ||
				parent.GetMemFlags().HasAnyFlags(CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_NO_ACCESS) && flags_.HasFlags(CL_MEM_HOST_WRITE_ONLY) ||
				parent.GetMemFlags().HasAnyFlags(CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS) && flags_.HasFlags(CL_MEM_HOST_READ_ONLY))
				throw Exception{CL_INVALID_VALUE};

			if (flags_.GetKernelAccessFlags() == 0)
				flags_ |= parent.GetMemFlags().GetKernelAccessFlags();

			if (flags_.GetHostAccessFlags() == 0)
				flags_ |= parent.GetMemFlags().GetHostAccessFlags();

			const auto buffer_create_type_ = BufferType{buffer_create_type};

			if (!buffer_create_type_.Validate())
				throw Exception{CL_INVALID_VALUE};

			auto subBuffer = Buffer{flags_};
			subBuffer.ctx = parent.ctx;

			switch (buffer_create_type_.GetValue())
			{
			case CL_BUFFER_CREATE_TYPE_REGION:
			{
				if (buffer_create_info == nullptr)
					throw Exception{CL_INVALID_VALUE};

				const auto& region = *reinterpret_cast<const cl_buffer_region*>(buffer_create_info);

				if (region.origin + region.size > parent.size)
					throw Exception{CL_INVALID_VALUE};

				subBuffer.start = parent.start + region.origin;
				subBuffer.size = region.size;

				if (parent.hostPtr != nullptr)
					subBuffer.hostPtr = parent.hostPtr + region.origin;

				break;
			}
			default:
				throw Exception{CL_INVALID_VALUE};
			}

			return MapType(new Buffer{std::move(subBuffer)});
		});
}

cl_int CL_API_CALL clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(memobj), [&]()
		{
			auto& buffer = MapType(memobj);

			if (!Buffer::Validate(&buffer))
				throw Exception{CL_INVALID_MEM_OBJECT};

			buffer.Retain();
		});
}

cl_int CL_API_CALL clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* ev) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(command_queue), [&]()
		{
			auto& queue = MapType(command_queue);
			auto& buffer_ = MapType(buffer);

			if (!Queue::Validate(&queue))
				throw Exception{CL_INVALID_COMMAND_QUEUE};
			if (!Buffer::Validate(&buffer_))
				throw Exception{CL_INVALID_MEM_OBJECT};
			if (queue.ctx != buffer_.ctx)
				throw Exception{CL_INVALID_CONTEXT};
			if (buffer_.size < offset + size || ptr == nullptr)
				throw Exception{CL_INVALID_VALUE};
			if (event_wait_list == nullptr && num_events_in_wait_list > 0 ||
				event_wait_list != nullptr && num_events_in_wait_list == 0)
				throw Exception{CL_INVALID_EVENT_WAIT_LIST};
			if (buffer_.GetMemFlags().HasAnyFlags(CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_NO_ACCESS))
				throw Exception{CL_INVALID_OPERATION};

			auto mockEvent = std::make_unique<Event>(
				std::vector<cl_event>{ event_wait_list, event_wait_list + num_events_in_wait_list },
				std::chrono::nanoseconds(1000 + rand() % 1000));

			queue.RegisterEvent(mockEvent.get());

			std::memcpy(buffer_.start + offset, ptr, size);
			buffer_.Dump("write");

			if (ev != nullptr)
				MapType(ev) = mockEvent.release();

			if (blocking_write)
				mockEvent->Wait();
		});
}

cl_int CL_API_CALL clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t  size, void* ptr, cl_uint  num_events_in_wait_list, const cl_event* event_wait_list, cl_event* ev) CL_API_SUFFIX__VERSION_1_0
{
	auto& queue = MapType(command_queue);
	const auto& buffer_ = MapType(buffer);

	return Try(queue, [&]()
		{
			if (!Queue::Validate(&queue))
				throw Exception{CL_INVALID_COMMAND_QUEUE};
			if (!Buffer::Validate(&buffer_))
				throw Exception{CL_INVALID_MEM_OBJECT};
			if (queue.ctx != buffer_.ctx)
				throw Exception{CL_INVALID_CONTEXT};
			// MIOpen relies on this size != 0:
			if (size != 0 && (buffer_.size < offset + size || ptr == nullptr))
				throw Exception{CL_INVALID_VALUE};
			if (event_wait_list == nullptr && num_events_in_wait_list > 0 ||
				event_wait_list != nullptr && num_events_in_wait_list == 0)
				throw Exception{CL_INVALID_EVENT_WAIT_LIST};
			if (buffer_.GetMemFlags().HasAnyFlags(CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS))
				throw Exception{CL_INVALID_OPERATION};

			auto mockEvent = std::make_unique<Event>(
				std::vector<cl_event>{ event_wait_list, event_wait_list + num_events_in_wait_list },
				std::chrono::nanoseconds(1000 + rand() % 1000));

			queue.RegisterEvent(mockEvent.get());

			if (ev != nullptr)
				MapType(ev) = mockEvent.release();

			if (blocking_read)
				mockEvent->Wait();
		});
}

cl_int CL_API_CALL clEnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* ev) CL_API_SUFFIX__VERSION_1_0
{
	auto& queue = MapType(command_queue);
	const auto& src = MapType(src_buffer);
	auto& dst = MapType(dst_buffer);

	return Try(queue, [&]()
		{
			if (!Queue::Validate(&queue))
				throw Exception{CL_INVALID_COMMAND_QUEUE, "clEnqueueCopyBuffer: Invalid command queue."};
			if (!Buffer::Validate(&src))
				throw Exception{CL_INVALID_MEM_OBJECT, "clEnqueueCopyBuffer: Invalid source buffer."};
			if (!Buffer::Validate(&dst))
				throw Exception{CL_INVALID_MEM_OBJECT, "clEnqueueCopyBuffer: Invalid destination buffer."};
			if (queue.ctx != src.ctx)
				throw Exception{CL_INVALID_CONTEXT, "clEnqueueCopyBuffer: Queue and source buffer must have the same context."};
			if (queue.ctx != dst.ctx)
				throw Exception{CL_INVALID_CONTEXT, "clEnqueueCopyBuffer: Queue and destination buffer must have the same context."};
			if (src.size < src_offset + size)
				throw Exception{CL_INVALID_VALUE, "clEnqueueCopyBuffer: Source region is outside of the buffer."};
			if (dst.size < dst_offset + size)
				throw Exception{CL_INVALID_VALUE, "clEnqueueCopyBuffer: Destination region is outside of the buffer."};
			if (event_wait_list == nullptr && num_events_in_wait_list != 0 ||
				event_wait_list != nullptr && num_events_in_wait_list == 0 ||
				!std::all_of(event_wait_list, event_wait_list + num_events_in_wait_list, [](auto event) { return Event::Validate(&MapType(event)); }))
				throw Exception{CL_INVALID_EVENT_WAIT_LIST};
			if (src.start + src_offset <= dst.start + dst_offset && dst.start + dst_offset <= src.start + src_offset + size + 1 ||
				dst.start + src_offset <= src.start + dst_offset && src.start + dst_offset <= dst.start + src_offset + size + 1)
				throw Exception{CL_MEM_COPY_OVERLAP};

			auto mockEvent = std::make_unique<Event>(
				std::vector<cl_event>{ event_wait_list, event_wait_list + num_events_in_wait_list },
				std::chrono::nanoseconds(1000 + rand() % 1000));

			queue.RegisterEvent(mockEvent.get());

			std::memcpy(dst.start + dst_offset, src.start + src_offset, size);
			dst.Dump("copy-from-" + std::to_string(reinterpret_cast<std::ptrdiff_t>(&src)));

			if (ev != nullptr)
				MapType(ev) = mockEvent.release();
		});
}

cl_int CL_API_CALL clGetCommandQueueInfo(cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(command_queue), [&]()
		{
			const auto& queue = MapType(command_queue);

			switch (param_name)
			{
			case CL_QUEUE_CONTEXT:
				if (!FillProperty(MapType(queue.ctx), param_value_size, param_value, param_value_size_ret, "clGetCommandQueueInfo(CL_QUEUE_CONTEXT)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_QUEUE_DEVICE:
				if (!FillProperty(MapType(queue.device), param_value_size, param_value, param_value_size_ret, "clGetCommandQueueInfo(CL_QUEUE_DEVICE)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			default:
				std::ostringstream ss;
				ss << "Unknown device info: " << std::hex << param_name << std::endl;
				throw Exception{CL_INVALID_VALUE, ss.str()};
			}
		});
}

cl_program CL_API_CALL clCreateProgramWithSource(cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try<cl_program>(errcode_ret, &MapType(context), nullptr, [&]()
		{
			if (!Context::Validate(&MapType(context)))
				throw Exception(CL_INVALID_CONTEXT);
			if (count == 0)
				throw Exception(CL_INVALID_VALUE, "clCreateProgramWithSource: count should not be 0.");
			if (strings == nullptr)
				throw Exception(CL_INVALID_VALUE, "clCreateProgramWithSource: strings should not be nullptr.");

			auto program = Program{};
			program.ctx = &MapType(context);

			for (auto i = 0; i < count; ++i)
			{
				if (lengths[i] == 0)
					throw Exception(CL_INVALID_VALUE, "clCreateProgramWithSource: lengths[" + std::to_string(i) + "] should not be 0.");
				if (strings[i] == nullptr)
					throw Exception(CL_INVALID_VALUE, "clCreateProgramWithSource: strings[" + std::to_string(i) + "] should not be nullptr.");
				program.sources.emplace_back(strings[i], lengths[i]);
			}

			return MapType(new Program{std::move(program)});
		});
}

cl_program CL_API_CALL clCreateProgramWithBinary(cl_context context, cl_uint num_devices, const cl_device_id* device_list, const size_t* lengths, const unsigned char** binaries, cl_int* /* binary_status */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try<cl_program>(errcode_ret, &MapType(context), nullptr, [&]()
		{
			auto& ctx = MapType(context);

			if (!Context::Validate(&ctx))
				throw Exception(CL_INVALID_CONTEXT);
			if (num_devices == 0)
				throw Exception(CL_INVALID_VALUE, "clCreateProgramWithBinary: num_devices should not be 0.");
			if (device_list == nullptr)
				throw Exception(CL_INVALID_VALUE, "clCreateProgramWithBinary: device_list should not be nullptr.");
			if (lengths == 0)
				throw Exception(CL_INVALID_VALUE, "clCreateProgramWithBinary: lengths should not be 0.");
			if (binaries == nullptr)
				throw Exception(CL_INVALID_VALUE, "clCreateProgramWithBinary: binaries should not be nullptr.");

			auto program = Program{};
			program.ctx = &ctx;

			for (auto i = 0; i < num_devices; ++i)
			{
				if (lengths[i] == 0)
					throw Exception(CL_INVALID_VALUE, "clCreateProgramWithBinary: lengths[" + std::to_string(i) + "] should not be 0.");
				if (binaries[i] == 0)
					throw Exception(CL_INVALID_VALUE, "clCreateProgramWithBinary: binaries[" + std::to_string(i) + "] should not be nullptr.");

				const auto device = &MapType(device_list[i]);

				if (!Device::Validate(device))
					throw Exception{CL_INVALID_DEVICE, "clCreateProgramWithBinary: device_list[" + std::to_string(i) + "] is invalid."};
				if (std::find(ctx.devices.begin(), ctx.devices.end(), device) == ctx.devices.end())
					throw Exception{CL_INVALID_DEVICE, "clCreateProgramWithBinary: device_list[" + std::to_string(i) + "] is not associated with the passed context."};

				program.devices.push_back(device);
				program.binaries.emplace_back();
				program.binaries.rbegin()->resize(lengths[i]);

				std::memcpy(program.binaries.rbegin()->data(), binaries[i], lengths[i]);
			}

			return MapType(new Program{std::move(program)});
		});
}

cl_int CL_API_CALL clRetainProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(program), [&]()
		{
			auto& program_ = MapType(program);

			if (!Program::Validate(&program_))
				throw Exception(CL_INVALID_PROGRAM);

			program_.Retain();
		});
}

cl_int CL_API_CALL clBuildProgram(cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (CL_CALLBACK* pfn_notify)(cl_program /* program */, void* /* user_data */), void* user_data) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(program), [&]()
		{
			auto& program_ = MapType(program);

			if (!Program::Validate(&program_))
				throw Exception(CL_INVALID_PROGRAM);
			if (!program_.kernels.empty())
				throw Exception(CL_INVALID_OPERATION, "clBuildProgram: attempt to build a program with attached kernels.");
			if (program_.sources.empty() && program_.binaries.empty())
				throw Exception(CL_INVALID_OPERATION, "clBuildProgram: attempt to build a program without sources or binaries.");
			if (num_devices == 0)
				throw Exception(CL_INVALID_VALUE, "clBuildProgram: num_devices should not be 0.");
			if (device_list == nullptr)
				throw Exception(CL_INVALID_VALUE, "clBuildProgram: device_list should not be nullptr.");
			if (pfn_notify == nullptr && user_data != nullptr)
				throw Exception(CL_INVALID_VALUE, "clBuildProgram: user_data should be nullptr, when pfn_notify is nullptr.");

			auto& ctx = *program_.ctx;
			const auto hadDevices = !program_.devices.empty();

			for (int i = 0; i < num_devices; ++i)
			{
				const auto device = &MapType(device_list[i]);

				if (!Device::Validate(device))
					throw Exception{CL_INVALID_DEVICE, "clBuildProgram: device_list[" + std::to_string(i) + "] is invalid."};
				if (hadDevices && std::find(program_.devices.begin(), program_.devices.end(), device) == ctx.devices.end())
					throw Exception{CL_INVALID_DEVICE, "clBuildProgram: device_list[" + std::to_string(i) + "] is not associated with the passed program."};

				if (!hadDevices)
					program_.devices.push_back(device);
			}

			program_.buildStatuses.resize(num_devices);
			program_.buildLogs.resize(num_devices);
			program_.binaries.resize(num_devices);

			for (int i = 0; i < num_devices; ++i)
				program_.buildStatuses[i] = BuildStatus::InProgress;
			program_.options = options != nullptr ? options : "";

			const auto hadBinaries = !program_.binaries.empty();

			if (pfn_notify == nullptr)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds{10 + rand() % 10});
				for (int i = 0; i < num_devices; ++i)
				{
					program_.buildStatuses[i] = BuildStatus::Success;
					if (!hadBinaries)
						program_.binaries[i] = {program_.sources[i].begin(), program_.sources[i].end()};
				}
				return;
			}

			std::thread([=]()
				{
					std::this_thread::sleep_for(std::chrono::milliseconds{10 + rand() % 10});

					for (int i = 0; i < num_devices; ++i)
					{
						auto& program_ = MapType(program);

						program_.buildStatuses[i] = BuildStatus::Success;
						if (!hadBinaries)
							program_.binaries[i] = {program_.sources[i].begin(), program_.sources[i].end()};
					}

					pfn_notify(program, user_data);
				});
		});
}

cl_int CL_API_CALL clGetProgramBuildInfo(cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(program), [&]()
		{
			const auto& program_ = MapType(program);
			const auto& device_ = &MapType(device);

			if (!Program::Validate(&program_))
				throw Exception{CL_INVALID_PROGRAM};
			if (!Device::Validate(device_))
				throw Exception{CL_INVALID_DEVICE};

			const auto it = std::find(program_.devices.begin(), program_.devices.end(), device_);

			if (it == program_.devices.end())
				throw Exception{CL_INVALID_DEVICE};

			const auto id = it - program_.devices.begin();

			switch (param_name)
			{
			case CL_PROGRAM_BUILD_STATUS:
			{
				const auto cl_status = [&]() -> cl_build_status
				{
					if (program_.buildStatuses.size() <= id)
						return CL_BUILD_NONE;

					switch (program_.buildStatuses[id])
					{
					default:
					case BuildStatus::None: return CL_BUILD_NONE;
					case BuildStatus::Error: return CL_BUILD_ERROR;
					case BuildStatus::Success: return CL_BUILD_SUCCESS;
					case BuildStatus::InProgress: return CL_BUILD_IN_PROGRESS;
					}
				};

				if (!FillProperty(cl_status(), param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			}
			case CL_PROGRAM_BUILD_OPTIONS:
				if (!FillStringProperty(program_.options, param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PROGRAM_BUILD_LOG:
				if (program_.buildStatuses.size() <= id)
				{
					if (!FillStringProperty("", param_value_size, param_value, param_value_size_ret))
						throw Exception{CL_INVALID_VALUE};
				}
				else if (!FillStringProperty(program_.buildLogs[id], param_value_size, param_value, param_value_size_ret))
					throw Exception{CL_INVALID_VALUE};
				return;
			default:
				throw Exception{CL_INVALID_VALUE};
			}
		});
}

cl_int CL_API_CALL clGetProgramInfo(cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(program), [&]()
		{
			const auto& program_ = MapType(program);

			if (!Program::Validate(&program_))
				throw Exception{CL_INVALID_PROGRAM};

			const auto& binaries = program_.binaries;

			switch (param_name)
			{
			case CL_PROGRAM_REFERENCE_COUNT:
				if (!FillProperty(program_.GetReferenceCount(), param_value_size, param_value, param_value_size_ret, "clGetProgramInfo(CL_PROGRAM_REFERENCE_COUNT)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PROGRAM_CONTEXT:
				if (!FillProperty(MapType(program_.ctx), param_value_size, param_value, param_value_size_ret, "clGetProgramInfo(CL_PROGRAM_CONTEXT)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PROGRAM_NUM_DEVICES:
				if (!FillProperty(program_.devices.size(), param_value_size, param_value, param_value_size_ret, "clGetProgramInfo(CL_PROGRAM_NUM_DEVICES)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PROGRAM_DEVICES:
			{
				auto devices = std::vector<cl_device_id>{};
				std::transform(program_.devices.begin(), program_.devices.end(), std::back_inserter(devices), [](auto&& device) { return MapType(device); });

				if (!FillArrayProperty(devices.data(), devices.size(), param_value_size, param_value, param_value_size_ret, "clGetProgramInfo(CL_PROGRAM_DEVICES)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			}
			case CL_PROGRAM_BINARY_SIZES:
			{
				auto sizes = std::vector<std::size_t>{};
				std::transform(binaries.begin(), binaries.end(), std::back_inserter(sizes), [](auto&& arr) { return arr.size(); });

				if (!FillArrayProperty(sizes.data(), sizes.size(), param_value_size, param_value, param_value_size_ret, "clGetProgramInfo(CL_PROGRAM_BINARY_SIZES)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			}
			case CL_PROGRAM_BINARIES:
			{
				if (ExtensiveLogging)
					std::cerr << "CL Mocker(clGetProgramInfo(CL_PROGRAM_BINARIES)): Writing " << binaries.size() << " binaries to 0x" << std::ios::hex << reinterpret_cast<std::ptrdiff_t>(param_value) << std::ios::dec << "." << std::endl;

				if (param_value_size / sizeof(std::size_t) != binaries.size() ||
					param_value_size % sizeof(std::size_t) != 0)
					throw Exception{CL_INVALID_VALUE};

				const auto resultNum = param_value_size < sizeof(std::size_t);
				char** results = reinterpret_cast<char**>(param_value);
				for (auto i = 0; i < resultNum; ++i)
					strncpy(results[i], binaries[i].data(), binaries[i].size());
				return;
			}
			default:
				std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
				throw Exception{CL_INVALID_VALUE};
			}
		});
}

cl_kernel CL_API_CALL clCreateKernel(cl_program program, const char* kernel_name, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	return Try<cl_kernel>(errcode_ret, MapType(program), nullptr, [&]()
		{
			auto& program_ = MapType(program);

			if (!Program::Validate(&program_))
				throw Exception{CL_INVALID_PROGRAM};
			if (program_.binaries.empty() && std::all_of(program_.buildStatuses.begin(), program_.buildStatuses.end(), [](auto status) { return status == BuildStatus::Success; }))
				throw Exception{CL_INVALID_BINARY};
			if (kernel_name == nullptr)
				throw Exception{CL_INVALID_VALUE};

			auto kernel = std::make_unique<Kernel>();
			kernel->ctx = program_.ctx;
			kernel->program = &program_;
			program_.kernels.push_back(kernel.get());
			kernel->name = kernel_name;

			return MapType(kernel.release());
		});
}

cl_int CL_API_CALL clRetainKernel(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(kernel), [&]()
		{
			if (!Kernel::Validate(&MapType(kernel)))
				throw Exception{CL_INVALID_KERNEL};

			MapType(kernel).Retain();
		});;
}

cl_int CL_API_CALL clGetKernelInfo(cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	const auto& k = MapType(kernel);

	return Try(k, [&]()
		{
			switch (param_name)
			{
			case CL_KERNEL_REFERENCE_COUNT:
				if (!FillProperty(k.GetReferenceCount(), param_value_size, param_value, param_value_size_ret, "clGetKernelInfo(CL_KERNEL_REFERENCE_COUNT)"))
					throw Exception{CL_INVALID_ARG_SIZE};
				return;
			case CL_KERNEL_CONTEXT:
				if (!FillProperty(MapType(k.ctx), param_value_size, param_value, param_value_size_ret, "clGetKernelInfo(CL_KERNEL_CONTEXT)"))
					throw Exception{CL_INVALID_ARG_SIZE};
				return;
			case CL_KERNEL_PROGRAM:
				if (!FillProperty(MapType(k.program), param_value_size, param_value, param_value_size_ret, "clGetKernelInfo(CL_KERNEL_PROGRAM)"))
					throw Exception{CL_INVALID_ARG_SIZE};
				return;
			case CL_KERNEL_FUNCTION_NAME:
				if (!FillStringProperty(k.name, param_value_size, param_value, param_value_size_ret, "clGetKernelInfo(CL_KERNEL_FUNCTION_NAME)"))
					throw Exception{CL_INVALID_ARG_SIZE};
				return;
			default:
				std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
				throw Exception{CL_INVALID_ARG_SIZE};
			}
		});
}

cl_int CL_API_CALL clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value) CL_API_SUFFIX__VERSION_1_0
{
	return Try(MapType(kernel), [&]()
		{
			auto& kernel_ = MapType(kernel);

			if (!Kernel::Validate(&kernel_))
				throw Exception{CL_INVALID_KERNEL};

			kernel_.SetArg(arg_index, arg_size, arg_value);
		});
}

cl_int CL_API_CALL clEnqueueNDRangeKernel(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* ev) CL_API_SUFFIX__VERSION_1_0
{
	auto& queue = MapType(command_queue);
	auto& kernel_ = MapType(kernel);

	return Try(queue, [&]()
		{
			if (!Queue::Validate(&queue))
				throw Exception{CL_INVALID_COMMAND_QUEUE};
			if (!Kernel::Validate(&kernel_))
				throw Exception{CL_INVALID_KERNEL};
			if (work_dim < 1)
				throw Exception{CL_INVALID_WORK_DIMENSION};
			if (num_events_in_wait_list != 0 && event_wait_list == nullptr ||
				num_events_in_wait_list == 0 && event_wait_list != nullptr)
				throw Exception{CL_INVALID_EVENT_WAIT_LIST};

			queue.EnqueueNDRangeKernel(
				kernel_,
				{global_work_offset, global_work_offset + work_dim},
				{global_work_size, global_work_size + work_dim},
				{local_work_size, local_work_size + work_dim},
				{event_wait_list, event_wait_list + num_events_in_wait_list},
				MapType(ev));
		});
}

cl_int CL_API_CALL clWaitForEvents(cl_uint num_events, const cl_event* event_list) CL_API_SUFFIX__VERSION_1_0
{
	if (num_events == 0)
		throw Exception(CL_INVALID_EVENT_WAIT_LIST, "clWaitForEvents: num_events should not be 0.");
	if (event_list == nullptr)
		throw Exception(CL_INVALID_EVENT_WAIT_LIST, "clWaitForEvents: event_list should not be nullpt.");

	return Try(MapType(event_list[0]), [&]()
		{
			for (auto i = 0; i < num_events; ++i)
			{
				auto& mockEvent = MapType(event_list[i]);

				if (!Event::Validate(&mockEvent))
					throw Exception{CL_INVALID_EVENT};
				if (i > 0 && mockEvent.ctx != MapType(event_list[0]).ctx)
					throw Exception(CL_INVALID_CONTEXT, "clWaitForEvents: event_list should only contain events with the same context.");

				if (!mockEvent.IsFinished())
					mockEvent.Wait();
			}
		});
}

cl_int CL_API_CALL clGetEventProfilingInfo(cl_event ev, cl_profiling_info  param_name, size_t  param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	auto& mockEvent = MapType(ev);

	return Try(mockEvent, [&]()
		{
			if (!Event::Validate(&mockEvent))
				throw Exception{CL_INVALID_EVENT};

			switch (param_name)
			{
			case CL_PROFILING_COMMAND_QUEUED:
				if (!FillProperty<cl_ulong>(mockEvent.GetQueued().time_since_epoch().count(), param_value_size, param_value, param_value_size_ret, "clGetEventProfilingInfo(CL_PROFILING_COMMAND_QUEUED)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PROFILING_COMMAND_SUBMIT:
				if (!FillProperty<cl_ulong>(mockEvent.GetSubmitted().time_since_epoch().count(), param_value_size, param_value, param_value_size_ret, "clGetEventProfilingInfo(CL_PROFILING_COMMAND_SUBMIT)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PROFILING_COMMAND_START:
				if (!FillProperty<cl_ulong>(mockEvent.GetStart().time_since_epoch().count(), param_value_size, param_value, param_value_size_ret, "clGetEventProfilingInfo(CL_PROFILING_COMMAND_START)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PROFILING_COMMAND_END:
				if (!FillProperty<cl_ulong>(mockEvent.GetEnd().time_since_epoch().count(), param_value_size, param_value, param_value_size_ret, "clGetEventProfilingInfo(CL_PROFILING_COMMAND_END)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			case CL_PROFILING_COMMAND_COMPLETE:
				if (!FillProperty<cl_ulong>(mockEvent.GetComplete().time_since_epoch().count(), param_value_size, param_value, param_value_size_ret, "clGetEventProfilingInfo(CL_PROFILING_COMMAND_COMPLETE)"))
					throw Exception{CL_INVALID_VALUE};
				return;
			default:
				std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
				throw Exception{CL_INVALID_VALUE};
			}
		});
}

cl_int CL_API_CALL clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
	auto& mem = MapType(memobj);
	return Try(mem, [&]()
		{
			if (!Buffer::Validate(&mem))
				throw Exception{CL_INVALID_MEM_OBJECT};

			if (mem.Release())
				delete& mem;
		});
}

cl_int CL_API_CALL clReleaseKernel(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0
{
	auto& k = MapType(kernel);
	return Try(k, [&]()
		{
			if (!Kernel::Validate(&k))
				throw Exception{CL_INVALID_KERNEL};

			if (k.Release())
				delete& k;
		});
}

cl_int CL_API_CALL clReleaseProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
	auto& p = MapType(program);
	return Try(p, [&]()
		{
			if (!Program::Validate(&p))
				throw Exception{CL_INVALID_PROGRAM};

			if (p.Release())
				delete& p;
		});
}

cl_int CL_API_CALL clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	auto& queue = MapType(command_queue);
	return Try(queue, [&]()
		{
			if (!Queue::Validate(&queue))
				throw Exception{CL_INVALID_COMMAND_QUEUE};

			if (queue.Release())
				delete& queue;
		});
}

cl_int CL_API_CALL clReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
	auto& ctx = MapType(context);
	return Try(&ctx, [&]()
		{
			if (!Context::Validate(&ctx))
				throw Exception{CL_INVALID_CONTEXT};

			if (ctx.Release())
				delete& ctx;
		});
}

cl_int CL_API_CALL clReleaseEvent(cl_event ev) CL_API_SUFFIX__VERSION_1_0
{
	auto& event = MapType(ev);
	return Try(event, [&]()
		{
			if (!Event::Validate(&event))
				throw Exception {CL_INVALID_EVENT};

			if (event.Release())
				delete& event;
		});
}