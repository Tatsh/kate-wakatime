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
    void testApiKey();
    void testApiUrl();
    void testConfigureDialogKeepsPointer();
    void testHideFilenames();
    void testInit();
    void testShowDialogClearApiKey();
    void testShowDialogDoesNothingIfNotConfigured();
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

void WakaTimeConfigTest::testShowDialogClearApiKey() {
    WakaTimeConfig config;
    config.configureDialog();
    QVERIFY(config.dialog_ != nullptr);
    QVERIFY(!config.apiKey().isEmpty());
    // Ctrl+A to select all text in the API key field and delete it.
    QApplication::postEvent(
        config.ui_.lineEdit_apiKey,
        new QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::KeyboardModifier::ControlModifier));
    QApplication::postEvent(
        config.ui_.lineEdit_apiKey,
        new QKeyEvent(QEvent::KeyRelease, Qt::Key_A, Qt::KeyboardModifier::ControlModifier));
    QApplication::postEvent(config.ui_.lineEdit_apiKey,
                            new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier));
    QApplication::postEvent(config.ui_.lineEdit_apiKey,
                            new QKeyEvent(QEvent::KeyRelease, Qt::Key_Delete, Qt::NoModifier));
    QApplication::postEvent(config.ui_.lineEdit_apiKey,
                            new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
    QApplication::postEvent(config.ui_.lineEdit_apiKey,
                            new QKeyEvent(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier));
    config.showDialog();
    QVERIFY(config.apiKey().isEmpty());
}

void WakaTimeConfigTest::testConfigureDialogKeepsPointer() {
    WakaTimeConfig config;
    config.configureDialog();
    QVERIFY(config.dialog_ != nullptr);
    QDialog *oldDialog = config.dialog_;
    config.configureDialog();
    QCOMPARE(config.dialog_, oldDialog);
}

void WakaTimeConfigTest::testShowDialogDoesNothingIfNotConfigured() {
    WakaTimeConfig config;
    QVERIFY(config.dialog_ == nullptr);
    // If the dialog is not configured, this will not hang on dialog_.exec() but instead returns
    // immediately.
    config.showDialog();
}

QTEST_MAIN(WakaTimeConfigTest)

#include "configtest.moc"
