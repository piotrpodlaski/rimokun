#pragma once

#include "GuiCommand.hpp"

class ResponseConsumer {
 public:
  virtual ~ResponseConsumer() = default;
  virtual bool suppressGlobalErrorPopup() const { return false; }
  virtual void processResponse(const GuiResponse& response) = 0;
};
