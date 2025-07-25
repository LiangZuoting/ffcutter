#pragma once

#include <QOpenGLWidget>

class FCPlayer  : public QOpenGLWidget
{
    Q_OBJECT

public:
    FCPlayer(QWidget *parent);
    ~FCPlayer();

protected:
    void paintEvent(QPaintEvent* event) override;

private:

};
