#pragma once

#include <cstdint>

#define MapToCl(TFrom, TTo) \
    inline TFrom& MapType(TTo ptr) { return *reinterpret_cast<TFrom*>(ptr); } \
    inline TFrom*& MapType(TTo* ptr) { return *reinterpret_cast<TFrom**>(ptr); } \
    inline TTo MapType(TFrom& ref) { return reinterpret_cast<TTo>(&ref); } \
    inline TTo MapType(TFrom* ref) { return reinterpret_cast<TTo>(ref); }
