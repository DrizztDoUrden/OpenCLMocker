#pragma once

#define ForbidCopy(...) \
public: \
    __VA_ARGS__(__VA_ARGS__ &) = delete; \
    __VA_ARGS__& operator=(__VA_ARGS__ &) = delete

#define ForbidMove(...) \
public: \
    __VA_ARGS__(__VA_ARGS__ &&) = delete; \
    __VA_ARGS__& operator=(__VA_ARGS__ &&) = delete

#define DefaultCopy(...) \
public: \
    __VA_ARGS__(__VA_ARGS__ &) = default; \
    __VA_ARGS__& operator=(__VA_ARGS__ &) = default

#define DefaultMove(...) \
public: \
    __VA_ARGS__(__VA_ARGS__ &&) = default; \
    __VA_ARGS__& operator=(__VA_ARGS__ &&) = default
