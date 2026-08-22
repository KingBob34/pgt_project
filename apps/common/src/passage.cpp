#include "loom/qt/passage.h"

#include "loom/qt/convert.h"
#include "loom/value/inspect.h"

namespace loom::qt
{
    namespace
    {
        QString spanOf(const TextRun& run)
        {
            QString style;

            if (!run.font.empty()) style += QString("font-family:'%1';").arg(toQt(run.font));
            if (run.size > 0)      style += QString("font-size:%1pt;").arg(run.size);

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
