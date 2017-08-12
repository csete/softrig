/*
 * Main control panel
 */
#pragma once

#include <QWidget>

namespace Ui {
    class ControlPanel;
}

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = 0);
    ~ControlPanel();

private:
    Ui::ControlPanel   *ui;
};
