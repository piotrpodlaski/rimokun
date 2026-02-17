#include <QtTest/QtTest>

#include <GuiStateStore.hpp>
#include <LedIndicator.hpp>
#include <ToolChanger.hpp>
#include <ToolChangerPresenter.hpp>

#include <QPushButton>

class ToolChangerPresenterQtTests final : public QObject {
  Q_OBJECT

 private slots:
  void mapsStatusIntoViewAndResetsOnDisconnect();
  void forwardsViewCommand();
};

void ToolChangerPresenterQtTests::mapsStatusIntoViewAndResetsOnDisconnect() {
  GuiStateStore store;
  ToolChanger view;
  ToolChangerPresenter presenter(&view, utl::EArm::Left, &store);

  utl::RobotStatus status;
  utl::ToolChangerStatus tc;
  tc.flags[utl::EToolChangerStatusFlags::ProxSen] = utl::ELEDState::On;
  tc.flags[utl::EToolChangerStatusFlags::OpenSen] = utl::ELEDState::Off;
  tc.flags[utl::EToolChangerStatusFlags::ClosedSen] = utl::ELEDState::On;
  tc.flags[utl::EToolChangerStatusFlags::OpenValve] = utl::ELEDState::Off;
  tc.flags[utl::EToolChangerStatusFlags::ClosedValve] = utl::ELEDState::On;
  status.toolChangers[utl::EArm::Left] = tc;

  store.onStatusReceived(status);

  auto* prox = view.findChild<LedIndicator*>("proxLamp");
  auto* open = view.findChild<LedIndicator*>("openLamp");
  auto* closed = view.findChild<LedIndicator*>("closedLamp");
  auto* valveOpen = view.findChild<LedIndicator*>("valveOpenLamp");
  auto* valveClosed = view.findChild<LedIndicator*>("valveClosedLamp");
  auto* openBtn = view.findChild<QPushButton*>("openButon");
  auto* closeBtn = view.findChild<QPushButton*>("closeButton");

  QVERIFY(prox != nullptr);
  QVERIFY(open != nullptr);
  QVERIFY(closed != nullptr);
  QVERIFY(valveOpen != nullptr);
  QVERIFY(valveClosed != nullptr);
  QVERIFY(openBtn != nullptr);
  QVERIFY(closeBtn != nullptr);

  QCOMPARE(prox->state(), utl::ELEDState::On);
  QCOMPARE(open->state(), utl::ELEDState::Off);
  QCOMPARE(closed->state(), utl::ELEDState::On);
  QCOMPARE(valveOpen->state(), utl::ELEDState::Off);
  QCOMPARE(valveClosed->state(), utl::ELEDState::On);
  QCOMPARE(openBtn->isEnabled(), true);
  QCOMPARE(closeBtn->isEnabled(), false);

  store.onServerDisconnected();
  QCOMPARE(prox->state(), utl::ELEDState::Off);
  QCOMPARE(open->state(), utl::ELEDState::Off);
  QCOMPARE(closed->state(), utl::ELEDState::Off);
  QCOMPARE(valveOpen->state(), utl::ELEDState::Off);
  QCOMPARE(valveClosed->state(), utl::ELEDState::Off);
  QCOMPARE(openBtn->isEnabled(), false);
  QCOMPARE(closeBtn->isEnabled(), false);
}

void ToolChangerPresenterQtTests::forwardsViewCommand() {
  GuiStateStore store;
  ToolChanger view;
  ToolChangerPresenter presenter(&view, utl::EArm::Right, &store);

  int callCount = 0;
  GuiCommand captured;
  QObject::connect(&presenter, &ToolChangerPresenter::commandIssued, &view,
                   [&](const GuiCommand& command) {
                     ++callCount;
                     captured = command;
                   });

  GuiCommand cmd;
  cmd.payload = GuiToolChangerCommand{
      .arm = utl::EArm::Right, .action = utl::EToolChangerAction::Open};
  qRegisterMetaType<GuiCommand>("GuiCommand");
  const bool invoked = QMetaObject::invokeMethod(
      &view, "buttonPressed", Qt::DirectConnection, Q_ARG(GuiCommand, cmd));
  QVERIFY(invoked);

  QCOMPARE(callCount, 1);
  const auto* tool = std::get_if<GuiToolChangerCommand>(&captured.payload);
  QVERIFY(tool != nullptr);
  QCOMPARE(tool->arm, utl::EArm::Right);
  QCOMPARE(tool->action, utl::EToolChangerAction::Open);
}

QTEST_MAIN(ToolChangerPresenterQtTests)
#include "ToolChangerPresenterQtTests.moc"
