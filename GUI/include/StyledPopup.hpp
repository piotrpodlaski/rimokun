#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

namespace StyledPopup {

void showErrorAsync(const QString& title, const QString& message,
                    QWidget* parent = nullptr);

QMessageBox::StandardButton askQuestion(
    const QString& title, const QString& message,
    QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
    QMessageBox::StandardButton defaultButton = QMessageBox::No,
    QWidget* parent = nullptr);

QMessageBox::StandardButton askWarning(
    const QString& title, const QString& message,
    QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
    QMessageBox::StandardButton defaultButton = QMessageBox::No,
    QWidget* parent = nullptr);

}  // namespace StyledPopup
