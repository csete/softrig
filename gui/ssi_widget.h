/*
 * Signal strength indicator widget
 */
#pragma once

#include <QtWidgets>

class SsiWidget : public QFrame
{
    Q_OBJECT

public:
    explicit SsiWidget(QWidget *parent = 0);

    QSize    minimumSizeHint() const;
    QSize    sizeHint() const;

public slots:
    void     setLevel(float dbfs);

protected:
    void     paintEvent(QPaintEvent *event);
    void     resizeEvent(QResizeEvent *event);

private:
    void     drawOverlay(void);
    void     draw(void);
//    void updateOverlay(void) { drawOverlay(); }

    QPixmap    main_pixmap;    // main pixmap containing everything
    QPixmap    overlay_pixmap; // pixmap with static elements
    QSize      widget_size;
    QString    level_str;      // SSI level as string (with unit)
    int        level_i;        // SSI level as int
    int        level_pix;      // SSI level in pixels

    float      alpha_decay;
    float      alpha_rise;
};
