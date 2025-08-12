#pragma once

#include <klibcpp/cstdint.hpp>

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) (((N) / (S)) * (S))

extern int32_t abs(int32_t in);
extern float pow(float x, int y);