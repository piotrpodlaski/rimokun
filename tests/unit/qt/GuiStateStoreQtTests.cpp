#include <QtTest/QtTest>

#include <GuiStateStore.hpp>

class GuiStateStoreQtTests final : public QObject {
  Q_OBJECT

 private slots:
  void firstStatusMarksConnectedAndEmitsSignals();
  void disconnectEmitsConnectionChangedFalseOnce();
};

void GuiStateStoreQtTests::firstStatusMarksConnectedAndEmitsSignals() {
  GuiStateStore store;
  QSignalSpy statusSpy(&store, &GuiStateStore::statusUpdated);
  QSignalSpy connSpy(&store, &GuiStateStore::connectionChanged);

  utl::RobotStatus status;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::On;
  store.onStatusReceived(status);

  QCOMPARE(statusSpy.count(), 1);
  QCOMPARE(connSpy.count(), 1);
  const auto args = connSpy.takeFirst();
  QCOMPARE(args.at(0).toBool(), true);
  QVERIFY(store.isConnected());
  QVERIFY(store.latestStatus().has_value());
}

void GuiStateStoreQtTests::disconnectEmitsConnectionChangedFalseOnce() {
  GuiStateStore store;
  QSignalSpy connSpy(&store, &GuiStateStore::connectionChanged);

  utl::RobotStatus status;
  store.onStatusReceived(status);
  QCOMPARE(connSpy.count(), 1);
  connSpy.clear();

  store.onServerDisconnected();
  QCOMPARE(connSpy.count(), 1);
  auto args = connSpy.takeFirst();
  QCOMPARE(args.at(0).toBool(), false);
  QVERIFY(!store.isConnected());

  store.onServerDisconnected();
  QCOMPARE(connSpy.count(), 0);
}

QTEST_MAIN(GuiStateStoreQtTests)
#include "GuiStateStoreQtTests.moc"
