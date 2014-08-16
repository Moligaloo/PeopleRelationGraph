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

#include <qmath.h>

static const qreal kPeopleNodeRadius = 50;
static const qreal kConnectorGap = 10;

class PeopleConnector;

static QList<PeopleConnector *> g_PeopleConnectors;

class PeopleConnector: public QGraphicsLineItem{
public:
    PeopleConnector():start_(NULL), end_(NULL){
        QPen pen;
        pen.setWidth(2);

        setPen(pen);
    }

    void set(QGraphicsItem *start, QGraphicsItem *end){
        start_ = start;
        end_ = end;
    }

    QGraphicsItem *start() const{
        return start_;
    }

    QGraphicsItem *end() const{
        return end_;
    }

    void updatePos(){
        QPointF c1 = start_->pos() + QPoint(kPeopleNodeRadius, kPeopleNodeRadius);
        QPointF c2 = end_->pos() + QPoint(kPeopleNodeRadius, kPeopleNodeRadius);

        QPointF delta = c2-c1;
        qreal deltaLength = QLineF(c1, c2).length();

        qreal ratio = (kPeopleNodeRadius + kConnectorGap) / deltaLength;

        QPointF p1 = c1 + delta * ratio;
        QPointF p2 = c2 - delta * ratio;

        QLineF line;
        line.setP1(p1);
        line.setP2(p2);

        setLine(line);
    }

private:
    QGraphicsItem *start_;
    QGraphicsItem *end_;
};

QGraphicsItem *FindPeopleNodeUnderMouse();

class PeopleConnectControl: public QGraphicsEllipseItem{
public:
    PeopleConnectControl():QGraphicsEllipseItem(QRectF(-10, -10, 20, 20)){
        setOpacity(0.5);
        setBrush(QBrush(Qt::white));
        connector_ = NULL;
    }

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event){
        QGraphicsEllipseItem::mousePressEvent(event);

        event->accept();

        connector_ = new PeopleConnector;
        scene()->addItem(connector_);
    }

    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event){
        QGraphicsEllipseItem::mouseMoveEvent(event);

        if(connector_){
            QLineF line;
            line.setP1(scenePos());
            line.setP2(event->scenePos());

            connector_->setLine(line);
        }
    }

    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
        QGraphicsEllipseItem::mouseReleaseEvent(event);

        if(connector_){
            QGraphicsItem *end = FindPeopleNodeUnderMouse();

            if(end){
                connector_->set(parentItem(), end);
                g_PeopleConnectors << connector_;

                connector_->updatePos();


            }else
                delete connector_;

            connector_ = NULL;
        }
    }

private:
    PeopleConnector *connector_;
};

class PeopleNode;

static QList<PeopleNode *> g_PeopleNodes;

class PeopleNode: public QGraphicsPixmapItem{
public:
    PeopleNode(const QString &filename){
        setFlag(ItemIsMovable);
        setFlag(ItemIsFocusable);
        setFlag(ItemSendsGeometryChanges);

        QPixmap pixmap(filename);
        pixmap = pixmap.scaled(QSize(kPeopleNodeRadius * 2, kPeopleNodeRadius * 2), Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

        connectCtrl_ = new PeopleConnectControl;
        connectCtrl_->setParentItem(this);
        connectCtrl_->setPos(kPeopleNodeRadius, kPeopleNodeRadius);
        connectCtrl_->hide();

        g_PeopleNodes << this;
    }

protected:
    virtual void focusInEvent(QFocusEvent *event){
        QGraphicsPixmapItem::focusInEvent(event);
        connectCtrl_->show();
    }

    virtual void focusOutEvent(QFocusEvent *event){
        QGraphicsPixmapItem::focusOutEvent(event);
        connectCtrl_->hide();
    }

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value){
        if(change == ItemPositionHasChanged){
            foreach(PeopleConnector *connector, g_PeopleConnectors){
                if(connector->start() == this || connector->end() == this)
                    connector->updatePos();
            }
        }

        return QGraphicsPixmapItem::itemChange(change, value);
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
    PeopleConnectControl *connectCtrl_;
};

QGraphicsItem *FindPeopleNodeUnderMouse(){
    foreach(PeopleNode *node, g_PeopleNodes){
        if(node->isUnderMouse())
            return node;
    }

    return NULL;
}

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
