#include <gtest/gtest.h>

#include <MachineStatusBuilder.hpp>

class FakeComponent final : public MachineComponent {
 public:
  explicit FakeComponent(const State state) { setState(state); }
  void initialize() override {}
  void reset() override {}
  [[nodiscard]] utl::ERobotComponent componentType() const override {
    return utl::ERobotComponent::Contec;
  }
};

TEST(MachineStatusBuilderTests, BuildsAndPublishesExpectedStatus) {
  MachineStatusBuilder builder;
  utl::RobotStatus status;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
      utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ProxSen] = utl::ELEDState::Off;

  FakeComponent contec(MachineComponent::State::Normal);
  MachineStatusBuilder::ComponentsMap components{
      {utl::ERobotComponent::Contec, &contec}};

  bool published = false;
  utl::RobotStatus publishedStatus;

  builder.updateAndPublish(
      status, components,
      []() {
        ControlPanel::Snapshot s;
        s.x = {0.1, 0.2, 0.3};
        s.y = {0.4, 0.5, 0.6};
        s.b = {true, false, true};
        return s;
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"button1", true}, {"button2", false}};
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"toolChangerLeft", true},
                            {"toolChangerRight", false}};
      },
      [&](const utl::RobotStatus& s) {
        published = true;
        publishedStatus = s;
      });

  ASSERT_TRUE(published);
  EXPECT_EQ(publishedStatus.robotComponents.at(utl::ERobotComponent::Contec),
            utl::ELEDState::On);
  EXPECT_TRUE(publishedStatus.joystics.at(utl::EArm::Left).btn);
  EXPECT_EQ(publishedStatus.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::On);
  EXPECT_EQ(publishedStatus.toolChangers.at(utl::EArm::Right)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::Off);
}

TEST(MachineStatusBuilderTests, MissingInputSnapshotSetsProximityFlagsToError) {
  MachineStatusBuilder builder;
  utl::RobotStatus status;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
      utl::ELEDState::On;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ProxSen] = utl::ELEDState::On;

  FakeComponent contec(MachineComponent::State::Normal);
  MachineStatusBuilder::ComponentsMap components{
      {utl::ERobotComponent::Contec, &contec}};

  bool published = false;
  builder.updateAndPublish(
      status, components,
      []() {
        ControlPanel::Snapshot s;
        s.x = {0.0, 0.0, 0.0};
        s.y = {0.0, 0.0, 0.0};
        s.b = {false, false, false};
        return s;
      },
      []() -> std::optional<signal_map_t> { return std::nullopt; },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"toolChangerLeft", true},
                            {"toolChangerRight", true}};
      },
      [&](const utl::RobotStatus&) { published = true; });

  ASSERT_TRUE(published);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::Error);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Right)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::Error);
}

TEST(MachineStatusBuilderTests, WarningComponentMapsToWarningLed) {
  MachineStatusBuilder builder;
  utl::RobotStatus status;

  FakeComponent contec(MachineComponent::State::Warning);
  MachineStatusBuilder::ComponentsMap components{
      {utl::ERobotComponent::Contec, &contec}};

  bool published = false;
  builder.updateAndPublish(
      status, components,
      []() {
        ControlPanel::Snapshot s;
        s.x = {0.0, 0.0, 0.0};
        s.y = {0.0, 0.0, 0.0};
        s.b = {false, false, false};
        return s;
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"button1", false}, {"button2", false}};
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"toolChangerLeft", false},
                            {"toolChangerRight", false}};
      },
      [&](const utl::RobotStatus&) { published = true; });

  ASSERT_TRUE(published);
  EXPECT_EQ(status.robotComponents.at(utl::ERobotComponent::Contec),
            utl::ELEDState::Warning);
}

TEST(MachineStatusBuilderTests, ContecErrorSetsAllToolChangerFlagsToError) {
  MachineStatusBuilder builder;
  utl::RobotStatus status;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::ProxSen] =
      utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Left].flags[utl::EToolChangerStatusFlags::OpenSen] =
      utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Left]
      .flags[utl::EToolChangerStatusFlags::ClosedSen] = utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Left]
      .flags[utl::EToolChangerStatusFlags::OpenValve] = utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Left]
      .flags[utl::EToolChangerStatusFlags::ClosedValve] = utl::ELEDState::Off;

  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ProxSen] = utl::ELEDState::On;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::OpenSen] = utl::ELEDState::On;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ClosedSen] = utl::ELEDState::On;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::OpenValve] = utl::ELEDState::On;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ClosedValve] = utl::ELEDState::On;

  FakeComponent contec(MachineComponent::State::Error);
  MachineStatusBuilder::ComponentsMap components{
      {utl::ERobotComponent::Contec, &contec}};

  bool published = false;
  builder.updateAndPublish(
      status, components,
      []() {
        ControlPanel::Snapshot s;
        s.x = {0.0, 0.0, 0.0};
        s.y = {0.0, 0.0, 0.0};
        s.b = {false, false, false};
        return s;
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"button1", true}, {"button2", true}};
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"toolChangerLeft", true},
                            {"toolChangerRight", false}};
      },
      [&](const utl::RobotStatus&) { published = true; });

  ASSERT_TRUE(published);
  for (const auto arm : {utl::EArm::Left, utl::EArm::Right}) {
    EXPECT_EQ(status.toolChangers.at(arm).flags.at(utl::EToolChangerStatusFlags::ProxSen),
              utl::ELEDState::Error);
    EXPECT_EQ(status.toolChangers.at(arm).flags.at(utl::EToolChangerStatusFlags::OpenSen),
              utl::ELEDState::Error);
    EXPECT_EQ(status.toolChangers.at(arm).flags.at(utl::EToolChangerStatusFlags::ClosedSen),
              utl::ELEDState::Error);
    EXPECT_EQ(status.toolChangers.at(arm).flags.at(utl::EToolChangerStatusFlags::OpenValve),
              utl::ELEDState::Error);
    EXPECT_EQ(status.toolChangers.at(arm).flags.at(utl::EToolChangerStatusFlags::ClosedValve),
              utl::ELEDState::Error);
  }
}

TEST(MachineStatusBuilderTests, PartialOutputSnapshotSetsValveFlagsToError) {
  MachineStatusBuilder builder;
  utl::RobotStatus status;
  status.toolChangers[utl::EArm::Left]
      .flags[utl::EToolChangerStatusFlags::OpenValve] = utl::ELEDState::On;
  status.toolChangers[utl::EArm::Left]
      .flags[utl::EToolChangerStatusFlags::ClosedValve] = utl::ELEDState::Off;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::OpenValve] = utl::ELEDState::On;
  status.toolChangers[utl::EArm::Right]
      .flags[utl::EToolChangerStatusFlags::ClosedValve] = utl::ELEDState::Off;

  FakeComponent contec(MachineComponent::State::Normal);
  MachineStatusBuilder::ComponentsMap components{
      {utl::ERobotComponent::Contec, &contec}};

  bool published = false;
  builder.updateAndPublish(
      status, components,
      []() {
        ControlPanel::Snapshot s;
        s.x = {0.0, 0.0, 0.0};
        s.y = {0.0, 0.0, 0.0};
        s.b = {false, false, false};
        return s;
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"button1", false}, {"button2", true}};
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"toolChangerLeft", true}};
      },
      [&](const utl::RobotStatus&) { published = true; });

  ASSERT_TRUE(published);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::OpenValve),
            utl::ELEDState::Error);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::ClosedValve),
            utl::ELEDState::Error);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Right)
                .flags.at(utl::EToolChangerStatusFlags::OpenValve),
            utl::ELEDState::Error);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Right)
                .flags.at(utl::EToolChangerStatusFlags::ClosedValve),
            utl::ELEDState::Error);
}

TEST(MachineStatusBuilderTests, MissingSnapshotAfterValidUpdateDoesNotLeaveStaleFlags) {
  MachineStatusBuilder builder;
  utl::RobotStatus status;
  FakeComponent contec(MachineComponent::State::Normal);
  MachineStatusBuilder::ComponentsMap components{
      {utl::ERobotComponent::Contec, &contec}};

  builder.updateAndPublish(
      status, components,
      []() {
        ControlPanel::Snapshot s;
        s.x = {0.0, 0.0, 0.0};
        s.y = {0.0, 0.0, 0.0};
        s.b = {false, false, false};
        return s;
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"button1", true}, {"button2", true}};
      },
      []() -> std::optional<signal_map_t> {
        return signal_map_t{{"toolChangerLeft", true},
                            {"toolChangerRight", false}};
      },
      [](const utl::RobotStatus&) {});

  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::On);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::OpenValve),
            utl::ELEDState::On);

  builder.updateAndPublish(
      status, components,
      []() {
        ControlPanel::Snapshot s;
        s.x = {0.0, 0.0, 0.0};
        s.y = {0.0, 0.0, 0.0};
        s.b = {false, false, false};
        return s;
      },
      []() -> std::optional<signal_map_t> { return std::nullopt; },
      []() -> std::optional<signal_map_t> { return std::nullopt; },
      [](const utl::RobotStatus&) {});

  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::Error);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Right)
                .flags.at(utl::EToolChangerStatusFlags::ProxSen),
            utl::ELEDState::Error);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Left)
                .flags.at(utl::EToolChangerStatusFlags::OpenValve),
            utl::ELEDState::Error);
  EXPECT_EQ(status.toolChangers.at(utl::EArm::Right)
                .flags.at(utl::EToolChangerStatusFlags::ClosedValve),
            utl::ELEDState::Error);
}
