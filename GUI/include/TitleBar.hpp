#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>

class TitleBar final : public QWidget
{
  Q_OBJECT

public:
  explicit TitleBar(QWidget *parent = nullptr);

  void setLeftLogo(const QPixmap &pixmap);
  void setRightLogo(const QPixmap &pixmap);
  void setTitleText(const QString &text);

private:
  QLabel *logoLeft;
  QLabel *titleLabel;
  QLabel *logoRight;
  QWidget *leftBox;
  QWidget *rightBox;

  void updateContainerWidths() const;

  static constexpr int kTitleBarHeight = 60;
};