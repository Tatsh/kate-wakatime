/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright 2019  <copyright holder> <email>
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
#include <QtSql/QSqlQuery>

#include "offlinequeue.h"

OfflineQueue::OfflineQueue()
    : db(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"))),
      m_failed(false) {
}

OfflineQueue::~OfflineQueue() {
    if (!m_failed) {
        db.close();
    }
}

QSqlDatabase &OfflineQueue::connect() {
    if (db.isOpen() || m_failed) {
        return db;
    }

    db.setDatabaseName(QDir::homePath() + QDir::separator() +
                       QStringLiteral(".wakatime.db"));
    if (!db.open()) {
        m_failed = true;
        return db;
    }
    QStringList tables = db.tables();
    if (!tables.contains(QStringLiteral("heartbeat_2"))) {
        QSqlQuery q(db);
        q.exec(QStringLiteral(
            "CREATE TABLE heartbeat_2 (id TEXT, heartbeat TEXT)"));
    }
    return db;
}

void OfflineQueue::push(QStringList &heartbeat) {
    connect();
    if (m_failed) {
        return;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO heartbeat_2 (id, heartbeat) VALUES (:id, :heartbeat)"));
    q.bindValue(QStringLiteral(":id"), heartbeat.at(0));
    q.bindValue(QStringLiteral(":heartbeat"), heartbeat.at(1));
    m_failed = !q.exec();
}

QStringList OfflineQueue::pop() {
    connect();
    if (m_failed) {
        return QStringList();
    }

    int tries = 3;
    bool loop = true;
    QStringList heartbeat;
    while (loop && tries > -1) {
        QSqlQuery q(db);
        if (!q.exec(QStringLiteral("BEGIN IMMEDIATE"))) {
            tries--;
            continue;
        }
        if (!q.exec(QStringLiteral("SELECT * FROM heartbeat_2 LIMIT 1"))) {
            tries--;
            continue;
        }
        while (q.next()) {
            QString id = q.value(0).toString();
            heartbeat << id;
            heartbeat << q.value(1).toString();
            q.prepare(
                QStringLiteral("DELETE FROM heartbeat_2 WHERE id = :id"));
            q.bindValue(QStringLiteral(":id"), id);
            if (!q.exec()) {
                tries--;
                continue;
            } else {
                loop = false;
                break;
            }
        }
    }

    return heartbeat;
}

void OfflineQueue::pushMany(QList<QStringList> &heartbeats) {
    for (QStringList heartbeat : heartbeats) {
        push(heartbeat);
    }
}

QList<QStringList> OfflineQueue::popMany(int limit) {
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
