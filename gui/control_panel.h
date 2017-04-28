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

signals:
    void confButtonClicked(void);

private slots:
    void on_confButton_clicked(void);

private:
    Ui::ControlPanel *ui;
};
