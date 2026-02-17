#include <ControlPanel.hpp>
#include <ControlPanelCommFactory.hpp>
#include <Config.hpp>
#include <Logger.hpp>

#include <algorithm>
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
    throw std::runtime_error("ControlPanel communication backend is null.");
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
    throw std::runtime_error("ControlPanel communication backend is null.");
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
  std::array<std::string, 9> tokens{};
  std::istringstream iss(line);
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (!(iss >> tokens[i])) {
      SPDLOG_WARN("ControlPanel malformed line (expected 9 fields): '{}'", line);
      return;
    }
  }

  std::array<double, 3> xRaw{};
  std::array<double, 3> yRaw{};
  std::array<bool, 3> b{};

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
      b[i] = (bv == 1);
    }
  } catch (const std::exception&) {
    SPDLOG_WARN("ControlPanel invalid numeric format in line: '{}'", line);
    return;
  }

  if (!_baselineReady) {
    for (std::size_t i = 0; i < 3; ++i) {
      _baselineXAcc[i] += xRaw[i];
      _baselineYAcc[i] += yRaw[i];
    }
    ++_baselineCount;
    if (_baselineCount >= _baselineSamples) {
      for (std::size_t i = 0; i < 3; ++i) {
        _baselineX[i] = _baselineXAcc[i] / static_cast<double>(_baselineCount);
        _baselineY[i] = _baselineYAcc[i] / static_cast<double>(_baselineCount);
      }
      _baselineReady = true;
      SPDLOG_INFO(
          "ControlPanel baseline ready after {} samples. "
          "X:[{:.3f}, {:.3f}, {:.3f}] Y:[{:.3f}, {:.3f}, {:.3f}]",
          _baselineCount, _baselineX[0], _baselineX[1], _baselineX[2],
          _baselineY[0], _baselineY[1], _baselineY[2]);
    }
  }

  std::array<double, 3> xFiltered{};
  std::array<double, 3> yFiltered{};
  for (std::size_t i = 0; i < 3; ++i) {
    _xWindow[i].push_back(xRaw[i]);
    _xWindowSum[i] += xRaw[i];
    if (_xWindow[i].size() > _movingAverageDepth) {
      _xWindowSum[i] -= _xWindow[i].front();
      _xWindow[i].pop_front();
    }

    _yWindow[i].push_back(yRaw[i]);
    _yWindowSum[i] += yRaw[i];
    if (_yWindow[i].size() > _movingAverageDepth) {
      _yWindowSum[i] -= _yWindow[i].front();
      _yWindow[i].pop_front();
    }

    xFiltered[i] = _xWindowSum[i] / static_cast<double>(_xWindow[i].size());
    yFiltered[i] = _yWindowSum[i] / static_cast<double>(_yWindow[i].size());

    double xOut = 0.0;
    double yOut = 0.0;
    if (_baselineReady) {
      xOut = clipToUnitRange((xFiltered[i] - _baselineX[i]) / 512.0);
      yOut = clipToUnitRange((yFiltered[i] - _baselineY[i]) / 512.0);
    }

    _x[i].store(xOut, std::memory_order_release);
    _y[i].store(yOut, std::memory_order_release);

    if (b[i] == _bStable[i]) {
      _bPending[i] = _bStable[i];
      _bPendingCount[i] = 0;
    } else {
      if (b[i] == _bPending[i]) {
        ++_bPendingCount[i];
      } else {
        _bPending[i] = b[i];
        _bPendingCount[i] = 1;
      }
      if (_bPendingCount[i] >= _buttonDebounceSamples) {
        _bStable[i] = _bPending[i];
        _bPendingCount[i] = 0;
      }
    }
    _b[i].store(_bStable[i], std::memory_order_release);
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
    _xWindow[i].clear();
    _yWindow[i].clear();
    _xWindowSum[i] = 0.0;
    _yWindowSum[i] = 0.0;
    _baselineX[i] = 512.0;
    _baselineY[i] = 512.0;
    _baselineXAcc[i] = 0.0;
    _baselineYAcc[i] = 0.0;
    _bStable[i] = false;
    _bPending[i] = false;
    _bPendingCount[i] = 0;
  }
  _baselineCount = 0;
  _baselineReady = false;
}

double ControlPanel::clipToUnitRange(double value) {
  return std::clamp(value, -1.0, 1.0);
}
