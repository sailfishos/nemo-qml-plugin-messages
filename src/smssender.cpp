
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

#include "smssender.h"

#include "channelmanager.h"

#include <CommHistory/groupmanager.h>
#include <CommHistory/singleeventmodel.h>
#include <QDebug>

using namespace CommHistory;

const QString ringBaseName = QStringLiteral("/org/freedesktop/Telepathy/Account/ring/tel");

SmsSender::SmsSender(QObject *parent)
    : QObject (parent)
    , m_groupManager(new CommHistory::GroupManager(this))
    , m_channelManager(new ChannelManager(this))
{

}

int SmsSender::sendSMS(const QString &modem, const QString &phoneNumber, const QString &text)
{
    QString localUid = ringBaseName + modem;
    QStringList remoteUids(phoneNumber);
    ConversationChannel *channel = m_channelManager->getConversation(localUid, phoneNumber);

    QObject::connect(channel, &ConversationChannel::sendingSucceeded, this, &SmsSender::channelSendingSucceeded, Qt::UniqueConnection);
    QObject::connect(channel, &ConversationChannel::sendingFailed, this, &SmsSender::channelSendingFailed, Qt::UniqueConnection);

    int groupId = ensureGroupExists(localUid, remoteUids);

    // Create event and add to model

    Event event;
    event.setType(Event::SMSEvent);
    event.setDirection(Event::Outbound);
    event.setIsRead(true);
    event.setGroupId(groupId);
    event.setLocalUid(localUid);
    event.setRecipients(RecipientList::fromUids(localUid, remoteUids));
    event.setFreeText(text);
    event.setStartTimeT(Event::currentTime_t());
    event.setEndTimeT(event.startTimeT());

    // If we don't get as far as marking this message as Sending, it should remain
    // in a temporarily-failed state to be manually retry-able
    event.setStatus(Event::TemporarilyFailedStatus);

    EventModel model;
    model.addEvent(event);

    channel->sendMessage(text, event.id());

    return event.id();
}

void SmsSender::channelSendingSucceeded(int eventId, ConversationChannel *sender)
{
    sender->deleteLater();
    emit sendingSucceeded(eventId);
}

void SmsSender::channelSendingFailed(int eventId, ConversationChannel *sender)
{
    qWarning() << Q_FUNC_INFO << "SMS send failed, marking it temporarily failed";

    SingleEventModel model;

    if (!model.getEventById(eventId)) {
        qWarning() << Q_FUNC_INFO << "No event with id" << eventId;
        return;
    }

    Event ev = model.event();
    const Event::EventStatus status = Event::TemporarilyFailedStatus;

    if (ev.status() != status) {
        ev.setStatus(Event::TemporarilyFailedStatus);
        bool success = model.modifyEvent(ev);

        if (!success) {
            qWarning() << Q_FUNC_INFO << "Could not set event status to temporarily failed:" << eventId;
        }
    }

    sender->deleteLater();
    emit sendingFailed(eventId);
}

int SmsSender::ensureGroupExists(const QString &localUid, const QStringList &remoteUids)
{
    // Try to find an appropriate group
    GroupObject *group = m_groupManager->findGroup(localUid, remoteUids);

    if (group) {
        return group->id();
    }

    Group g;
    g.setLocalUid(localUid);
    g.setRecipients(RecipientList::fromUids(localUid, remoteUids));
    g.setChatType(Group::ChatTypeP2P);

    if (!m_groupManager->addGroup(g)) {
        qWarning() << Q_FUNC_INFO << "Failed creating group";
        return -1;
    }

    return g.id();
}
