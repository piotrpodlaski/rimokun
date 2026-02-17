#include <gtest/gtest.h>

#include <RimoTransportWorker.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace {
class FakeRimoGuiClient final : public IRimoGuiClient {
 public:
  void init() override { ++initCalls; }

  std::optional<utl::RobotStatus> receiveRobotStatus() override {
    std::lock_guard<std::mutex> lock(mutex);
    if (statuses.empty()) {
      return std::nullopt;
    }
    auto status = statuses.front();
    statuses.pop();
    return status;
  }

  std::optional<YAML::Node> sendCommand(const YAML::Node& command) override {
    std::lock_guard<std::mutex> lock(mutex);
    sentCommands.push_back(command);
    if (commandResponses.empty()) {
      return std::nullopt;
    }
    auto response = commandResponses.front();
    commandResponses.pop();
    return response;
  }

  std::atomic<int> initCalls{0};
  std::mutex mutex;
  std::queue<utl::RobotStatus> statuses;
  std::queue<YAML::Node> commandResponses;
  std::vector<YAML::Node> sentCommands;
};

bool waitFor(const std::function<bool()>& predicate, const int timeoutMs = 500) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return predicate();
}
}  // namespace

TEST(RimoTransportWorkerTests, EmitsStatusAndConnectionLostCallbacks) {
  auto fakeClient = std::make_unique<FakeRimoGuiClient>();
  auto* clientRaw = fakeClient.get();

  utl::RobotStatus status;
  status.robotComponents[utl::ERobotComponent::Contec] = utl::ELEDState::On;
  {
    std::lock_guard<std::mutex> lock(clientRaw->mutex);
    clientRaw->statuses.push(status);
  }

  RimoTransportWorker worker(std::move(fakeClient));
  std::atomic<int> statusCalls{0};
  std::atomic<int> lostCalls{0};
  worker.start([&](const utl::RobotStatus&) { ++statusCalls; },
               [&]() { ++lostCalls; }, [&](const ITransportWorker::ResponseEvent&) {});

  ASSERT_TRUE(waitFor([&]() { return statusCalls.load() > 0; }));
  ASSERT_TRUE(waitFor([&]() { return lostCalls.load() > 0; }));
  worker.stop();

  EXPECT_EQ(clientRaw->initCalls.load(), 1);
}

TEST(RimoTransportWorkerTests, ProcessesQueuedCommandAndEmitsResponse) {
  auto fakeClient = std::make_unique<FakeRimoGuiClient>();
  auto* clientRaw = fakeClient.get();

  YAML::Node responseNode;
  responseNode["status"] = "OK";
  responseNode["message"] = "ok";
  {
    std::lock_guard<std::mutex> lock(clientRaw->mutex);
    clientRaw->commandResponses.push(responseNode);
  }

  RimoTransportWorker worker(std::move(fakeClient));
  std::mutex responseMutex;
  std::condition_variable responseCv;
  std::optional<ITransportWorker::ResponseEvent> responseEvent;

  worker.start(
      [](const utl::RobotStatus&) {}, []() {},
      [&](const ITransportWorker::ResponseEvent& event) {
        {
          std::lock_guard<std::mutex> lock(responseMutex);
          responseEvent = event;
        }
        responseCv.notify_one();
      });

  GuiCommand command;
  command.payload = GuiReconnectCommand{.component = utl::ERobotComponent::Contec};
  worker.enqueue({.id = 77, .command = command});

  {
    std::unique_lock<std::mutex> lock(responseMutex);
    const bool ready = responseCv.wait_for(lock, std::chrono::milliseconds(500),
                                           [&] { return responseEvent.has_value(); });
    ASSERT_TRUE(ready);
  }

  worker.stop();

  ASSERT_TRUE(responseEvent.has_value());
  EXPECT_EQ(responseEvent->id, 77U);
  ASSERT_TRUE(responseEvent->response.has_value());
  EXPECT_TRUE(responseEvent->response->ok);
  std::lock_guard<std::mutex> lock(clientRaw->mutex);
  ASSERT_EQ(clientRaw->sentCommands.size(), 1U);
}
