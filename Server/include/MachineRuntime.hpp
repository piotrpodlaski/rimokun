#pragma once

#include "Machine.hpp"

class MachineRuntime {
 public:
  MachineRuntime() = default;

  static void wireMachine(Machine& machine);
  void initialize();
  void shutdown();

 private:
  Machine _machine;
};
