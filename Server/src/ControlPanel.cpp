#include <ControlPanel.hpp>
#include <ExceptionUtils.hpp>
#include <ControlPanelCommFactory.hpp>
#include <Config.hpp>
#include <Logger.hpp>
#include <TimingMetrics.hpp>

#include <sstream>
#include <stdexcept>
#include <utility>

ControlPanel::ControlPanel(std::unique_ptr<IControlPanelComm> comm,
                           const std::size_t movingAverageDepth,
                           const std::size_t baselineSamples,
                           const std::size_t buttonDebounceSamples)
    : _comm(std::move(comm)),
      _movingAverageDepth(std::max<std::size_t>(1u, movingAverageDepth)),
      _baselineSamples(std::max<std::size_t>(1u, baselineSamples)),
      _buttonDebounceSamples(std::max<std::size_t>(1u, buttonDebounceSamples)) {
  if (!_comm) {
    utl::throwRuntimeError("ControlPanel communication backend is null.");
  }
  resetSignalProcessingState();
}

ControlPanel::ControlPanel() {
  auto& cfg = utl::Config::instance();
  const auto controlPanelCfg = cfg.getClassConfig("ControlPanel");

  auto commCfg = controlPanelCfg["comm"];
  if (!commCfg || !commCfg.IsMap()) {
    SPDLOG_WARN(
        "ControlPanel config does not define 'comm' object. Falling back to "
        "legacy top-level serial keys.");
    commCfg = YAML::Node(YAML::NodeType::Map);
    const auto legacyTransport = controlPanelCfg["transport"];
    commCfg["type"] =
        legacyTransport ? legacyTransport.as<std::string>() : "serial";
    auto serialCfg = YAML::Node(YAML::NodeType::Map);
    auto copyLegacy = [&](const char* key) {
      const auto n = controlPanelCfg[key];
      if (n) serialCfg[key] = n;
    };
    copyLegacy("port");
    copyLegacy("baudRate");
    copyLegacy("characterSize");
    copyLegacy("flowControl");
    copyLegacy("parity");
    copyLegacy("stopBits");
    copyLegacy("readTimeoutMS");
    copyLegacy("lineTerminator");
    commCfg["serial"] = serialCfg;
  }
  _comm = makeControlPanelComm(commCfg);

  const auto processingCfg = controlPanelCfg["processing"];
  auto getProcessingOrLegacy = [&](const char* key,
                                   const std::size_t fallback) -> std::size_t {
    if (processingCfg && processingCfg[key]) {
      return processingCfg[key].as<std::size_t>();
    }
    if (controlPanelCfg[key]) {
      return controlPanelCfg[key].as<std::size_t>();
    }
    return fallback;
  };

  _movingAverageDepth =
      std::max<std::size_t>(1u, getProcessingOrLegacy("movingAverageDepth", 5u));
  _baselineSamples =
      std::max<std::size_t>(1u, getProcessingOrLegacy("baselineSamples", 50u));
  _buttonDebounceSamples = std::max<std::size_t>(
      1u, getProcessingOrLegacy("buttonDebounceSamples", 3u));
  if (!_comm) {
    utl::throwRuntimeError("ControlPanel communication backend is null.");
  }
  resetSignalProcessingState();
}

ControlPanel::~ControlPanel() {
  try {
    reset();
  } catch (...) {
    SPDLOG_ERROR("Unexpected exception during ControlPanel destruction.");
  }
}

void ControlPanel::initialize() {
  reset();
  SPDLOG_INFO("Initializing ControlPanel communication via {}", _comm->describe());
  try {
    _comm->open();
    resetSignalProcessingState();
    _readerRunning = true;
    _readerThread = std::thread(&ControlPanel::readerLoop, this);
    setState(State::Normal);
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to initialize ControlPanel communication: {}",
                 e.what());
    _comm->closeNoThrow();
    setState(State::Error);
    throw;
  }
}

void ControlPanel::reset() {
  SPDLOG_INFO("Resetting ControlPanel component.");
  setState(State::Error);
  _readerRunning = false;
  if (_readerThread.joinable()) {
    _readerThread.join();
  }
  _comm->closeNoThrow();
}

void ControlPanel::readerLoop() {
  RIMO_TIMED_SCOPE("ControlPanel::readerLoop");
  auto sanitizeLine = [](std::string& line) {
    while (!line.empty() &&
           (line.back() == '\r' || line.back() == '\n' || line.back() == '\0')) {
      line.pop_back();
    }
    while (!line.empty() &&
           (line.front() == '\r' || line.front() == '\n' || line.front() == '\0')) {
      line.erase(line.begin());
    }
  };

  while (_readerRunning) {
    try {
      auto lineOpt = _comm->readLine();
      if (!lineOpt) {
        continue;
      }
      auto line = std::move(*lineOpt);
      if (!_readerRunning) {
        break;
      }
      sanitizeLine(line);
      if (line.empty()) {
        continue;
      }
      processLine(line);
    } catch (const std::exception& e) {
      SPDLOG_ERROR("ControlPanel communication read failed: {}", e.what());
      setState(State::Error);
      _readerRunning = false;
      _comm->closeNoThrow();
      break;
    }
  }
}

void ControlPanel::processLine(const std::string& line) {
  RIMO_TIMED_SCOPE("ControlPanel::processLine");
  std::array<std::string, 9> tokens{};
  std::istringstream iss(line);
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (!(iss >> tokens[i])) {
      SPDLOG_WARN("ControlPanel malformed line (expected 9 fields): '{}'", line);
      return;
    }
  }

  // Parse all 9 values first — only update processors if the whole line is valid.
  std::array<double, 3> xRaw{};
  std::array<double, 3> yRaw{};
  std::array<bool, 3> bRaw{};
  try {
    for (std::size_t i = 0; i < 3; ++i) {
      const int xv = std::stoi(tokens[3 * i]);
      const int yv = std::stoi(tokens[3 * i + 1]);
      const int bv = std::stoi(tokens[3 * i + 2]);
      if (xv < 0 || xv > 1023 || yv < 0 || yv > 1023 || (bv != 0 && bv != 1)) {
        SPDLOG_WARN("ControlPanel invalid values in line: '{}'", line);
        return;
      }
      xRaw[i] = static_cast<double>(xv);
      yRaw[i] = static_cast<double>(yv);
      bRaw[i] = (bv == 1);
    }
  } catch (const std::exception&) {
    SPDLOG_WARN("ControlPanel invalid numeric format in line: '{}'", line);
    return;
  }

  for (std::size_t i = 0; i < 3; ++i) {
    const bool wasReady = _processors[i].isBaselineReady();
    const auto out = _processors[i].process(xRaw[i], yRaw[i], bRaw[i]);
    if (!wasReady && _processors[i].isBaselineReady()) {
      SPDLOG_INFO("ControlPanel joystick[{}] baseline ready. x={:.3f} y={:.3f}",
                  i, out.x, out.y);
    }
    _x[i].store(out.x, std::memory_order_release);
    _y[i].store(out.y, std::memory_order_release);
    _b[i].store(out.button, std::memory_order_release);
  }
}

ControlPanel::Snapshot ControlPanel::getSnapshot() const {
  Snapshot s;
  for (std::size_t i = 0; i < 3; ++i) {
    s.x[i] = _x[i].load(std::memory_order_acquire);
    s.y[i] = _y[i].load(std::memory_order_acquire);
    s.b[i] = _b[i].load(std::memory_order_acquire);
  }
  return s;
}

void ControlPanel::resetSignalProcessingState() {
  for (std::size_t i = 0; i < 3; ++i) {
    _x[i].store(0.0, std::memory_order_release);
    _y[i].store(0.0, std::memory_order_release);
    _b[i].store(false, std::memory_order_release);
    _processors[i].reconfigure(_movingAverageDepth, _baselineSamples, _buttonDebounceSamples);
  }
}

