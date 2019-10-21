/*
 * Offline queue database for WakaTime.
 * Copyright 2019 Andrew Udvare <audvare@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtCore/QDir>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "offlinequeue.h"

Q_LOGGING_CATEGORY(gLogOfflineQueue, "offlinequeue")

OfflineQueue::OfflineQueue()
    : db(QSqlDatabase::addDatabase(QLatin1String("QSQLITE"))),
      m_failed(false) {
}

OfflineQueue::~OfflineQueue() {
    if (!m_failed) {
        db.close();
    }
}

QSqlDatabase &OfflineQueue::connect() {
    if (db.isOpen()) {
        qCDebug(gLogOfflineQueue)
            << "connect(): Returning existing db instance";
        return db;
    }
    if (m_failed) {
        return db;
    }

    db.setDatabaseName(QDir::homePath() + QDir::separator() +
                       QLatin1String(".wakatime.db"));
    if (!db.open()) {
        qCCritical(gLogOfflineQueue) << "db.open() failed!";
        m_failed = true;
        return db;
    }
    QStringList tables = db.tables();
    if (!tables.contains(QLatin1String("heartbeat_2"))) {
        qCDebug(gLogOfflineQueue) << "Creating database table heartbeat_2";
        QSqlQuery q(db);
        q.exec(QLatin1String(
            "CREATE TABLE heartbeat_2 (id TEXT, heartbeat TEXT)"));
    }
    qCDebug(gLogOfflineQueue) << "connect() successful";
    return db;
}

void OfflineQueue::push(QStringList &heartbeat) {
    connect();
    if (m_failed) {
        qCCritical(gLogOfflineQueue) << "push(): Failed to connect";
        return;
    }

    QSqlQuery q(db);
    q.prepare(QLatin1String(
        "INSERT INTO heartbeat_2 (id, heartbeat) VALUES (:id, :heartbeat)"));
    q.bindValue(QLatin1String(":id"), heartbeat.at(0));
    q.bindValue(QLatin1String(":heartbeat"), heartbeat.at(1));
    m_failed = !q.exec();
    if (m_failed) {
        qCCritical(gLogOfflineQueue) << "push(): Failed to insert row";
    }
}

QStringList OfflineQueue::pop() {
    connect();
    if (m_failed) {
        qCCritical(gLogOfflineQueue) << "pop(): Failed to connect";
        return QStringList();
    }

    int tries = 3;
    bool loop = true;
    QStringList heartbeat;

    while (loop && tries > -1) {
        QSqlQuery q(db);
        if (!q.exec(QLatin1String("SELECT * FROM heartbeat_2 LIMIT 1"))) {
            qCCritical(gLogOfflineQueue) << "pop(): SELECT failed!";
            tries--;
            continue;
        }
        while (q.next()) {
            QString id = q.value(0).toString();
            heartbeat << id;
            heartbeat << q.value(1).toString();
            QSqlQuery delQuery(db);
            const QString queryStr =
                QStringLiteral("DELETE FROM heartbeat_2 WHERE id = '%1'")
                    .arg(id);
            if (db.transaction() && delQuery.exec(queryStr)) {
                db.commit();
                qCDebug(gLogOfflineQueue)
                    << "pop(): executed" << delQuery.executedQuery();
                break;
            } else {
                db.rollback();
                qCCritical(gLogOfflineQueue) << "pop(): DELETE failed!";
                tries--;
            }
        }
        loop = false;
    }

    return heartbeat;
}

void OfflineQueue::pushMany(QList<QStringList> &heartbeats) {
    for (QStringList heartbeat : heartbeats) {
        push(heartbeat);
    }
}

QList<QStringList> OfflineQueue::popMany(const int limit) {
    QList<QStringList> ret;
    for (int i = 0; limit == -1 || i < limit; i++) {
        QStringList poppedRecord = pop();
        if (poppedRecord.isEmpty()) {
            break;
        }
        ret << poppedRecord;
    }
    return ret;
}
