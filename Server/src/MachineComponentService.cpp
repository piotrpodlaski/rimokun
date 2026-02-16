#include "MachineComponentService.hpp"

#include <format>
#include <stdexcept>

#include <Logger.hpp>
#include <magic_enum/magic_enum.hpp>

MachineComponentService::MachineComponentService(
    std::map<utl::ERobotComponent, MachineComponent*>& components)
    : _components(components) {}

std::string MachineComponentService::reconnect(
    const utl::ERobotComponent componentId) const {
  const auto it = _components.find(componentId);
  if (it == _components.end() || !it->second) {
    return std::format("Resetting of '{}' is not implemented!",
                       magic_enum::enum_name(componentId));
  }
  auto* component = it->second;
  SPDLOG_INFO("Reconnecting {}...", magic_enum::enum_name(componentId));
  component->reset();
  try {
    component->initialize();
  } catch (const std::exception& e) {
    return std::format("Resetting '{}' failed: {}",
                       magic_enum::enum_name(componentId), e.what());
  }
  if (component->state() != MachineComponent::State::Normal) {
    return std::format("Resetting '{}' was unsuccessful!",
                       magic_enum::enum_name(componentId));
  }
  return "";
}

void MachineComponentService::initializeAll() const {
  for (const auto& [componentType, component] : _components) {
    if (!component) {
      continue;
    }
    try {
      component->initialize();
    } catch (const std::exception& e) {
      SPDLOG_ERROR("{} initialization failed: {}",
                   magic_enum::enum_name(componentType), e.what());
    }
  }
}
