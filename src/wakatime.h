// SPDX-License-Identifier: MIT
#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>

Q_DECLARE_LOGGING_CATEGORY(gLogWakaTime)

class QFileInfo;

/** Wrapper for the `wakatime-cli` binary. */
class WakaTime : public QObject {
    Q_OBJECT
#ifdef TESTING
    friend class WakaTimeClientTest;
#endif

public:
    /** State returned by send(). */
    enum State {
        ErrorSending,         /**< `wakatime-cli` exited with non-zero. */
        NothingToSend,        /**< Filename was empty. */
        SentSuccessfully,     /**< Successful request. */
        TooSoon,              /**< send() called too soon since last time. */
        WakaTimeCliNotInPath, /**< `wakatime` or `wakatime-cli` not in `PATH`. */
    };

    /** Constructor. */
    WakaTime(QObject *parent = nullptr);
    /** Find the full path to a file based on the `PATH` environment variable. Equivalent to
     * `command -v` or `which`. Does not check if the file is executable.
     *
     * @param binName The name of the binary to find.
     * @return The full path to the binary if found, otherwise an empty string.
     */
    QString getBinPath(const QString &binName);
    /**
     * Get the project name by traversing up until `.git` or `.svn` is found.
     *
     * @param fileInfo The QFileInfo of the file to get the project directory for.
     * @return The project directory name if found, otherwise an empty string.
     */
    QString getProjectDirectory(const QFileInfo &fileInfo);
    /**
     * Send statistics to WakaTime.
     *
     * @param filePath The file path to send statistics for.
     * @param mode The language mode of the file.
     * @param lineNumber The line number of the cursor position.
     * @param cursorPosition The column number of the cursor position.
     * @param linesInFile The total number of lines in the file.
     * @param isWrite Whether this is a write event (`true`) or just a heartbeat (`false`).
     * @return WakaTime::State indicating the result of the send operation.
     */
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
