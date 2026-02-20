#pragma once

#include <modbus.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <expected>
#include <format>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <TimingMetrics.hpp>

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

    return ModbusClient(ctx, TransportKind::RtuSerial);
  }

  // Factory for RTU-over-raw-TCP (e.g. serial device servers like Moxa NPort)
  static ModbusResult<ModbusClient> rtu_over_tcp(std::string_view host, int port,
                                                 int slave_id) {
    if (host.empty()) {
      return std::unexpected(ModbusError{EINVAL, "Host is empty"});
    }
    if (port <= 0 || port > 65535) {
      return std::unexpected(ModbusError{EINVAL, "Port must be in range 1..65535"});
    }
    if (slave_id < 0 || slave_id > 247) {
      return std::unexpected(ModbusError{EINVAL, "Slave id must be in range 0..247"});
    }
    return ModbusClient(RtuOverTcpContext{.host = std::string(host),
                                          .port = port,
                                          .slave_id = slave_id});
  }

  // Non-copyable, movable
  ModbusClient(const ModbusClient&) = delete;
  ModbusClient& operator=(const ModbusClient&) = delete;

  ModbusClient(ModbusClient&& other) noexcept
      : ctx_(other.ctx_),
        backend_(other.backend_),
        transport_kind_(other.transport_kind_),
        rtu_tcp_(std::move(other.rtu_tcp_)) {
    other.ctx_ = nullptr;
    other.backend_ = Backend::LibModbus;
    other.transport_kind_ = TransportKind::Tcp;
    inter_request_delay_ = other.inter_request_delay_;
    last_transaction_completion_ = other.last_transaction_completion_;
    has_last_transaction_completion_ = other.has_last_transaction_completion_;
    other.inter_request_delay_ = std::chrono::milliseconds{0};
    other.has_last_transaction_completion_ = false;
  }

  ModbusClient& operator=(ModbusClient&& other) noexcept {
    if (this != &other) {
      cleanup();
      ctx_ = other.ctx_;
      backend_ = other.backend_;
      transport_kind_ = other.transport_kind_;
      rtu_tcp_ = std::move(other.rtu_tcp_);
      other.ctx_ = nullptr;
      other.backend_ = Backend::LibModbus;
      other.transport_kind_ = TransportKind::Tcp;
      inter_request_delay_ = other.inter_request_delay_;
      last_transaction_completion_ = other.last_transaction_completion_;
      has_last_transaction_completion_ = other.has_last_transaction_completion_;
      other.inter_request_delay_ = std::chrono::milliseconds{0};
      other.has_last_transaction_completion_ = false;
    }
    return *this;
  }

  ~ModbusClient() { cleanup(); }

  // Connect / close
  ModbusResult<void> connect() {
    RIMO_TIMED_SCOPE("ModbusClient::connect");
    if (backend_ == Backend::RtuOverTcp) {
      return connect_rtu_over_tcp();
    }
    if (!ctx_) return std::unexpected(ModbusError{0, "Null context"});
    if (modbus_connect(ctx_) == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  void close() noexcept {
    if (backend_ == Backend::RtuOverTcp) {
      close_rtu_over_tcp();
      return;
    }
    if (ctx_) {
      modbus_close(ctx_);
    }
  }

  // C++ chrono timeout
  ModbusResult<void> set_response_timeout(std::chrono::milliseconds timeout) {
    RIMO_TIMED_SCOPE("ModbusClient::set_response_timeout");
    if (backend_ == Backend::RtuOverTcp) {
      return set_response_timeout_rtu_over_tcp(timeout);
    }
    const auto sec = static_cast<uint32_t>(timeout.count() / 1000);
    const auto usec = static_cast<uint32_t>((timeout.count() % 1000) * 1000);

    if (modbus_set_response_timeout(ctx_, sec, usec) == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  ModbusResult<void> set_connect_timeout(std::chrono::milliseconds timeout) {
    if (backend_ == Backend::RtuOverTcp) {
      if (!rtu_tcp_) {
        return std::unexpected(ModbusError{0, "Null RTU-over-TCP context"});
      }
      rtu_tcp_->connect_timeout = timeout;
      return {};
    }
    // No dedicated connect-timeout control for libmodbus contexts here.
    return {};
  }

  ModbusResult<void> set_inter_request_delay(std::chrono::milliseconds delay) {
    if (delay < std::chrono::milliseconds{0}) {
      return std::unexpected(ModbusError{EINVAL,
                                         "Inter-request delay cannot be negative"});
    }
    inter_request_delay_ = delay;
    return {};
  }

  ModbusResult<void> set_slave(int slave_id) {
    RIMO_TIMED_SCOPE("ModbusClient::set_slave");
    if (backend_ == Backend::RtuOverTcp) {
      if (!rtu_tcp_) {
        return std::unexpected(ModbusError{0, "Null RTU-over-TCP context"});
      }
      if (slave_id < 0 || slave_id > 247) {
        return std::unexpected(ModbusError{EINVAL,
                                           "Slave id must be in range 0..247"});
      }
      rtu_tcp_->slave_id = slave_id;
      return {};
    }
    if (!ctx_) {
      return std::unexpected(ModbusError{0, "Null context"});
    }
    if (modbus_set_slave(ctx_, slave_id) == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  // ---- Register operations ------------------------------------------------

  ModbusResult<std::vector<std::uint16_t>> read_holding_registers(int addr,
                                                                  int count) {
    RIMO_TIMED_SCOPE("ModbusClient::read_holding_registers");
    wait_inter_request_gap_if_needed();
    if (backend_ == Backend::RtuOverTcp) {
      auto res = read_registers_rtu_over_tcp(0x03, addr, count);
      mark_transaction_completed();
      return res;
    }
    std::vector<std::uint16_t> buffer(static_cast<std::size_t>(count));
    int rc = modbus_read_registers(ctx_, addr, count, buffer.data());
    mark_transaction_completed();
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    buffer.resize(static_cast<std::size_t>(rc));
    return buffer;
  }

  ModbusResult<std::vector<std::uint16_t>> read_input_registers(int addr,
                                                                int count) {
    RIMO_TIMED_SCOPE("ModbusClient::read_input_registers");
    wait_inter_request_gap_if_needed();
    if (backend_ == Backend::RtuOverTcp) {
      auto res = read_registers_rtu_over_tcp(0x04, addr, count);
      mark_transaction_completed();
      return res;
    }
    std::vector<std::uint16_t> buffer(static_cast<std::size_t>(count));
    int rc = modbus_read_input_registers(ctx_, addr, count, buffer.data());
    mark_transaction_completed();
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    buffer.resize(static_cast<std::size_t>(rc));
    return buffer;
  }

  ModbusResult<void> write_single_register(int addr, std::uint16_t value) {
    RIMO_TIMED_SCOPE("ModbusClient::write_single_register");
    wait_inter_request_gap_if_needed();
    if (backend_ == Backend::RtuOverTcp) {
      auto res = write_single_register_rtu_over_tcp(addr, value);
      mark_transaction_completed();
      return res;
    }
    int rc = modbus_write_register(ctx_, addr, value);
    mark_transaction_completed();
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  ModbusResult<void> write_multiple_registers(
      int addr, std::span<const std::uint16_t> values) {
    RIMO_TIMED_SCOPE("ModbusClient::write_multiple_registers");
    wait_inter_request_gap_if_needed();
    if (backend_ == Backend::RtuOverTcp) {
      auto res = write_multiple_registers_rtu_over_tcp(addr, values);
      mark_transaction_completed();
      return res;
    }
    // libmodbus needs non-const pointer
    auto* data = const_cast<std::uint16_t*>(values.data());
    int rc = modbus_write_registers(ctx_, addr, static_cast<int>(values.size()),
                                    data);
    mark_transaction_completed();
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  ModbusResult<std::vector<bool>> read_bits(int addr, int count) {
    RIMO_TIMED_SCOPE("ModbusClient::read_bits");
    wait_inter_request_gap_if_needed();
    if (backend_ == Backend::RtuOverTcp) {
      auto res = read_bits_rtu_over_tcp(0x01, addr, count);
      mark_transaction_completed();
      return res;
    }
    std::vector<uint8_t> raw(static_cast<std::size_t>(count));

    int rc = modbus_read_bits(ctx_, addr, count, raw.data());
    mark_transaction_completed();
    if (rc == -1) {
      return std::unexpected(last_error());
    }

    std::vector<bool> bits;
    bits.reserve(static_cast<std::size_t>(rc));

    for (int i = 0; i < rc; i++) {
      bits.push_back(raw[i] != 0);  // take only lowest bit
    }
    return bits;
  }

  ModbusResult<std::vector<bool>> read_input_bits(int addr, int count) {
    RIMO_TIMED_SCOPE("ModbusClient::read_input_bits");
    wait_inter_request_gap_if_needed();
    if (backend_ == Backend::RtuOverTcp) {
      auto res = read_bits_rtu_over_tcp(0x02, addr, count);
      mark_transaction_completed();
      return res;
    }
    std::vector<uint8_t> raw(static_cast<std::size_t>(count));

    int rc = modbus_read_input_bits(ctx_, addr, count, raw.data());
    mark_transaction_completed();
    if (rc == -1) {
      return std::unexpected(last_error());
    }

    std::vector<bool> bits;
    bits.reserve(static_cast<std::size_t>(rc));

    for (int i = 0; i < rc; i++) {
      bits.push_back(raw[i] != 0);  // take only lowest bit
    }

    return bits;
  }

  ModbusResult<void> write_bit(int addr, bool value) {
    RIMO_TIMED_SCOPE("ModbusClient::write_bit");
    wait_inter_request_gap_if_needed();
    if (backend_ == Backend::RtuOverTcp) {
      auto res = write_single_coil_rtu_over_tcp(addr, value);
      mark_transaction_completed();
      return res;
    }
    int rc = modbus_write_bit(ctx_, addr, value ? 1 : 0);
    mark_transaction_completed();
    if (rc == -1) {
      return std::unexpected(last_error());
    }
    return {};
  }

  ModbusResult<void> write_bits(int addr, const std::vector<bool>& values) {
    RIMO_TIMED_SCOPE("ModbusClient::write_bits");
    wait_inter_request_gap_if_needed();
    if (backend_ == Backend::RtuOverTcp) {
      auto res = write_multiple_bits_rtu_over_tcp(addr, values);
      mark_transaction_completed();
      return res;
    }
    // Copy bools into a buffer of uint8_t (0 or 1)
    std::vector<uint8_t> raw;
    raw.reserve(values.size());

    for (bool b : values) {
      raw.push_back(b ? 1 : 0);
    }

    int rc =
        modbus_write_bits(ctx_, addr, static_cast<int>(raw.size()), raw.data());
    mark_transaction_completed();

    if (rc == -1) {
      return std::unexpected(last_error());
    }

    return {};
  }

  // You can add coils/discrete input helpers similarly…

 private:
  enum class Backend {
    LibModbus,
    RtuOverTcp,
  };

  enum class TransportKind {
    Tcp,
    RtuSerial,
    RtuOverTcp,
  };

  struct RtuOverTcpContext {
    std::string host;
    int port{};
    int slave_id{};
    int fd{-1};
    std::chrono::milliseconds connect_timeout{100};
    std::chrono::milliseconds timeout{100};
  };

  explicit ModbusClient(modbus_t* ctx,
                        TransportKind transport = TransportKind::Tcp)
      : ctx_(ctx), transport_kind_(transport) {}
  explicit ModbusClient(RtuOverTcpContext ctx)
      : backend_(Backend::RtuOverTcp),
        transport_kind_(TransportKind::RtuOverTcp),
        rtu_tcp_(std::make_unique<RtuOverTcpContext>(std::move(ctx))) {}

  bool is_rtu_transport() const {
    return transport_kind_ == TransportKind::RtuSerial ||
           transport_kind_ == TransportKind::RtuOverTcp;
  }

  void wait_inter_request_gap_if_needed() {
    if (!is_rtu_transport() || inter_request_delay_ <= std::chrono::milliseconds{0} ||
        !has_last_transaction_completion_) {
      return;
    }
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_transaction_completion_);
    if (elapsed < inter_request_delay_) {
      std::this_thread::sleep_for(inter_request_delay_ - elapsed);
    }
  }

  void mark_transaction_completed() {
    if (!is_rtu_transport()) {
      return;
    }
    has_last_transaction_completion_ = true;
    last_transaction_completion_ = std::chrono::steady_clock::now();
  }

  void cleanup() noexcept {
    if (backend_ == Backend::RtuOverTcp) {
      close_rtu_over_tcp();
      rtu_tcp_.reset();
      return;
    }
    if (ctx_) {
      modbus_close(ctx_);
      modbus_free(ctx_);
      ctx_ = nullptr;
    }
  }

  static ModbusError last_error() {
    return ModbusError{errno, modbus_strerror(errno)};
  }

  static std::uint16_t crc16_modbus(const std::vector<std::uint8_t>& bytes) {
    std::uint16_t crc = 0xFFFFu;
    for (const auto b : bytes) {
      crc ^= b;
      for (int i = 0; i < 8; ++i) {
        const bool lsb = (crc & 0x0001u) != 0;
        crc >>= 1u;
        if (lsb) crc ^= 0xA001u;
      }
    }
    return crc;
  }

  static void append_crc(std::vector<std::uint8_t>& frame) {
    const auto crc = crc16_modbus(frame);
    frame.push_back(static_cast<std::uint8_t>(crc & 0xFFu));         // low
    frame.push_back(static_cast<std::uint8_t>((crc >> 8u) & 0xFFu)); // high
  }

  static bool validate_crc(const std::vector<std::uint8_t>& frame) {
    if (frame.size() < 3) return false;
    const std::vector<std::uint8_t> payload(frame.begin(), frame.end() - 2);
    const auto expected = crc16_modbus(payload);
    const auto received = static_cast<std::uint16_t>(frame[frame.size() - 2]) |
                          (static_cast<std::uint16_t>(frame[frame.size() - 1])
                           << 8u);
    return expected == received;
  }

  ModbusResult<void> connect_rtu_over_tcp() {
    if (!rtu_tcp_) {
      return std::unexpected(ModbusError{0, "Null RTU-over-TCP context"});
    }
    if (rtu_tcp_->fd >= 0) {
      return {};
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    const auto port = std::to_string(rtu_tcp_->port);
    const int gai = getaddrinfo(rtu_tcp_->host.c_str(), port.c_str(), &hints, &result);
    if (gai != 0) {
      return std::unexpected(
          ModbusError{EINVAL, std::string("getaddrinfo failed: ") + gai_strerror(gai)});
    }

    int fd = -1;
    ModbusError lastErr{ETIMEDOUT, "connection timed out"};
    for (auto* rp = result; rp != nullptr; rp = rp->ai_next) {
      fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (fd < 0) continue;
      auto cr = connect_with_timeout(fd, rp->ai_addr, rp->ai_addrlen,
                                     rtu_tcp_->connect_timeout);
      if (cr) {
        break;
      }
      lastErr = cr.error();
      ::close(fd);
      fd = -1;
    }
    freeaddrinfo(result);

    if (fd < 0) {
      return std::unexpected(lastErr);
    }

    const int one = 1;
    (void)::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    rtu_tcp_->fd = fd;
    return set_response_timeout_rtu_over_tcp(rtu_tcp_->timeout);
  }

  static ModbusResult<void> connect_with_timeout(
      const int fd, const sockaddr* addr, const socklen_t addr_len,
      const std::chrono::milliseconds timeout) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
      return std::unexpected(ModbusError{errno, std::strerror(errno)});
    }
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
      return std::unexpected(ModbusError{errno, std::strerror(errno)});
    }

    const int rc = ::connect(fd, addr, addr_len);
    if (rc == 0) {
      (void)::fcntl(fd, F_SETFL, flags);
      return {};
    }
    if (errno != EINPROGRESS) {
      const auto err = errno;
      (void)::fcntl(fd, F_SETFL, flags);
      return std::unexpected(ModbusError{err, std::strerror(err)});
    }

    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLOUT;
    const int waitMs = static_cast<int>(std::max<std::int64_t>(1, timeout.count()));
    const int prc = ::poll(&pfd, 1, waitMs);
    if (prc == 0) {
      (void)::fcntl(fd, F_SETFL, flags);
      return std::unexpected(ModbusError{ETIMEDOUT, "Connection timed out"});
    }
    if (prc < 0) {
      const auto err = errno;
      (void)::fcntl(fd, F_SETFL, flags);
      return std::unexpected(ModbusError{err, std::strerror(err)});
    }

    int so_error = 0;
    socklen_t so_len = sizeof(so_error);
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_len) != 0) {
      const auto err = errno;
      (void)::fcntl(fd, F_SETFL, flags);
      return std::unexpected(ModbusError{err, std::strerror(err)});
    }
    (void)::fcntl(fd, F_SETFL, flags);
    if (so_error != 0) {
      return std::unexpected(ModbusError{so_error, std::strerror(so_error)});
    }
    return {};
  }

  void close_rtu_over_tcp() noexcept {
    if (!rtu_tcp_) return;
    if (rtu_tcp_->fd >= 0) {
      ::close(rtu_tcp_->fd);
      rtu_tcp_->fd = -1;
    }
  }

  ModbusResult<void> set_response_timeout_rtu_over_tcp(
      const std::chrono::milliseconds timeout) {
    if (!rtu_tcp_) {
      return std::unexpected(ModbusError{0, "Null RTU-over-TCP context"});
    }
    rtu_tcp_->timeout = timeout;
    if (rtu_tcp_->fd < 0) return {};

    timeval tv{};
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);
    if (::setsockopt(rtu_tcp_->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) !=
            0 ||
        ::setsockopt(rtu_tcp_->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) !=
            0) {
      return std::unexpected(ModbusError{errno, std::strerror(errno)});
    }
    return {};
  }

  ModbusResult<void> write_all_rtu_over_tcp(
      const std::vector<std::uint8_t>& data) const {
    if (!rtu_tcp_ || rtu_tcp_->fd < 0) {
      return std::unexpected(ModbusError{ENOTCONN, "RTU-over-TCP is not connected"});
    }
    std::size_t sent = 0;
    while (sent < data.size()) {
      const auto rc = ::send(rtu_tcp_->fd, data.data() + sent, data.size() - sent, 0);
      if (rc <= 0) {
        return std::unexpected(ModbusError{errno ? errno : EIO, std::strerror(errno)});
      }
      sent += static_cast<std::size_t>(rc);
    }
    return {};
  }

  ModbusResult<std::vector<std::uint8_t>> read_exact_rtu_over_tcp(
      const std::size_t n) const {
    if (!rtu_tcp_ || rtu_tcp_->fd < 0) {
      return std::unexpected(ModbusError{ENOTCONN, "RTU-over-TCP is not connected"});
    }
    std::vector<std::uint8_t> out(n);
    std::size_t got = 0;
    while (got < n) {
      const auto rc = ::recv(rtu_tcp_->fd, out.data() + got, n - got, 0);
      if (rc <= 0) {
        return std::unexpected(ModbusError{errno ? errno : ETIMEDOUT,
                                           std::strerror(errno)});
      }
      got += static_cast<std::size_t>(rc);
    }
    return out;
  }

  ModbusResult<std::vector<std::uint8_t>> read_variable_response_rtu_over_tcp(
      const std::uint8_t expected_function) const {
    auto hdrRes = read_exact_rtu_over_tcp(3);
    if (!hdrRes) return std::unexpected(hdrRes.error());
    auto frame = std::move(*hdrRes);

    if (frame[0] != static_cast<std::uint8_t>(rtu_tcp_->slave_id)) {
      return std::unexpected(
          ModbusError{EIO, "Unexpected slave id in RTU-over-TCP response"});
    }

    const auto function = frame[1];
    if (function == static_cast<std::uint8_t>(expected_function | 0x80u)) {
      auto rest = read_exact_rtu_over_tcp(2);
      if (!rest) return std::unexpected(rest.error());
      frame.insert(frame.end(), rest->begin(), rest->end());
      if (!validate_crc(frame)) {
        return std::unexpected(ModbusError{EIO, "Invalid CRC in exception response"});
      }
      return std::unexpected(
          ModbusError{EIO, std::format("Modbus exception code 0x{:02X}", frame[2])});
    }
    if (function != expected_function) {
      return std::unexpected(
          ModbusError{EIO, "Unexpected function code in RTU-over-TCP response"});
    }

    const auto byteCount = frame[2];
    auto rest = read_exact_rtu_over_tcp(static_cast<std::size_t>(byteCount) + 2u);
    if (!rest) return std::unexpected(rest.error());
    frame.insert(frame.end(), rest->begin(), rest->end());
    if (!validate_crc(frame)) {
      return std::unexpected(ModbusError{EIO, "Invalid CRC in RTU-over-TCP response"});
    }
    return frame;
  }

  ModbusResult<std::vector<std::uint8_t>> exchange_fixed_response_rtu_over_tcp(
      const std::vector<std::uint8_t>& request, const std::uint8_t function,
      const std::size_t response_size = 8u) const {
    if (auto wr = write_all_rtu_over_tcp(request); !wr) {
      return std::unexpected(wr.error());
    }
    auto response = read_exact_rtu_over_tcp(response_size);
    if (!response) return std::unexpected(response.error());
    if ((*response)[0] != static_cast<std::uint8_t>(rtu_tcp_->slave_id) ||
        ((*response)[1] != function &&
         (*response)[1] != static_cast<std::uint8_t>(function | 0x80u))) {
      return std::unexpected(ModbusError{EIO, "Invalid RTU-over-TCP response header"});
    }
    if (!validate_crc(*response)) {
      return std::unexpected(ModbusError{EIO, "Invalid CRC in RTU-over-TCP response"});
    }
    if ((*response)[1] == static_cast<std::uint8_t>(function | 0x80u)) {
      return std::unexpected(
          ModbusError{EIO, std::format("Modbus exception code 0x{:02X}", (*response)[2])});
    }
    return response;
  }

  ModbusResult<std::vector<std::uint16_t>> read_registers_rtu_over_tcp(
      const std::uint8_t function, const int addr, const int count) const {
    if (count <= 0 || count > 125) {
      return std::unexpected(ModbusError{EINVAL, "Invalid register count"});
    }
    std::vector<std::uint8_t> request{
        static_cast<std::uint8_t>(rtu_tcp_->slave_id), function,
        static_cast<std::uint8_t>((addr >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(addr & 0xFFu),
        static_cast<std::uint8_t>((count >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(count & 0xFFu)};
    append_crc(request);
    if (auto wr = write_all_rtu_over_tcp(request); !wr) {
      return std::unexpected(wr.error());
    }
    auto frame = read_variable_response_rtu_over_tcp(function);
    if (!frame) return std::unexpected(frame.error());
    const auto byteCount = (*frame)[2];
    if (byteCount != static_cast<std::uint8_t>(count * 2)) {
      return std::unexpected(ModbusError{EIO, "Unexpected byte count in response"});
    }
    std::vector<std::uint16_t> out;
    out.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
      const auto hi = (*frame)[3 + i * 2];
      const auto lo = (*frame)[3 + i * 2 + 1];
      out.push_back(static_cast<std::uint16_t>((hi << 8u) | lo));
    }
    return out;
  }

  ModbusResult<void> write_single_register_rtu_over_tcp(const int addr,
                                                         const std::uint16_t value) const {
    std::vector<std::uint8_t> request{
        static_cast<std::uint8_t>(rtu_tcp_->slave_id), 0x06,
        static_cast<std::uint8_t>((addr >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(addr & 0xFFu),
        static_cast<std::uint8_t>((value >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(value & 0xFFu)};
    append_crc(request);
    auto response = exchange_fixed_response_rtu_over_tcp(request, 0x06);
    if (!response) return std::unexpected(response.error());
    return {};
  }

  ModbusResult<void> write_multiple_registers_rtu_over_tcp(
      const int addr, std::span<const std::uint16_t> values) const {
    if (values.empty() || values.size() > 123) {
      return std::unexpected(ModbusError{EINVAL, "Invalid number of registers"});
    }
    const auto count = static_cast<std::uint16_t>(values.size());
    const auto byteCount = static_cast<std::uint8_t>(count * 2u);
    std::vector<std::uint8_t> request{
        static_cast<std::uint8_t>(rtu_tcp_->slave_id), 0x10,
        static_cast<std::uint8_t>((addr >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(addr & 0xFFu),
        static_cast<std::uint8_t>((count >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(count & 0xFFu), byteCount};
    request.reserve(request.size() + values.size() * 2u + 2u);
    for (const auto v : values) {
      request.push_back(static_cast<std::uint8_t>((v >> 8u) & 0xFFu));
      request.push_back(static_cast<std::uint8_t>(v & 0xFFu));
    }
    append_crc(request);
    auto response = exchange_fixed_response_rtu_over_tcp(request, 0x10);
    if (!response) return std::unexpected(response.error());
    return {};
  }

  ModbusResult<std::vector<bool>> read_bits_rtu_over_tcp(const std::uint8_t function,
                                                          const int addr,
                                                          const int count) const {
    if (count <= 0 || count > 2000) {
      return std::unexpected(ModbusError{EINVAL, "Invalid bit count"});
    }
    std::vector<std::uint8_t> request{
        static_cast<std::uint8_t>(rtu_tcp_->slave_id), function,
        static_cast<std::uint8_t>((addr >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(addr & 0xFFu),
        static_cast<std::uint8_t>((count >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(count & 0xFFu)};
    append_crc(request);
    if (auto wr = write_all_rtu_over_tcp(request); !wr) {
      return std::unexpected(wr.error());
    }
    auto frame = read_variable_response_rtu_over_tcp(function);
    if (!frame) return std::unexpected(frame.error());
    std::vector<bool> out;
    out.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
      const auto byte = (*frame)[3 + (i / 8)];
      out.push_back(((byte >> (i % 8)) & 0x01u) != 0);
    }
    return out;
  }

  ModbusResult<void> write_single_coil_rtu_over_tcp(const int addr,
                                                     const bool value) const {
    const std::uint16_t raw = value ? 0xFF00u : 0x0000u;
    std::vector<std::uint8_t> request{
        static_cast<std::uint8_t>(rtu_tcp_->slave_id), 0x05,
        static_cast<std::uint8_t>((addr >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(addr & 0xFFu),
        static_cast<std::uint8_t>((raw >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(raw & 0xFFu)};
    append_crc(request);
    auto response = exchange_fixed_response_rtu_over_tcp(request, 0x05);
    if (!response) return std::unexpected(response.error());
    return {};
  }

  ModbusResult<void> write_multiple_bits_rtu_over_tcp(
      const int addr, const std::vector<bool>& values) const {
    if (values.empty() || values.size() > 1968) {
      return std::unexpected(ModbusError{EINVAL, "Invalid number of bits"});
    }
    const auto count = static_cast<std::uint16_t>(values.size());
    const auto byteCount = static_cast<std::uint8_t>((values.size() + 7u) / 8u);
    std::vector<std::uint8_t> request{
        static_cast<std::uint8_t>(rtu_tcp_->slave_id), 0x0F,
        static_cast<std::uint8_t>((addr >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(addr & 0xFFu),
        static_cast<std::uint8_t>((count >> 8u) & 0xFFu),
        static_cast<std::uint8_t>(count & 0xFFu), byteCount};
    request.resize(request.size() + byteCount, 0u);
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (values[i]) {
        request[7 + (i / 8u)] |= static_cast<std::uint8_t>(1u << (i % 8u));
      }
    }
    append_crc(request);
    auto response = exchange_fixed_response_rtu_over_tcp(request, 0x0F);
    if (!response) return std::unexpected(response.error());
    return {};
  }

  modbus_t* ctx_ = nullptr;
  Backend backend_{Backend::LibModbus};
  TransportKind transport_kind_{TransportKind::Tcp};
  std::unique_ptr<RtuOverTcpContext> rtu_tcp_;
  std::chrono::milliseconds inter_request_delay_{0};
  std::chrono::steady_clock::time_point last_transaction_completion_{};
  bool has_last_transaction_completion_{false};
};
