#pragma once

#include "Machine.hpp"

class MachineRuntime {
 public:
  MachineRuntime() = default;

  void initialize();
  void shutdown();

 private:
  Machine _machine;
};
