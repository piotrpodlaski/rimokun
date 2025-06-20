#pragma once

#include <spdlog/sinks/base_sink.h>
#include <QPlainTextEdit>
#include <mutex>

class QtLogSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
  explicit QtLogSink(QTextEdit* widget) : logWidget_(widget) {}

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t formatted;
    formatter_->format(msg, formatted);
    const std::string raw = fmt::to_string(formatted);

    QString html = formatToHtml(msg.level, raw);

    QMetaObject::invokeMethod(
        logWidget_, [logWidget = logWidget_, html]() {
            logWidget->append(html);
        }, Qt::QueuedConnection);
  }

  void flush_() override {}

private:
  QTextEdit* logWidget_;

 static QString formatToHtml(const spdlog::level::level_enum level, const std::string& text) {
    QString safeText = QString::fromStdString(text).toHtmlEscaped();
    QString color;

    switch (level) {
      case spdlog::level::trace:    color = "#999999"; break;
      case spdlog::level::debug:    color = "#007acc"; break;
      case spdlog::level::info:     color = "#00aa00"; break;
      case spdlog::level::warn:     color = "#ffaa00"; break;
      case spdlog::level::err:      color = "#ff4444"; break;
      case spdlog::level::critical: color = "#ff0000"; break;
      default:                      color = "#ffffff"; break;
    }

    return QString("<span style=\"color:%1;\">%2</span>").arg(color, safeText);
  }
};
