// SPDX-License-Identifier: MIT
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtTest/QTest>

#include "wakatimeconfig.h"

Q_DECLARE_LOGGING_CATEGORY(gLogWakaTimeConfigTest)
Q_LOGGING_CATEGORY(gLogWakaTimeConfigTest, "wakatime-config-test")

class WakaTimeConfigTest : public QObject {
    Q_OBJECT

public:
    WakaTimeConfigTest(QObject *parent = nullptr);
    ~WakaTimeConfigTest() override;

private Q_SLOTS:
    void testInit();
    void testApiKey();
    void testApiUrl();
    void testHideFilenames();
};

WakaTimeConfigTest::WakaTimeConfigTest(QObject *parent) : QObject(parent) {
    Q_UNUSED(parent);
    QDir tempDir(QDir::tempPath() + QDir::separator() + QStringLiteral("kate-wakatime-test"));
    qputenv("HOME", tempDir.absolutePath().toUtf8());
}

WakaTimeConfigTest::~WakaTimeConfigTest() {
}

void WakaTimeConfigTest::testInit() {
    WakaTimeConfig config;
    QVERIFY(config.config_->fileName().endsWith(QStringLiteral(".wakatime.cfg")));
    QVERIFY(config.config_->fileName().startsWith(QDir::tempPath()));
}

void WakaTimeConfigTest::testApiKey() {
    WakaTimeConfig config;
    QString apiKey = QStringLiteral("test_api_key");
    config.setApiKey(apiKey);
    QCOMPARE(config.apiKey(), apiKey);
    config.save();
    WakaTimeConfig newConfig;
    QCOMPARE(newConfig.apiKey(), apiKey);
}

void WakaTimeConfigTest::testApiUrl() {
    WakaTimeConfig config;
    QString apiUrl = QStringLiteral("https://example.com/api/v1/");
    config.setApiUrl(apiUrl);
    QCOMPARE(config.apiUrl(), apiUrl);
    config.save();
    WakaTimeConfig newConfig;
    QCOMPARE(newConfig.apiUrl(), apiUrl);
}

void WakaTimeConfigTest::testHideFilenames() {
    WakaTimeConfig config;
    bool hideFilenames = true;
    config.setHideFilenames(hideFilenames);
    QCOMPARE(config.hideFilenames(), hideFilenames);
    config.save();
    WakaTimeConfig newConfig;
    QCOMPARE(newConfig.hideFilenames(), hideFilenames);
}

QTEST_MAIN(WakaTimeConfigTest)

#include "configtest.moc"
