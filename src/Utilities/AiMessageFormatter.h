#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>

inline QString formatAiMessageToHtml(const QString& message)
{
    QString html = message.trimmed().toHtmlEscaped();

    static const QRegularExpression boldPattern(R"(\*\*(.+?)\*\*)");
    html.replace(boldPattern, "<strong>\\1</strong>");

    const QStringList lines = html.split('\n');
    QStringList formattedLines;
    bool inBulletList = false;
    bool inNumberedList = false;

    const auto closeLists = [&formattedLines, &inBulletList, &inNumberedList]() {
        if (inBulletList)
        {
            formattedLines.append("</ul>");
            inBulletList = false;
        }
        if (inNumberedList)
        {
            formattedLines.append("</ol>");
            inNumberedList = false;
        }
    };

    static const QRegularExpression numberedListPattern(R"(^\d+\.\s+(.+)$)");

    for (const QString& rawLine : lines)
    {
        const QString line = rawLine.trimmed();
        if (line.isEmpty())
        {
            closeLists();
            formattedLines.append("<br/>");
            continue;
        }

        if (line.startsWith("### "))
        {
            closeLists();
            formattedLines.append(QString("<div style='font-weight:700; margin:8px 0 4px 0;'>%1</div>").arg(line.mid(4)));
            continue;
        }

        if (line.startsWith("## "))
        {
            closeLists();
            formattedLines.append(QString("<div style='font-weight:700; font-size:14px; margin:10px 0 4px 0;'>%1</div>").arg(line.mid(3)));
            continue;
        }

        if (line.startsWith("# "))
        {
            closeLists();
            formattedLines.append(QString("<div style='font-weight:700; font-size:15px; margin:12px 0 6px 0;'>%1</div>").arg(line.mid(2)));
            continue;
        }

        if (line.startsWith("- ") || line.startsWith("* "))
        {
            if (!inBulletList)
            {
                closeLists();
                formattedLines.append("<ul style='margin:4px 0 8px 18px; padding:0;'>");
                inBulletList = true;
            }
            formattedLines.append(QString("<li style='margin:2px 0;'>%1</li>").arg(line.mid(2)));
            continue;
        }

        const QRegularExpressionMatch numberedMatch = numberedListPattern.match(line);
        if (numberedMatch.hasMatch())
        {
            if (!inNumberedList)
            {
                closeLists();
                formattedLines.append("<ol style='margin:4px 0 8px 18px; padding:0;'>");
                inNumberedList = true;
            }
            formattedLines.append(QString("<li style='margin:2px 0;'>%1</li>").arg(numberedMatch.captured(1)));
            continue;
        }

        closeLists();
        formattedLines.append(QString("<div>%1</div>").arg(line));
    }

    closeLists();
    return formattedLines.join(QString());
}
