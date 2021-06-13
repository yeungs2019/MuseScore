/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "ipcserver.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>

#ifdef Q_OS_UNIX
#include <QFile>
#include <QDir>
#endif

#include "async/async.h"
#include "ipclock.h"
#include "ipclog.h"

using namespace mu::ipc;

IpcServer::~IpcServer()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
    }
    delete m_server;
}

bool IpcServer::listen(const QString& serverName)
{
    if (!m_server) {
        m_lock = new IpcLock(serverName);
        m_server = new QLocalServer();
    }

    bool ok = m_server->listen(serverName);

#ifdef Q_OS_UNIX
    // Workaround
    if (!ok && m_server->serverError() == QAbstractSocket::AddressInUseError) {
        QFile::remove(QDir::cleanPath(QDir::tempPath()) + QLatin1Char('/') + serverName);
        ok = m_server->listen(serverName);
    }
#endif

    if (!ok) {
        LOGE() << "failed listen: " << m_server->errorString();
        return false;
    }

    QObject::connect(m_server, &QLocalServer::newConnection, [this]() {
        QLocalSocket* socket = m_server->nextPendingConnection();
        IPCLOG() << "newConnection socket: " << socket;
        if (!socket) {
            return;
        }

        QObject::connect(socket, &QLocalSocket::errorOccurred, [](QLocalSocket::LocalSocketError err) {
            LOGE() << "socket error: " << socketErrorToString(err);
        });

        QObject::connect(socket, &QLocalSocket::disconnected, [socket, this]() {
            IPCLOG() << "disconnected socket: " << socket;
            onDisconnected(socket);
        });

        socket->waitForReadyRead(TIMEOUT_MSEC);
        QByteArray id = socket->readAll();

        IncomingSocket inc;
        inc.socket = socket;
        inc.meta.id = QString::fromUtf8(id);
        m_incomingSockets.append(inc);

        LOGI() << "id: " << id;

        QObject::connect(socket, &QLocalSocket::readyRead, [socket, this]() {
            onIncomingReadyRead(socket);
        });

        // askWhoIs(socket);

        sendMetaInfoToAllIncoming();
    });

    if (!m_thread) {
        m_thread = new QThread();
    }

    m_server->moveToThread(m_thread);
    m_thread->start();

    return true;
}

void IpcServer::onIncomingReadyRead(QLocalSocket* socket)
{
    QByteArray data = socket->readAll();

    IPCLOG() << data;

//    async::Async::call(this, [this, socket, data]() {
    Meta meta;
    Msg msg;
    deserialize(data, meta, msg);

    IPCLOG() << "incoming [" << meta.id << "] data: " << data;

    if (msg.method == IPC_INIT) {
        onIncomingInit(socket, meta, msg);
    }

    if (msg.method == IPC_WHOIS) {
        onIncomingWhoIs(socket, meta, msg);
    }

    if (msg.method == IPC_PING) {
        onIncomingPing(socket, meta, msg);
    }

    //! NOTE Resend to others (broadcast)
    if (msg.destID == BROADCAST_ID) {
        for (IncomingSocket& s : m_incomingSockets) {
            //! NOTE We do not resend to incoming socket
            if (socket != s.socket) {
                IPCLOG() << "resend to " << s.meta.id;
                doSendToSocket(s.socket, data);
            }
        }
    }
//    });
}

void IpcServer::onIncomingInit(QLocalSocket* socket, const Meta& meta, const Msg& msg)
{
    UNUSED(msg);

    IPCLOG() << "init from: " << meta.id;

    IncomingSocket& s = incomingSocket(socket);
    if (!s.socket) {
        LOGE() << "not found incoming socket";
        return;
    }
    s.meta = meta;

    sendMetaInfoToAllIncoming();
}

void IpcServer::onIncomingWhoIs(QLocalSocket* socket, const Meta& meta, const Msg& msg)
{
    UNUSED(msg);

    IPCLOG() << "who is answer: " << meta.id;

    IncomingSocket& s = incomingSocket(socket);
    if (!s.socket) {
        LOGE() << "not found incoming socket";
        return;
    }
    s.meta = meta;

    sendMetaInfoToAllIncoming();
}

void IpcServer::onIncomingPing(QLocalSocket* socket, const Meta& meta, const Msg& msg)
{
    UNUSED(msg);

    IPCLOG() << "ping from: " << meta.id;

    IncomingSocket& s = incomingSocket(socket);
    if (!s.socket) {
        LOGE() << "not found incoming socket";
        return;
    }
    s.meta = meta;

    sendMetaInfoToAllIncoming();
}

bool IpcServer::doSendToSocket(QLocalSocket* socket, const QByteArray& data)
{
    IPCLOG() << data;

    m_lock->lock();

    socket->write(data);
    bool ok = socket->waitForBytesWritten(TIMEOUT_MSEC);
    if (!ok) {
        LOGE() << "failed write data to socket";
        return false;
    }

    m_lock->unlock();

    return true;
}

void IpcServer::sendToSocket(QLocalSocket* socket, const Msg& msg)
{
    Meta meta;
    meta.id = SERVER_ID;

    QByteArray data;
    serialize(meta, msg, data);

    doSendToSocket(socket, data);
}

void IpcServer::askWhoIs(QLocalSocket* socket)
{
    Msg askMsg;
    askMsg.destID = DIRECT_SOCKET_ID;
    askMsg.method = IPC_WHOIS;
    sendToSocket(socket, askMsg);
}

void IpcServer::sendMetaInfoToAllIncoming()
{
    Meta meta;
    meta.id = SERVER_ID;

    Msg msg;
    msg.destID = DIRECT_SOCKET_ID;
    msg.method = IPC_METAINFO;

    msg.args << QString::number(m_incomingSockets.count());
    for (const IncomingSocket& s : qAsConst(m_incomingSockets)) {
        msg.args << s.meta.id;
    }

    QByteArray data;
    serialize(meta, msg, data);

    for (IncomingSocket& s : m_incomingSockets) {
        doSendToSocket(s.socket, data);
    }
}

void IpcServer::onDisconnected(QLocalSocket* socket)
{
    int index = -1;
    for (int i = 0; i < m_incomingSockets.count(); ++i) {
        if (m_incomingSockets.at(i).socket == socket) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        LOGW() << "not found socket";
        return;
    }

    m_incomingSockets.removeAt(index);

    sendMetaInfoToAllIncoming();
}

IpcServer::IncomingSocket& IpcServer::incomingSocket(QLocalSocket* socket)
{
    for (IncomingSocket& s : m_incomingSockets) {
        if (s.socket == socket) {
            return s;
        }
    }

    static IncomingSocket null;
    return null;
}

const IpcServer::IncomingSocket& IpcServer::incomingSocket(QLocalSocket* socket) const
{
    for (const IncomingSocket& s : m_incomingSockets) {
        if (s.socket == socket) {
            return s;
        }
    }

    static IncomingSocket null;
    return null;
}