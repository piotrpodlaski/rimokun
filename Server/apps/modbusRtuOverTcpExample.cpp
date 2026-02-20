#include "ModbusClient.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
constexpr std::string_view kHost = "10.1.1.102";
constexpr int kPort = 4002;
constexpr int kSlaveId = 2;
constexpr auto kResponseTimeout = std::chrono::milliseconds{1000};
constexpr auto kConnectTimeout = std::chrono::milliseconds{1000};

void printU32Row(const std::string_view title, const int addr,
                 const std::vector<std::uint32_t>& values) {
  std::cout << title << " @0x" << std::hex << addr << std::dec << " (" << values.size()
            << " u32):";
  for (const auto value : values) {
    std::cout << ' ' << value;
  }
  std::cout << '\n';
}

ModbusResult<std::vector<std::uint32_t>> readU32Row(ModbusClient& client, const int addr,
                                                     const int count,
                                                     const bool inputRegs) {
  if (count < 1) {
    return std::unexpected(
        ModbusError{EINVAL, "u32 read count must be positive"});
  }
  const int registerCount = count * 2;
  ModbusResult<std::vector<std::uint16_t>> regs =
      inputRegs ? client.read_input_registers(addr, registerCount)
                : client.read_holding_registers(addr, registerCount);
  if (!regs) {
    return std::unexpected(regs.error());
  }

  std::vector<std::uint32_t> values;
  values.reserve(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    const auto hi = (*regs)[static_cast<std::size_t>(2 * i)];
    const auto lo = (*regs)[static_cast<std::size_t>(2 * i + 1)];
    values.push_back((static_cast<std::uint32_t>(hi) << 16) |
                     static_cast<std::uint32_t>(lo));
  }
  return values;
}

ModbusResult<void> writeU16(ModbusClient& client, const int addr,
                            const std::uint16_t value) {
  if (auto wr = client.write_single_register(addr, value); !wr) {
    return std::unexpected(wr.error());
  }
  return {};
}

ModbusResult<void> writeU32Row(ModbusClient& client, const int addr,
                               const std::vector<std::uint32_t>& values) {
  if (values.empty()) {
    return std::unexpected(ModbusError{EINVAL, "u32 write list must not be empty"});
  }
  std::vector<std::uint16_t> regs;
  regs.reserve(values.size() * 2);
  for (const auto value : values) {
    regs.push_back(static_cast<std::uint16_t>((value >> 16) & 0xFFFFu));
    regs.push_back(static_cast<std::uint16_t>(value & 0xFFFFu));
  }
  if (auto wr = client.write_multiple_registers(addr, regs); !wr) {
    return std::unexpected(wr.error());
  }
  return {};
}
}  // namespace

int main() {
  // Hardcoded RTU-over-TCP endpoint.
  auto clientRes = ModbusClient::rtu_over_tcp(kHost, kPort, kSlaveId);
  if (!clientRes) {
    std::cerr << "Failed to create RTU-over-TCP client: "
              << clientRes.error().message << "\n";
    return 1;
  }
  auto client = std::move(*clientRes);

  if (auto timeoutRes = client.set_connect_timeout(kConnectTimeout); !timeoutRes) {
    std::cerr << "Failed to set connect timeout: " << timeoutRes.error().message
              << "\n";
    return 1;
  }
  if (auto timeoutRes = client.set_response_timeout(kResponseTimeout); !timeoutRes) {
    std::cerr << "Failed to set response timeout: " << timeoutRes.error().message
              << "\n";
    return 1;
  }

  if (auto connectRes = client.connect(); !connectRes) {
    std::cerr << "Connect failed: " << connectRes.error().message << "\n";
    return 1;
  }

  // Placeholder tasks: replace register IDs/counts with target values.
  struct ReadTask {
    std::string_view name;
    int addr;
    int countU32;
    bool inputRegs;
  };
  struct WriteU16Task {
    std::string_view name;
    int addr;
    std::uint16_t value;
  };
  struct WriteU32Task {
    std::string_view name;
    int addr;
    std::vector<std::uint32_t> values;
  };

  const std::vector<ReadTask> tasks{
      {"NET-IN 0-7 ", 0x1160, 8, false},
      {"NET-IN 8-15", 0x1170, 8, false},
  };
  const std::vector<WriteU16Task> writeU16Tasks{
      // Replace with real writable register IDs and values.
  //    {"WriteU16Example", 0x007C, 0x0000},
  };
  const std::vector<WriteU32Task> writeU32Tasks{
      // Replace with real writable register IDs and values.
      {"Set C-ON", 0x116E, {17}},
      {"save to flash", 0x0192, {1}},
      {"save to flash", 0x0190, {1}},
  };


  int failures = 0;
  for (const auto& task : writeU16Tasks) {
    if (const auto result = writeU16(client, task.addr, task.value); !result) {
      ++failures;
      std::cerr << std::format(
          "{} write_single_register 0x{:04X}=0x{:04X} failed: {}\n", task.name,
          task.addr, task.value, result.error().message);
    } else {
      std::cout << std::format("{} wrote 0x{:04X}=0x{:04X}\n", task.name,
                               task.addr, task.value);
    }
  }

  for (const auto& task : writeU32Tasks) {
    if (const auto result = writeU32Row(client, task.addr, task.values); !result) {
      ++failures;
      std::cerr << std::format(
          "{} write_multiple_registers {} u32 at 0x{:04X} failed: {}\n",
          task.name, task.values.size(), task.addr, result.error().message);
    } else {
      std::cout << std::format("{} wrote {} u32 at 0x{:04X}\n", task.name,
                               task.values.size(), task.addr);
    }
  }

  for (const auto& task : tasks) {
    const auto values = readU32Row(client, task.addr, task.countU32, task.inputRegs);
    if (!values) {
      ++failures;
      std::cerr << std::format(
          "{} {} {} u32 from 0x{:04X} failed: {}\n", task.name,
          task.inputRegs ? "read_input_registers" : "read_holding_registers",
          task.countU32, task.addr, values.error().message);
      continue;
    }
    printU32Row(task.name, task.addr, *values);
  }

  client.close();
  if (failures > 0) {
    std::cerr << "Completed with " << failures << " failure(s)\n";
    return 2;
  }
  return 0;
}
