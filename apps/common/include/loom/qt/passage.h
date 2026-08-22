#ifndef LOOM_QT_PASSAGE_H
#define LOOM_QT_PASSAGE_H
#include <vector>

#include <QString>

#include "loom/graph/execution.h"

namespace loom::qt
{
    // One passage as a paragraph of styled spans, ready for a QTextEdit. What
    // a run left unset is left out of its span, so the reader's own default
    // shows through.
    QString passageHtml(const std::vector<TextRun>& runs);
}

#endif //LOOM_QT_PASSAGE_H
