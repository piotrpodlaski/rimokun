#include "TitleBar.hpp"
#include <algorithm>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kTitleBarHeight);

    // Initialize labels
    logoLeft = new QLabel;
    logoRight = new QLabel;
    titleLabel = new QLabel("My Application");

    logoLeft->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    logoRight->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");

    // Wrap logos in containers
    leftBox = new QWidget;
    rightBox = new QWidget;

    auto leftLayout = new QHBoxLayout(leftBox);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(logoLeft, 0, Qt::AlignLeft | Qt::AlignVCenter);

    auto rightLayout = new QHBoxLayout(rightBox);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(logoRight, 0, Qt::AlignRight | Qt::AlignVCenter);

    // Main layout
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 0, 10, 0);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(leftBox);
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(rightBox);
}

void TitleBar::setLeftLogo(const QPixmap &pixmap)
{
    QPixmap scaled = pixmap.scaledToHeight(kTitleBarHeight, Qt::SmoothTransformation);
    logoLeft->setPixmap(scaled);
    logoLeft->adjustSize();
    updateContainerWidths();
}

void TitleBar::setRightLogo(const QPixmap &pixmap)
{
    QPixmap scaled = pixmap.scaledToHeight(kTitleBarHeight, Qt::SmoothTransformation);
    logoRight->setPixmap(scaled);
    logoRight->adjustSize();
    updateContainerWidths();
}

void TitleBar::setTitleText(const QString &text)
{
    titleLabel->setText(text);
}

void TitleBar::updateContainerWidths() const {
  const int leftW = logoLeft->pixmap().width();
  const int rightW = logoRight->pixmap().width();
  const int maxW = std::max(leftW, rightW);

  leftBox->setFixedWidth(maxW);
  rightBox->setFixedWidth(maxW);
}
