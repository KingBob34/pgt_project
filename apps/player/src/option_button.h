#ifndef LOOM_PLAYER_OPTION_BUTTON_H
#define LOOM_PLAYER_OPTION_BUTTON_H
#include <QPushButton>

// A choice the player can take. An option is often a whole sentence, and a
// QPushButton draws its label on one line and cuts off the rest, so this one
// lays the text out itself and asks for the height that layout needs.
class OptionButton : public QPushButton
{
    Q_OBJECT

public:
    OptionButton(const QString& label, int fontSize, QWidget* parent = nullptr);

    bool  hasHeightForWidth() const override { return true; }
    int   heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // Breaks the label into lines that fit the given width and returns how tall
    // they came out. The one place the spacing between lines is decided.
    double layOut(class QTextLayout& lines, double width) const;

    // What is left of the button once its padding is taken off.
    double textWidth(int width) const;
};

#endif //LOOM_PLAYER_OPTION_BUTTON_H
