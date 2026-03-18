#include <gtest/gtest.h>

#include <MachineController.hpp>

#include <stdexcept>
#include <string>
#include <vector>

namespace {
class FakeControlPolicy final : public IRobotControlPolicy {
 public:
  ControlDecision decide(const std::optional<SignalMap>& inputs,
                         const std::optional<SignalMap>&,
                         const MachineComponent::State contecState,
                         const utl::RobotStatus&) override {
    ++decideCalls;
    seenInputs = inputs;
    seenState = contecState;
    return decisionToReturn;
  }

  int decideCalls{0};
  std::optional<SignalMap> seenInputs;
  MachineComponent::State seenState{MachineComponent::State::Error};
  ControlDecision decisionToReturn;
};
}  // namespace

TEST(MachineControllerTests, ErrorDecisionDoesNotMutateToolChangerStatusInController) {
  utl::RobotStatus status;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
      utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ProxSen] = utl::ELEDState::Off;
  auto policy = std::make_unique<FakeControlPolicy>();
  policy->decisionToReturn.setToolChangerErrorBlinking = true;
  auto* policyPtr = policy.get();

  bool outputsCalled = false;
  MachineController controller(
      MachineController::IoOps{
          .readInputs = []() -> std::optional<signal_map_t> { return signal_map_t{{"button1", true}}; },
          .setOutputs = [&](const signal_map_t&) { outputsCalled = true; },
          .readOutputs = []() -> std::optional<signal_map_t> { return std::nullopt; },
          .contecState = []() { return MachineComponent::State::Error; },
      },
      MachineController::MotorOps{.isConfigured = [](utl::EMotor) { return true; }},
      status, std::move(policy));

  controller.runControlLoopTasks();

  EXPECT_EQ(policyPtr->decideCalls, 1);
  EXPECT_FALSE(outputsCalled);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::Off);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Right)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::Off);
}

TEST(MachineControllerTests, ToolChangerCommandSetsExpectedOutputSignal) {
  utl::RobotStatus status;
  signal_map_t seenOutputs;
  auto policy = std::make_unique<FakeControlPolicy>();

  MachineController controller(
      MachineController::IoOps{
          .readInputs = []() -> std::optional<signal_map_t> { return signal_map_t{}; },
          .setOutputs = [&](const signal_map_t& outputs) { seenOutputs = outputs; },
          .readOutputs = []() -> std::optional<signal_map_t> {
              return signal_map_t{{"toolChangerLeft", true}};
          },
          .contecState = []() { return MachineComponent::State::Normal; },
      },
      MachineController::MotorOps{.isConfigured = [](utl::EMotor) { return true; }},
      status, std::move(policy));

  controller.handleToolChangerCommand(
      cmd::ToolChangerCommand{utl::EArm::Left, utl::EToolChangerAction::Open});

  ASSERT_TRUE(seenOutputs.contains("toolChangerLeft"));
  EXPECT_TRUE(seenOutputs.at("toolChangerLeft"));
}

TEST(MachineControllerTests, PolicyOutputsAreForwardedToOutputsWriter) {
  utl::RobotStatus status;
  signal_map_t seenOutputs;
  auto policy = std::make_unique<FakeControlPolicy>();
  policy->decisionToReturn.outputs = signal_map_t{{"light1", true}, {"light2", false}};

  MachineController controller(
      MachineController::IoOps{
          .readInputs = []() -> std::optional<signal_map_t> { return signal_map_t{{"button1", true}}; },
          .setOutputs = [&](const signal_map_t& outputs) { seenOutputs = outputs; },
          .readOutputs = []() -> std::optional<signal_map_t> { return std::nullopt; },
          .contecState = []() { return MachineComponent::State::Normal; },
      },
      MachineController::MotorOps{.isConfigured = [](utl::EMotor) { return true; }},
      status, std::move(policy));

  controller.runControlLoopTasks();

  ASSERT_TRUE(seenOutputs.contains("light1"));
  ASSERT_TRUE(seenOutputs.contains("light2"));
  EXPECT_TRUE(seenOutputs.at("light1"));
  EXPECT_FALSE(seenOutputs.at("light2"));
}

TEST(MachineControllerTests, ToolChangerCommandThrowsWhenContecIsInErrorState) {
  utl::RobotStatus status;
  bool outputsCalled = false;
  auto policy = std::make_unique<FakeControlPolicy>();

  MachineController controller(
      MachineController::IoOps{
          .readInputs = []() -> std::optional<signal_map_t> { return signal_map_t{}; },
          .setOutputs = [&](const signal_map_t&) { outputsCalled = true; },
          .readOutputs = []() -> std::optional<signal_map_t> { return signal_map_t{}; },
          .contecState = []() { return MachineComponent::State::Error; },
      },
      MachineController::MotorOps{.isConfigured = [](utl::EMotor) { return true; }},
      status, std::move(policy));

  EXPECT_THROW((void)controller.handleToolChangerCommand(
                   cmd::ToolChangerCommand{utl::EArm::Right,
                                           utl::EToolChangerAction::Open}),
               std::runtime_error);
  EXPECT_FALSE(outputsCalled);
}

TEST(MachineControllerTests, ToolChangerCommandThrowsWhenOutputReadbackFails) {
  utl::RobotStatus status;
  bool outputsCalled = false;
  auto policy = std::make_unique<FakeControlPolicy>();

  MachineController controller(
      MachineController::IoOps{
          .readInputs = []() -> std::optional<signal_map_t> { return signal_map_t{}; },
          .setOutputs = [&](const signal_map_t&) { outputsCalled = true; },
          .readOutputs = []() -> std::optional<signal_map_t> { return std::nullopt; },
          .contecState = []() { return MachineComponent::State::Normal; },
      },
      MachineController::MotorOps{.isConfigured = [](utl::EMotor) { return true; }},
      status, std::move(policy));

  EXPECT_THROW((void)controller.handleToolChangerCommand(
                   cmd::ToolChangerCommand{utl::EArm::Left,
                                           utl::EToolChangerAction::Close}),
               std::runtime_error);
  EXPECT_TRUE(outputsCalled);
}

TEST(MachineControllerTests, PolicyMotorIntentsAreAppliedInExpectedOrder) {
  utl::RobotStatus status;
  auto policy = std::make_unique<FakeControlPolicy>();
  policy->decisionToReturn.motorIntents.push_back(
      {.motorId = utl::EMotor::XLeft,
       .mode = MotorControlMode::Speed,
       .direction = MotorControlDirection::Reverse,
       .speed = 1200,
       .startMovement = true});

  std::vector<std::string> applied;
  MachineController controller(
      MachineController::IoOps{
          .readInputs = []() -> std::optional<signal_map_t> { return signal_map_t{{"button1", true}}; },
          .setOutputs = [](const signal_map_t&) {},
          .readOutputs = []() -> std::optional<signal_map_t> { return signal_map_t{}; },
          .contecState = []() { return MachineComponent::State::Normal; },
      },
      MachineController::MotorOps{
          .setMode = [&](utl::EMotor, MotorControlMode) { applied.emplace_back("mode"); },
          .setSpeed = [&](utl::EMotor, std::int32_t) { applied.emplace_back("speed"); },
          .setAcceleration = [&](utl::EMotor, std::int32_t) { applied.emplace_back("acceleration"); },
          .setDeceleration = [&](utl::EMotor, std::int32_t) { applied.emplace_back("deceleration"); },
          .setPosition = [&](utl::EMotor, std::int32_t) { applied.emplace_back("position"); },
          .setDirection = [&](utl::EMotor, MotorControlDirection) { applied.emplace_back("direction"); },
          .start = [&](utl::EMotor) { applied.emplace_back("start"); },
          .stop = [&](utl::EMotor) { applied.emplace_back("stop"); },
          .isConfigured = [](utl::EMotor) { return true; },
      },
      status, std::move(policy));

  controller.runControlLoopTasks();

  ASSERT_EQ(applied.size(), 4u);
  EXPECT_EQ(applied[0], "mode");
  EXPECT_EQ(applied[1], "direction");
  EXPECT_EQ(applied[2], "speed");
  EXPECT_EQ(applied[3], "start");
}

TEST(MachineControllerTests, UnconfiguredMotorIntentsAreIgnored) {
  utl::RobotStatus status;
  auto policy = std::make_unique<FakeControlPolicy>();
  policy->decisionToReturn.motorIntents.push_back(
      {.motorId = utl::EMotor::XRight,
       .mode = MotorControlMode::Speed,
       .direction = MotorControlDirection::Forward,
       .speed = 900,
       .startMovement = true});

  std::vector<std::string> applied;
  MachineController controller(
      MachineController::IoOps{
          .readInputs = []() -> std::optional<signal_map_t> { return signal_map_t{{"button1", true}}; },
          .setOutputs = [](const signal_map_t&) {},
          .readOutputs = []() -> std::optional<signal_map_t> { return signal_map_t{}; },
          .contecState = []() { return MachineComponent::State::Normal; },
      },
      MachineController::MotorOps{
          .setMode = [&](utl::EMotor, MotorControlMode) { applied.emplace_back("mode"); },
          .setSpeed = [&](utl::EMotor, std::int32_t) { applied.emplace_back("speed"); },
          .setAcceleration = [&](utl::EMotor, std::int32_t) { applied.emplace_back("acceleration"); },
          .setDeceleration = [&](utl::EMotor, std::int32_t) { applied.emplace_back("deceleration"); },
          .setPosition = [&](utl::EMotor, std::int32_t) { applied.emplace_back("position"); },
          .setDirection = [&](utl::EMotor, MotorControlDirection) { applied.emplace_back("direction"); },
          .start = [&](utl::EMotor) { applied.emplace_back("start"); },
          .stop = [&](utl::EMotor) { applied.emplace_back("stop"); },
          .isConfigured = [](const utl::EMotor motorId) { return motorId == utl::EMotor::XLeft; },
      },
      status, std::move(policy));

  controller.runControlLoopTasks();

  EXPECT_TRUE(applied.empty());
}

TEST(MachineControllerTests, AccelerationAndDecelerationIntentsAreApplied) {
  utl::RobotStatus status;
  auto policy = std::make_unique<FakeControlPolicy>();
  policy->decisionToReturn.motorIntents.push_back(
      {.motorId = utl::EMotor::XLeft,
       .mode = MotorControlMode::Speed,
       .acceleration = 1234,
       .deceleration = 4321,
       .speed = 900,
       .startMovement = true});

  std::vector<std::string> applied;
  MachineController controller(
      MachineController::IoOps{
          .readInputs = []() -> std::optional<signal_map_t> { return signal_map_t{{"button1", true}}; },
          .setOutputs = [](const signal_map_t&) {},
          .readOutputs = []() -> std::optional<signal_map_t> { return signal_map_t{}; },
          .contecState = []() { return MachineComponent::State::Normal; },
      },
      MachineController::MotorOps{
          .setMode = [&](utl::EMotor, MotorControlMode) { applied.emplace_back("mode"); },
          .setSpeed = [&](utl::EMotor, std::int32_t) { applied.emplace_back("speed"); },
          .setAcceleration = [&](utl::EMotor, std::int32_t) { applied.emplace_back("acceleration"); },
          .setDeceleration = [&](utl::EMotor, std::int32_t) { applied.emplace_back("deceleration"); },
          .setPosition = [&](utl::EMotor, std::int32_t) { applied.emplace_back("position"); },
          .setDirection = [&](utl::EMotor, MotorControlDirection) { applied.emplace_back("direction"); },
          .start = [&](utl::EMotor) { applied.emplace_back("start"); },
          .stop = [&](utl::EMotor) { applied.emplace_back("stop"); },
          .isConfigured = [](utl::EMotor) { return true; },
      },
      status, std::move(policy));

  controller.runControlLoopTasks();

  EXPECT_NE(std::find(applied.begin(), applied.end(), "acceleration"), applied.end());
  EXPECT_NE(std::find(applied.begin(), applied.end(), "deceleration"), applied.end());
}
