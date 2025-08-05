// SPDX-License-Identifier: MIT
#include "wakatimeconfig.h"

Q_LOGGING_CATEGORY(gLogWakaTimeConfig, "wakatime-config")

void WakaTimeConfig::configureDialog(QWidget *parent, Qt::WindowFlags flags) {
    if (dialog_) {
        dialog_->setParent(parent);
        dialog_->setWindowFlags(flags);
        return;
    }
    dialog_ = new QDialog(parent, flags);
    ui_.setupUi(dialog_);
    ui_.lineEdit_apiKey->setClearButtonEnabled(true);
    ui_.lineEdit_apiUrl->setClearButtonEnabled(true);
    ui_.lineEdit_apiKey->setPlaceholderText(i18n("Enter your WakaTime API key."));
    ui_.lineEdit_apiKey->setFocus();
    ui_.lineEdit_apiUrl->setPlaceholderText(i18n("Enter your WakaTime API URL."));
    dialog_->setWindowTitle(i18n("Configure WakaTime"));
    ui_.lineEdit_apiKey->setText(apiKey());
    ui_.lineEdit_apiUrl->setText(apiUrl());
    ui_.checkBox_hideFilenames->setChecked(hideFilenames());
};

void WakaTimeConfig::showDialog() {
    if (!dialog_) {
        qCWarning(gLogWakaTimeConfig) << "Dialog not configured. Call configureDialog() first.";
        return;
    }
    if (dialog_->exec() == QDialog::Accepted) {
        setApiKey(ui_.lineEdit_apiKey->text());
        setApiUrl(ui_.lineEdit_apiUrl->text());
        setHideFilenames(ui_.checkBox_hideFilenames->isChecked());
        save();
    }
}
