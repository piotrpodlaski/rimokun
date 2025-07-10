#pragma once
#include <cxxabi.h>
#include <memory>

namespace utl {

template <typename T>
std::string getClassName([[maybe_unused]] const T* ptr) {
  const char* mangled = typeid(*ptr).name();

  int status = 0;
  const std::unique_ptr<char, void (*)(void*)> demangled(
      abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);

  return (status == 0 && demangled) ? demangled.get() : mangled;
}
}  // namespace utl