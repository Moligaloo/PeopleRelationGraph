#include "mainwindow.h"
#include "imagecropdialog.h"

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

        filename_ = filename;

        nameItem_ = new QGraphicsTextItem(this);
        nameItem_->setPos(0, 100);

        nameItem_->setTextWidth(100);
        nameItem_->setTextInteractionFlags(Qt::TextEditorInteraction);

        QTextBlockFormat format;
        format.setAlignment(Qt::AlignHCenter);

        nameItem_->textCursor().setBlockFormat(format);

        QString basename = QFileInfo(filename).baseName();
        if(basename.endsWith("_cropped"))
            basename.remove("_cropped");

        nameItem_->textCursor().insertText(basename);
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

    fileMenu->addAction(tr("Add People"), this, SLOT(addPeople()), Qt::CTRL + Qt::Key_1);
    fileMenu->addAction(tr("Crop and Add People"), this, SLOT(cropAndAddPeople()), Qt::CTRL + Qt::Key_2);

    menuBar()->addMenu(fileMenu);

    scene_ = scene;
}

void MainWindow::addPeople(){
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a people image (cropped)"), QString(), tr("Image files (*.jpg *.png)"));
    if(filename.isEmpty())
        return;

    addPeopleFile(filename);
}

void MainWindow::addPeopleFile(const QString &filename){
    PeopleNode *people = new PeopleNode(filename);
    scene_->addItem(people);
}

void MainWindow::cropAndAddPeople(){
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a people image (not cropped)"), QString(), tr("Image files (*.jpg *.png)"));
    if(filename.isEmpty())
        return;

    ImageCropDialog *dialog = new ImageCropDialog(filename,this);
    connect(dialog, SIGNAL(imageCropped(QString)), this, SLOT(addPeopleFile(QString)));

    dialog->exec();
}

MainWindow::~MainWindow()
{

}
