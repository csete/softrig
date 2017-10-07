/*
 * FFT widget with panadapter and waterfall
 */
#pragma once

#include <QtWidgets>

class FftWidget : public QFrame
{
    Q_OBJECT

public:
    explicit FftWidget(QWidget *parent = 0);

    QSize    minimumSizeHint() const;
    QSize    sizeHint() const;

protected:
    void     resizeEvent(QResizeEvent *);

private:
    QSize    widget_size;
};
