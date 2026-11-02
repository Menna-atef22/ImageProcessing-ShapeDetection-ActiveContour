#include "ImageView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

ImageView::ImageView(const QString& placeholder, QWidget* parent) : QWidget(parent), m_placeholder(placeholder) {
    setMinimumSize(160, 140);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ImageView::setImage(const RgbImage& img) {
    QImage q(img.width, img.height, QImage::Format_RGB888);
    for (int y = 0; y < img.height; ++y) std::copy(img.px(0, y), img.px(0, y) + size_t(img.width) * 3, q.scanLine(y));
    bool sameSize = !m_img.isNull() && m_img.size() == q.size();
    m_img = q;
    if (!sameSize) resetView();  // keep the zoom when only the content changed (same image size)
    update();
}

void ImageView::clear() {
    m_img = QImage();
    resetView();
    update();
}

void ImageView::setColors(const QColor& bg, const QColor& fg) {
    m_bg = bg;
    m_fg = fg;
    update();
}

void ImageView::setPlaceholder(const QString& t) {
    m_placeholder = t;
    update();
}

void ImageView::resetView() {
    m_zoom = 1.0;
    m_center = {0.5, 0.5};
    update();
    emit viewChanged(m_zoom, m_center);
}

void ImageView::setView(double zoom, QPointF center) {
    m_zoom = zoom;
    m_center = center;
    clampCenter();
    update();
}

double ImageView::baseScale() const {
    if (m_img.isNull()) return 1.0;
    double s = std::min(double(width()) / m_img.width(), double(height()) / m_img.height());
    return std::min(s, 4.0);
}

void ImageView::clampCenter() {
    if (m_img.isNull()) return;
    double s = baseScale() * m_zoom;
    double w = m_img.width() * s, h = m_img.height() * s;
    auto clampAxis = [](double c, double view, double size) {
        if (size <= view) return 0.5;
        double half = view / 2.0 / size;
        return std::clamp(c, half, 1.0 - half);
    };
    m_center.setX(clampAxis(m_center.x(), width(), w));
    m_center.setY(clampAxis(m_center.y(), height(), h));
}

QRectF ImageView::targetRect() const {
    if (m_img.isNull()) return {};
    double s = baseScale() * m_zoom;
    double w = m_img.width() * s, h = m_img.height() * s;
    return QRectF(width() / 2.0 - m_center.x() * w, height() / 2.0 - m_center.y() * h, w, h);
}

void ImageView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(rect(), 10, 10);
    p.setClipPath(clip);
    p.fillRect(rect(), m_bg);
    if (m_img.isNull()) {
        p.setPen(m_fg);
        p.drawText(rect().adjusted(16, 16, -16, -16), Qt::AlignCenter | Qt::TextWordWrap, m_placeholder);
        return;
    }
    double scale = baseScale() * m_zoom;
    p.setRenderHint(QPainter::SmoothPixmapTransform, scale < 3.0);  // show real pixels when zoomed in a lot
    p.drawImage(targetRect(), m_img);
    if (m_zoom > 1.01) {
        QString t = QString("%1%  -  double-click to reset").arg(int(std::lround(scale * 100)));
        QFontMetrics fm(font());
        QRect r(10, height() - fm.height() - 14, fm.horizontalAdvance(t) + 16, fm.height() + 6);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 150));
        p.drawRoundedRect(r, 6, 6);
        p.setPen(Qt::white);
        p.drawText(r, Qt::AlignCenter, t);
    }
}

void ImageView::resizeEvent(QResizeEvent*) { clampCenter(); }

// Qt5/Qt6 compatible mouse position
static QPointF mousePos(QMouseEvent* e) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->localPos();
#endif
}

void ImageView::mousePressEvent(QMouseEvent* e) {
    if (m_img.isNull() || e->button() != Qt::LeftButton) return;
    m_down = true;
    m_dragging = false;
    m_pressPos = mousePos(e);
    m_pressCenter = m_center;
}

void ImageView::mouseMoveEvent(QMouseEvent* e) {
    if (!m_down) return;
    QPointF d = mousePos(e) - m_pressPos;
    if (!m_dragging && std::hypot(d.x(), d.y()) < 4) return;
    m_dragging = true;
    setCursor(Qt::ClosedHandCursor);
    double s = baseScale() * m_zoom;
    m_center = m_pressCenter - QPointF(d.x() / (m_img.width() * s), d.y() / (m_img.height() * s));
    clampCenter();
    update();
    emit viewChanged(m_zoom, m_center);
}

void ImageView::mouseReleaseEvent(QMouseEvent* e) {
    if (!m_down) return;
    m_down = false;
    unsetCursor();
    if (m_dragging) return;
    QRectF r = targetRect();
    QPointF pos = mousePos(e);
    if (!r.contains(pos)) return;
    int x = int((pos.x() - r.x()) * m_img.width() / r.width());
    int y = int((pos.y() - r.y()) * m_img.height() / r.height());
    emit clicked(std::clamp(x, 0, m_img.width() - 1), std::clamp(y, 0, m_img.height() - 1));
}

void ImageView::mouseDoubleClickEvent(QMouseEvent*) { resetView(); }

void ImageView::wheelEvent(QWheelEvent* e) {
    if (m_img.isNull()) return;
    double steps = e->angleDelta().y() / 120.0;
    if (steps == 0) return;
    QRectF r = targetRect();
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF cur = e->position();
#else
    QPointF cur = e->posF();
#endif
    QPointF imgPt((cur.x() - r.x()) / r.width(), (cur.y() - r.y()) / r.height());  // point under the cursor
    m_zoom = std::clamp(m_zoom * std::pow(1.2, steps), 1.0, 40.0);
    double s = baseScale() * m_zoom;
    double w = m_img.width() * s, h = m_img.height() * s;
    m_center = QPointF(imgPt.x() - (cur.x() - width() / 2.0) / w, imgPt.y() - (cur.y() - height() / 2.0) / h);
    clampCenter();
    update();
    emit viewChanged(m_zoom, m_center);
    e->accept();
}
