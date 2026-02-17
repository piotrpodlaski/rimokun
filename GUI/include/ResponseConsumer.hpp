#pragma once

#include "GuiCommand.hpp"

class ResponseConsumer {
 public:
  virtual ~ResponseConsumer() = default;
  virtual void processResponse(const GuiResponse& response) = 0;
};
