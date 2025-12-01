#pragma once

#include <modbus.h>

#include <chrono>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct ModbusError {
  int errno_value{};
  std::string message{};
};

template <typename T>
using ModbusResult = std::expected<T, ModbusError>;

class ModbusClient {
 public:
  // Factory for TCP
  static ModbusResult<ModbusClient> tcp(std::string_view host, int port,
                                        int slave_id) {
    modbus_t* ctx = modbus_new_tcp(host.data(), port);
    if (!ctx) {
      return std::unexpected(ModbusError{errno, "modbus_new_tcp failed"});
    }

    if (modbus_set_slave(ctx, slave_id) == -1) {
      ModbusError err{errno, modbus_strerror(errno)};
      modbus_free(ctx);
      return std::unexpected(err);
    }

    return ModbusClient(ctx);
  }

  // Factory for RTU (/dev/ttyM0, COM5, etc.)
  static ModbusResult<ModbusClient> rtu(std::string_view device, int baud,
                                        char parity, int data_bits,
                                        int stop_bits, int slave_id) {
    modbus_t* ctx =
        modbus_new_rtu(device.data(), baud, parity, data_bits, stop_bits);
    if (!ctx) {
      return std::unexpected(ModbusError{errno, "modbus_new_rtu failed"});
    }

    if (modbus_set_slave(ctx, slave_id) == -1) {
      ModbusError err{errno, modbus_strerror(errno)};
      modbus_free(ctx);
      return std::unexpected(err);
    }

    return ModbusClient(ctx);
  }

  // Non-copyable, movable
  ModbusClient(const ModbusClient&) = delete;
  ModbusClient& operator=(const ModbusClient&) = delete;

  ModbusClient(ModbusClient&& other) noexcept : ctx_(other.ctx_) {
    other.ctx_ = nullptr;
  }

  ModbusClient& operator=(ModbusClient&& other) noexcept {
    if (this != &other) {
      cleanup();
      ctx_ = other.ctx_;
      other.ctx_ = nullptr;
    }
    return *this;
  }

  ~ModbusClient() { cleanup(); }

  // Connect / close
  ModbusResult<void> connect() {
    if (!ctx_) {
      return std::unexpected(ModbusError{0, "Null context"});
    }
    if (modbus_connect(ctx_) == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  void close() const noexcept {
    if (ctx_) {
      modbus_close(ctx_);
    }
  }

  // C++ chrono timeout
  ModbusResult<void> set_response_timeout(std::chrono::milliseconds timeout) {
    const auto sec = static_cast<uint32_t>(timeout.count() / 1000);
    const auto usec = static_cast<uint32_t>((timeout.count() % 1000) * 1000);


    if (modbus_set_response_timeout(ctx_, sec, usec) == -1) {
      return std::unexpected(last_error());
    }
    return{};
  }

  // ---- Register operations ------------------------------------------------

  ModbusResult<std::vector<std::uint16_t>> read_holding_registers(int addr,
                                                                  int count) {
    std::vector<std::uint16_t> buffer(static_cast<std::size_t>(count));
    int rc = modbus_read_registers(ctx_, addr, count, buffer.data());
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    buffer.resize(static_cast<std::size_t>(rc));
    return buffer;
  }

  ModbusResult<std::vector<std::uint16_t>> read_input_registers(int addr,
                                                                int count) {
    std::vector<std::uint16_t> buffer(static_cast<std::size_t>(count));
    int rc = modbus_read_input_registers(ctx_, addr, count, buffer.data());
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    buffer.resize(static_cast<std::size_t>(rc));
    return buffer;
  }

  ModbusResult<void> write_single_register(int addr, std::uint16_t value) {
    int rc = modbus_write_register(ctx_, addr, value);
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  ModbusResult<void> write_multiple_registers(
      int addr, std::span<const std::uint16_t> values) {
    // libmodbus needs non-const pointer
    auto* data = const_cast<std::uint16_t*>(values.data());
    int rc = modbus_write_registers(ctx_, addr, static_cast<int>(values.size()),
                                    data);
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  // You can add coils/discrete input helpers similarly…

 private:
  explicit ModbusClient(modbus_t* ctx) : ctx_(ctx) {}

  void cleanup() noexcept {
    if (ctx_) {
      modbus_close(ctx_);
      modbus_free(ctx_);
      ctx_ = nullptr;
    }
  }

  static ModbusError last_error() {
    return ModbusError{errno, modbus_strerror(errno)};
  }

  modbus_t* ctx_ = nullptr;
};
