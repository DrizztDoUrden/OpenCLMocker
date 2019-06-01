#include <OpenCLMocker/Buffer.hpp>
#include <OpenCLMocker/Context.hpp>
#include <OpenCLMocker/Device.hpp>
#include <OpenCLMocker/Event.hpp>
#include <OpenCLMocker/Kernel.hpp>
#include <OpenCLMocker/Platform.hpp>
#include <OpenCLMocker/Program.hpp>
#include <OpenCLMocker/Queue.hpp>

#include <CL/cl.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace
{
	template <class TElement>
	bool FillArrayProperty(const TElement* arr, std::size_t size, size_t param_value_size, void* param_value, size_t* param_value_size_ret)
	{
		const auto memory = size * sizeof(TElement);

		if (param_value != nullptr && param_value_size != 0)
		{
			if (param_value_size < memory)
				return false;
			std::memcpy(param_value, arr, memory);
		}

		if (param_value_size_ret != nullptr)
			*param_value_size_ret = memory;

		return true;
	}

	template <class TValue>
	bool FillProperty(TValue value, size_t param_value_size, void* param_value, size_t* param_value_size_ret)
	{
		if (param_value != nullptr && param_value_size != 0)
		{
			if (param_value_size < sizeof(TValue))
				return false;
			*reinterpret_cast<TValue*>(param_value) = value;
		}

		if (param_value_size_ret != nullptr)
			*param_value_size_ret = sizeof(TValue);

		return true;
	}

	bool FillStringProperty(const std::string& value, size_t param_value_size, void* param_value, size_t* param_value_size_ret)
	{
		const auto memory = (value.length() + 1) * sizeof(char);

		if (param_value != nullptr && param_value_size != 0)
		{
			if (param_value_size < memory)
				return false;
			std::strcpy(reinterpret_cast<char*>(param_value), value.c_str());
		}

		if (param_value_size_ret != nullptr)
			*param_value_size_ret = memory;

		return true;
	}

	template<class TProperty, class THandler>
	cl_int IterateOverProperties(const TProperty* properties, const THandler& handler)
	{
		auto curPtr = properties;

		while (curPtr != nullptr && *curPtr != 0)
		{
			const auto& name = *curPtr;

			if (++curPtr == nullptr)
				return CL_INVALID_PROPERTY;

			const auto tret = handler(name, *curPtr);

			if (tret != CL_SUCCESS)
				return tret;

			++curPtr;
		}

		return CL_SUCCESS;
	}

	OpenCL::Platform* GetPlatformByDeviceType(cl_device_type device_type)
	{
		for (auto& platform : OpenCL::Platform::Get())
			for (auto& device : platform->devices)
				if (device.type == device_type)
					return platform;

		return nullptr;
	}
}

cl_int CL_API_CALL clGetPlatformIDs(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms) CL_API_SUFFIX__VERSION_1_0
{
	auto& existingPlatforms = OpenCL::Platform::Get();

	if (num_platforms != nullptr)
		* num_platforms = existingPlatforms.size();

	if (platforms != nullptr && num_entries > 0)
		for (auto i = 0; i < std::min(num_entries, static_cast<cl_uint>(existingPlatforms.size())); ++i)
			platforms[i] = MapType(existingPlatforms[i]);

	return CL_SUCCESS;
}

cl_int CL_API_CALL clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	switch (param_name)
	{
	case CL_PLATFORM_NAME:
		if (!FillStringProperty(MapType(platform).name, param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_PLATFORM_VENDOR:
		if (!FillStringProperty(MapType(platform).vendor, param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_PLATFORM_PROFILE:
		if (!FillStringProperty(MapType(platform).profile, param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_PLATFORM_VERSION:
		if (!FillStringProperty(MapType(platform).version, param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	default:
		std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
		return CL_INVALID_ARG_VALUE;
	}
}

cl_int CL_API_CALL clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices) CL_API_SUFFIX__VERSION_1_0
{
	auto& mockPlatform = MapType(platform);

	if (num_devices != nullptr)
		* num_devices = mockPlatform.devices.size();

	if (devices != nullptr && num_entries > 0)
		for (auto i = 0; i < std::min<cl_uint>(num_entries, mockPlatform.devices.size()); i++)
			devices[i] = MapType(mockPlatform.devices[i]);

	return CL_SUCCESS;
}

cl_context CL_API_CALL clCreateContext(const cl_context_properties* properties, cl_uint num_devices, const cl_device_id* devices, void (CL_CALLBACK* /* pfn_notify */)(const char*, const void*, size_t, void*), void* /* user_data */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (num_devices <= 0 || devices == nullptr)
	{
		if (errcode_ret != nullptr)
			*errcode_ret = CL_INVALID_ARG_VALUE;

		return nullptr;
	}

	auto ctx = OpenCL::Context{};

	const auto propertyHandler = [&ctx](const cl_context_properties& name, const cl_context_properties& value)
	{
		switch (reinterpret_cast<const cl_int&>(name))
		{
		default:
			return CL_INVALID_PROPERTY;
		}
	};

	const auto errcode = IterateOverProperties(properties, propertyHandler);

	if (errcode != CL_SUCCESS)
	{
		if (errcode_ret != nullptr)
			* errcode_ret = errcode;
		return nullptr;
	}

	for (auto i = 0; i < num_devices; ++i)
	{
		if (devices[i] == nullptr)
		{
			if (errcode_ret != nullptr)
				* errcode_ret = CL_INVALID_ARG_VALUE;

			return nullptr;
		}

		ctx.devices.push_back(&MapType(devices[i]));
	}

	ctx.platform = ctx.devices.front()->GetPlatform();

	if (errcode_ret != nullptr)
		* errcode_ret = CL_SUCCESS;
	return MapType(new OpenCL::Context{ std::move(ctx) });
}

cl_context CL_API_CALL clCreateContextFromType(const cl_context_properties* properties, cl_device_type device_type, void (CL_CALLBACK* /* pfn_notify */)(const char*, const void*, size_t, void*), void* /* user_data */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	auto ctx = OpenCL::Context{};

	const auto propertyHandler = [&ctx](const cl_context_properties& name, const cl_context_properties& value)
	{
		switch (reinterpret_cast<const cl_int&>(name))
		{
		case CL_CONTEXT_PLATFORM:
			ctx.platform = &MapType(reinterpret_cast<const cl_platform_id&>(value));
			return CL_SUCCESS;
		default:
			return CL_INVALID_PROPERTY;
		}
	};

	const auto errcode = IterateOverProperties(properties, propertyHandler);

	if (errcode != CL_SUCCESS)
	{
		if (errcode_ret != nullptr)
			* errcode_ret = errcode;
		return nullptr;
	}

	ctx.platform = GetPlatformByDeviceType(device_type);

	for (auto& device : ctx.platform->devices)
		ctx.devices.push_back(&device);

	if (errcode_ret != nullptr)
		*errcode_ret = CL_SUCCESS;
	return MapType(new OpenCL::Context{ std::move(ctx) });
}

cl_int CL_API_CALL clRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
	MapType(context).Retain();
	return CL_SUCCESS;
}

cl_int CL_API_CALL clGetContextInfo(cl_context context, cl_context_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	const auto& ctx = MapType(context);

	switch (param_name)
	{
	case CL_CONTEXT_PLATFORM:
		if (!FillProperty(MapType(ctx.platform), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_CONTEXT_NUM_DEVICES:
		if (!FillProperty(static_cast<cl_uint>(ctx.devices.size()), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_CONTEXT_DEVICES:
		if (!FillArrayProperty(ctx.devices.data(), ctx.devices.size(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	default:
		std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
		return CL_INVALID_ARG_VALUE;
	}
}

cl_int CL_API_CALL clGetDeviceInfo(cl_device_id device, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	const auto& dev = MapType(device);

	switch (param_name)
	{
	case CL_DEVICE_NAME:
		if (!FillStringProperty("", param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_VERSION:
		if (!FillStringProperty("", param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DRIVER_VERSION:
		if (!FillStringProperty("", param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
		if (!FillProperty(std::numeric_limits<cl_ulong>::max(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_GLOBAL_MEM_SIZE:
		if (!FillProperty(std::numeric_limits<cl_ulong>::max(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_LOCAL_MEM_SIZE:
		if (!FillProperty(std::numeric_limits<cl_ulong>::max(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_MAX_WORK_GROUP_SIZE:
		if (!FillProperty(std::numeric_limits<size_t>::max(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_MAX_CLOCK_FREQUENCY:
		if (!FillProperty(std::numeric_limits<cl_uint>::max(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_MAX_COMPUTE_UNITS:
		if (!FillProperty(static_cast<cl_uint>(64), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_VENDOR_ID:
		if (!FillProperty(static_cast<cl_uint>(0x1002), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_AVAILABLE:
		if (!FillProperty(static_cast<cl_bool>(true), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_DEVICE_PLATFORM:
		if (!FillProperty(MapType(dev.GetPlatform()), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	default:
		std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
		return CL_INVALID_ARG_VALUE;
	}
}

cl_command_queue CL_API_CALL clCreateCommandQueue(cl_context context, cl_device_id device, cl_command_queue_properties /* properties */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_2_0
{
	auto queue = OpenCL::Queue{};

	queue.ctx = &MapType(context);
	queue.device = &MapType(device);

	if (errcode_ret != nullptr)
		* errcode_ret = CL_SUCCESS;

	return MapType(new OpenCL::Queue{ std::move(queue) });
}

cl_int CL_API_CALL clRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	MapType(command_queue).Retain();
	return CL_SUCCESS;
}

cl_int CL_API_CALL clFlush(cl_command_queue /* command_queue */) CL_API_SUFFIX__VERSION_1_0 { return CL_SUCCESS; }

cl_int CL_API_CALL clFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	MapType(command_queue).Wait();
	return CL_SUCCESS;
}

cl_mem CL_API_CALL clCreateBuffer(cl_context /* context */, cl_mem_flags /* flags */, size_t /* size */, void* /* host_ptr */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (errcode_ret != nullptr)
		*errcode_ret = CL_SUCCESS;

	return MapType(new OpenCL::Buffer{});
}

cl_mem CL_API_CALL clCreateSubBuffer(cl_mem /* buffer */, cl_mem_flags /* flags */, cl_buffer_create_type /* buffer_create_type */, const void* /* buffer_create_info */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
	if (errcode_ret != nullptr)
		*errcode_ret = CL_SUCCESS;

	return MapType(new OpenCL::Buffer{});
}

cl_int CL_API_CALL clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
	MapType(memobj).Retain();
	return CL_SUCCESS;
}

cl_int CL_API_CALL clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem /* buffer */, cl_bool blocking_write, size_t /* offset */, size_t /* size */, const void* /* ptr */, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* ev) CL_API_SUFFIX__VERSION_1_0
{
	const auto mockEvent = new OpenCL::Event{ { event_wait_list, event_wait_list + num_events_in_wait_list }, std::chrono::nanoseconds(1000 + rand() % 1000) };
	MapType(command_queue).RegisterEvent(mockEvent);

	if (ev != nullptr)
		MapType(ev) = mockEvent;

	if (blocking_write)
		mockEvent->Wait();

	return CL_SUCCESS;
}

cl_int CL_API_CALL clGetCommandQueueInfo(cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	const auto& queue = MapType(command_queue);

	switch (param_name)
	{
	case CL_QUEUE_CONTEXT:
		if (!FillProperty(MapType(queue.ctx), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_QUEUE_DEVICE:
		if (!FillProperty(MapType(queue.device), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	default:
		std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
		return CL_INVALID_ARG_VALUE;
	}
}

cl_program CL_API_CALL clCreateProgramWithSource(cl_context /* context */, cl_uint /* count */, const char** /* strings */, const size_t* /* lengths */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (errcode_ret != nullptr)
		*errcode_ret = CL_SUCCESS;

	return MapType(new OpenCL::Program{});
}

cl_program CL_API_CALL clCreateProgramWithBinary(cl_context /* context */, cl_uint /* num_devices */, const cl_device_id* /* device_list */, const size_t* /* lengths */, const unsigned char** /* binaries */, cl_int* /* binary_status */, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	if (errcode_ret != nullptr)
		* errcode_ret = CL_SUCCESS;

	return MapType(new OpenCL::Program{});
}

cl_int CL_API_CALL clRetainProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
	MapType(program).Retain();
	return CL_SUCCESS;
}

cl_int CL_API_CALL clBuildProgram(cl_program /* program */, cl_uint /* num_devices */, const cl_device_id* /* device_list */, const char* /* options */, void (CL_CALLBACK* /* pfn_notify */)(cl_program /* program */, void* /* user_data */), void* /* user_data */) CL_API_SUFFIX__VERSION_1_0
{
	std::this_thread::sleep_for(std::chrono::milliseconds{ 10 + rand() % 10 });
	return CL_SUCCESS;
}

cl_int CL_API_CALL clGetProgramBuildInfo(cl_program /* program */, cl_device_id /* device */, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	switch (param_name)
	{
	case CL_PROGRAM_BUILD_LOG:
		if (!FillStringProperty("", param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	default:
		return CL_INVALID_ARG_VALUE;
	}
}

cl_int CL_API_CALL clGetProgramInfo(cl_program /* program */, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	std::array<char, 1024> binary;

	switch (param_name)
	{
	case CL_PROGRAM_BINARY_SIZES:
		if (!FillProperty(static_cast<size_t>(binary.size()), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_PROGRAM_BINARIES:
		if (!FillArrayProperty(binary.data(), binary.size(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	default:
		std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
		return CL_INVALID_ARG_VALUE;
	}
}

cl_kernel CL_API_CALL clCreateKernel(cl_program /* program */, const char* kernel_name, cl_int* errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
	auto kernel = OpenCL::Kernel{};

	kernel.name = kernel_name;

	if (errcode_ret != nullptr)
		* errcode_ret = CL_SUCCESS;

	return MapType(new OpenCL::Kernel{ std::move(kernel) });
}

cl_int CL_API_CALL clRetainKernel(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0
{
	MapType(kernel).Retain();
	return CL_SUCCESS;
}

cl_int CL_API_CALL clGetKernelInfo(cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	const auto& k = MapType(kernel);

	switch (param_name)
	{
	case CL_KERNEL_FUNCTION_NAME:
		if (!FillStringProperty(k.name, param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	default:
		std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
		return CL_INVALID_ARG_VALUE;
	}
}

cl_int CL_API_CALL clSetKernelArg(cl_kernel /* kernel */, cl_uint /* arg_index */, size_t /* arg_size */, const void* /* arg_value */) CL_API_SUFFIX__VERSION_1_0
{
	return CL_SUCCESS;
}

cl_int CL_API_CALL clEnqueueNDRangeKernel(cl_command_queue command_queue, cl_kernel /* kernel */, cl_uint /* work_dim */, const size_t* /* global_work_offset */, const size_t* /* global_work_size */, const size_t* /* local_work_size */, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* ev) CL_API_SUFFIX__VERSION_1_0
{
	const auto mockEvent = new OpenCL::Event{ { event_wait_list, event_wait_list + num_events_in_wait_list }, std::chrono::nanoseconds(3000 + rand() % 3000) };
	MapType(command_queue).RegisterEvent(mockEvent);

	if (ev != nullptr)
		MapType(ev) = mockEvent;

	return CL_SUCCESS;
}

cl_int CL_API_CALL clWaitForEvents(cl_uint num_events, const cl_event* event_list) CL_API_SUFFIX__VERSION_1_0
{
	for (auto i = 0; i < num_events; ++i)
	{
		auto& mockEvent = MapType(event_list[i]);

		if (!mockEvent.IsFinished())
			mockEvent.Wait();
	}

	return CL_SUCCESS;
}

cl_int CL_API_CALL clGetEventProfilingInfo(cl_event ev, cl_profiling_info  param_name, size_t  param_value_size, void* param_value, size_t* param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
	auto& mockEvent = MapType(ev);

	switch (param_name)
	{
	case CL_PROFILING_COMMAND_START:
		if (!FillProperty<cl_ulong>(mockEvent.GetStart().time_since_epoch().count(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	case CL_PROFILING_COMMAND_END:
		if (!FillProperty<cl_ulong>(mockEvent.GetEnd().time_since_epoch().count(), param_value_size, param_value, param_value_size_ret))
			return CL_INVALID_ARG_SIZE;
		return CL_SUCCESS;
	default:
		std::cerr << "Unknown device info: " << std::hex << param_name << std::endl;
		return CL_INVALID_ARG_VALUE;
	}
}

cl_int CL_API_CALL clEnqueueCopyBuffer(cl_command_queue command_queue, cl_mem /* src_buffer */, cl_mem /* dst_buffer */, size_t /* src_offset */, size_t /* dst_offset */, size_t /* size */, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* ev) CL_API_SUFFIX__VERSION_1_0
{
	const auto mockEvent = new OpenCL::Event{ { event_wait_list, event_wait_list + num_events_in_wait_list }, std::chrono::nanoseconds(1000 + rand() % 1000) };
	MapType(command_queue).RegisterEvent(mockEvent);

	if (ev != nullptr)
		MapType(ev) = mockEvent;

	return CL_SUCCESS;
}

cl_int CL_API_CALL clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem /* buffer */, cl_bool blocking_read, size_t /* offset */, size_t  /* size */, void* /* ptr */, cl_uint  num_events_in_wait_list, const cl_event* event_wait_list, cl_event* ev) CL_API_SUFFIX__VERSION_1_0
{
	auto mockEvent = OpenCL::Event{ { event_wait_list, event_wait_list + num_events_in_wait_list }, std::chrono::nanoseconds(1000 + rand() % 1000) };

	if (blocking_read)
	{
		mockEvent.Wait();
		return CL_SUCCESS;
	}

	MapType(command_queue).RegisterEvent(new OpenCL::Event{std::move(mockEvent)});
	return CL_SUCCESS;
}

cl_int CL_API_CALL clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
	auto& mem = MapType(memobj);
	if (mem.Release())
		delete& mem;
	return CL_SUCCESS;
}

cl_int CL_API_CALL clReleaseKernel(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0
{
	auto& k = MapType(kernel);
	if (k.Release())
		delete& k;
	return CL_SUCCESS;
}

cl_int CL_API_CALL clReleaseProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0
{
	auto& p = MapType(program);
	if (p.Release())
		delete& p;
	return CL_SUCCESS;
}

cl_int CL_API_CALL clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
	auto& queue = MapType(command_queue);
	if (queue.Release())
		delete& queue;
	return CL_SUCCESS;
}

cl_int CL_API_CALL clReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
	auto& ctx = MapType(context);
	if (ctx.Release())
		delete& ctx;
	return CL_SUCCESS;
}
