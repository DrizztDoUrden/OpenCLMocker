#include <OpenCLMocker/Environment.hpp>

namespace OpenCL
{
	std::map<std::string, EnvVariable> EnvVariable::values;

	DEFINE_ENV_VARIABLE(CLMOCKER_DUMP_BUFFERS_ROOT, std::filesystem::path, std::nullopt);
	DEFINE_ENV_VARIABLE(CLMOCKER_DUMP_BUFFERS_OP_FILTER, std::vector<std::string>, std::nullopt);
}
