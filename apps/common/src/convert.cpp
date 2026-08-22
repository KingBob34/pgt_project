#include "loom/qt/convert.h"

#include "loom/value/inspect.h"

namespace loom::qt
{
    namespace
    {
        // JSON drops a zero fraction, so a channel comes back as either.
        int channel(const Value& source, const std::string& key)
        {
            const Value* component = objectGet(source, key);
            if (component == nullptr) return 0;

            const double scaled = isInt(*component) ? static_cast<double>(asInt(*component))
                                                    : asFloat(*component);

            return static_cast<int>(scaled * 255.0);
        }
    }

    QString toQt(const std::string& text)
    {
        return QString::fromStdString(text);
    }

    QColor toColour(const Value& value)
    {
        if (!isObject(value)) return QColor();

        return QColor(channel(value, "r"), channel(value, "g"), channel(value, "b"));
    }

    Value fromColour(const QColor& colour)
    {
        Value stored = makeObject();

        objectSet(stored, "r", colour.redF());
        objectSet(stored, "g", colour.greenF());
        objectSet(stored, "b", colour.blueF());

        return stored;
    }
}
