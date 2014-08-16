#include "mainwindow.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMenuBar>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QtDebug>


class PeopleNode: public QGraphicsPixmapItem{
public:
    PeopleNode(const QString &filename){
        setFlag(ItemIsMovable);
        setFlag(ItemIsFocusable);

        QPixmap pixmap(filename);
        pixmap = pixmap.scaled(QSize(100, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        setPixmap(pixmap);

        nameItem_ = new QGraphicsTextItem(this);
        nameItem_->setPos(0, 100);

        nameItem_->setTextWidth(100);
        nameItem_->setTextInteractionFlags(Qt::TextEditorInteraction);

        QTextBlockFormat format;
        format.setAlignment(Qt::AlignHCenter);

        nameItem_->textCursor().setBlockFormat(format);
        nameItem_->textCursor().insertText(QFileInfo(filename).baseName());
    }

private:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
        QGraphicsPixmapItem::paint(painter, option, widget);

        if(hasFocus()){
            QPen pen(Qt::blue);

            painter->setPen(pen);
            painter->drawRect(boundingRect());
        }
    }

    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event){
        QGraphicsPixmapItem::mouseDoubleClickEvent(event);

        event->accept();

        QString newName = QInputDialog::getText(NULL,
                                                QObject::tr("Modify name"),
                                                QObject::tr("Please input the new name:"),
                                                QLineEdit::Normal,
                                                nameItem_->toPlainText());
        if(newName.isEmpty())
            nameItem_->setPlainText(newName);
    }

    QString filename_;
    QGraphicsTextItem *nameItem_;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(800, 600);

    QGraphicsScene *scene = new QGraphicsScene(QRectF(0, 0, 800, 600));

    QGraphicsView *view = new QGraphicsView(scene, this);

    setCentralWidget(view);

    QMenu *fileMenu = new QMenu(tr("File"));

    fileMenu->addAction(tr("Add People"), this, SLOT(addPeople()), Qt::CTRL + Qt::Key_N);

    menuBar()->addMenu(fileMenu);

    scene_ = scene;
}

void MainWindow::addPeople(){
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a people image"), QString(), tr("PNG file (*.png)"));
    if(filename.isEmpty())
        return;

    PeopleNode *people = new PeopleNode(filename);
    scene_->addItem(people);
}

MainWindow::~MainWindow()
{

}
