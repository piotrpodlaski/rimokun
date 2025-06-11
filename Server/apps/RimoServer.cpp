#include "RimoServer.hpp"

#include <chrono>
#include <iostream>

#include "logger.hpp"

using namespace utl;

using namespace std::chrono_literals;

RobotStatus prepareFakeStatus() {
  static double t = 0;
  constexpr double dt = 0.05;
  RobotStatus status;
  const std::vector motors = {EMotor::XLeft,  EMotor::XRight, EMotor::YLeft,
                              EMotor::YRight, EMotor::ZLeft,  EMotor::ZRight};
  const std::map<EMotor, double> amplitudes = {
      {EMotor::XLeft, 100},  {EMotor::YLeft, 100}, {EMotor::XRight, 100},
      {EMotor::YRight, 100}, {EMotor::ZLeft, 100}, {EMotor::ZRight, 100}};

  const std::map<EMotor, double> offsets = {
      {EMotor::XLeft, 300},  {EMotor::YLeft, 100}, {EMotor::XRight, 700},
      {EMotor::YRight, 100}, {EMotor::ZLeft, 100}, {EMotor::ZRight, 100}};

  const std::map<EMotor, double> omegas = {
      {EMotor::XLeft, 2},  {EMotor::YLeft, 2}, {EMotor::XRight, 1},
      {EMotor::YRight, 1}, {EMotor::ZLeft, 1}, {EMotor::ZRight, 1}};

  const std::map<EMotor, double> phases = {
      {EMotor::XLeft, 0},  {EMotor::YLeft, 0}, {EMotor::XRight, 0},
      {EMotor::YRight, 0}, {EMotor::ZLeft, 0}, {EMotor::ZRight, 0}};

  for (const auto& motor : motors) {
    const auto A = amplitudes.at(motor);
    const auto omega = omegas.at(motor);
    const auto phase = phases.at(motor);
    const auto offset = offsets.at(motor);
    status.motors[motor] = {
        .currentPosition = A * std::sin(omega * t + phase) + offset,
        .targetPosition = 0,
        .speed = A * omega * std::cos(omega * t + phase),
        .torque = static_cast<int>(
            10 * std::fabs(omega * omega * std::sin(omega * t + phase)))};
  }
  t += dt;
  return status;
}

[[noreturn]] int main(int argc, char** argv) {
  configure_logger();
  std::cout << "Hello World!\n";
  auto srv = RimoServer();

  while (true) {
    srv.publish(prepareFakeStatus());
    std::this_thread::sleep_for(100ms);
  }
}