#include "FakeModbus.hpp"

#include <modbus.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct FakeContext {
  int slave{1};
  bool connected{false};
  uint32_t timeoutSec{0};
  uint32_t timeoutUsec{0};
};

struct BackendState {
  std::mutex mutex;
  std::unordered_map<int, std::unordered_map<int, std::uint16_t>> holdingBySlave;
  std::vector<fake_modbus::WriteRecord> writeHistory;
  std::unordered_map<fake_modbus::FailurePoint, std::string> failNextMessage;
  std::string lastError{"fake-modbus error"};
};

BackendState& state() {
  static BackendState s;
  return s;
}

bool consumeFailure(const fake_modbus::FailurePoint point, int err = EIO) {
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  const auto it = st.failNextMessage.find(point);
  if (it == st.failNextMessage.end()) {
    return false;
  }
  st.lastError = it->second;
  st.failNextMessage.erase(it);
  errno = err;
  return true;
}

FakeContext* asCtx(modbus_t* ctx) {
  return reinterpret_cast<FakeContext*>(ctx);
}

const FakeContext* asCtx(const modbus_t* ctx) {
  return reinterpret_cast<const FakeContext*>(ctx);
}

}  // namespace

namespace fake_modbus {

void reset() {
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  st.holdingBySlave.clear();
  st.writeHistory.clear();
  st.failNextMessage.clear();
  st.lastError = "fake-modbus error";
}

void setHoldingRegister(const int slave, const int addr, const std::uint16_t value) {
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  st.holdingBySlave[slave][addr] = value;
}

std::uint16_t getHoldingRegister(const int slave, const int addr) {
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  const auto slaveIt = st.holdingBySlave.find(slave);
  if (slaveIt == st.holdingBySlave.end()) {
    return 0;
  }
  const auto regIt = slaveIt->second.find(addr);
  if (regIt == slaveIt->second.end()) {
    return 0;
  }
  return regIt->second;
}

void failNext(const FailurePoint point, std::string message) {
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  st.failNextMessage[point] = std::move(message);
}

std::vector<WriteRecord> writes() {
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  return st.writeHistory;
}

}  // namespace fake_modbus

extern "C" {

modbus_t* modbus_new_tcp(const char*, int) {
  if (consumeFailure(fake_modbus::FailurePoint::NewTcp)) {
    return nullptr;
  }
  auto* ctx = new FakeContext();
  return reinterpret_cast<modbus_t*>(ctx);
}

modbus_t* modbus_new_rtu(const char*, int, char, int, int) {
  if (consumeFailure(fake_modbus::FailurePoint::NewRtu)) {
    return nullptr;
  }
  auto* ctx = new FakeContext();
  return reinterpret_cast<modbus_t*>(ctx);
}

int modbus_set_slave(modbus_t* ctx, const int slave) {
  if (consumeFailure(fake_modbus::FailurePoint::SetSlave)) {
    return -1;
  }
  asCtx(ctx)->slave = slave;
  return 0;
}

int modbus_connect(modbus_t* ctx) {
  if (consumeFailure(fake_modbus::FailurePoint::Connect)) {
    return -1;
  }
  asCtx(ctx)->connected = true;
  return 0;
}

void modbus_close(modbus_t* ctx) {
  if (ctx != nullptr) {
    asCtx(ctx)->connected = false;
  }
}

void modbus_free(modbus_t* ctx) {
  delete asCtx(ctx);
}

const char* modbus_strerror(int) {
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  return st.lastError.c_str();
}

int modbus_set_response_timeout(modbus_t* ctx, const uint32_t to_sec,
                                const uint32_t to_usec) {
  if (consumeFailure(fake_modbus::FailurePoint::SetResponseTimeout)) {
    return -1;
  }
  asCtx(ctx)->timeoutSec = to_sec;
  asCtx(ctx)->timeoutUsec = to_usec;
  return 0;
}

int modbus_read_registers(modbus_t* ctx, const int addr, const int nb,
                          uint16_t* dest) {
  if (consumeFailure(fake_modbus::FailurePoint::ReadRegisters)) {
    return -1;
  }
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  auto& regs = st.holdingBySlave[asCtx(ctx)->slave];
  for (int i = 0; i < nb; ++i) {
    const auto it = regs.find(addr + i);
    dest[i] = (it == regs.end()) ? 0 : it->second;
  }
  return nb;
}

int modbus_read_input_registers(modbus_t* ctx, const int addr, const int nb,
                                uint16_t* dest) {
  if (consumeFailure(fake_modbus::FailurePoint::ReadInputRegisters)) {
    return -1;
  }
  return modbus_read_registers(ctx, addr, nb, dest);
}

int modbus_write_register(modbus_t* ctx, const int reg_addr,
                          const uint16_t value) {
  if (consumeFailure(fake_modbus::FailurePoint::WriteRegister)) {
    return -1;
  }
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  st.holdingBySlave[asCtx(ctx)->slave][reg_addr] = value;
  st.writeHistory.push_back(fake_modbus::WriteRecord{
      .slave = asCtx(ctx)->slave, .addr = reg_addr, .values = {value}});
  return 1;
}

int modbus_write_registers(modbus_t* ctx, const int addr, const int nb,
                           const uint16_t* data) {
  if (consumeFailure(fake_modbus::FailurePoint::WriteRegisters)) {
    return -1;
  }
  auto& st = state();
  std::lock_guard<std::mutex> lock(st.mutex);
  auto& regs = st.holdingBySlave[asCtx(ctx)->slave];
  std::vector<std::uint16_t> values;
  values.reserve(static_cast<std::size_t>(nb));
  for (int i = 0; i < nb; ++i) {
    regs[addr + i] = data[i];
    values.push_back(data[i]);
  }
  st.writeHistory.push_back(fake_modbus::WriteRecord{
      .slave = asCtx(ctx)->slave, .addr = addr, .values = std::move(values)});
  return nb;
}

int modbus_read_bits(modbus_t*, int, int nb, uint8_t* dest) {
  if (consumeFailure(fake_modbus::FailurePoint::ReadBits)) {
    return -1;
  }
  std::fill(dest, dest + nb, 0);
  return nb;
}

int modbus_read_input_bits(modbus_t*, int, int nb, uint8_t* dest) {
  if (consumeFailure(fake_modbus::FailurePoint::ReadInputBits)) {
    return -1;
  }
  std::fill(dest, dest + nb, 0);
  return nb;
}

int modbus_write_bit(modbus_t*, int, int) {
  if (consumeFailure(fake_modbus::FailurePoint::WriteBit)) {
    return -1;
  }
  return 1;
}

int modbus_write_bits(modbus_t*, int, int nb, const uint8_t*) {
  if (consumeFailure(fake_modbus::FailurePoint::WriteBits)) {
    return -1;
  }
  return nb;
}

}  // extern "C"

