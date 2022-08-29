#pragma once

#include <CL/cl.h>

#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace OpenCL
{

	class Exception : public std::exception
	{
	public:
		Exception(cl_int status, std::string description = "", std::vector<uint8_t> miscData = {})
			: status(status)
			, description(std::move(description))
			, miscData(std::move(miscData))
		{
		}

		cl_int GetStatus() const { return status; }
		const std::string& GetDescription() const { return description; }
		const std::vector<uint8_t>& GetMiscData() const { return miscData; }

		const char* what() const noexcept override { return description.c_str(); }

	private:
		cl_int status;
		std::string description;
		std::vector<uint8_t> miscData;
	};

}
