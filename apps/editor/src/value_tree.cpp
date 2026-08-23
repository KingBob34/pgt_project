#include "value_tree.h"

#include <utility>
#include <vector>

#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QPalette>
#include <QStyledItemDelegate>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "pin_editor.h"

#include "loom/qt/convert.h"
#include "loom/value/inspect.h"
#include "loom/value/parse.h"

using loom::qt::toQt;

namespace
{
    constexpr int kIndent = 12;

    // The two fixed columns; the value column is whatever is left over.
    constexpr int kNameWidth = 96;
    constexpr int kTypeWidth = 96;
    constexpr int kNarrowestColumn = 48;

    // A list is indexed from zero, but the panel is not the file and the
    // author is not a programmer: rows are counted the way people count.
    std::string rowLabel(int at)
    {
        return std::to_string(at + 1);
    }

    // The type and value columns are a widget or a summary the tree wrote
    // itself, so a typed edit only ever belongs to the name.
    class NameOnlyDelegate : public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override
        {
            if (index.column() != 0) return nullptr;

            QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
            if (editor == nullptr) return nullptr;

            // The view goes on drawing the row underneath the editor, so an
            // editor that does not fill its own background leaves the old name
            // showing through whatever is typed over it. Both background roles
            // are set, because which one is filled depends on the widget.
            QPalette solid = editor->palette();
            solid.setColor(QPalette::Window, solid.color(QPalette::Base));

            editor->setPalette(solid);
            editor->setAutoFillBackground(true);

            return editor;
        }
    };

    // The declared types an author may pick, named as the pins are.
    const std::vector<std::pair<const char*, const char*>>& typeChoices()
    {
        static const std::vector<std::pair<const char*, const char*>> table = {
            { loom::VariableType::String, "String" },
            { loom::VariableType::Int,    "Integer" },
            { loom::VariableType::Float,  "Float" },
            { loom::VariableType::Bool,   "Bool" },
            { loom::VariableType::List,   "List" },
            { loom::VariableType::Group,  "Group" },
        };

        return table;
    }

    // What a variable of a type holds before the author writes anything in it.
    loom::Value blankValue(const std::string& type)
    {
        if (type == loom::VariableType::Int)   return loom::Value(0);
        if (type == loom::VariableType::Float) return loom::Value(0.0);
        if (type == loom::VariableType::Bool)  return loom::Value(false);
        if (type == loom::VariableType::List)  return loom::makeList();
        if (type == loom::VariableType::Group) return loom::makeObject();

        return loom::Value("");
    }

    // The row keeps its value as the text it will be written to file as.
    QString asText(const loom::Value& value)
    {
        return toQt(loom::writeJson(value));
    }

    loom::Value fromText(const QString& text)
    {
        loom::Value parsed;
        std::string error;

        if (!loom::parseJson(text.toStdString(), parsed, error)) return loom::Value();

        return parsed;
    }

    bool isContainer(const std::string& type)
    {
        return type == loom::VariableType::List || type == loom::VariableType::Group;
    }

    // Only a declared variable carries its type in the file; a row nested inside
    // one is whatever its stored value already is.
    std::string typeFromValue(const loom::Value& value)
    {
        if (loom::isBool(value))   return loom::VariableType::Bool;
        if (loom::isInt(value))    return loom::VariableType::Int;
        if (loom::isFloat(value))  return loom::VariableType::Float;
        if (loom::isList(value))   return loom::VariableType::List;
        if (loom::isObject(value)) return loom::VariableType::Group;

        return loom::VariableType::String;
    }
}

ValueTree::ValueTree(QWidget* parent)
    : QWidget(parent)
{
    tree = new QTreeWidget;
    tree->setColumnCount(3);
    tree->setHeaderLabels({ "Name", "Type", "Value" });
    tree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    tree->setItemDelegate(new NameOnlyDelegate(tree));

    // Nesting is paid for out of the name column, so the other two keep their
    // width and the tree scrolls sideways once names have nowhere left to go.
    tree->setIndentation(kIndent);
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // The value column takes up whatever is left, and both dividers still drag
    // because the two columns beside them are the ones being resized.
    //
    // Stretch, rather than setStretchLastSection: that one will not take the
    // last column below a hundred pixels whatever it is told, so the three
    // together outgrow a narrow panel and the name column is carried off the
    // left edge.
    QHeaderView* header = tree->header();
    header->setMinimumSectionSize(kNarrowestColumn);

    // A stretched column will not go below the default section size either, so
    // that has to come down with the minimum or the three still outgrow a
    // narrow panel.
    header->setDefaultSectionSize(kNarrowestColumn);
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->resizeSection(0, kNameWidth);
    header->resizeSection(1, kTypeWidth);

    connect(tree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* row, int column)
    {
        if (filling) return;

        QTreeWidgetItem* parent = row->parent();

        // A list numbers its own rows, so only named ones can collide, and two
        // under one name would fold into one on the way to the file.
        const bool named = parent == nullptr || typeOf(parent) != loom::VariableType::List;

        if (column == 0 && named)
        {
            const std::string typed = row->text(0).toStdString();

            int seen = 0;
            for (int at = 0; at < siblingCount(parent); ++at)
            {
                if (sibling(parent, at)->text(0).toStdString() == typed) ++seen;
            }

            if (seen > 1)
            {
                filling = true;
                row->setText(0, toQt(uniqueName(parent, typed)));
                filling = false;
            }
        }

        // A nested field is named by its path, so a rename anywhere but inside
        // a list is worth reporting.
        if (column == 0 && named)
        {
            const QString was   = row->data(0, Qt::UserRole + 1).toString();
            const QString now   = row->text(0);
            const QString ahead = parent == nullptr ? QString() : pathOf(parent) + ".";

            if (!was.isEmpty() && was != now)
            {
                filling = true;
                row->setData(0, Qt::UserRole + 1, now);
                filling = false;

                Q_EMIT renamed(ahead + was, ahead + now);
            }
        }

        Q_EMIT changed();
    });

    // Deleting is a rare, aimed act, so it lives on the row itself rather than
    // beside the button that creates one.
    tree->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& at)
    {
        QTreeWidgetItem* row = tree->itemAt(at);
        if (row == nullptr) return;

        tree->setCurrentItem(row);

        QMenu menu(this);
        menu.addAction("Delete", this, &ValueTree::removeVariable);
        menu.exec(tree->viewport()->mapToGlobal(at));
    });

    QPushButton* add = new QPushButton("+");
    add->setFixedWidth(28);
    add->setToolTip("Add a variable, or a row inside the selected list or group");

    connect(add, &QPushButton::clicked, this, &ValueTree::addVariable);

    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->setSpacing(4);
    buttons->addWidget(add);
    buttons->addStretch();

    QVBoxLayout* column = new QVBoxLayout(this);
    column->setContentsMargins(4, 4, 4, 4);
    column->addLayout(buttons);
    column->addWidget(tree, 1);
}

void ValueTree::setVariables(const std::map<std::string, loom::VariableSpec>& variables)
{
    filling = true;

    tree->clear();

    for (const auto& entry : variables)
    {
        addRow(nullptr, entry.first, entry.second.type, entry.second.value);
    }

    filling = false;
}

std::map<std::string, loom::VariableSpec> ValueTree::variables() const
{
    std::map<std::string, loom::VariableSpec> out;

    for (int at = 0; at < tree->topLevelItemCount(); ++at)
    {
        QTreeWidgetItem* row = tree->topLevelItem(at);

        loom::VariableSpec spec;
        spec.type = typeOf(row);
        spec.value = valueOf(row);

        out[row->text(0).toStdString()] = spec;
    }

    return out;
}

loom::Value ValueTree::valueOf(QTreeWidgetItem* row) const
{
    const std::string type = typeOf(row);

    if (type == loom::VariableType::List)
    {
        loom::Value out = loom::makeList();
        for (int at = 0; at < row->childCount(); ++at) loom::listAppend(out, valueOf(row->child(at)));

        return out;
    }

    if (type == loom::VariableType::Group)
    {
        loom::Value out = loom::makeObject();
        for (int at = 0; at < row->childCount(); ++at)
        {
            QTreeWidgetItem* child = row->child(at);
            loom::objectSet(out, child->text(0).toStdString(), valueOf(child));
        }

        return out;
    }

    return fromText(row->data(0, Qt::UserRole).toString());
}

std::string ValueTree::typeOf(QTreeWidgetItem* row) const
{
    if (QComboBox* types = qobject_cast<QComboBox*>(tree->itemWidget(row, 1)))
    {
        return types->currentData().toString().toStdString();
    }

    return loom::VariableType::String;
}

QTreeWidgetItem* ValueTree::addRow(QTreeWidgetItem* parent, const std::string& name,
                                   const std::string& type, const loom::Value& value)
{
    QTreeWidgetItem* row = parent == nullptr ? new QTreeWidgetItem(tree)
                                             : new QTreeWidgetItem(parent);

    fillRow(row, name, type, value);

    return row;
}

void ValueTree::fillRow(QTreeWidgetItem* row, const std::string& name,
                        const std::string& type, const loom::Value& value)
{
    QTreeWidgetItem* parent = row->parent();
    const bool named = parent == nullptr || typeOf(parent) != loom::VariableType::List;

    if (named) row->setFlags(row->flags() | Qt::ItemIsEditable);
    else       row->setFlags(row->flags() & ~Qt::ItemIsEditable);

    row->setText(0, toQt(name));
    row->setData(0, Qt::UserRole, asText(value));
    row->setData(0, Qt::UserRole + 1, toQt(name));

    QComboBox* types = new QComboBox;
    for (const auto& entry : typeChoices()) types->addItem(entry.second, QString(entry.first));

    const int picked = types->findData(toQt(type));
    types->setCurrentIndex(picked < 0 ? 0 : picked);

    // Connected last: setCurrentIndex would otherwise blank the value.
    connect(types, &QComboBox::currentIndexChanged, this, [this, row, types](int)
    {
        retype(row, types->currentData().toString().toStdString());
    });

    tree->setItemWidget(row, 1, types);

    showValue(row, types->currentData().toString().toStdString());
}

void ValueTree::showValue(QTreeWidgetItem* row, const std::string& type)
{
    for (QTreeWidgetItem* gone : row->takeChildren()) delete gone;

    if (isContainer(type))
    {
        tree->setItemWidget(row, 2, nullptr);

        const loom::Value held = fromText(row->data(0, Qt::UserRole).toString());

        if (type == loom::VariableType::List && loom::isList(held))
        {
            int index = 0;
            for (const loom::Value& item : held)
            {
                addRow(row, rowLabel(index++), typeFromValue(item), item);
            }
        }
        else if (type == loom::VariableType::Group && loom::isObject(held))
        {
            for (const std::string& key : loom::objectKeys(held))
            {
                if (const loom::Value* item = loom::objectGet(held, key))
                {
                    addRow(row, key, typeFromValue(*item), *item);
                }
            }
        }

        row->setExpanded(true);
        refreshCount(row);

        return;
    }

    row->setText(2, QString());

    loom::PinSpec pin;
    pin.name = "value";
    pin.label = "Value";
    pin.type = type;

    const PinEditor made = makePinEditor(pin, fromText(row->data(0, Qt::UserRole).toString()),
                                         [this, row](loom::Value written)
    {
        row->setData(0, Qt::UserRole, asText(written));
        Q_EMIT changed();
    });

    // A spin box asks for room to show its widest possible number. Left to
    // insist on it, the tree grows past the panel and the name column is
    // carried off the edge.
    made.widget->setMinimumWidth(kNarrowestColumn);
    made.widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    tree->setItemWidget(row, 2, made.widget);
}

void ValueTree::retype(QTreeWidgetItem* row, const std::string& type)
{
    filling = true;

    row->setData(0, Qt::UserRole, asText(blankValue(type)));
    showValue(row, type);

    filling = false;

    Q_EMIT changed();
}

void ValueTree::refreshCount(QTreeWidgetItem* row)
{
    const std::string type = typeOf(row);
    if (!isContainer(type)) return;

    const int count = row->childCount();

    const QString noun = type == loom::VariableType::List
                       ? (count == 1 ? " item" : " items")
                       : (count == 1 ? " field" : " fields");

    const bool was = filling;
    filling = true;

    row->setText(2, QString::number(count) + noun);

    filling = was;
}

void ValueTree::renumber(QTreeWidgetItem* list)
{
    const bool was = filling;
    filling = true;

    for (int at = 0; at < list->childCount(); ++at)
    {
        list->child(at)->setText(0, toQt(rowLabel(at)));
    }

    filling = was;
}

void ValueTree::addVariable()
{
    QTreeWidgetItem* current = tree->currentItem();

    // A container takes the new row inside it; anything else gets a neighbour
    // at the top of the tree.
    QTreeWidgetItem* parent =
        current != nullptr && isContainer(typeOf(current)) ? current : nullptr;

    const bool indexed = parent != nullptr && typeOf(parent) == loom::VariableType::List;

    const std::string name = indexed
                           ? rowLabel(parent->childCount())
                           : uniqueName(parent, parent == nullptr ? "variable" : "field");

    filling = true;

    QTreeWidgetItem* row = addRow(parent, name, loom::VariableType::String,
                                  blankValue(loom::VariableType::String));

    filling = false;

    if (parent != nullptr)
    {
        parent->setExpanded(true);
        refreshCount(parent);
    }

    tree->setCurrentItem(row);

    if (!indexed) tree->editItem(row, 0);

    Q_EMIT changed();
}

void ValueTree::removeVariable()
{
    QTreeWidgetItem* row = tree->currentItem();
    if (row == nullptr) return;

    QTreeWidgetItem* parent = row->parent();

    // A list numbers its own rows, so nothing outside can be naming one.
    const bool named = parent == nullptr || typeOf(parent) != loom::VariableType::List;
    const QString gone = named ? pathOf(row) : QString();

    if (parent == nullptr) delete tree->takeTopLevelItem(tree->indexOfTopLevelItem(row));
    else                   delete parent->takeChild(parent->indexOfChild(row));

    if (parent != nullptr)
    {
        if (typeOf(parent) == loom::VariableType::List) renumber(parent);

        refreshCount(parent);
    }

    Q_EMIT changed();

    if (!gone.isEmpty()) Q_EMIT removed(gone);
}

int ValueTree::siblingCount(QTreeWidgetItem* parent) const
{
    return parent == nullptr ? tree->topLevelItemCount() : parent->childCount();
}

QTreeWidgetItem* ValueTree::sibling(QTreeWidgetItem* parent, int at) const
{
    return parent == nullptr ? tree->topLevelItem(at) : parent->child(at);
}

QString ValueTree::pathOf(QTreeWidgetItem* row) const
{
    QString path = row->text(0);

    for (QTreeWidgetItem* above = row->parent(); above != nullptr; above = above->parent())
    {
        path = above->text(0) + "." + path;
    }

    return path;
}

std::string ValueTree::uniqueName(QTreeWidgetItem* parent, const std::string& wanted) const
{
    const std::string base = wanted.empty() ? "field" : wanted;

    std::string name = base;

    for (int suffix = 2; ; ++suffix)
    {
        bool taken = false;

        for (int at = 0; at < siblingCount(parent); ++at)
        {
            if (sibling(parent, at)->text(0).toStdString() == name) taken = true;
        }

        if (!taken) return name;

        name = base + std::to_string(suffix);
    }
}
