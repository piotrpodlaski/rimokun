#pragma once

#include <magic_enum/magic_enum.hpp>
#include <string>

namespace utl {
enum class EMotor { XLeft, XRight, YLeft, YRight, ZLeft, ZRight };

enum class ELEDState {On, Off, Error};

std::string getMotorName(EMotor em);
EMotor getMotorType(std::string name);
}  // namespace utl