#ifndef CLICKABLESLIDER_H
#define CLICKABLESLIDER_H

#include <QSlider>
#include <QMouseEvent>

class ClickableSlider : public QSlider
{
    Q_OBJECT

public:
    explicit ClickableSlider(QWidget *parent = nullptr) : QSlider(parent) {}
    explicit ClickableSlider(Qt::Orientation orientation, QWidget *parent = nullptr) : QSlider(orientation, parent) {}

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            int value = minimum() + ((maximum() - minimum()) * event->pos().x()) / width();
            setValue(value);
            event->accept();
        }
        QSlider::mousePressEvent(event);
    }
};

#endif // CLICKABLESLIDER_H