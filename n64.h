#pragma once
#ifndef TKPEMU_N64_H
#define TKPEMU_N64_H
#include <bit>
// System must be little endian because of union use
static_assert(std::endian::native == std::endian::little);
#endif