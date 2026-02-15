#pragma once

#include <memory>

#include <yaml-cpp/yaml.h>

#include "IControlPanelComm.hpp"

std::unique_ptr<IControlPanelComm> makeControlPanelComm(
    const YAML::Node& commConfig);
