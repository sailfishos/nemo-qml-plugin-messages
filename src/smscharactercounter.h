/* Copyright (C) 2015 Jolla Ltd
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

#ifndef SMSCHARACTERCOUNTER_H
#define SMSCHARACTERCOUNTER_H

#include <QObject>
#include <QVector>

class SmsCharacterCounter : public QObject
{
    Q_OBJECT
   
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int messageCount READ messageCount NOTIFY messageCountChanged)
    Q_PROPERTY(int remainingCharacterCount READ remainingCharacterCount NOTIFY remainingCharacterCountChanged)

public:
    SmsCharacterCounter(QObject *parent = 0);

    QString text() const;
    void setText(const QString &t);

    int messageCount() const;
    int remainingCharacterCount() const;

signals:
    void textChanged();
    void messageCountChanged();
    void remainingCharacterCountChanged();

private:
    // Extension language sets are not supported by ofono
    enum Encoding { Default, Spanish, Portuguese, Turkish, UCS2 };

    void appendCharacter(const QChar &c);
    void removeCharacter();
    bool reencode(Encoding newEncoding);

    QString m_text;
    int m_messageCount;
    int m_remainingCharacterCount;
    int m_characterCount;
    Encoding m_baseEncoding;
    Encoding m_shiftEncoding;
    const QVector<uint> *m_baseSet;
    const QVector<uint> *m_shiftSet;
};

#endif

