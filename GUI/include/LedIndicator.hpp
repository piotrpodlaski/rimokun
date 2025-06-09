#pragma once

#include <QColor>
#include <QWidget>

class LedIndicator final : public QWidget {
  Q_OBJECT

 public:
  explicit LedIndicator(QWidget* parent = nullptr);

  ~LedIndicator() override = default;

  enum class State { Off, On, Error };

  void setState(State state);
  [[nodiscard]] State state() const;

 protected:
  void paintEvent(QPaintEvent* event) override;
  [[nodiscard]] QSize sizeHint() const override;

 private:
  State m_state;
  [[nodiscard]] QColor currentColor() const;
};
