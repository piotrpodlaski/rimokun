//
// Created by piotrek on 6/9/25.
//

#include "CommonDefinitions.hpp"

#include "EnumStrings.hpp"

namespace utl {
std::string getMotorName(const EMotor em) { return enumToString(em); }

EMotor getMotorType(const std::string name) { return stringToEnum<EMotor>(name); }
}  // namespace utl
