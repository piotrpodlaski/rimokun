#include <QtTest/QtTest>

#include <GuiStateStore.hpp>
#include <LedIndicator.hpp>
#include <ResetControls.hpp>
#include <ResetControlsPresenter.hpp>

#include <QPushButton>

namespace {
class TestableResetControls final : public ResetControls {
 public:
  using ResetControls::ResetControls;

  void emitButton(const GuiCommand& command) { emit buttonPressed(command); }
};
}  // namespace

class ResetControlsPresenterQtTests final : public QObject {
  Q_OBJECT

 private slots:
  void mapsStatusAndConnectionStateIntoView();
  void forwardsViewCommand();
};

void ResetControlsPresenterQtTests::mapsStatusAndConnectionStateIntoView() {
  GuiStateStore store;
  TestableResetControls view(nullptr);
  ResetControlsPresenter presenter(&view, &store);

  auto* server = view.findChild<LedIndicator*>("ledServer");
  auto* contec = view.findChild<LedIndicator*>("ledContec");
  auto* motors = view.findChild<LedIndicator*>("ledMotors");
  auto* panel = view.findChild<LedIndicator*>("ledControlPanel");
  auto* resetContec = view.findChild<QPushButton*>("resetContec");
  auto* resetMotors = view.findChild<QPushButton*>("resetMotors");
  auto* resetPanel = view.findChild<QPushButton*>("resetControlPanel");

  QVERIFY(server != nullptr);
  QVERIFY(contec != nullptr);
  QVERIFY(motors != nullptr);
  QVERIFY(panel != nullptr);
  QVERIFY(resetContec != nullptr);
  QVERIFY(resetMotors != nullptr);
  QVERIFY(resetPanel != nullptr);

  utl::RobotStatus status;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::Error;
  status.robotComponents[utl::ERobotComponent::MotorControl] = utl::ELEDState::On;
  status.robotComponents[utl::ERobotComponent::ControlPanel] =
      utl::ELEDState::Off;
  store.onStatusReceived(status);

  QCOMPARE(server->state(), utl::ELEDState::On);
  QCOMPARE(contec->state(), utl::ELEDState::Error);
  QCOMPARE(motors->state(), utl::ELEDState::On);
  QCOMPARE(panel->state(), utl::ELEDState::Off);
  QCOMPARE(resetContec->isEnabled(), true);
  QCOMPARE(resetMotors->isEnabled(), false);
  QCOMPARE(resetPanel->isEnabled(), false);

  store.onServerDisconnected();
  QCOMPARE(server->state(), utl::ELEDState::Error);
  QCOMPARE(contec->state(), utl::ELEDState::Off);
  QCOMPARE(motors->state(), utl::ELEDState::Off);
  QCOMPARE(panel->state(), utl::ELEDState::Off);
  QCOMPARE(resetContec->isEnabled(), false);
  QCOMPARE(resetMotors->isEnabled(), false);
  QCOMPARE(resetPanel->isEnabled(), false);
}

void ResetControlsPresenterQtTests::forwardsViewCommand() {
  GuiStateStore store;
  TestableResetControls view(nullptr);
  ResetControlsPresenter presenter(&view, &store);

  int callCount = 0;
  GuiCommand captured;
  QObject::connect(&presenter, &ResetControlsPresenter::commandIssued, &view,
                   [&](const GuiCommand& command) {
                     ++callCount;
                     captured = command;
                   });

  GuiCommand cmd;
  cmd.payload =
      GuiReconnectCommand{.component = utl::ERobotComponent::MotorControl};
  view.emitButton(cmd);

  QCOMPARE(callCount, 1);
  const auto* reconnect = std::get_if<GuiReconnectCommand>(&captured.payload);
  QVERIFY(reconnect != nullptr);
  QCOMPARE(reconnect->component, utl::ERobotComponent::MotorControl);
}

QTEST_MAIN(ResetControlsPresenterQtTests)
#include "ResetControlsPresenterQtTests.moc"
