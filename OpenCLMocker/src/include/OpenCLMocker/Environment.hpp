#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>

#include <cstdlib>
#include <filesystem>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace OpenCL
{
	class EnvVariable
	{
		ForbidCopy(EnvVariable);
		DefaultMove(EnvVariable);

	public:
		static const std::optional<std::string>& Get(const std::string& name)
		{
			const auto found = values.find(name);

			if (found != values.end())
				return found->second.value;

			auto var = EnvVariable{name};
			return values.emplace(name, std::move(var)).first->second.value;
		}

	private:
		inline EnvVariable(const std::string& name)
		{
			const auto envValue = std::getenv(name.c_str());

			if (envValue == nullptr)
			{
				value = std::nullopt;
				return;
			}

			value = envValue;
		}

		static std::map<std::string, EnvVariable> values;
		std::optional<std::string> value;
	};

	template <class TType>
	constexpr bool IsVector = false;

	template <class TElement>
	constexpr bool IsVector<std::vector<TElement>> = true;

	template <class TValue>
	class EnvVariableDefinition
	{
		ForbidCopy(EnvVariableDefinition);
		ForbidMove(EnvVariableDefinition);

	public:
		inline EnvVariableDefinition(const char* name, std::optional<TValue>&& default)
		{
			const auto envValue = EnvVariable::Get(name);

			if (!envValue.has_value())
			{
				value = std::forward(default);
				return;
			}

			if constexpr (std::is_same_v<std::string, TValue>)
			{
				value = *envValue;
			}
			else
			{
				auto tmp = TValue{};
				auto ss = std::istringstream{envValue};

				if constexpr (IsVector<TValue>)
				{
					using TElement = typename TValue::value_type;
					auto part = std::string{};

					while (std::getline(ss, part, ','))
					{
						auto element = TElement{};
						std::istringstream{std::move(part)} >> element;
						tmp.emplace_back(std::move(element));
					}
				}
				else
				{
					ss >> tmp;
				}

				value = std::move(tmp);
			}
		}

		inline bool HasValue() const
		{
			return value.has_value();
		}

		const TValue& operator const TValue&() const
		{
			return *value;
		}

	private:
		std::optional<TValue> value;
	};

#define DECLARE_ENV_VARIABLE(NAME, TYPE) \
	const EnvVariableDefinition<TYPE>& NAME()

#define DEFINE_ENV_VARIABLE(NAME, TYPE, DEFAULT) \
	const EnvVariableDefinition<TYPE>& NAME() \
	{ \
		static auto instance = EnvVariableDefinition<TYPE>{#NAME, DEFAULT};\
		return instance;\
	}

	DECLARE_ENV_VARIABLE(CLMOCKER_DUMP_BUFFERS_ROOT, std::filesystem::path);
	DECLARE_ENV_VARIABLE(CLMOCKER_DUMP_BUFFERS_OP_FILTER, std::vector<std::string>);
}
