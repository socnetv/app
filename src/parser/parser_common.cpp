/**
 * @file parser_common.cpp
 * @brief Common parser helpers for SocNetV
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */

#include "parser.h"
#include "global.h"

SOCNETV_USE_NAMESPACE


/**
 * @brief Helper. Checks if the string parameter is a comment (starts with a known char, i.e #).
 *
 * @param str
 * @return  bool
 */
bool Parser::isComment(QString str)
{
    if (str.startsWith("#", Qt::CaseInsensitive) || str.startsWith("/*", Qt::CaseInsensitive) || str.startsWith("%", Qt::CaseInsensitive) || str.startsWith("/*", Qt::CaseInsensitive) || str.startsWith("//", Qt::CaseInsensitive) || str.isEmpty())
    {
        qDebug() << "Parser::isComment() - Comment or an empty line was found. "
                    "Skipping...";
        return true;
    }
    return false;
}
