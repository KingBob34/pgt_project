#include "prose_editor.h"

#include <QComboBox>
#include <QFontMetricsF>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextObjectInterface>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "loom/qt/convert.h"
#include "loom/value/inspect.h"

using loom::qt::toQt;

namespace
{
    // The chip is a text object rather than a run of characters, which is what
    // makes it indivisible: the document holds one character for it, so one
    // backspace takes the whole thing away.
    constexpr int kSlotObject = QTextFormat::UserObject + 1;

    constexpr int kSlotIndex  = QTextFormat::UserProperty + 1;
    constexpr int kSlotLabel  = QTextFormat::UserProperty + 2;
    constexpr int kSlotColour = QTextFormat::UserProperty + 3;

    // Around the words on a chip, and how round its corners are.
    constexpr double kChipPadX = 3.0;
    constexpr double kChipRadius = 4.0;

    // A chip is drawn a shade smaller than the words around it, so its box
    // fits inside the line instead of prising it open. This is only how the
    // chip looks here: the value it stands for reaches the page at whatever
    // size the author gave the passage.
    constexpr double kChipTextScale = 0.8;

    constexpr int kBandHeight = 24;

    // What an untouched passage is written in. The band opens showing these,
    // so choosing what is already there is never a silent no-op.
    // Georgia was drawn for reading on a screen and ships with Windows, so a
    // passage looks the same on the machine the story is played on.
    const char* const kDefaultFamily = "Georgia";
    constexpr int kDefaultSize = 16;

    const char* const kFamilies[] = {
        "Segoe UI", "Arial", "Calibri", "Tahoma", "Verdana", "Trebuchet MS",
        "Georgia", "Times New Roman", "Cambria", "Garamond", "Palatino Linotype",
        "Courier New", "Consolas", "Comic Sans MS", "Impact"
    };

    const int kSizes[] = { 10, 12, 14, 16, 18, 20, 22, 26, 32, 40, 56, 72 };

    // Two rows of eight. No greys: black and white change places between this
    // box and the page, and a grey has no partner to change places with.
    const char* const kSwatches[] = {
        "#000000", "#ffffff", "#c0392b", "#e8663d", "#e0a020", "#c9a227", "#7fae3a", "#3f9a4f",
        "#2fa88f", "#2f8fa8", "#3a6fd8", "#2f5fd0", "#7d4fc0", "#a94fc0", "#c04f92", "#8b5a2b"
    };

    // The editor is dark and the page a player reads is white, so a passage
    // written in black shows white here and arrives black there. Every other
    // colour reads the same on both, and swapping twice gives back what went
    // in, so one function serves both directions.
    QColor swapped(const QColor& colour)
    {
        if (colour == QColor(Qt::black)) return QColor(Qt::white);
        if (colour == QColor(Qt::white)) return QColor(Qt::black);

        return colour;
    }

    constexpr int kSwatchSize = 18;
    constexpr int kSwatchColumns = 8;

    // The look on a span, written the same way whether the span holds words or
    // stands for a pin: a quoted value takes the styling around it into the
    // game, even though the chip on the canvas keeps its own colour.
    void styleOnto(loom::Value& span, const QTextCharFormat& format)
    {
        const QStringList families = format.fontFamilies().toStringList();

        if (!families.isEmpty())
        {
            loom::objectSet(span, "font", families.first().toStdString());
        }

        if (format.fontPointSize() > 0.0)
        {
            loom::objectSet(span, "size", static_cast<long long>(format.fontPointSize()));
        }

        if (format.foreground().style() != Qt::NoBrush)
        {
            loom::objectSet(span, "color",
                            loom::qt::fromColour(swapped(format.foreground().color())));
        }

        if (format.fontWeight() >= QFont::Bold) loom::objectSet(span, "bold", true);
        if (format.fontItalic()) loom::objectSet(span, "italic", true);
        if (format.fontUnderline()) loom::objectSet(span, "underline", true);
    }

    // The other way round: the look a span was written with, put back on a
    // format. Reading and writing are kept next to each other so a key added
    // to one is not forgotten by the other.
    QTextCharFormat styleFrom(const loom::Value& span)
    {
        QTextCharFormat format;

        if (const loom::Value* font = loom::objectGet(span, "font"))
        {
            const QString named = toQt(loom::asString(*font));

            if (!named.isEmpty()) format.setFontFamilies({ named });
        }

        if (const loom::Value* points = loom::objectGet(span, "size"))
        {
            const int wanted = static_cast<int>(loom::asInt(*points));

            if (wanted > 0) format.setFontPointSize(wanted);
        }

        if (const loom::Value* colour = loom::objectGet(span, "color"))
        {
            const QColor drawn = loom::qt::toColour(*colour);

            if (drawn.isValid()) format.setForeground(swapped(drawn));
        }

        const loom::Value* bold = loom::objectGet(span, "bold");
        if (bold != nullptr && loom::asBool(*bold)) format.setFontWeight(QFont::Bold);

        const loom::Value* italic = loom::objectGet(span, "italic");
        if (italic != nullptr && loom::asBool(*italic)) format.setFontItalic(true);

        const loom::Value* underline = loom::objectGet(span, "underline");
        if (underline != nullptr && loom::asBool(*underline)) format.setFontUnderline(true);

        return format;
    }

    QString labelOf(const QTextFormat& format)
    {
        return format.property(kSlotLabel).toString();
    }

    // The font a chip is written in: the document's, with whatever this run
    // says about itself laid over it. Measuring and drawing both work it out
    // this way, or the box comes out sized for one font and filled with
    // another -- a format on its own does not know the document's default.
    QFont fontOf(QTextDocument* document, const QTextFormat& format)
    {
        QFont font = document != nullptr ? document->defaultFont() : QFont();

        const QTextCharFormat& chars = static_cast<const QTextCharFormat&>(format);
        const QStringList families = chars.fontFamilies().toStringList();

        if (!families.isEmpty()) font.setFamily(families.first());

        if (chars.fontPointSize() > 0.0) font.setPointSizeF(chars.fontPointSize());

        if (chars.hasProperty(QTextFormat::FontWeight))
        {
            font.setWeight(QFont::Weight(chars.fontWeight()));
        }

        font.setItalic(chars.fontItalic());

        return font;
    }

    // The same font, taken down enough to leave a margin inside the box.
    QFont chipFontOf(QTextDocument* document, const QTextFormat& format)
    {
        QFont font = fontOf(document, format);
        font.setPointSizeF(font.pointSizeF() * kChipTextScale);

        return font;
    }

    // Draws the chips. Qt will not take a handler without a metaobject, which
    // is why this is a QObject rather than a plain interface.
    class SlotChip : public QObject, public QTextObjectInterface
    {
        Q_OBJECT
        Q_INTERFACES(QTextObjectInterface)

    public:
        QSizeF intrinsicSize(QTextDocument* document, int, const QTextFormat& format) override
        {
            const QFontMetricsF around(fontOf(document, format));
            const QFontMetricsF inside(chipFontOf(document, format));

            // An inline object stands with its foot on the baseline, so asking
            // for anything past the ascent lifts the chip off the bottom of the
            // line and prises the line open above it by the same amount. The
            // ascent is the whole of the room there is.
            return QSizeF(inside.horizontalAdvance(labelOf(format)) + 2 * kChipPadX,
                          around.ascent());
        }

        void drawObject(QPainter* painter, const QRectF& rect, QTextDocument* document, int,
                        const QTextFormat& format) override
        {
            const QColor colour = format.property(kSlotColour).value<QColor>();

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);

            // The painter arrives carrying whatever font it last had, which is
            // not the one the box was measured with.
            painter->setFont(chipFontOf(document, format));

            // Only the ascent was reserved, so the line is not prised open, but
            // painting is not held to that rectangle: reaching down to where
            // the words around it end seats the chip on the line instead of
            // leaving it hanging off the baseline.
            const QFontMetricsF around(fontOf(document, format));
            const QRectF box = rect.adjusted(0.0, 0.0, 0.0, around.descent());

            painter->setPen(Qt::NoPen);
            painter->setBrush(colour);
            painter->drawRoundedRect(box, kChipRadius, kChipRadius);

            // Middled in the box, which the smaller font leaves room for.
            painter->setPen(colour.lightness() < 140 ? Qt::white : Qt::black);
            painter->drawText(box, Qt::AlignCenter, labelOf(format));

            painter->restore();
        }
    };

    QPixmap swatchOf(const QColor& colour)
    {
        QPixmap drawn(kSwatchSize, kSwatchSize);
        drawn.fill(colour);

        return drawn;
    }
}

ProseEditor::ProseEditor(ProseSlots slotSource, QWidget* parent)
    : QWidget(parent), inked(Qt::black), offer(std::move(slotSource))
{
    text = new QTextEdit(this);
    text->setAcceptRichText(false);

    // The page the player will read, so that unstyled words look here the way
    // they will look there. Left to the desktop theme the two disagree, and a
    // dark one makes black text invisible on one side or the other.
    text->setStyleSheet("QTextEdit { background: #1e1e20; color: #ececec; }");
    text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    text->setTabChangesFocus(true);

    // What the band opens showing has to be what an untouched passage is
    // really written in, or picking that entry changes nothing and reads as a
    // broken control.
    text->document()->setDefaultFont(QFont(kDefaultFamily, kDefaultSize));

    text->document()->documentLayout()->registerHandler(kSlotObject, new SlotChip);

    QVBoxLayout* stack = new QVBoxLayout(this);
    stack->setContentsMargins(0, 0, 0, 0);
    stack->setSpacing(2);
    stack->addWidget(buildBand());
    stack->addWidget(text, 1);

    connect(text, &QTextEdit::textChanged, this, [this]
    {
        if (!loading) Q_EMIT edited();
    });

    connect(text, &QTextEdit::cursorPositionChanged, this, [this]
    {
        if (text->hasFocus()) mark = text->textCursor();

        followCursor();
    });

    connect(text, &QTextEdit::selectionChanged, this, [this]
    {
        if (text->hasFocus()) mark = text->textCursor();
    });

    followCursor();
}

QToolButton* ProseEditor::buildWeight(QWidget* band, const QString& mark, const QString& hint)
{
    QToolButton* button = new QToolButton(band);
    button->setText(mark);
    button->setToolTip(hint);
    button->setCheckable(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setFixedWidth(24);

    return button;
}

QWidget* ProseEditor::buildBand()
{
    QWidget* band = new QWidget(this);
    band->setFixedHeight(kBandHeight);

    QHBoxLayout* row = new QHBoxLayout(band);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);

    // Nothing on the band takes the keyboard, so the place the author was
    // working keeps the cursor and a selection stays lit while it is used.
    row->addWidget(new QLabel("Font:", band));

    family = new QComboBox(band);
    for (const char* name : kFamilies) family->addItem(name);
    family->setFocusPolicy(Qt::NoFocus);
    family->setFixedWidth(132);
    row->addWidget(family);

    // activated rather than currentIndexChanged: picking the entry already
    // showing is still the author asking for it.
    connect(family, &QComboBox::activated, this, [this](int)
    {
        applyFont(family->currentText());
    });

    row->addWidget(new QLabel("Size:", band));

    size = new QComboBox(band);
    for (int points : kSizes) size->addItem(QString::number(points), points);
    size->setFocusPolicy(Qt::NoFocus);
    size->setFixedWidth(58);
    row->addWidget(size);

    connect(size, &QComboBox::activated, this, [this](int)
    {
        applySize(size->currentData().toInt());
    });

    boldButton      = buildWeight(band, "B", "Bold");
    italicButton    = buildWeight(band, "I", "Italic");
    underlineButton = buildWeight(band, "U", "Underline");

    boldButton->setStyleSheet("QToolButton { font-weight: bold; }");
    italicButton->setStyleSheet("QToolButton { font-style: italic; }");
    underlineButton->setStyleSheet("QToolButton { text-decoration: underline; }");

    row->addWidget(boldButton);
    row->addWidget(italicButton);
    row->addWidget(underlineButton);

    connect(boldButton, &QToolButton::clicked, this, [this](bool on)
    {
        restyle([on](QTextCharFormat& format)
        {
            format.setFontWeight(on ? QFont::Bold : QFont::Normal);
        });
    });

    connect(italicButton, &QToolButton::clicked, this, [this](bool on)
    {
        restyle([on](QTextCharFormat& format) { format.setFontItalic(on); });
    });

    connect(underlineButton, &QToolButton::clicked, this, [this](bool on)
    {
        restyle([on](QTextCharFormat& format) { format.setFontUnderline(on); });
    });

    row->addWidget(new QLabel("Colour:", band));

    colourButton = new QToolButton(band);
    colourButton->setFocusPolicy(Qt::NoFocus);
    colourButton->setPopupMode(QToolButton::InstantPopup);
    colourButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    colourButton->setIcon(swatchOf(inked));
    colourButton->setMenu(buildSwatches());
    row->addWidget(colourButton);

    QToolButton* insert = new QToolButton(band);
    insert->setText("Insert Variable");
    insert->setFocusPolicy(Qt::NoFocus);
    insert->setPopupMode(QToolButton::InstantPopup);
    row->addWidget(insert);

    // Built afresh on every showing, so a pin rewired a moment ago is named
    // the way it is named now.
    QMenu* values = new QMenu(insert);
    insert->setMenu(values);

    connect(values, &QMenu::aboutToShow, this, [this, values] { fillSlotMenu(values); });

    row->addStretch(1);

    return band;
}

QMenu* ProseEditor::buildSwatches()
{
    QMenu* menu = new QMenu(this);

    QWidget* board = new QWidget(menu);

    QGridLayout* grid = new QGridLayout(board);
    grid->setContentsMargins(4, 4, 4, 4);
    grid->setSpacing(2);

    int cell = 0;

    for (const char* name : kSwatches)
    {
        const QColor colour(name);

        QToolButton* swatch = new QToolButton(board);
        swatch->setFixedSize(kSwatchSize, kSwatchSize);
        swatch->setFocusPolicy(Qt::NoFocus);
        swatch->setToolTip(colour.name());
        swatch->setStyleSheet(QString("QToolButton { background: %1;"
                                      " border: 1px solid #5a5a5a; }"
                                      "QToolButton:hover { border: 1px solid #ffffff; }")
                                  .arg(colour.name()));

        connect(swatch, &QToolButton::clicked, this, [this, colour, menu]
        {
            applyColour(colour);

            menu->close();
        });

        grid->addWidget(swatch, cell / kSwatchColumns, cell % kSwatchColumns);
        ++cell;
    }

    QWidgetAction* held = new QWidgetAction(menu);
    held->setDefaultWidget(board);
    menu->addAction(held);

    return menu;
}

void ProseEditor::fillSlotMenu(QMenu* menu)
{
    menu->clear();

    const std::vector<ProseSlot> offered = offer ? offer() : std::vector<ProseSlot>();

    if (offered.empty())
    {
        menu->addAction("No value pins on this node")->setEnabled(false);
        return;
    }

    for (const ProseSlot& slot : offered)
    {
        QAction* entry = menu->addAction(QIcon(swatchOf(slot.colour)), slot.label);

        connect(entry, &QAction::triggered, this, [this, slot] { insertSlot(slot); });
    }
}

QTextCursor ProseEditor::working() const
{
    return mark.isNull() ? text->textCursor() : mark;
}

void ProseEditor::followCursor()
{
    if (syncing) return;

    syncing = true;

    const QTextCharFormat format = text->currentCharFormat();

    const QStringList families = format.fontFamilies().toStringList();
    const QString shown = families.isEmpty() ? QString(kDefaultFamily) : families.first();

    const int named = family->findText(shown);
    if (named >= 0) family->setCurrentIndex(named);

    const int points = format.fontPointSize() > 0.0 ? int(format.fontPointSize()) : kDefaultSize;

    const int sized = size->findData(points);
    if (sized >= 0) size->setCurrentIndex(sized);

    boldButton->setChecked(format.fontWeight() >= QFont::Bold);
    italicButton->setChecked(format.fontItalic());
    underlineButton->setChecked(format.fontUnderline());

    inked = format.foreground().style() == Qt::NoBrush
          ? QColor(Qt::black)
          : swapped(format.foreground().color());

    colourButton->setIcon(swatchOf(inked));

    syncing = false;
}

void ProseEditor::restyle(const std::function<void(QTextCharFormat&)>& change)
{
    // Starts empty so that merging touches only what was asked for, and the
    // object type of a chip beside the cursor is left alone.
    QTextCharFormat format;
    change(format);

    QTextCursor cursor = working();

    if (cursor.hasSelection()) cursor.mergeCharFormat(format);

    // The one the widget types through next, which is a different cursor from
    // the one just merged through.
    text->setTextCursor(cursor);
    text->mergeCurrentCharFormat(format);
    text->setFocus();
}

void ProseEditor::applyFont(const QString& chosen)
{
    restyle([&chosen](QTextCharFormat& format) { format.setFontFamilies({ chosen }); });
}

void ProseEditor::applySize(int points)
{
    restyle([points](QTextCharFormat& format) { format.setFontPointSize(points); });
}

void ProseEditor::applyColour(const QColor& colour)
{
    // Everything outside here speaks of the colour the story will carry. The
    // document is the one place shown the other one.
    inked = colour;
    colourButton->setIcon(swatchOf(colour));

    const QColor drawn = swapped(colour);

    restyle([drawn](QTextCharFormat& format) { format.setForeground(drawn); });
}

void ProseEditor::insertSlot(const ProseSlot& slot)
{
    QTextCursor cursor = working();

    // The look of the words it is dropped among, so a chip in small text is a
    // small chip. Left bare it would be measured against the document's own
    // font instead, and in a passage written smaller than that it would stand
    // taller than the line and prise it open.
    QTextCharFormat format = cursor.charFormat();
    format.setObjectType(kSlotObject);
    format.setProperty(kSlotIndex, slot.index);
    format.setProperty(kSlotLabel, slot.label);
    format.setProperty(kSlotColour, slot.colour);

    cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);

    text->setTextCursor(cursor);
    text->setFocus();
}

void ProseEditor::setPassage(const loom::Value& passage)
{
    const std::vector<ProseSlot> offered = offer ? offer() : std::vector<ProseSlot>();

    loading = true;

    text->clear();

    QTextCursor cursor = text->textCursor();

    const loom::Value* spans = loom::objectGet(passage, "spans");

    for (std::size_t index = 0; spans != nullptr && index < loom::listSize(*spans); ++index)
    {
        const loom::Value& span = *loom::listAt(*spans, index);
        const loom::Value* slot = loom::objectGet(span, "slot");

        if (slot != nullptr)
        {
            const int wanted = static_cast<int>(loom::asInt(*slot));

            // A chip is drawn from what the node offers now, so a pin that has
            // been rewired since carries its new name.
            ProseSlot chip{ wanted, QString("Value %1").arg(wanted + 1), QColor(120, 120, 120) };

            for (const ProseSlot& known : offered)
            {
                if (known.index == wanted) chip = known;
            }

            QTextCharFormat format = styleFrom(span);
            format.setObjectType(kSlotObject);
            format.setProperty(kSlotIndex, chip.index);
            format.setProperty(kSlotLabel, chip.label);
            format.setProperty(kSlotColour, chip.colour);

            cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
            continue;
        }

        cursor.insertText(toQt(loom::asString(*loom::objectGet(span, "text"))),
                          styleFrom(span));
    }

    loading = false;

    followCursor();
}

loom::Value ProseEditor::passage() const
{
    loom::Value spans = loom::makeList();

    const QTextDocument* document = text->document();

    for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
    {
        // A block boundary is a line the author broke; the runs carry it as an
        // ordinary newline rather than a structure of their own.
        if (block != document->begin())
        {
            loom::Value span = loom::makeObject();
            loom::objectSet(span, "text", "\n");

            loom::listAppend(spans, span);
        }

        for (QTextBlock::iterator part = block.begin(); !part.atEnd(); ++part)
        {
            const QTextFragment fragment = part.fragment();
            if (!fragment.isValid()) continue;

            const QTextCharFormat format = fragment.charFormat();

            if (format.objectType() == kSlotObject)
            {
                // Two identical chips side by side are one fragment, so the
                // run is written out once per character it holds.
                for (int copy = 0; copy < fragment.text().length(); ++copy)
                {
                    loom::Value span = loom::makeObject();
                    loom::objectSet(span, "slot", format.property(kSlotIndex).toInt());
                    styleOnto(span, format);

                    loom::listAppend(spans, span);
                }

                continue;
            }

            loom::Value span = loom::makeObject();
            loom::objectSet(span, "text", fragment.text().toStdString());
            styleOnto(span, format);

            loom::listAppend(spans, span);
        }
    }

    loom::Value stored = loom::makeObject();
    loom::objectSet(stored, "spans", spans);

    return stored;
}

#include "prose_editor.moc"

