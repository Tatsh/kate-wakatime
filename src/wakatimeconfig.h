#pragma once

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QSettings>

const auto kSettingsKeyApiKey = QStringLiteral("settings/api_key");
const auto kSettingsKeyApiUrl = QStringLiteral("settings/api_url");
const auto kSettingsKeyHideFilenames = QStringLiteral("settings/hidefilenames");

class WakaTimeConfig : public QObject {
    Q_OBJECT
public:
    explicit WakaTimeConfig(QObject *parent = nullptr) {
        Q_UNUSED(parent);
        config_ =
            new QSettings(QDir::homePath() + QDir::separator() + QStringLiteral(".wakatime.cfg"),
                          QSettings::IniFormat,
                          this);
    };
    ~WakaTimeConfig() override {
        delete config_;
    };
    QString apiKey() const {
        return config_->value(kSettingsKeyApiKey).toString();
    };
    void setApiKey(const QString &key) {
        config_->setValue(kSettingsKeyApiKey, key);
    };
    QString apiUrl() const {
        return config_->value(kSettingsKeyApiUrl, QStringLiteral("https://wakatime.com/api/v1/"))
            .toString();
    };
    void setApiUrl(const QString &url) {
        config_->setValue(kSettingsKeyApiUrl, url);
    };
    bool hideFilenames() const {
        return config_->value(kSettingsKeyHideFilenames, false).toBool();
    };
    void setHideFilenames(bool hide) {
        config_->setValue(kSettingsKeyHideFilenames, hide);
    };
    void save() const {
        config_->sync();
    };

private:
    QSettings *config_;
};
