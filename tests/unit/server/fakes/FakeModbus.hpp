#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fake_modbus {

enum class FailurePoint {
  NewRtu,
  NewTcp,
  Connect,
  SetSlave,
  SetResponseTimeout,
  ReadRegisters,
  ReadInputRegisters,
  WriteRegister,
  WriteRegisters,
  ReadBits,
  ReadInputBits,
  WriteBit,
  WriteBits,
};

struct WriteRecord {
  int slave{0};
  int addr{0};
  std::vector<std::uint16_t> values;
};

void reset();
void setHoldingRegister(int slave, int addr, std::uint16_t value);
[[nodiscard]] std::uint16_t getHoldingRegister(int slave, int addr);
void failNext(FailurePoint point, std::string message = "forced fake-modbus error");
[[nodiscard]] std::vector<WriteRecord> writes();

}  // namespace fake_modbus

