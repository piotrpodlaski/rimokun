#pragma once

#include <map>

#include "CommonDefinitions.hpp"
#include "MachineComponent.hpp"

class MachineComponentService {
 public:
  explicit MachineComponentService(
      std::map<utl::ERobotComponent, MachineComponent*>& components);

  std::string reconnect(utl::ERobotComponent componentId) const;
  void initializeAll() const;

 private:
  std::map<utl::ERobotComponent, MachineComponent*>& _components;
};
