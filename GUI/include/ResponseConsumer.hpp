#pragma once

#include <yaml-cpp/yaml.h>

#include <chrono>

class ResponseConsumer {
 public:
  virtual ~ResponseConsumer() = default;
  virtual void processResponse(YAML::Node response) = 0;
};