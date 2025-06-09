#pragma once

#include <magic_enum/magic_enum.hpp>
#include <string>

namespace utl {
enum class EMotor { XLeft, XRight, YLeft, YRight, ZLeft, ZRight };

std::string getMotorName(EMotor em);
EMotor getMotorType(std::string name);
}  // namespace utl