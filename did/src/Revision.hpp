#pragma once
#include <string>

#include "../buildnumber.hpp"
#define MAJOR 1
#define MINOR 0

static const std::string REVISION_STRING(std::to_string(MAJOR) + "." + std::to_string(MINOR) + "." + BUILDNUMBER_STR);