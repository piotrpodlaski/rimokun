#include "ControlPanelCommFactory.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <stdexcept>

#include "SerialControlPanelComm.hpp"

namespace {
std::string toLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
}  // namespace

std::unique_ptr<IControlPanelComm> makeControlPanelComm(
    const YAML::Node& commConfig) {
  const auto transportNode = commConfig["type"];
  if (!transportNode || !transportNode.IsScalar()) {
    throw std::runtime_error(
        "ControlPanel comm config must define scalar 'type'.");
  }
  const auto transport = transportNode.as<std::string>();
  const auto normalized = toLower(transport);
  if (normalized == "serial") {
    return std::make_unique<SerialControlPanelComm>(commConfig);
  }
  if (normalized == "tcp" || normalized == "socket" ||
      normalized == "tcpsocket") {
    throw std::runtime_error(
        "ControlPanel transport 'tcp' is not implemented yet.");
  }
  throw std::runtime_error(
      std::format("Unsupported ControlPanel transport '{}'", transport));
}
