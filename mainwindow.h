#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QGraphicsScene;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void addPeople();
    void cropAndAddPeople();
    void addPeopleFile(const QString &filename);

private:
    QGraphicsScene *scene_;
};

#endif // MAINWINDOW_H
