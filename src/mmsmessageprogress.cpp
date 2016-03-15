/*
 * Copyright (C) 2016 Jolla Ltd
 * Contact: Slava Monich <slava.monich@jolla.com>
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

#include "mmsmessageprogress.h"
#include "mmstransfer_interface.h"
#include "mmstransferlist_interface.h"

#include <QDBusServiceWatcher>

static const QString kMmsService("org.nemomobile.MmsEngine");
#define kMmsServiceBus QDBusConnection::systemBus()

static bool isTimeout(QDBusError aError)
{
    switch (aError.type()) {
    case QDBusError::NoReply:
    case QDBusError::Timeout:
    case QDBusError::TimedOut:
        return true;
    default:
        return false;
    }
}

// ==========================================================================
// MmsMessageTransferList
//
// org.nemomobile.MmsEngine.TransferList client shared by all
// MmsMessageProgress objects. Maintains the list of active transfers
// (currently no more than one transfer at a time).
// ==========================================================================

class MmsMessageTransferList : public MmsMessageTransferListInterface
{
    Q_OBJECT
public:
    class RefHolder;
    static QSharedPointer<MmsMessageTransferList> instance();

    MmsMessageTransferList();

private Q_SLOTS:
    void requestTransferList();
    void onServiceUnregistered();
    void onTransferListFinished(QDBusPendingCallWatcher* aWatcher);
    void onTransferFinished(QDBusObjectPath aPath);
    void onTransferStarted(QDBusObjectPath aPath);

Q_SIGNALS:
    void transferFinished(QString aPath);
    void transferStarted(QString aPath);
    void validChanged();

public:
    bool iValid;
    QStringList iList;

private:
    QDBusPendingCallWatcher* iPendingCall;
};

QSharedPointer<MmsMessageTransferList> MmsMessageTransferList::instance()
{
    static QWeakPointer<MmsMessageTransferList> sharedInstance;
    QSharedPointer<MmsMessageTransferList> ptr(sharedInstance);
    if (ptr.isNull()) {
        ptr = QSharedPointer<MmsMessageTransferList>(new MmsMessageTransferList, &QObject::deleteLater);
        sharedInstance = ptr;
    }
    return ptr;
}

MmsMessageTransferList::MmsMessageTransferList() :
    MmsMessageTransferListInterface(kMmsService, "/", kMmsServiceBus),
    iValid(false),
    iPendingCall(NULL)
{
    QDBusConnection dbus = connection();
    QDBusServiceWatcher* serviceWatcher = new QDBusServiceWatcher(kMmsService,
        dbus, QDBusServiceWatcher::WatchForRegistration |
        QDBusServiceWatcher::WatchForUnregistration, this);

    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered,
        this, &MmsMessageTransferList::requestTransferList);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
        this, &MmsMessageTransferList::onServiceUnregistered);
    connect(this, &MmsMessageTransferListInterface::TransferStarted,
        this, &MmsMessageTransferList::onTransferStarted);
    connect(this, &MmsMessageTransferListInterface::TransferFinished,
        this, &MmsMessageTransferList::onTransferFinished);

    if (dbus.interface()->isServiceRegistered(kMmsService)) {
        requestTransferList();
    }
}

void MmsMessageTransferList::requestTransferList()
{
    delete iPendingCall;
    iPendingCall = new QDBusPendingCallWatcher(Get(), this);
    connect(iPendingCall, &QDBusPendingCallWatcher::finished,
        this, &MmsMessageTransferList::onTransferListFinished);
}

void MmsMessageTransferList::onServiceUnregistered()
{
    if (iPendingCall) {
        delete iPendingCall;
        iPendingCall = NULL;
    }
    if (iValid) {
        iValid = false;
        Q_EMIT validChanged();
    }
}

void MmsMessageTransferList::onTransferListFinished(QDBusPendingCallWatcher* aWatcher)
{
    QDBusPendingReply<QList<QDBusObjectPath> > reply(*aWatcher);
    iPendingCall = NULL;
    if (reply.isError()) {
        // Repeat the call on timeout
        qWarning() << reply.error();
        if (isTimeout(reply.error())) {
            requestTransferList();
        }
    } else {
        iList.clear();
        QList<QDBusObjectPath> list = reply.argumentAt<0>();
        for (int i=0; i<list.count(); i++) {
            iList.append(list.at(i).path());
        }
        iValid = true;
        Q_EMIT validChanged();
    }
    aWatcher->deleteLater();
}

void MmsMessageTransferList::onTransferStarted(QDBusObjectPath aPath)
{
    QString path(aPath.path());
    if (!iList.contains(path)) {
        iList.append(path);
    }
    Q_EMIT transferStarted(path);
}

void MmsMessageTransferList::onTransferFinished(QDBusObjectPath aPath)
{
    QString path(aPath.path());
    iList.removeOne(path);
    Q_EMIT transferFinished(path);
}

// ==========================================================================
// MmsMessageTransferList::RefHolder
//
// Holds a reference to MmsMessageTransferList for certain period of time.
// Even if there's more than one transfer queued, there's a small period
// of time (typically, no more than a few seconds) between one transfer is
// done and the next one starts. In order to avoid unnecessary reallocations
// of MmsMessageTransferList, it makes sense to hold a reference for a short
// while after a transfer completes.
// ==========================================================================

class MmsMessageTransferList::RefHolder : public QTimer {
    Q_OBJECT
public:
    enum { HOLD_REF_MSEC = 10000 };
    RefHolder(QSharedPointer<MmsMessageTransferList> aPointer);
    QSharedPointer<MmsMessageTransferList> iPointer;
private Q_SLOTS:
    void onTimeout();
};

MmsMessageTransferList::RefHolder::RefHolder(QSharedPointer<MmsMessageTransferList> aPointer) :
    iPointer(aPointer)
{
    setSingleShot(true);
    connect(this, &QTimer::timeout, this, &RefHolder::onTimeout);
    start(HOLD_REF_MSEC);
}

void MmsMessageTransferList::RefHolder::onTimeout()
{
    deleteLater();
}

// ==========================================================================
// MmsMessageProgress::Private
//
// D-Bus proxy for org.nemomobile.MmsEngine.Transfer, created only when the
// path property of MmsMessageProgress is set to a non-empty string.
// ==========================================================================

class MmsMessageProgress::Private : public MmsMessageTransferInterface
{
    Q_OBJECT

    enum {
        UPDATE_SEND = 0x01,
        UPDATE_RECEIVE = 0x02,
        UPDATE_ALL = 0x03
    };

public:
    Private(QString aPath, bool aInbound, MmsMessageProgress* aParent);
    ~Private();

    void setInbound(bool aInbound);

private:
    void getAll();
    void enableUpdates();
    void updateProgress();
    void updateRunning();

private Q_SLOTS:
    void onTransferListChanged();
    void onGetAllFinished(QDBusPendingCallWatcher* aWatcher);
    void onEnableUpdatesFinished(QDBusPendingCallWatcher* aWatcher);
    void onSendProgressChanged(uint aSent, uint aTotal);
    void onReceiveProgressChanged(uint aReceived, uint aTotal);

public:
    bool iValid;
    bool iRunning;
    bool iInbound;
    qreal iProgress;

private:
    uint iCookie;
    uint iBytesSent;
    uint iBytesToSend;
    uint iBytesReceived;
    uint iBytesToReceive;
    MmsMessageProgress* iParent;
    QDBusPendingCallWatcher* iPendingGetAll;
    QSharedPointer<MmsMessageTransferList> iTransferList;
};

MmsMessageProgress::Private::Private(QString aPath, bool aInbound, MmsMessageProgress* aParent) :
    MmsMessageTransferInterface(kMmsService, aPath, kMmsServiceBus, aParent),
    iValid(false),
    iRunning(false),
    iInbound(aInbound),
    iProgress(0.0),
    iCookie(0),
    iBytesSent(0),
    iBytesToSend(0),
    iBytesReceived(0),
    iBytesToReceive(0),
    iParent(aParent),
    iPendingGetAll(NULL),
    iTransferList(MmsMessageTransferList::instance())
{
    connect(this, &MmsMessageTransferInterface::SendProgressChanged,
        this, &Private::onSendProgressChanged);
    connect(this, &MmsMessageTransferInterface::ReceiveProgressChanged,
        this, &Private::onReceiveProgressChanged);
    connect(iTransferList.data(), &MmsMessageTransferList::validChanged,
        this, &Private::onTransferListChanged);
    connect(iTransferList.data(), &MmsMessageTransferList::transferStarted,
        this, &Private::onTransferListChanged);
    connect(iTransferList.data(), &MmsMessageTransferList::transferFinished,
        this, &Private::onTransferListChanged);
    onTransferListChanged();
}

MmsMessageProgress::Private::~Private()
{
    if (iCookie) {
        DisableUpdates(iCookie);
    }
    new MmsMessageTransferList::RefHolder(iTransferList);
}

void MmsMessageProgress::Private::setInbound(bool aInbound)
{
    iInbound = aInbound;
    if (iValid) {
        enableUpdates();
        updateProgress();
    }
}

void MmsMessageProgress::Private::getAll()
{
    delete iPendingGetAll;
    iPendingGetAll = new QDBusPendingCallWatcher(GetAll(), this);
    connect(iPendingGetAll, &QDBusPendingCallWatcher::finished,
        this, &Private::onGetAllFinished);
}

void MmsMessageProgress::Private::enableUpdates()
{
    if (iCookie) {
        DisableUpdates(iCookie);
        iCookie = 0;
    }
    connect(new QDBusPendingCallWatcher(
        EnableUpdates(iInbound ? UPDATE_RECEIVE : UPDATE_SEND), this),
        &QDBusPendingCallWatcher::finished, this,
        &Private::onEnableUpdatesFinished);
}

void MmsMessageProgress::Private::onTransferListChanged()
{
    bool pathValid = iTransferList->iValid &&
        iTransferList->iList.contains(path());
    if (pathValid) {
        if (!iValid && !iPendingGetAll) {
            getAll();
        }
    } else {
        iCookie = 0;
        if (iPendingGetAll) {
            delete iPendingGetAll;
            iPendingGetAll = NULL;
        }
        if (iValid) {
            iValid = false;
            Q_EMIT iParent->validChanged();
            updateRunning();
        }
    }
}

void MmsMessageProgress::Private::onGetAllFinished(QDBusPendingCallWatcher* aWatcher)
{
    // 0. version
    // 1. bytes_sent
    // 2. bytes_to_send
    // 3. bytes_received
    // 4. bytes_to_receive
    QDBusPendingReply<uint,uint,uint,uint,uint> reply(*aWatcher);
    iPendingGetAll = NULL;
    if (reply.isError()) {
        // Repeat the call on timeout
        qWarning() << reply.error();
        if (isTimeout(reply.error())) {
            getAll();
        }
    } else {
        iBytesSent = reply.argumentAt<1>();
        iBytesToSend = reply.argumentAt<2>();
        iBytesReceived = reply.argumentAt<3>();
        iBytesToReceive = reply.argumentAt<4>();
        iValid = true;
        enableUpdates();
        updateProgress();
        Q_EMIT iParent->validChanged();
    }
    aWatcher->deleteLater();
}

void MmsMessageProgress::Private::onEnableUpdatesFinished(QDBusPendingCallWatcher* aWatcher)
{
    QDBusPendingReply<uint> reply(*aWatcher);
    if (reply.isError()) {
        // Repeat the call on timeout
        qWarning() << reply.error();
        if (isTimeout(reply.error())) {
            enableUpdates();
        }
    } else {
        iCookie = reply.value();
    }
    aWatcher->deleteLater();
}

void MmsMessageProgress::Private::onSendProgressChanged(uint aSent, uint aTotal)
{
    iBytesSent = aSent;
    iBytesToSend = aTotal;
    if (!iInbound) {
        updateProgress();
    }
}

void MmsMessageProgress::Private::onReceiveProgressChanged(uint aReceived, uint aTotal)
{
    iBytesReceived = aReceived;
    iBytesToReceive = aTotal;
    if (iInbound) {
        updateProgress();
    }
}

void MmsMessageProgress::Private::updateProgress()
{
    uint transmitted, total;
    if (iInbound) {
        transmitted = iBytesReceived;
        total = iBytesToReceive;
    } else {
        transmitted = iBytesSent;
        total = iBytesToSend;
    }
    const qreal prevProgress = iProgress;
    if (total) {
        iProgress = ((qreal)qMin(transmitted, total))/total;
    } else {
        iProgress = 0.0;
    }
    if (prevProgress != iProgress) {
        Q_EMIT iParent->progressChanged();
    }
    updateRunning();
}

void MmsMessageProgress::Private::updateRunning()
{
    const bool wasRunning = iRunning;
    iRunning = iValid && iProgress > 0.0 && iProgress < 1.0;
    if (wasRunning != iRunning) {
        Q_EMIT iParent->runningChanged();
    }
}

// ==========================================================================
// MmsMessageProgress
//
// Lightweight wrapper for org.nemomobile.MmsEngine.Transfer D-Bus client.
// Doesn't do much until the path is set. Typically, the path would be set
// to "/msg/<id>/Retrieve" or "/msg/<id>/Send" where <id> is the id of the
// MMS message in the commhistory database.
// ==========================================================================

MmsMessageProgress::MmsMessageProgress(QObject* aParent) :
    QObject(aParent),
    iPrivate(NULL)
{
}

void MmsMessageProgress::setPath(QString aPath)
{
    QString prevPath = path();
    if (prevPath != aPath) {
        bool wasValid = valid();
        bool wasRunning = running();
        delete iPrivate;
        iPrivate = aPath.isEmpty() ? NULL : new Private(aPath, iInbound, this);
        Q_EMIT pathChanged();
        if (valid() != wasValid) {
            Q_EMIT validChanged();
        }
        if (running() != wasRunning) {
            Q_EMIT runningChanged();
        }
    }
}

void MmsMessageProgress::setInbound(bool aInbound)
{
    if (iInbound != aInbound) {
        iInbound = aInbound;
        if (iPrivate) {
            iPrivate->setInbound(iInbound);
        }
        Q_EMIT inboundChanged();
    }
}

QString MmsMessageProgress::path() const
{
    return iPrivate ? iPrivate->path() : QString();
}

bool MmsMessageProgress::valid() const
{
    return iPrivate && iPrivate->iValid;
}

bool MmsMessageProgress::running() const
{
    return iPrivate && iPrivate->iRunning;
}

qreal MmsMessageProgress::progress() const
{
    return iPrivate ? iPrivate->iProgress : 0;
}

#include "mmsmessageprogress.moc"
