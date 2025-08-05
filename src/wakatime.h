// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>

Q_DECLARE_LOGGING_CATEGORY(gLogWakaTime)

class QFileInfo;

class WakaTime : public QObject {
    Q_OBJECT
#ifdef TESTING
    friend class WakaTimeClientTest;
#endif

public:
    enum State {
        ErrorSending,
        NothingToSend,
        SentSuccessfully,
        TooSoon,
        WakaTimeCliNotInPath,
    };

    WakaTime(QObject *parent = nullptr);
    QString getBinPath(const QString &binName);
    QString getProjectDirectory(const QFileInfo &fileInfo);
    WakaTime::State send(const QString &filePath,
                         const QString &mode,
                         int lineNumber,
                         int cursorPosition,
                         int linesInFile,
                         bool isWrite);

private:
    QDateTime lastTimeSent;
    QMap<QString, QString> binPathCache;
    QString lastFileSent;
    bool hasSent = false;
};
