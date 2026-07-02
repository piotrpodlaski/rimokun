#pragma once

#include <optional>
#include <variant>

#include <QMetaType>

#include "CommonDefinitions.hpp"
#include "CommandInterface.hpp"
#include "nlohmann/json.hpp"

struct GuiRawCommand {
  nlohmann::json node;
};

template <typename Variant, typename Extra>
struct AppendVariantAlternative;

template <typename... Ts, typename Extra>
struct AppendVariantAlternative<std::variant<Ts...>, Extra> {
  using type = std::variant<Ts..., Extra>;
};

using GuiCommandPayload =
    AppendVariantAlternative<cmd::Payload, GuiRawCommand>::type;

struct GuiCommand {
  GuiCommandPayload payload;
};

struct GuiResponse {
  bool ok{false};
  std::string message;
  std::optional<nlohmann::json> payload;
};

Q_DECLARE_METATYPE(GuiCommand)
