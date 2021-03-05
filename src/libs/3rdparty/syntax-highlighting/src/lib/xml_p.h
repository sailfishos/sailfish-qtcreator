/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_XML_P_H
#define KSYNTAXHIGHLIGHTING_XML_P_H

#include <QString>

namespace KSyntaxHighlighting
{
/** Utilities for XML parsing. */
namespace Xml
{
/** Parse a xs:boolean attribute. */
inline bool attrToBool(const QStringView &str)
{
    return str == QLatin1String("1") || str.compare(QString("true"), Qt::CaseInsensitive) == 0;
}

}
}

#endif
