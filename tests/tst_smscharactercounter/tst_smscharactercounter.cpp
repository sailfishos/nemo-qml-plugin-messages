/*
 * Copyright (C) 2015 Jolla Ltd.
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

#include <QObject>
#include <QtTest>

#include "smscharactercounter.h"


class tst_SmsCharacterCounter : public QObject
{
    Q_OBJECT

public:
    tst_SmsCharacterCounter();

private slots:
    void test_data();
    void test();

private:
    SmsCharacterCounter counter;
};


tst_SmsCharacterCounter::tst_SmsCharacterCounter()
{
}

void tst_SmsCharacterCounter::test_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("messageCount");
    QTest::addColumn<int>("remainingCharacterCount");

    QTest::newRow("empty")
        << ""
        << 0
        << 0;
    QTest::newRow("single 7-bit character")
        << "A"
        << 1
        << 159;
    QTest::newRow("multiple 7-bit characters, replacement")
        << "0123456789"
        << 1
        << 150;
    QTest::newRow("remove a 7-bit character")
        << "012345678"
        << 1
        << 151;
    QTest::newRow("multiple 7-bit characters, extended")
        << "0123456789ABC"
        << 1
        << 147;
    QTest::newRow("remove multiple 7-bit characters")
        << "01234"
        << 1
        << 155;
    QTest::newRow("revert to empty")
        << ""
        << 0
        << 0;
    QTest::newRow("single 7-bit shift character")
        << "\u20AC"
        << 1
        << 158;
    QTest::newRow("multiple 7-bit characters with shift, extended")
        << "\u20ACABC\u20AC"
        << 1
        << 153;
    QTest::newRow("remove 7-bit characters with shift")
        << "\u20ACAB"
        << 1
        << 156;
    QTest::newRow("single 16-bit character")
        << "\u2022"
        << 1
        << 69;
    QTest::newRow("multiple 16-bit characters, extended")
        << "\u2022ABC"
        << 1
        << 66;
    QTest::newRow("multiple 16-bit characters, replacement")
        << "ABC\u2022"
        << 1
        << 66;
    QTest::newRow("remove 16-bit character")
        << "ABC"
        << 1
        << 157;
    QTest::newRow("fill a single message, 7-bit characters")
        << "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
        << 1
        << 0;
    QTest::newRow("exceed a single message, 7-bit characters")
        << "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0"
        << 2
        << 145;
    QTest::newRow("fill a second message, 7-bit characters")
        << "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "012345"
        << 2
        << 0;
    QTest::newRow("exceed a second message, 7-bit characters")
        << "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "012345"
           "6"
        << 3
        << 152;
    QTest::newRow("fill a single message, 16-bit characters")
        << "\u2022"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "012345678"
        << 1
        << 0;
    QTest::newRow("exceed a single message, 16-bit characters")
        << "\u2022"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "012345678"
           "9"
        << 2
        << 63;

    QTest::newRow("fill a second message, 16-bit characters")
        << "\u2022"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "012"
        << 2
        << 0;
    QTest::newRow("exceed a second message, 16-bit characters")
        << "\u2022"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "0123456789" "0123456789" "0123456789"
           "0123456789" "012"
           "3"
        << 3
        << 66;
}

void tst_SmsCharacterCounter::test()
{
    QFETCH(QString, text);
    QFETCH(int, messageCount);
    QFETCH(int, remainingCharacterCount);

    counter.setText(text);
    QCOMPARE(counter.messageCount(), messageCount);
    QCOMPARE(counter.remainingCharacterCount(), remainingCharacterCount);
}

#include "tst_smscharactercounter.moc"
QTEST_APPLESS_MAIN(tst_SmsCharacterCounter)
