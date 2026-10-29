#pragma once
#include <functional>

#include <QWidget>

class QSlider;
class QLineEdit;
class QComboBox;

// Mouse wheel over a slider / combo box / number box only changes it when it has keyboard focus;
// otherwise the wheel scrolls the page (so scrolling the parameter panel never gets "stuck" on a control).
void installWheelGuard(QWidget* w);

// Gives a combo box the rounded popup list used by the theme (call once after creating it).
void polishCombo(QComboBox* c);

// One labelled parameter: name + description + slider + number box (kept in sync).
// The number box is a normal text field: click, type a number, press Enter (or leave the box).
class ParamRow : public QWidget {
public:
    ParamRow(const QString& name, const QString& help, double minV, double maxV, double step, double value,
             int decimals = 2, QWidget* parent = nullptr);

    double value() const { return m_value; }
    int intValue() const;
    void setValue(double v);
    void setRange(double lo, double hi);
    void setOnChange(std::function<void()> fn) { m_onChange = std::move(fn); }
    const QString& label() const { return m_label; }
    QString valueText() const;  // value formatted with the row's decimals

private:
    double normalise(double v) const;  // clamp + round to the row's decimals
    int posFromValue(double v) const;
    void commitText();
    void setValueInternal(double v, bool fromSlider);

    QSlider* m_slider;
    QLineEdit* m_edit;
    double m_min, m_max, m_step, m_value;
    int m_decimals;
    QString m_label;
    std::function<void()> m_onChange;
};
