#pragma once

#include <optional>
#include <string>

class IControlPanelComm {
 public:
  virtual ~IControlPanelComm() = default;

  virtual void open() = 0;
  virtual void closeNoThrow() noexcept = 0;
  [[nodiscard]] virtual std::optional<std::string> readLine() = 0;
  [[nodiscard]] virtual std::string describe() const = 0;
};
