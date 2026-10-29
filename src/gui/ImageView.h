#pragma once
#include <QColor>
#include <QImage>
#include <QPointF>
#include <QWidget>

#include "../common/Image.h"

// Shows an image scaled to fit. Mouse wheel = zoom, drag = pan, double-click = reset.
// A click (without dragging) reports image coordinates. Several views can be linked
// (viewChanged -> setView) so zooming one zooms the others too.
class ImageView : public QWidget {
    Q_OBJECT
public:
    explicit ImageView(const QString& placeholder, QWidget* parent = nullptr);

    void setImage(const RgbImage& img);
    void clear();
    bool hasImage() const { return !m_img.isNull(); }
    void setColors(const QColor& bg, const QColor& fg);
    void setPlaceholder(const QString& t);
    void setView(double zoom, QPointF center);  // does not emit viewChanged
    void resetView();

signals:
    void clicked(int x, int y);
    void viewChanged(double zoom, QPointF center);

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    double baseScale() const;
    QRectF targetRect() const;
    void clampCenter();

    QImage m_img;
    QString m_placeholder;
    QColor m_bg{20, 20, 20}, m_fg{140, 140, 140};
    double m_zoom = 1.0;
    QPointF m_center{0.5, 0.5};  // image point (0..1) shown in the middle of the widget
    QPointF m_pressPos, m_pressCenter;
    bool m_dragging = false, m_down = false;
};
