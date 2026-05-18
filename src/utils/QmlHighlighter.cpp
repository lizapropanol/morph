#include "QmlHighlighter.h"

QmlHighlighter::QmlHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {
    HighlightingRule rule;

    keywordFormat.setForeground(QColor("#c586c0"));
    keywordFormat.setFontWeight(QFont::Bold);
    const QStringList keywordPatterns = {
        "\\bimport\\b", "\\bproperty\\b", "\\bid\\b", "\\bfunction\\b",
        "\\bvar\\b", "\\blet\\b", "\\bconst\\b", "\\bif\\b", "\\belse\\b",
        "\\breturn\\b", "\\btrue\\b", "\\bfalse\\b", "\\bnull\\b", "\\bnew\\b"
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    classFormat.setForeground(QColor("#4ec9b0"));
    rule.pattern = QRegularExpression("\\b[A-Z][A-Za-z0-9_]*\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    functionFormat.setForeground(QColor("#9cdcfe"));
    rule.pattern = QRegularExpression("\\b[a-z0-9_]+\\s*:");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    numberFormat.setForeground(QColor("#b5cea8"));
    rule.pattern = QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    quotationFormat.setForeground(QColor("#ce9178"));
    rule.pattern = QRegularExpression("\".*?\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    singleLineCommentFormat.setForeground(QColor("#6a9955"));
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void QmlHighlighter::highlightBlock(const QString &text) {
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, singleLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
