// SPDX-License-Identifier: MIT
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtTest/QTest>

#include "wakatime.h"

Q_DECLARE_LOGGING_CATEGORY(gLogWakaTimeClientTest)
Q_LOGGING_CATEGORY(gLogWakaTimeClientTest, "wakatime-config-test")

class WakaTimeClientTest : public QObject {
    Q_OBJECT

public:
    WakaTimeClientTest(QObject *parent = nullptr);
    ~WakaTimeClientTest() override;

private Q_SLOTS:
    void testGetBinPathNotFound();
    void testGetBinPathFound();
    void testGetProjectDirectoryNotFound();
    void testGetProjectDirectoryFound();
    void testSendWakaTimeCliNotInPath();
    void testSendEmptyFilePath();
    void testSendSuccessful();
    void testSendErrorSending();
    void testSendTooSoon();

private:
    char *oldHome;
    char *oldPath;
};

WakaTimeClientTest::WakaTimeClientTest(QObject *parent)
    : QObject(parent), oldHome(getenv("HOME")), oldPath(getenv("PATH")) {
    Q_UNUSED(parent);
}

WakaTimeClientTest::~WakaTimeClientTest() {
}

void WakaTimeClientTest::testGetBinPathNotFound() {
    qputenv("PATH", QByteArrayLiteral(""));
    WakaTime wakatime;
    QString binPath = wakatime.getBinPath(QStringLiteral("zzzzzzzzzzzzz"));
    QVERIFY(binPath.isEmpty());
    qputenv("PATH", QByteArray(oldPath));
}

void WakaTimeClientTest::testGetBinPathFound() {
    QDir tempDir(QDir::tempPath() + QDir::separator() +
                 QStringLiteral("kate-wakatime-client-test"));
    auto newPath = tempDir.absolutePath().toUtf8();
    qputenv("PATH", newPath);

    QDir::temp().mkdir(QStringLiteral("kate-wakatime-client-test"));
    QFile someExec(tempDir.filePath(QStringLiteral("some-executable")));
    someExec.open(QIODevice::WriteOnly);
    someExec.write("echo 'Hello World'");
    someExec.close();

    WakaTime wakatime;
    QString binPath = wakatime.getBinPath(QStringLiteral("some-executable"));
    QVERIFY(!binPath.isEmpty());
    QVERIFY(binPath.endsWith(QStringLiteral("/some-executable")));

    // Cache call.
    QVERIFY(wakatime.binPathCache.contains(QStringLiteral("some-executable")));
    wakatime.getBinPath(QStringLiteral("some-executable"));

    qputenv("PATH", QByteArray(oldPath));
}

void WakaTimeClientTest::testGetProjectDirectoryNotFound() {
    WakaTime wakatime;
    QFileInfo fileInfo(QStringLiteral("/"));
    QString projectDir = wakatime.getProjectDirectory(fileInfo);
    QVERIFY(projectDir.isEmpty());
}

void WakaTimeClientTest::testGetProjectDirectoryFound() {
    WakaTime wakatime;
    QFileInfo fileInfo(QDir::tempPath() + QDir::separator() +
                       QStringLiteral("kate-wakatime-client-test"));
    QDir tempDir(fileInfo.absoluteFilePath());
    tempDir.mkdir(QStringLiteral(".git"));
    tempDir.mkdir(QStringLiteral("src"));
    tempDir.mkdir(QStringLiteral("src/autotests"));
    QFileInfo autotestsDir(tempDir.filePath(QStringLiteral("src/autotests")));
    QString projectDir = wakatime.getProjectDirectory(autotestsDir);
    QVERIFY(!projectDir.isEmpty());
    QVERIFY(projectDir.endsWith(QStringLiteral("kate-wakatime-client-test")));
}

void WakaTimeClientTest::testSendWakaTimeCliNotInPath() {
    qputenv("HOME", QByteArrayLiteral("/non/existent/path"));
    qputenv("PATH", QByteArrayLiteral(""));
    WakaTime wakatime;
    auto state = wakatime.send(
        QStringLiteral("/path/to/some/file.cpp"), QStringLiteral("cpp"), 10, 5, 100, false);
    QVERIFY(state == WakaTime::WakaTimeCliNotInPath);
    qputenv("PATH", QByteArray(oldPath));
    qputenv("HOME", QByteArray(oldHome));
}

void WakaTimeClientTest::testSendEmptyFilePath() {
    qputenv("HOME", QByteArrayLiteral("/non/existent/path"));
    QDir tempDir(QDir::tempPath() + QDir::separator() +
                 QStringLiteral("kate-wakatime-client-test"));
    auto newPath = tempDir.absolutePath().toUtf8();
    qputenv("PATH", newPath);

    QDir::temp().mkdir(QStringLiteral("kate-wakatime-client-test"));
    QFile someExec(tempDir.filePath(QStringLiteral("wakatime")));
    someExec.open(QIODevice::WriteOnly);
    someExec.write("#!/bin/sh\n");
    someExec.write("exit 1\n");
    someExec.close();
    someExec.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser |
                            QFileDevice::ReadGroup | QFileDevice::ReadOther);

    WakaTime wakatime;
    auto state = wakatime.send(QString(), QStringLiteral("cpp"), 10, 5, 100, false);
    QVERIFY(state == WakaTime::NothingToSend);

    qputenv("PATH", QByteArray(oldPath));
    qputenv("HOME", QByteArray(oldHome));
}

void WakaTimeClientTest::testSendSuccessful() {
    qputenv("HOME", QByteArrayLiteral("/non/existent/path"));
    QDir tempDir(QDir::tempPath() + QDir::separator() +
                 QStringLiteral("kate-wakatime-client-test"));
    auto newPath = tempDir.absolutePath().toUtf8();
    qputenv("PATH", newPath);

    QDir::temp().mkdir(QStringLiteral("kate-wakatime-client-test"));
    QFile someExec(tempDir.filePath(QStringLiteral("wakatime")));
    someExec.open(QIODevice::WriteOnly);
    someExec.write("#!/bin/sh\n");
    someExec.write("echo 'Hello World'\n");
    someExec.close();
    someExec.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser |
                            QFileDevice::ReadGroup | QFileDevice::ReadOther);

    WakaTime wakatime;
    auto state = wakatime.send(
        tempDir.filePath(QStringLiteral("some-file.cpp")), QStringLiteral("cpp"), 10, 5, 100, true);
    QVERIFY(state == WakaTime::SentSuccessfully);

    qputenv("PATH", QByteArray(oldPath));
    qputenv("HOME", QByteArray(oldHome));
}

void WakaTimeClientTest::testSendErrorSending() {
    qputenv("HOME", QByteArrayLiteral("/non/existent/path"));
    QDir tempDir(QDir::tempPath() + QDir::separator() +
                 QStringLiteral("kate-wakatime-client-test"));
    auto newPath = tempDir.absolutePath().toUtf8();
    qputenv("PATH", newPath);

    QDir::temp().mkdir(QStringLiteral("kate-wakatime-client-test"));
    QFile someExec(tempDir.filePath(QStringLiteral("wakatime")));
    someExec.open(QIODevice::WriteOnly);
    someExec.write("#!/bin/sh\n");
    someExec.write("exit 1\n");
    someExec.close();
    someExec.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser |
                            QFileDevice::ReadGroup | QFileDevice::ReadOther);

    WakaTime wakatime;
    auto state = wakatime.send(
        tempDir.filePath(QStringLiteral("some-file.cpp")), QStringLiteral("cpp"), 10, 5, 100, true);
    QVERIFY(state == WakaTime::ErrorSending);

    qputenv("PATH", QByteArray(oldPath));
    qputenv("HOME", QByteArray(oldHome));
}

void WakaTimeClientTest::testSendTooSoon() {
    qputenv("HOME", QByteArrayLiteral("/non/existent/path"));
    QDir tempDir(QDir::tempPath() + QDir::separator() +
                 QStringLiteral("kate-wakatime-client-test"));
    auto newPath = tempDir.absolutePath().toUtf8();
    qputenv("PATH", newPath);

    QDir::temp().mkdir(QStringLiteral("kate-wakatime-client-test"));
    QFile someExec(tempDir.filePath(QStringLiteral("wakatime")));
    someExec.open(QIODevice::WriteOnly);
    someExec.write("#!/bin/sh\n");
    someExec.write("echo 'Hello World'\n");
    someExec.close();
    someExec.setPermissions(QFileDevice::ExeUser | QFileDevice::ReadUser | QFileDevice::WriteUser |
                            QFileDevice::ReadGroup | QFileDevice::ReadOther);

    WakaTime wakatime;
    auto state = wakatime.send(tempDir.filePath(QStringLiteral("some-file.cpp")),
                               QStringLiteral("cpp"),
                               10,
                               5,
                               100,
                               false);
    QVERIFY(state == WakaTime::SentSuccessfully);

    // Send again immediately, should be too soon.
    state = wakatime.send(tempDir.filePath(QStringLiteral("some-file.cpp")),
                          QStringLiteral("cpp"),
                          10,
                          5,
                          100,
                          false);
    QVERIFY(state == WakaTime::TooSoon);

    qputenv("PATH", QByteArray(oldPath));
    qputenv("HOME", QByteArray(oldHome));
}

QTEST_MAIN(WakaTimeClientTest)

#include "clienttest.moc"
