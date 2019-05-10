
/* Copyright (C) 2019 Jolla Ltd
 * Contact: Timur Kristóf <timur.kristof@jolla.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SMSSENDER_H
#define SMSSENDER_H

#include <QObject>
#include <QString>

namespace CommHistory {
    class GroupManager;
}

class ConversationChannel;
class ChannelManager;

class SmsSender : public QObject
{
    Q_OBJECT

public:
    explicit SmsSender(QObject *parent = nullptr);

    Q_INVOKABLE int sendSMS(const QString &modem, const QString &phoneNumber, const QString &text);

private Q_SLOTS:
    void channelSendingSucceeded(int eventId, ConversationChannel *sender);
    void channelSendingFailed(int eventId, ConversationChannel *sender);

Q_SIGNALS:
    void sendingSucceeded(int eventId);
    void sendingFailed(int eventId);

private:
    int ensureGroupExists(const QString &localUid, const QStringList &remoteUids);

    CommHistory::GroupManager *m_groupManager;
    ChannelManager *m_channelManager;

};

#endif // SMSSENDER_H
