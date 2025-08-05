// SPDX-License-Identifier: MIT
#include <QtCore/QDir>
#include <QtCore/QProcess>

#include "wakatime.h"

Q_LOGGING_CATEGORY(gLogWakaTime, "wakatime")

const auto kStringLiteralSlash = QStringLiteral("/");
const auto kWakaTimeCli = QStringLiteral("wakatime-cli");

WakaTime::WakaTime(QObject *parent) : lastTimeSent(QDateTime::fromMSecsSinceEpoch(0)) {
    Q_UNUSED(parent);
}

QString WakaTime::getBinPath(const QString &binName) {
#ifdef Q_OS_WIN
    return QString();
#endif
    if (binPathCache.contains(binName)) {
        return binPathCache.value(binName);
    }
    auto dotWakaTime = QStringLiteral("%1/.wakatime").arg(QDir::homePath());
    static const auto *const kDefaultPath = "/usr/bin:/usr/local/bin:/opt/bin:/opt/local/bin";
    static const auto colon = QStringLiteral(":");
    const auto *const path = getenv("PATH");
    auto paths = QString::fromUtf8(path ? path : kDefaultPath).split(colon, Qt::SkipEmptyParts);
    paths.insert(0, dotWakaTime);
    for (auto path : paths) {
        for (auto entry : QDir(path).entryList()) {
            if (entry == binName) {
                entry = path.append(kStringLiteralSlash).append(entry);
                binPathCache[binName] = entry;
                return entry;
            }
        }
    }
    return QString();
}

QString WakaTime::getProjectDirectory(const QFileInfo &fileInfo) {
    QDir currentDirectory(fileInfo.canonicalPath());
    static QStringList filters;
    static const auto gitStr = QStringLiteral(".git");
    static const auto svnStr = QStringLiteral(".svn");
    filters << gitStr << svnStr;
    bool vcDirFound = false;
    while (!vcDirFound) {
        if (!currentDirectory.canonicalPath().compare(kStringLiteralSlash)) {
            break;
        }
        auto entries =
            currentDirectory.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
        for (const auto &entry : entries) {
            auto name = entry.fileName();
            if ((name == gitStr || name == svnStr) && entry.isDir()) {
                vcDirFound = true;
                return currentDirectory.dirName();
            }
        }
        currentDirectory.cdUp();
    }
    return QString();
}

WakaTime::State WakaTime::send(const QString &filePath,
                               const QString &mode,
                               int lineNumber,
                               int cursorPosition,
                               int linesInFile,
                               bool isWrite) {
    auto wakatimeCliPath = getBinPath(kWakaTimeCli);
    if (wakatimeCliPath.isEmpty()) {
        wakatimeCliPath = getBinPath(QStringLiteral("wakatime"));
        if (wakatimeCliPath.isEmpty()) {
            qCWarning(gLogWakaTime) << "wakatime-cli not found in PATH.";
            return WakaTimeCliNotInPath;
        }
    }
    // Could be untitled, or a URI (including HTTP). Only local files are handled for now.
    if (filePath.isEmpty()) {
        qCDebug(gLogWakaTime) << "Nothing to send about";
        return NothingToSend;
    }
    QStringList arguments;
    const QFileInfo fileInfo(filePath);
    // They have it sending the real file path, maybe not respecting symlinks, etc.
    auto canonicalFilePath = fileInfo.canonicalFilePath();
    qCDebug(gLogWakaTime) << "File path:" << canonicalFilePath;
    // Compare date and make sure it has been at least 15 minutes.
    const auto currentMs = QDateTime::currentMSecsSinceEpoch();
    const auto deltaMs = currentMs - lastTimeSent.toMSecsSinceEpoch();
    static const auto intervalMs = 120000; // ms
    // If the current file has not changed and it has not been 2 minutes since the last heartbeat
    // was sent, do NOT send this heartbeat. This does not apply to write events as they are always
    // sent.
    if (!isWrite) {
        if (hasSent && deltaMs <= intervalMs && lastFileSent == canonicalFilePath) {
            qCDebug(gLogWakaTime) << "Not enough time has passed since last send";
            qCDebug(gLogWakaTime) << "Delta:" << deltaMs / 1000 / 60 << "/ 2 minutes";
            return TooSoon;
        }
    }
    arguments << QStringLiteral("--entity") << canonicalFilePath;
    arguments << QStringLiteral("--plugin")
              << QStringLiteral("ktexteditor-wakatime/%1").arg(VERSION);
    auto projectName = getProjectDirectory(fileInfo);
    if (!projectName.isEmpty()) {
        arguments << QStringLiteral("--alternate-project") << projectName;
    } else {
        // LCOV_EXCL_START
        qCDebug(gLogWakaTime) << "Warning: No project name found";
        // LCOV_EXCL_STOP
    }
    if (isWrite) {
        arguments << QStringLiteral("--write");
    }
    if (!mode.isEmpty()) {
        arguments << QStringLiteral("--language") << mode;
    }
    arguments << QStringLiteral("--lineno") << QString::number(lineNumber);
    arguments << QStringLiteral("--cursorpos") << QString::number(cursorPosition);
    arguments << QStringLiteral("--lines-in-file") << QString::number(linesInFile);
    qCDebug(gLogWakaTime) << "Running:" << wakatimeCliPath << arguments.join(QStringLiteral(" "));
    auto ret = QProcess::execute(wakatimeCliPath, arguments);
    if (ret != 0) {
        qCWarning(gLogWakaTime) << "wakatime-cli returned error code" << ret;
        return ErrorSending;
    }
    lastTimeSent = QDateTime::currentDateTime();
    lastFileSent = canonicalFilePath;
    hasSent = true;
    return SentSuccessfully;
}
