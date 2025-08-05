// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QSettings>

const auto kSettingsKeyApiKey = QStringLiteral("settings/api_key");
const auto kSettingsKeyApiUrl = QStringLiteral("settings/api_url");
const auto kSettingsKeyHideFilenames = QStringLiteral("settings/hidefilenames");

/**
 * Basic wrapper around QSettings to use WakaTime settings. Note that the save() method must be
 * called to persist changes.
 */
class WakaTimeConfig : public QObject {
    Q_OBJECT
#ifdef TESTING
    friend class WakaTimeConfigTest;
#endif

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
    /** Get the API key from the configuration.
     *
     * @return The API key.
     */
    QString apiKey() const {
        return config_->value(kSettingsKeyApiKey).toString();
    };
    /**
     * Set the API key.
     *
     * @param key The API key to set.
     */
    void setApiKey(const QString &key) {
        config_->setValue(kSettingsKeyApiKey, key);
    };
    /**
     * Get the API URL from the configuration.
     *
     * @return The API URL.
     */
    QString apiUrl() const {
        return config_->value(kSettingsKeyApiUrl, QStringLiteral("https://wakatime.com/api/v1/"))
            .toString();
    };
    /**
     * Set the API URL.
     *
     * @param url The API URL to set.
     */
    void setApiUrl(const QString &url) {
        config_->setValue(kSettingsKeyApiUrl, url);
    };
    /**
     * Check if filenames should be hidden.
     *
     * @return `true` if filenames should be hidden, `false` otherwise.
     */
    bool hideFilenames() const {
        return config_->value(kSettingsKeyHideFilenames, false).toBool();
    };
    /**
     * Set whether filenames should be hidden.
     *
     * @param hide Flag.
     */
    void setHideFilenames(bool hide) {
        config_->setValue(kSettingsKeyHideFilenames, hide);
    };
    /** Save the configuration settings to disk. */
    void save() const {
        config_->sync();
    };

private:
    QSettings *config_;
};
