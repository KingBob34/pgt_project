#include "loom/qt/passage.h"

#include "loom/qt/convert.h"
#include "loom/value/inspect.h"

namespace loom::qt
{
    namespace
    {
        const char* const kDefaultFamily = "Georgia";
        constexpr int kDefaultSize = 16;

        QString spanOf(const TextRun& run)
        {
            const QString family = run.font.empty() ? QString(kDefaultFamily) : toQt(run.font);
            const long long size = run.size > 0 ? run.size : kDefaultSize;

            QString style = QString("font-family:'%1';font-size:%2pt;").arg(family).arg(size);

            if (run.bold)          style += "font-weight:bold;";
            if (run.italic)        style += "font-style:italic;";
            if (run.underline)     style += "text-decoration:underline;";

            if (!isNull(run.color))
            {
                style += QString("color:%1;").arg(toColour(run.color).name());
            }

            const QString words = toQt(run.text).toHtmlEscaped().replace("\n", "<br>");

            return QString("<span style=\"%1\">%2</span>").arg(style, words);
        }
    }

    QString passageHtml(const std::vector<TextRun>& runs)
    {
        QString html;

        for (const TextRun& run : runs) html += spanOf(run);

        return "<p>" + html + "</p>";
    }
}
