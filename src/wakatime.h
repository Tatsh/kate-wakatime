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

public:
    WakaTime(QObject *parent = nullptr);
    QString getBinPath(const QString &binName);
    QString getProjectDirectory(const QFileInfo &fileInfo);
    void send(const QString &filePath,
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
