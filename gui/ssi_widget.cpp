/*
 * Signal strength indicator widget (originally from CuteSDR / Gqrx)
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2010 Moe Wheatley AE4JY
 * Copyright 2012-2019 Alexandru Csete OZ9AEC
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <math.h>
#include <QDebug>
#include <QtWidgets>

#include "ssi_widget.h"

// ratio to total control width or height
#define CTRL_MARGIN         0.07f   // left/right margin
#define CTRL_MAJOR_START    0.35f   // top of major tic line
#define CTRL_MINOR_START    0.35f   // top of minor tic line
#define CTRL_XAXIS_HEGHT    0.45f   // vertical position of horizontal axis
#define CTRL_NEEDLE_TOP     0.44f   // vertical position of top of needle triangle

#define MIN_DB      -100.0f
#define MAX_DB         0.0f

SsiWidget::SsiWidget(QWidget *parent) : QFrame(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_PaintOnScreen, false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);

    main_pixmap = QPixmap(0, 0);
    overlay_pixmap = QPixmap(0, 0);
    widget_size = QSize(0, 0);
    level_pix = 0;
    level_i = -120;
    alpha_decay = 0.10f;    // FIXME: Should set delta-t and Fs instead
    alpha_rise = 0.5f;      // FIXME: Should set delta-t and Fs instead
}

QSize SsiWidget::minimumSizeHint() const
{
    return QSize(150, 40);
}

QSize SsiWidget::sizeHint() const
{
    return QSize(200, 60);
}

void SsiWidget::resizeEvent(QResizeEvent *)
{
    if (!size().isValid())
        return;

    if (widget_size != size())
    {
        // if size changed, resize pixmaps to new screensize
        widget_size = size();
        overlay_pixmap = QPixmap(widget_size.width(), widget_size.height());
        overlay_pixmap.fill(Qt::black);
        main_pixmap = QPixmap(widget_size.width(), widget_size.height());
        main_pixmap.fill(Qt::black);
    }

    drawOverlay();
    draw();
}

void SsiWidget::setLevel(float dbfs)
{
    if (dbfs < MIN_DB)
        dbfs = MIN_DB;
    else if (dbfs > MAX_DB)
        dbfs = MAX_DB;

    float    level = level_i;
    float    alpha = dbfs < level ? alpha_decay : alpha_rise;
    level += alpha * (dbfs - level);
    level_i = int(level);

    float    w = main_pixmap.width();
    w -= 2 * CTRL_MARGIN * w;       // width of meter scale in pixels

    // pixels / dB
    float    pixperdb = w / fabsf(MAX_DB - MIN_DB);
    level_pix = int((level - MIN_DB) * pixperdb);

    draw();
}

// Called by QT when screen needs to be redrawn
void SsiWidget::paintEvent(QPaintEvent *)
{
    QPainter    painter(this);

    painter.drawPixmap(0, 0, main_pixmap);
}

// Called to update s-meter data for displaying on the screen
void SsiWidget::draw(void)
{
    int    w;
    int    h;

    if (main_pixmap.isNull())
        return;

    // get/draw the 2D spectrum
    w = main_pixmap.width();
    h = main_pixmap.height();

    // first copy into 2Dbitmap the overlay bitmap.
    main_pixmap = overlay_pixmap.copy(0, 0, w, h);
    QPainter    painter(&main_pixmap);

    // DrawCurrent position indicator
    int     hline = h * CTRL_XAXIS_HEGHT;
    int     marg = w * CTRL_MARGIN;
    int     ht = h * CTRL_NEEDLE_TOP;
    int     x = int(marg + level_pix);
    QPoint  pts[3];
    pts[0].setX(x);
    pts[0].setY(ht + 2);
    pts[1].setX(x - 6);
    pts[1].setY(hline + 8);
    pts[2].setX(x + 6);
    pts[2].setY(hline + 8);

    painter.setBrush(QBrush(QColor(0, 230, 80, 255)));
    painter.setOpacity(1.0);

    // Qt 4.8+ has a 1-pixel error (or they fixed line drawing)
    // see http://stackoverflow.com/questions/16990326
#if QT_VERSION >= 0x040800
    painter.drawRect(marg - 1, ht + 1, x - marg, 6);
#else
    painter.drawRect(marg, ht + 2, x - marg, 6);
#endif

    // create Font to use for scales
    QFont    Font("Arial", 10);
    Font.setWeight(QFont::Normal);
    painter.setFont(Font);

    painter.setPen(QColor(0xDA, 0xDA, 0xDA, 0xFF));
    painter.setOpacity(1.0);
    level_str.setNum(level_i);
    painter.drawText(marg, h - 7, level_str + " dBFS");

    update();
}

// Called to draw an overlay bitmap containing items that
// does not need to be recreated every fft data update.
void SsiWidget::drawOverlay(void)
{
    if (overlay_pixmap.isNull())
        return;

    int         w = overlay_pixmap.width();
    int         h = overlay_pixmap.height();
    int         x;
    QRect       rect;
    QPainter    painter(&overlay_pixmap);

    overlay_pixmap.fill(QColor(0x1F, 0x1D, 0x1D, 0xFF));

    // Draw scale lines
    qreal    marg = (qreal) w * CTRL_MARGIN;
    qreal    xpos = marg;
    qreal    hline = (qreal)h * CTRL_XAXIS_HEGHT;
    qreal    magstart = (qreal) h * CTRL_MAJOR_START;
    qreal    minstart = (qreal) h * CTRL_MINOR_START;
    qreal    hstop = (qreal) w - marg;

    painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));
    painter.drawLine(QLineF(marg, hline, hstop, hline)); // top line
    painter.drawLine(QLineF(marg, hline + 8, hstop, hline + 8)); // bottom line
    for (x = 0; x < 13; x++)
    {
        if (x & 1)
            // minor tics
            painter.drawLine(QLineF(xpos, minstart, xpos, hline));
        else
            painter.drawLine(QLineF(xpos, magstart, xpos, hline));
        xpos += (hstop - marg) / 10.0;
    }

    // draw scale text
    // create Font to use for scales
    QFont    Font("Arial", 10);
    Font.setWeight(QFont::Normal);
    painter.setFont(Font);
    int    rwidth = (int)((hstop - marg) / 5.0);
    rect.setRect(marg / 2 - 8, 0, rwidth, magstart);

    for (x = MIN_DB; x <= MAX_DB; x += 20)
    {
        level_str.setNum(x);
        painter.drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, level_str);
        rect.translate(rwidth, 0);
    }
}
