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

#include "smscharactercounter.h"

#include <QChar>
#include <QVector>
#include <QtDebug>

#include <algorithm>

namespace {

void populateSet(QVector<uint> &set, const uint *chars, std::size_t n)
{
    set.reserve(n);
    for (const uint *it = chars, *end = it + n; it != end; ++it) {
        set.append(*it);
    }
    std::sort(set.begin(), set.end());
}

template <std::size_t N>
void populateSet(QVector<uint> &set, const uint (&chars)[N])
{
    return populateSet(set, chars, N);
}

const QVector<uint> *defaultCharacterSet() {
    static QVector<uint> set;
    static bool initialised = false;

    if (!initialised) {
        // Unicode values for the characters in the default GSM base character set
        const uint baseChars[] = {
            0x0040, 0x00A3, 0x0024, 0x00A5, 0x00E8, 0x00E9, 0x00F9, 0x00EC,
            0x00F2, 0x00C7, 0x000A, 0x00D8, 0x00F8, 0x000D, 0x00C5, 0x00E5,
            0x0394, 0x005F, 0x03A6, 0x0393, 0x039B, 0x03A9, 0x03A0, 0x03A8,
            0x03A3, 0x0398, 0x039E, 0x00A0, 0x00C6, 0x00E6, 0x00DF, 0x00C9,
            0x0020, 0x0021, 0x0022, 0x0023, 0x00A4, 0x0025, 0x0026, 0x0027,
            0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
            0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
            0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
            0x00A1, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
            0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
            0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
            0x0058, 0x0059, 0x005A, 0x00C4, 0x00D6, 0x00D1, 0x00DC, 0x00A7,
            0x00BF, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
            0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
            0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
            0x0078, 0x0079, 0x007A, 0x00E4, 0x00F6, 0x00F1, 0x00FC, 0x00E0
        };

        populateSet(set, baseChars);
        initialised = true;
    }

    return &set;
}

const QVector<uint> *defaultShiftCharacterSet() {
    static QVector<uint> set;
    static bool initialised = false;

    if (!initialised) {
        // Unicode values for the characters in the default GSM shift character set
        const uint shiftChars[] = {
            0x000C,
            0x005E,
            0x007B,
            0x007D,
            0x005C,
            0x005B,
            0x007E,
            0x005D,
            0x007C,
            0x20AC
        };

        populateSet(set, shiftChars);
        initialised = true;
    }

    return &set;
}

bool setContains(const QVector<uint> &set, const QChar &c)
{
    return std::binary_search(set.constBegin(), set.constEnd(), c.unicode());
}

}

SmsCharacterCounter::SmsCharacterCounter(QObject *parent)
    : QObject(parent)
    , m_messageCount(0)
    , m_remainingCharacterCount(0)
    , m_characterCount(0)
    , m_baseEncoding(Default)
    , m_shiftEncoding(Default)
    , m_baseSet(0)
    , m_shiftSet(0)
{
}

QString SmsCharacterCounter::text() const
{
    return m_text;
}

void SmsCharacterCounter::setText(const QString &t)
{
    // We need to determine how this string will be encoded into SMS by ofono;
    // the relevant function is sms_text_prepare_with_alphabet() in smsutil.c

    // Ofono will encode the text using the first possible from:
    // a) the default GSM 03.38 7-bit dialect
    // b) the dialect corresponding to the ofono 'sms/Alphabet' setting value
    //    (note: ofono tries first with single shift, then with both single and locking shift)
    // c) UCS-2 encoding

    // I'm not sure how ofono gets configured to use a specific alphabet (possibly
    // derived from locale environment settings) but it is entirely possible that
    // it is effectively unused in mer. In any case, it is not currently implemented
    // here - which means that this estimate might be pessimistic by suggesting the
    // UCS-2 encoding counts when a GSM dialect may actually be used by ofono.

    // Ensure that our string is fully normalized
    const QString normalized(t.normalized(QString::NormalizationForm_KC));
    if (normalized != m_text) {
        if (!m_text.isEmpty() && normalized.startsWith(m_text)) {
            // This is an extension to the previous string, append the remainder
            foreach (const QChar &c, normalized.mid(m_text.length())) {
                appendCharacter(c);
            }
        } else if (m_baseEncoding == Default && !m_text.isEmpty() && m_text.startsWith(normalized)) {
            // Characters have been removed from our text - we can only handle this by subtraction
            // if the default character set is is use; otherwise we don't know which characters
            // were responsible for forcing us to use an encoding, and we need to restart
            int diff = m_text.count() - normalized.count();
            while (--diff >= 0) {
                removeCharacter();
            }
        } else {
            // Start from the beginning
            m_text.clear();
            m_text.reserve(normalized.count());
            m_characterCount = 0;
            m_baseEncoding = Default;
            m_shiftEncoding = Default;
            m_baseSet = defaultCharacterSet();
            m_shiftSet = defaultShiftCharacterSet();

            foreach (const QChar &c, normalized) {
                appendCharacter(c);
            }
        }

        // The text has been modified
        emit textChanged();

        // Update the counts

        // If we encode with alternate character sets, we must include that information in the header
        const int overheadFromBaseEncoding = (m_baseEncoding == Default || m_baseEncoding == UCS2) ? 0 : 3;
        const int overheadFromShiftEncoding = m_shiftEncoding == Default ? 0 : 3;
        const int overheadFromEncoding = overheadFromBaseEncoding + overheadFromShiftEncoding;

        // A single SMS allows 140 bytes (160 septets), for both data and header
        const int maxSegmentBytes = 140;

        // UCS2 requires 16 bits per character, all other encodings require 7 bits
        const int bitsPerCharacter(m_baseEncoding == UCS2 ? 16 : 7);

        // If the header is required, 1 additional byte is needed for the header length field
        int overhead(overheadFromEncoding ? overheadFromEncoding + 1 : 0);

        int segmentBytesAvailable(maxSegmentBytes - overhead);
        int segmentCharactersAvailable((segmentBytesAvailable * 8) / bitsPerCharacter);

        if (m_characterCount > segmentCharactersAvailable) {
            // We must allocate 5 additional header bytes for each packet's segmentation framing
            // Note: ofono does not currently use the 16-bit segmentation count option
            overhead += (overhead ? 5 : 6);
            segmentBytesAvailable = maxSegmentBytes - overhead;
            segmentCharactersAvailable = (segmentBytesAvailable * 8) / bitsPerCharacter;
        }

        const int remainder = (m_characterCount % segmentCharactersAvailable);

        const int messageCount = (m_characterCount / segmentCharactersAvailable) + (remainder ? 1 : 0);
        if (m_messageCount != messageCount) {
            m_messageCount = messageCount;
            emit messageCountChanged();
        }

        const int remainingCharacterCount = remainder ? (segmentCharactersAvailable - remainder) : 0;
        if (m_remainingCharacterCount != remainingCharacterCount) {
            m_remainingCharacterCount = remainingCharacterCount;
            emit remainingCharacterCountChanged();
        }
    }
}

int SmsCharacterCounter::messageCount() const
{
    return m_messageCount;
}

int SmsCharacterCounter::remainingCharacterCount() const
{
    return m_remainingCharacterCount;
}

void SmsCharacterCounter::appendCharacter(const QChar &c)
{
    if (m_baseEncoding == UCS2) {
        ++m_characterCount;
    } else {
        // Test for representability in the current character sets
        if (setContains(*m_baseSet, c)) {
            // This character is representable in the current base set
            m_characterCount += 1;
        } else {
            if (setContains(*m_shiftSet, c)) {
                // This character is representable in the current shift set with an escape sequence
                m_characterCount += 2;
            } else {
                // This character cannot be represented in the current encoding

                if (m_baseEncoding == Default) {
                    /* TODO: Switch to alternative alphabet encoding if possible
                    Encoding alternateEncoding(...);
                    if (reencode(alternateEncoding)) {
                        return appendCharacter(c);
                    }
                    */ 
                }

                // Fallback to UCS2
                reencode(UCS2);
                return appendCharacter(c);
            }
        }
    }

    // The character is now part of the message
    m_text.append(c);
}

void SmsCharacterCounter::removeCharacter()
{
    const QChar c(m_text.at(m_text.count() - 1));

    // We only remove characters if using the default set
    if (setContains(*m_shiftSet, c)) {
        m_characterCount -= 2;
    } else {
        // the character must be in the default base set
        m_characterCount -= 1;
    }

    m_text.chop(1);
}

bool SmsCharacterCounter::reencode(Encoding newEncoding)
{
    // Test if the text can be represented in the new encoding, and if so, determine the new character count
    if (newEncoding == UCS2) {
        // All characters can be represented, each as a single 16-bit value
        m_characterCount = m_text.length();
        m_baseEncoding = UCS2;
        m_shiftEncoding = Default;
        return true;
    } else {
        // TODO: test against alternative language sets
    }

    return false;
}

