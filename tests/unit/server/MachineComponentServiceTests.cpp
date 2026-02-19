#include <gtest/gtest.h>

#include <MachineComponentService.hpp>

#include <stdexcept>

namespace {
class FakeComponent final : public MachineComponent {
 public:
  explicit FakeComponent(const utl::ERobotComponent id,
                         const bool throwOnInitialize = false,
                         const State initializeFinalState = State::Normal)
      : _id(id),
        _throwOnInitialize(throwOnInitialize),
        _initializeFinalState(initializeFinalState) {
    setState(State::Error);
  }

  void initialize() override {
    ++initializeCalls;
    if (_throwOnInitialize) {
      throw std::runtime_error("init failed");
    }
    setState(_initializeFinalState);
  }

  void reset() override {
    ++resetCalls;
    setState(State::Error);
  }

  [[nodiscard]] utl::ERobotComponent componentType() const override {
    return _id;
  }

  int initializeCalls{0};
  int resetCalls{0};

 private:
  utl::ERobotComponent _id;
  bool _throwOnInitialize;
  State _initializeFinalState;
};
}  // namespace

TEST(MachineComponentServiceTests, ReconnectReturnsEmptyOnSuccess) {
  FakeComponent component(utl::ERobotComponent::ControlPanel);
  std::map<utl::ERobotComponent, MachineComponent*> components{
      {utl::ERobotComponent::ControlPanel, &component}};
  MachineComponentService service(components);

  const auto result = service.reconnect(utl::ERobotComponent::ControlPanel);

  EXPECT_TRUE(result.empty());
  EXPECT_EQ(component.resetCalls, 1);
  EXPECT_EQ(component.initializeCalls, 1);
  EXPECT_EQ(component.state(), MachineComponent::State::Normal);
}

TEST(MachineComponentServiceTests, ReconnectReturnsEmptyOnWarningState) {
  FakeComponent component(utl::ERobotComponent::ControlPanel, false,
                          MachineComponent::State::Warning);
  std::map<utl::ERobotComponent, MachineComponent*> components{
      {utl::ERobotComponent::ControlPanel, &component}};
  MachineComponentService service(components);

  const auto result = service.reconnect(utl::ERobotComponent::ControlPanel);

  EXPECT_TRUE(result.empty());
  EXPECT_EQ(component.resetCalls, 1);
  EXPECT_EQ(component.initializeCalls, 1);
  EXPECT_EQ(component.state(), MachineComponent::State::Warning);
}

TEST(MachineComponentServiceTests, ReconnectUnknownComponentReturnsError) {
  std::map<utl::ERobotComponent, MachineComponent*> components;
  MachineComponentService service(components);

  const auto result = service.reconnect(utl::ERobotComponent::Contec);

  EXPECT_NE(result.find("not implemented"), std::string::npos);
}

TEST(MachineComponentServiceTests, ReconnectInitializeThrowReturnsError) {
  FakeComponent component(utl::ERobotComponent::Contec, true);
  std::map<utl::ERobotComponent, MachineComponent*> components{
      {utl::ERobotComponent::Contec, &component}};
  MachineComponentService service(components);

  const auto result = service.reconnect(utl::ERobotComponent::Contec);

  EXPECT_NE(result.find("failed"), std::string::npos);
  EXPECT_EQ(component.resetCalls, 1);
  EXPECT_EQ(component.initializeCalls, 1);
  EXPECT_EQ(component.state(), MachineComponent::State::Error);
}

TEST(MachineComponentServiceTests, ReconnectNonNormalAfterInitReturnsError) {
  FakeComponent component(utl::ERobotComponent::MotorControl, false,
                          MachineComponent::State::Error);
  std::map<utl::ERobotComponent, MachineComponent*> components{
      {utl::ERobotComponent::MotorControl, &component}};
  MachineComponentService service(components);

  const auto result = service.reconnect(utl::ERobotComponent::MotorControl);

  EXPECT_NE(result.find("unsuccessful"), std::string::npos);
  EXPECT_EQ(component.initializeCalls, 1);
}

TEST(MachineComponentServiceTests, InitializeAllContinuesOnFailures) {
  FakeComponent failing(utl::ERobotComponent::Contec, true);
  FakeComponent healthy(utl::ERobotComponent::ControlPanel);
  std::map<utl::ERobotComponent, MachineComponent*> components{
      {utl::ERobotComponent::Contec, &failing},
      {utl::ERobotComponent::ControlPanel, &healthy}};
  MachineComponentService service(components);

  service.initializeAll();

  EXPECT_EQ(failing.initializeCalls, 1);
  EXPECT_EQ(healthy.initializeCalls, 1);
  EXPECT_EQ(healthy.state(), MachineComponent::State::Normal);
}
