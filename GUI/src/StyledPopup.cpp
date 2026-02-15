#include "StyledPopup.hpp"

#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QPalette>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <algorithm>
#include <cmath>

namespace {
QWidget* bestParentWindow(QWidget* preferredParent) {
  if (preferredParent) {
    return preferredParent;
  }
  if (QWidget* w = QApplication::activeWindow()) {
    return w;
  }
  const auto tops = QApplication::topLevelWidgets();
  for (QWidget* w : tops) {
    if (w && w->isVisible()) {
      return w;
    }
  }
  return nullptr;
}

QScreen* bestScreen(QWidget* parent) {
  if (parent && parent->windowHandle() && parent->windowHandle()->screen()) {
    return parent->windowHandle()->screen();
  }
  if (QScreen* screen = QGuiApplication::screenAt(QCursor::pos())) {
    return screen;
  }
  return QGuiApplication::primaryScreen();
}

void centerOnScreen(QWidget* widget, QWidget* parent = nullptr) {
  QScreen* screen = bestScreen(parent);
  if (!screen) {
    return;
  }
  const QRect avail = screen->availableGeometry();
  widget->adjustSize();
  const QSize size = widget->size();
  const QPoint position = avail.center() - QPoint(size.width() / 2, size.height() / 2);
  widget->move(position);
}

double luminance01(const QColor& color) {
  auto gamma = [](double value) {
    value /= 255.0;
    return (value <= 0.04045) ? (value / 12.92)
                              : std::pow((value + 0.055) / 1.055, 2.4);
  };
  const double red = gamma(color.red());
  const double green = gamma(color.green());
  const double blue = gamma(color.blue());
  return 0.2126 * red + 0.7152 * green + 0.0722 * blue;
}

QColor offsetShade(const QColor& base, bool makeLighter, int amount) {
  return makeLighter ? base.lighter(amount) : base.darker(amount);
}

QString cssColor(const QColor& color) { return color.name(QColor::HexArgb); }

void applyStyle(QMessageBox& msgBox, const QString& text) {
  const QString safe = text.toHtmlEscaped();
  msgBox.setTextFormat(Qt::RichText);
  msgBox.setText(QString(
      "<div style='font-size:20pt; font-weight:800; margin:8px 0 6px 0;'>%1</div>")
                     .arg(safe));

  QFont font = msgBox.font();
  font.setPointSizeF(std::max(12.0, font.pointSizeF() + 3.0));
  msgBox.setFont(font);
  msgBox.setMinimumWidth(560);

  const QPalette palette = QApplication::palette();
  const QColor win = palette.color(QPalette::Window);
  const QColor textColor = palette.color(QPalette::WindowText);
  const QColor button = palette.color(QPalette::Button);
  const QColor buttonText = palette.color(QPalette::ButtonText);
  const QColor highlight = palette.color(QPalette::Highlight);

  const bool isDark = luminance01(win) < 0.35;
  const QColor panelBg = offsetShade(win, !isDark, 112);
  const QColor accent = offsetShade(highlight, !isDark, 120);
  const QColor buttonHover = offsetShade(button, !isDark, 112);
  const QColor buttonBorder = offsetShade(button, isDark, 125);

  msgBox.setStyleSheet(QString(R"(
    QMessageBox {
      background: %1;
      color: %2;
      border: 2px solid %3;
      border-radius: 12px;
    }
    QMessageBox QLabel {
      color: %2;
      padding: 10px 14px;
    }
    QMessageBox QPushButton {
      font-size: 13pt;
      padding: 8px 18px;
      border-radius: 10px;
      background: %4;
      border: 1px solid %5;
      min-width: 120px;
      color: %6;
    }
    QMessageBox QPushButton:hover { background: %7; }
    QMessageBox QPushButton:default { border: 2px solid %3; }
  )")
                       .arg(cssColor(panelBg), cssColor(textColor), cssColor(accent),
                            cssColor(button), cssColor(buttonBorder),
                            cssColor(buttonText), cssColor(buttonHover)));
}

QMessageBox::StandardButton askInternal(QMessageBox::Icon icon, const QString& title,
                                        const QString& text,
                                        QMessageBox::StandardButtons buttons,
                                        QMessageBox::StandardButton defaultButton,
                                        QWidget* parent) {
  QWidget* resolvedParent = bestParentWindow(parent);
  QMessageBox msgBox(icon, title, "", buttons, resolvedParent);
  msgBox.setDefaultButton(defaultButton);
  applyStyle(msgBox, text);
  QTimer::singleShot(0, &msgBox, [&msgBox, resolvedParent] {
    centerOnScreen(&msgBox, resolvedParent);
    msgBox.raise();
    msgBox.activateWindow();
  });
  return static_cast<QMessageBox::StandardButton>(msgBox.exec());
}
}  // namespace

namespace StyledPopup {

void showErrorAsync(const QString& title, const QString& message, QWidget* parent) {
  QWidget* resolvedParent = bestParentWindow(parent);
  auto* msgBox = new QMessageBox(QMessageBox::Critical, title, "",
                                 QMessageBox::Ok, resolvedParent);
  applyStyle(*msgBox, message);
  msgBox->setAttribute(Qt::WA_DeleteOnClose);
  msgBox->setModal(false);
  msgBox->open();
  QTimer::singleShot(0, msgBox, [msgBox, resolvedParent] {
    centerOnScreen(msgBox, resolvedParent);
    msgBox->raise();
    msgBox->activateWindow();
  });
}

QMessageBox::StandardButton askQuestion(const QString& title, const QString& message,
                                        QMessageBox::StandardButtons buttons,
                                        QMessageBox::StandardButton defaultButton,
                                        QWidget* parent) {
  return askInternal(QMessageBox::Question, title, message, buttons, defaultButton,
                     parent);
}

QMessageBox::StandardButton askWarning(const QString& title, const QString& message,
                                       QMessageBox::StandardButtons buttons,
                                       QMessageBox::StandardButton defaultButton,
                                       QWidget* parent) {
  return askInternal(QMessageBox::Warning, title, message, buttons, defaultButton,
                     parent);
}

}  // namespace StyledPopup
