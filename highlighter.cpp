/**********************************************************************
  Highlighter - Code highlighting for the packmol extension

  Copyright (C) 2010 by Tim Vandermeersch

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "highlighter.h"

#include <QRegExp>

namespace Avogadro {

  Highlighter::Highlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
  {
    HighlightingRule rule;

    /*
    promptFormat.setForeground(Qt::blue);
    promptFormat.setFontWeight(QFont::Bold);
    QStringList promptPatterns;
    promptPatterns << "#*";

    foreach (const QString &pattern, promptPatterns) {
        rule.pattern = QRegExp(pattern);
        rule.format = promptFormat;
        highlightingRules.append(rule);
    }
    */

  
    
    keywordFormat.setForeground(Qt::darkGreen);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;

    // Python keywords
    keywordPatterns << "\\bstructure\\b" << "\\bend\\b" << "\\btolerance\\b"
                    << "\\boutput\\b" << "\\bfiletype\\b" << "\\bnumber\\b"
                    << "\\binside\\b" << "\\bcube\\b" << "\\bbox\\b"
                    << "\\bsphere\\b" << "\\boutside\\b" << "\\batoms\\b"
                    << "\\bcenter\\b" << "\\bfixed\\b" << "\\bellipsoid\\b"
                    << "\\bplane\\b" << "\\bover\\b" << "\\bbelow\\b"
                    << "\\bcylinder\\b";

    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegExp(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    /*
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(Qt::red);
    rule.pattern = QRegExp("#[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::red);

    quotationFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);
    rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    */
    //commentStartExpression = QRegExp("/\\*");
    //commentEndExpression = QRegExp("\\*/");
  }

  void Highlighter::highlightBlock(const QString &text)
  {
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = text.indexOf(expression);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length);
        }
    }
    setCurrentBlockState(0);
    /*

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        int endIndex = text.indexOf(commentEndExpression, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + commentEndExpression.matchedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression,
                                                startIndex + commentLength);
    }
    */
  }

} // namespace

#include "highlighter.moc"
