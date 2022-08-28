#pragma once

#include <OpenCLMocker/ForbidCopy.hpp>

#include <initializer_list>
#include <vector>

template <class TValue>
class EnumType
{
	ForbidCopy(EnumType);
	DefaultMove(EnumType);

public:
	const std::vector<TValue>& GetValues() const { return values; }

protected:
	EnumType(std::initializer_list<TValue> values) : values(std::move(values)) {}

private:
	std::vector<TValue> values;
};

template <class TValue, class TDerived>
class Enum
{
public:
	virtual ~Enum() = default;

	virtual bool Validate() const
	{
		for (const auto& possible_value : type->GetValues())
			if (value == possible_value)
				return true;
		return false;
	}

	bool operator ==(TValue other) const { return value == other; }
	bool operator !=(TValue other) const { return !(*this == other); }

	const EnumType<TValue>* GetType() const { return type; }
	TValue GetValue() const { return value; }

protected:
	Enum(const EnumType<TValue>* type, TValue value) : type(type), value(value) {}

private:
	const EnumType<TValue>* type;
	TValue value;
};

template <class TValue, class TDerived>
class FlagsEnum : public Enum<TValue, TDerived>
{
public:
	TDerived operator|(TValue other) const { return this->GetValue() | other; }
	friend TDerived operator|(TValue other, const TDerived& self) { return self.GetValue() | other; }
	TDerived& operator|=(TValue other) { *this = *this | other; return static_cast<TDerived&>(*this); }
	TDerived operator|(const TDerived& other) const { return this->GetValue() | other; }
	TDerived& operator|=(const TDerived& other) { *this = *this | other; return static_cast<TDerived&>(*this); }

	bool HasFlags(TValue mask) const { return (this->GetValue() & mask) == mask; }
	bool HasAnyFlags(TValue mask) const { return (this->GetValue() & mask) != 0; }

protected:
	FlagsEnum(const EnumType<TValue>* type, TValue value) : Enum<TValue, TDerived>(type, value) {}
};
