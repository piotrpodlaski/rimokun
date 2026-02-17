#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include <ITransportWorker.hpp>
#include <Updater.hpp>

#include <thread>
#include <vector>

namespace {
class FakeTransportWorker final : public ITransportWorker {
 public:
  void start(StatusCallback onStatus, ConnectionLostCallback onConnectionLost,
             ResponseCallback onResponse) override {
    started = true;
    statusCb = std::move(onStatus);
    lostCb = std::move(onConnectionLost);
    responseCb = std::move(onResponse);
  }

  void stop() override { stopped = true; }

  void enqueue(Request request) override { requests.push_back(std::move(request)); }

  void emitStatus(const utl::RobotStatus& status) const {
    if (statusCb) {
      statusCb(status);
    }
  }

  void emitConnectionLost() const {
    if (lostCb) {
      lostCb();
    }
  }

  void emitResponse(const ResponseEvent& event) const {
    if (responseCb) {
      responseCb(event);
    }
  }

  bool started{false};
  bool stopped{false};
  std::vector<Request> requests;
  StatusCallback statusCb;
  ConnectionLostCallback lostCb;
  ResponseCallback responseCb;
};

QCoreApplication& ensureApp() {
  if (QCoreApplication::instance()) {
    return *QCoreApplication::instance();
  }
  static int argc = 1;
  static char appName[] = "gui_glue_tests";
  static char* argv[] = {appName, nullptr};
  static QCoreApplication app(argc, argv);
  return app;
}

void pumpEvents(const int iterations = 10) {
  (void)ensureApp();
  for (int i = 0; i < iterations; ++i) {
    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
}  // namespace

TEST(UpdaterCoordinationTests, NotifiesErrorWhenSendingWithoutStart) {
  (void)ensureApp();
  auto fakeWorker = std::make_unique<FakeTransportWorker>();
  auto* workerRaw = fakeWorker.get();
  int errorCalls = 0;

  Updater updater(nullptr, std::move(fakeWorker),
                  [&](const QString&, const QString&) { ++errorCalls; });
  GuiCommand command;
  command.payload = GuiReconnectCommand{.component = utl::ERobotComponent::Contec};
  updater.sendCommand(command);

  EXPECT_EQ(errorCalls, 1);
  EXPECT_TRUE(workerRaw->requests.empty());
}

TEST(UpdaterCoordinationTests, ForwardsStatusConnectionAndResponses) {
  (void)ensureApp();
  auto fakeWorker = std::make_unique<FakeTransportWorker>();
  auto* workerRaw = fakeWorker.get();
  int errorCalls = 0;

  Updater updater(nullptr, std::move(fakeWorker),
                  [&](const QString&, const QString&) { ++errorCalls; });
  QSignalSpy statusSpy(&updater, &Updater::newDataArrived);
  QSignalSpy disconnectedSpy(&updater, &Updater::serverNotConnected);

  updater.startUpdaterThread();
  EXPECT_TRUE(workerRaw->started);

  utl::RobotStatus status;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::On;
  workerRaw->emitStatus(status);
  workerRaw->emitConnectionLost();
  pumpEvents();

  EXPECT_EQ(statusSpy.count(), 1);
  EXPECT_EQ(disconnectedSpy.count(), 1);

  GuiCommand command;
  command.payload = GuiReconnectCommand{
      .component = utl::ERobotComponent::MotorControl};
  updater.sendCommand(command);
  ASSERT_EQ(workerRaw->requests.size(), 1U);
  const auto requestId = workerRaw->requests.front().id;
  EXPECT_NE(requestId, 0U);

  workerRaw->emitResponse({.id = requestId, .response = std::nullopt});
  pumpEvents();
  EXPECT_EQ(errorCalls, 1);

  updater.sendCommand(command);
  ASSERT_EQ(workerRaw->requests.size(), 2U);
  const auto requestId2 = workerRaw->requests.back().id;

  GuiResponse errorResponse;
  errorResponse.ok = false;
  errorResponse.message = "forced";
  workerRaw->emitResponse({.id = requestId2, .response = errorResponse});
  pumpEvents();
  EXPECT_EQ(errorCalls, 2);

  updater.stopUpdaterThread();
  EXPECT_TRUE(workerRaw->stopped);
}
