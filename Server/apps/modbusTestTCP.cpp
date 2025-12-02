#include <iostream>
#include <thread>

#include "ModbusClient.hpp"

int main() {
  // Create a Modbus TCP client to host 192.168.1.50:502, unit/slave id = 1
  auto cli_res = ModbusClient::tcp("10.1.1.101", 502, 1);
  if (!cli_res) {
    std::cerr << "Failed to create TCP client: "
              << cli_res.error().message << "\n";
    return 1;
  }

  auto client = std::move(*cli_res);

  // Connect to the device
  if (auto ok = client.connect(); !ok) {
    std::cerr << "Connection failed: "
              << ok.error().message << "\n";
    return 1;
  }

  // Set timeout to 1 second
  client.set_response_timeout(std::chrono::milliseconds{1000});

  // ---------------------------
  // READ registers
  // ---------------------------
  // auto regs = client.read_holding_registers(0, 10);
  // if (!regs) {
  //   std::cerr << "Read failed: "
  //             << regs.error().message << "\n";
  //   return 1;
  // }

  client.write_bit(0, false);

  while (true) {
    auto regs = client.read_input_bits(0,16);

    std::cout << "Read OK, received " << regs->size() << " registers:\n";
    for (size_t i = 0; i < regs->size(); i++) {
      std::cout << "  reg[" << i << "] = " << regs->at(i) << "\n";
    }

    regs->erase(regs->begin(),regs->begin()+8);
    client.write_bits(0,*regs);
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  // Close the connection
  client.close();

  return 0;
}
