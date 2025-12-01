#include "ModbusClient.hpp"
#include <iostream>

int main() {
  // Linux: "/dev/ttyM0"
  // Windows: "COM5"
  auto client_res = ModbusClient::rtu("/dev/ttyM0", 9600, 'N', 8, 1, /*slave id*/ 3);
  if (!client_res) {
    std::cerr << "Failed to create client: "
              << client_res.error().message << "\n";
    return 1;
  }

  auto client = std::move(*client_res);

  if (auto c = client.connect(); !c) {
    std::cerr << "Connect failed: " << c.error().message << "\n";
    return 1;
  }

  client.set_response_timeout(std::chrono::milliseconds{1000});

  if (auto regs = client.read_holding_registers(0, 10)) {
    std::cout << "Registers: ";
    for (auto v : *regs) {
      std::cout << v << ' ';
    }
    std::cout << '\n';
  } else {
    std::cerr << "Read failed: " << regs.error().message << "\n";
  }

  client.close();
}
