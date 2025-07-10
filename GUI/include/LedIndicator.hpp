#pragma once

#include <QTimer>
#include <QWidget>

#include "CommonDefinitions.hpp"

class LedIndicator final : public QWidget {
  Q_OBJECT

 public:
  explicit LedIndicator(QWidget* parent = nullptr);
  explicit operator bool() const;

  ~LedIndicator() override;

  void setState(utl::ELEDState state);
  [[nodiscard]] utl::ELEDState state() const;
  static void initBlinkTimer();

 protected:
  void paintEvent(QPaintEvent* event) override;
  [[nodiscard]] QSize sizeHint() const override;

 private:
  static void toggleBlinkVisibility();
  bool _blinkVisible = false;
  utl::ELEDState _ledState;
  [[nodiscard]] QColor currentColor() const;

  static inline bool _globalBlinkState = false;
  static inline std::unique_ptr<QTimer> _blinkTimer;
  static inline QList<LedIndicator*> _blinkingLeds;
};
