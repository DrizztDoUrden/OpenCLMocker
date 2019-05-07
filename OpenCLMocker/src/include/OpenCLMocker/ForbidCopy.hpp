#pragma once

#define ForbidCopy(TType) \
public: \
    TType(TType&) = delete; \
    TType& operator=(TType&) = delete; \
    TType(TType&&) = default; \
    TType& operator=(TType&&) = default
