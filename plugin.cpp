/*
 * Copyright (C) 2012 Jolla Ltd.
 * Contact: John Brooks <john.brooks@jollamobile.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
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
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include <QtGlobal>
#include <QtDeclarative>
#include <QDeclarativeEngine>
#include <QDeclarativeExtensionPlugin>

#include "src/accountsmodel.h"
#include "src/conversationchannel.h"
#include "src/groupmanager.h"
#include "src/clienthandler.h"

class Q_DECL_EXPORT NemoMessagesPlugin : public QDeclarativeExtensionPlugin
{
public:
    virtual ~NemoMessagesPlugin() { }

    void initializeEngine(QDeclarativeEngine *engine, const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("org.nemomobile.messages.internal"));
    }

    void registerTypes(const char *uri)
    {
        Q_ASSERT(uri == QLatin1String("org.nemomobile.messages.internal"));

        qmlRegisterType<AccountsModel>(uri, 1, 0, "TelepathyAccountsModel");
        qmlRegisterUncreatableType<ConversationChannel>(uri, 1, 0, "ConversationChannel",
                QLatin1String("Must be created via GroupManager"));
        qmlRegisterType<GroupManager>(uri, 1, 0, "GroupManager");
        qmlRegisterType<ClientHandler>(uri, 1, 0, "TelepathyClientHandler");
    }
};

Q_EXPORT_PLUGIN2(nemomessages, NemoMessagesPlugin);
