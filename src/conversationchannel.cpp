/* Copyright (C) 2013 Jolla Ltd
 * Copyright (C) 2012 John Brooks <john.brooks@dereferenced.net>
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

#include "conversationchannel.h"
#include "channelmanager.h"

#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/Channel>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Contact>
#include <TelepathyQt/Account>

ConversationChannel::ConversationChannel(const QString &localUid, const QString &remoteUid, QObject *parent)
    : QObject(parent), mPendingRequest(0), mState(Null), mLocalUid(localUid), mRemoteUid(remoteUid), mSequence(0)
{
}

ConversationChannel::~ConversationChannel()
{
}

void ConversationChannel::ensureChannel()
{
    if (!mChannel.isNull() || mPendingRequest || !mRequest.isNull())
        return;

    if (!mAccount) {
        mAccount = Tp::Account::create(TP_QT_ACCOUNT_MANAGER_BUS_NAME, mLocalUid);
    }
    if (!mAccount) {
        qWarning() << "ConversationChannel::ensureChannel no account for" << mLocalUid;
        setState(Error);
        return;
    }

    if (mAccount->isReady()) {
        accountReadyForChannel(0);
    } else {
        connect(mAccount->becomeReady(), SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(accountReadyForChannel(Tp::PendingOperation*)));
    }
}

bool ConversationChannel::eventIsPending(int eventId) const
{
    QList<QPair<Tp::MessagePartList, int> >::const_iterator mit = mPendingMessages.constBegin(), mend = mPendingMessages.constEnd();
    for ( ; mit != mend; ++mit) {
        if ((*mit).second == eventId) {
            return true;
        }
    }
    QList<QPair<Tp::PendingOperation *, int> >::const_iterator sit = mPendingSends.begin(), send = mPendingSends.end();
    for ( ; sit != send; ++sit) {
        if ((*sit).second == eventId) {
            return true;
        }
    }
    return false;
}

void ConversationChannel::accountReadyForChannel(Tp::PendingOperation *op)
{
    if (op && op->isError()) {
        qWarning() << "No account for" << mLocalUid;
        setState(Error);
        return;
    }

    Tp::PendingChannelRequest *req = mAccount->ensureTextChat(mRemoteUid,
            QDateTime::currentDateTime(),
            QLatin1String("org.freedesktop.Telepathy.Client.qmlmessages"));
    start(req);
}

void ConversationChannel::start(Tp::PendingChannelRequest *pendingRequest)
{
    Q_ASSERT(state() == Null || state() == Error);
    Q_ASSERT(!mPendingRequest);
    if (state() != Null && state() != Error)
        return;

    mPendingRequest = pendingRequest;
    connect(mPendingRequest, SIGNAL(channelRequestCreated(Tp::ChannelRequestPtr)),
            SLOT(channelRequestCreated(Tp::ChannelRequestPtr)));

    setState(PendingRequest);
}

void ConversationChannel::setChannel(const Tp::ChannelPtr &c)
{
    if (mChannel && c && mChannel->objectPath() == c->objectPath())
        return;
    if (!mChannel.isNull() || c.isNull()) {
        qWarning() << Q_FUNC_INFO << "called with existing channel set";
        return;
    }

    mChannel = c;
    connect(mChannel->becomeReady(Tp::TextChannel::FeatureMessageQueue),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(channelReady()));
    connect(mChannel.data(), SIGNAL(messageReceived(Tp::ReceivedMessage)),
            SLOT(messageReceived(Tp::ReceivedMessage)));
    connect(mChannel.data(), SIGNAL(invalidated(Tp::DBusProxy*,QString,QString)),
            SLOT(channelInvalidated(Tp::DBusProxy*,QString,QString)));

    setState(PendingReady);

    /* setChannel may be called by the client handler before channelRequestSucceeded
     * returns. Either path is equivalent. */
    if (!mRequest.isNull()) {
        mRequest.reset();
        emit requestSucceeded();
    }
}

void ConversationChannel::channelRequestCreated(const Tp::ChannelRequestPtr &r)
{
    Q_ASSERT(state() == PendingRequest);
    Q_ASSERT(mRequest.isNull());
    Q_ASSERT(!r.isNull());
    if (state() != PendingRequest)
        return;

    qDebug() << Q_FUNC_INFO;

    mRequest = r;
    connect(mRequest.data(), SIGNAL(succeeded(Tp::ChannelPtr)),
            SLOT(channelRequestSucceeded(Tp::ChannelPtr)));
    connect(mRequest.data(), SIGNAL(failed(QString,QString)),
            SLOT(channelRequestFailed(QString,QString)));

    mPendingRequest = 0;
    setState(Requested);
}

void ConversationChannel::channelRequestSucceeded(const Tp::ChannelPtr &channel)
{
    if (state() > Requested)
        return;

    // Telepathy docs note that channel may be null if the dispatcher is too old.
    if (channel.isNull()) {
        qWarning() << Q_FUNC_INFO << "channel is null (dispatcher too old?)";
        reportPendingFailed();
        Q_ASSERT(!channel.isNull());
        return;
    }

    Q_ASSERT(!mRequest.isNull());
    setChannel(channel);
}

void ConversationChannel::channelRequestFailed(const QString &errorName,
        const QString &errorMessage)
{
    Q_ASSERT(state() == Requested);

    mRequest.reset();
    setState(Error);
    emit requestFailed(errorName, errorMessage);

    qDebug() << Q_FUNC_INFO << errorName << errorMessage;
}

void ConversationChannel::channelReady()
{
    if (state() != PendingReady || mChannel.isNull())
        return;

    Tp::TextChannelPtr textChannel = Tp::SharedPtr<Tp::TextChannel>::dynamicCast(mChannel);
    Q_ASSERT(!textChannel.isNull());

    setState(Ready);

    if (!mPendingMessages.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Sending" << mPendingMessages.size() << "buffered messages to:" << mRemoteUid;
        QList<QPair<Tp::MessagePartList, int> >::const_iterator it = mPendingMessages.constBegin(), end = mPendingMessages.constEnd();
        for ( ; it != end; ++it)
            sendMessage((*it).first, (*it).second, true);

        // We haven't changed the pending set here, as all buffered messages are now pending send operations
        mPendingMessages.clear();
    }

    // Blindly acknowledge all messages, assuming commhistory handled them
    if (!textChannel->messageQueue().isEmpty())
        textChannel->acknowledge(textChannel->messageQueue());
}

void ConversationChannel::channelDestroyed()
{
    qWarning() << Q_FUNC_INFO;

    reportPendingFailed();
}

void ConversationChannel::setState(State newState)
{
    if (mState == newState)
        return;

    mState = newState;
    emit stateChanged(newState);

    if (mState == Error && !mPendingMessages.isEmpty()) {
        reportPendingFailed();
    }
}

void ConversationChannel::messageReceived(const Tp::ReceivedMessage &message)
{
    if (mChannel.isNull())
        return;

    Tp::TextChannelPtr textChannel = Tp::SharedPtr<Tp::TextChannel>::dynamicCast(mChannel);
    Q_ASSERT(!textChannel.isNull());

    textChannel->acknowledge(QList<Tp::ReceivedMessage>() << message);
}

void ConversationChannel::sendMessage(const QString &text, int eventId)
{
    Tp::MessagePart header;
    if (eventId >= 0)
        header.insert("x-commhistory-event-id", QDBusVariant(eventId));
    else
        qWarning() << "No event Id in message!";

    Tp::MessagePart body;
    body.insert("content-type", QDBusVariant(QLatin1String("text/plain")));
    body.insert("content", QDBusVariant(text));

    Tp::MessagePartList parts;
    parts << header << body;

    sendMessage(parts, eventId, false);
}

void ConversationChannel::sendMessage(const Tp::MessagePartList &parts, int eventId, bool alreadyPending)
{
    if (mChannel.isNull() || !mChannel->isReady()) {
        Q_ASSERT(state() != Ready);
        qDebug() << Q_FUNC_INFO << "Buffering message until channel is ready for:" << mRemoteUid;
        mPendingMessages.append(qMakePair(parts, eventId));
        if (mPendingMessages.count() == 1) {
            ensureChannel();
        }
        reportPendingSetChanged();
        return;
    }

    Tp::TextChannelPtr textChannel = Tp::SharedPtr<Tp::TextChannel>::dynamicCast(mChannel);
    Q_ASSERT(!textChannel.isNull());
    if (textChannel.isNull()) {
        qWarning() << Q_FUNC_INFO << "Channel is not a text channel; cannot send messages";
        return;
    }

    Tp::PendingSendMessage *msg = textChannel->send(parts);
    mPendingSends.append(qMakePair(msg, eventId));
    connect(msg, SIGNAL(finished(Tp::PendingOperation*)), SLOT(sendingFinished(Tp::PendingOperation*)));

    if (!alreadyPending) {
        // If alreadyPending is false, this message was not previously buffered, so
        // we have now added it to the pending set
        reportPendingSetChanged();
    }
}

void ConversationChannel::sendingFinished(Tp::PendingOperation *op)
{
    if (!op->isError() && !op->isValid())
        return;

    const bool sendFailed(op->isError());

    int eventId = -1;
    QList<QPair<Tp::PendingOperation *, int> >::iterator it = mPendingSends.begin(), end = mPendingSends.end();
    for ( ; it != end; ++it) {
        if ((*it).first == op) {
            eventId = (*it).second;

            if (sendFailed) {
                // We're about to report that this event is no longer pending
                mPendingSends.erase(it);
            } else if (eventId != -1) {
                // Don't remove this event from the pending set - it will now be reported as
                // Sending/Sent by commhistoryd, at which point being part of the pending set
                // no longer has any relevance.  These events are asynchronous, so don't remove
                // the item from the pending set until after the status change gets a chance to occur
                mTimer.stop();
                mSentEvents.append(eventId);
                mTimer.start(1000, this);
            }
            break;
        }
    }

    if (eventId == -1) {
        // If we didn't find the event ID, try to extract the ID from the message content
        Tp::Message msg = static_cast<Tp::PendingSendMessage*>(op)->message();
        eventId = parseEventId(msg.parts());
    }
    if (eventId == -1)
        return;

    if (sendFailed) {
        emit sendingFailed(eventId, this);

        // Sending failed - commhistoryd does not update the message in this case, so
        // we should report that it is no longer pending
        reportPendingSetChanged();
    } else if (op->isValid()) {
        emit sendingSucceeded(eventId, this);
    }
}

int ConversationChannel::parseEventId(const Tp::MessagePartList &parts) const
{
    bool hasId = false;
    int eventId = parts.at(0).value("x-commhistory-event-id").variant().toInt(&hasId);
    if (!hasId)
        eventId = -1;
    return eventId;
}

void ConversationChannel::channelInvalidated(Tp::DBusProxy *proxy,
        const QString &errorName, const QString &errorMessage)
{
    Q_UNUSED(proxy);
    qDebug() << "Channel invalidated:" << errorName << errorMessage;

    reportPendingFailed();

    mChannel.reset();
    setState(Null);
}

void ConversationChannel::reportPendingFailed()
{
    if (!mPendingMessages.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Failed sending" << mPendingMessages.size() << "buffered messages to:" << mRemoteUid;
        QList<QPair<Tp::MessagePartList, int> > failed = mPendingMessages;
        mPendingMessages.clear();

        QList<QPair<Tp::MessagePartList, int> >::const_iterator it = failed.constBegin(), end = failed.constEnd();
        for ( ; it != end; ++it)
            emit sendingFailed((*it).second, this);

        reportPendingSetChanged();
    }
}

void ConversationChannel::reportPendingSetChanged()
{
    ++mSequence;
    emit sequenceChanged();
}

void ConversationChannel::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() == mTimer.timerId()) {
        mTimer.stop();

        // Remove any sent operations that have expired
        foreach (int eventId, mSentEvents) {
            QList<QPair<Tp::PendingOperation *, int> >::iterator it = mPendingSends.begin();
            while (it != mPendingSends.end()) {
                if ((*it).second == eventId) {
                    it = mPendingSends.erase(it);
                } else {
                    ++it;
                }
            }
        }
        mSentEvents.clear();
    }
}

