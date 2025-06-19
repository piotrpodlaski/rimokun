#pragma once

#include <QColor>
#include <QWidget>

#include "CommonDefinitions.hpp"

class LedIndicator final : public QWidget {
  Q_OBJECT

 public:
  explicit LedIndicator(QWidget* parent = nullptr);
  explicit operator bool() const;

  ~LedIndicator() override = default;

  void setState(utl::ELEDState state);
  [[nodiscard]] utl::ELEDState state() const;

 protected:
  void paintEvent(QPaintEvent* event) override;
  [[nodiscard]] QSize sizeHint() const override;

 private:
  utl::ELEDState m_state;
  [[nodiscard]] QColor currentColor() const;
};
