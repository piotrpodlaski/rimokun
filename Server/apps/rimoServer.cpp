#include <iostream>
#include "RimoServer.h"
#include <chrono>

int main(int argc, char** argv) {
  std::cout << "Hello World!\n";
  auto srv = utl::RimoServer();
  srv.spawn();
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}