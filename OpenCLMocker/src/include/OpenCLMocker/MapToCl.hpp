#pragma once

#include <OpenCLMocker/Object.hpp>

#include <atomic>
#include <cstdint>
#include <memory>

namespace OpenCL
{
	struct ObjectHandle
	{
		std::unique_ptr<Object> ptr;
		std::atomic<int> useCount = 1;

		inline ObjectHandle(std::unique_ptr<Object> ptr_)
			: ptr(std::move(ptr_))
		{
		}
	};
}

#define MapToCl(TFrom, TTo) \
	inline OpenCL::ObjectHandle& MapHandle(TTo ptr) { return *reinterpret_cast<OpenCL::ObjectHandle*>(ptr); } \
	inline OpenCL::ObjectHandle*& MapHandle(TTo* ptr) { return *reinterpret_cast<OpenCL::ObjectHandle**>(ptr); } \
	inline TFrom& MapType(TTo ptr) { return *static_cast<TFrom*>(reinterpret_cast<OpenCL::ObjectHandle*>(ptr)->ptr.get()); } \
	inline TTo MakeHandle(std::unique_ptr<TFrom> ptr) \
	{ \
		auto weakPtr = ptr.get();\
		weakPtr->handle = new OpenCL::ObjectHandle{std::move(ptr)}; \
		return reinterpret_cast<TTo>(weakPtr->handle); \
	} \
	inline TTo MakeHandle(TFrom ref) \
	{ \
		return MakeHandle(std::make_unique<TFrom>(std::move(ref))); \
	} \
	inline TTo MakeHandle(TFrom* ref) \
	{ \
		ref->handle = new OpenCL::ObjectHandle{std::unique_ptr<TFrom>{ref}}; \
		return reinterpret_cast<TTo>(ref->handle); \
	}

#define DirectMapToCl(TFrom, TTo) \
	inline TFrom& MapType(TTo ptr) { return *reinterpret_cast<TFrom*>(ptr); } \
	inline TFrom*& MapType(TTo* ptr) { return *reinterpret_cast<TFrom**>(ptr); } \
	inline TTo MapType(TFrom& ref) { return reinterpret_cast<TTo>(&ref); } \
	inline TTo MapType(TFrom* ref) { return reinterpret_cast<TTo>(ref); }
