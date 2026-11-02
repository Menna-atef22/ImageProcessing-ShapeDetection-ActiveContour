#include "ParamRow.h"

#include <QApplication>
#include <QComboBox>
#include <QListView>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace {
class WheelGuard : public QObject {
public:
    using QObject::QObject;
    bool eventFilter(QObject* o, QEvent* e) override {
        if (e->type() != QEvent::Wheel) return false;
        auto* w = qobject_cast<QWidget*>(o);
        if (!w || w->hasFocus()) return false;  // focused control: let it react to the wheel
        // forward to the enclosing scroll area so the page scrolls instead
        for (QWidget* p = w->parentWidget(); p; p = p->parentWidget())
            if (auto* sa = qobject_cast<QScrollArea*>(p)) {
                auto* we = static_cast<QWheelEvent*>(e);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                QWheelEvent copy(we->position(), we->globalPosition(), we->pixelDelta(), we->angleDelta(), we->buttons(),
                                 we->modifiers(), we->phase(), we->inverted());
#else
                QWheelEvent copy(we->posF(), we->globalPosF(), we->pixelDelta(), we->angleDelta(), we->buttons(),
                                 we->modifiers(), we->phase(), we->inverted());
#endif
                QApplication::sendEvent(sa->viewport(), &copy);
                break;
            }
        return true;
    }
};
}  // namespace

void installWheelGuard(QWidget* w) {
    static WheelGuard* guard = new WheelGuard(qApp);
    w->setFocusPolicy(Qt::StrongFocus);
    w->installEventFilter(guard);
}

void polishCombo(QComboBox* c) {
    c->setView(new QListView(c));
    c->setMaxVisibleItems(12);
    // make the popup window transparent so the list's rounded corners show (no black corners)
    QWidget* popup = c->view()->window();
    popup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    popup->setAttribute(Qt::WA_TranslucentBackground);
}

ParamRow::ParamRow(const QString& name, const QString& help, double minV, double maxV, double step, double value,
                   int decimals, QWidget* parent)
    : QWidget(parent), m_min(minV), m_max(maxV), m_step(step), m_value(value), m_decimals(decimals), m_label(name) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 4, 0, 6);
    v->setSpacing(4);

    auto* top = new QHBoxLayout;
    auto* lbl = new QLabel(name);
    lbl->setObjectName("paramName");
    m_edit = new QLineEdit;
    m_edit->setObjectName("numBox");
    m_edit->setFixedSize(84, 30);
    m_edit->setAlignment(Qt::AlignCenter);
    m_edit->setToolTip(QString("Type a value between %1 and %2, then press Enter").arg(minV).arg(maxV));
    installWheelGuard(m_edit);
    top->addWidget(lbl);
    top->addStretch();
    top->addWidget(m_edit);
    v->addLayout(top);

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, int(std::lround((maxV - minV) / step)));
    installWheelGuard(m_slider);
    v->addWidget(m_slider);

    auto* h = new QLabel(help);
    h->setObjectName("help");
    h->setWordWrap(true);
    v->addWidget(h);
    setToolTip(help);

    m_value = normalise(value);
    m_edit->setText(valueText());
    m_slider->setValue(posFromValue(m_value));

    connect(m_slider, &QSlider::valueChanged, this, [this](int pos) { setValueInternal(m_min + pos * m_step, true); });
    connect(m_edit, &QLineEdit::editingFinished, this, [this] { commitText(); });
}

double ParamRow::normalise(double v) const {
    v = std::clamp(v, m_min, m_max);
    double f = std::pow(10.0, m_decimals);
    return std::round(v * f) / f;
}

int ParamRow::posFromValue(double v) const { return int(std::lround((v - m_min) / m_step)); }

int ParamRow::intValue() const { return int(std::lround(m_value)); }

QString ParamRow::valueText() const { return QString::number(m_value, 'f', m_decimals); }

void ParamRow::commitText() {
    QString t = m_edit->text().trimmed();
    t.replace(',', '.');
    bool ok = false;
    double v = t.toDouble(&ok);
    if (!ok) {  // not a number -> go back to the current value
        m_edit->setText(valueText());
        return;
    }
    setValueInternal(v, false);
    m_edit->setText(valueText());  // shows the clamped / rounded value
}

void ParamRow::setValueInternal(double v, bool fromSlider) {
    double nv = normalise(v);
    bool changed = nv != m_value;
    m_value = nv;
    if (fromSlider || changed) m_edit->setText(valueText());
    if (!fromSlider) {
        m_slider->blockSignals(true);
        m_slider->setValue(posFromValue(m_value));
        m_slider->blockSignals(false);
    }
    if (changed && m_onChange) m_onChange();
}

void ParamRow::setValue(double v) {
    setValueInternal(v, false);
    m_edit->setText(valueText());
}

void ParamRow::setRange(double lo, double hi) {
    m_min = lo;
    m_max = hi;
    m_slider->blockSignals(true);
    m_slider->setRange(0, int(std::lround((hi - lo) / m_step)));
    m_slider->blockSignals(false);
    m_value = normalise(m_value);
    m_edit->setText(valueText());
    m_slider->blockSignals(true);
    m_slider->setValue(posFromValue(m_value));
    m_slider->blockSignals(false);
    m_edit->setToolTip(QString("Type a value between %1 and %2, then press Enter").arg(lo).arg(hi));
}
