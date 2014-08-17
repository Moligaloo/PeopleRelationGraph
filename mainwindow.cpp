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
#include <QDomDocument>

#include <qmath.h>

static const qreal kPeopleNodeRadius = 50;
static const qreal kConnectorGap = 10;
static const qreal kArrowLength = 10;
static const qreal kArrowAngle = M_PI / 6;

class PeopleConnector;

static QList<PeopleConnector *> g_PeopleConnectors;

class PeopleConnector: public QGraphicsLineItem{
public:
    PeopleConnector():start_(NULL), end_(NULL){
        QPen pen;
        pen.setWidth(2);

        setPen(pen);

        label_ = new QGraphicsTextItem(this);
        label_->setTextInteractionFlags(Qt::TextEditorInteraction);
        label_->setPlainText(QObject::tr("Text"));
        label_->hide();

        arrow[0] = new QGraphicsLineItem;
        arrow[1] = new QGraphicsLineItem;

        arrow[0]->setParentItem(this);
        arrow[1]->setParentItem(this);
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

    QString label() const{
        return label_->toPlainText();
    }

    void setLabel(const QString &string){
        label_->setPlainText(string);
    }

    void showLabel(){
        label_->show();
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

        QPointF d = p1-p2;

        {
            static const qreal sinv = sin(kArrowAngle);
            static const qreal cosv = cos(kArrowAngle);

            QPointF vec;
            vec.setX(d.x() * cosv - d.y() * sinv);
            vec.setY(d.x() * sinv + d.y() * cosv);

            qreal ratio = qSqrt(vec.x() * vec.x() + vec.y() * vec.y())/kArrowLength;

            vec /= ratio;

            arrow[0]->setLine(QLineF(p2, p2 + vec));

            vec.setX(d.x() * cosv + d.y() * sinv);
            vec.setY(-d.x() * sinv + d.y() * cosv);
            vec /= ratio;

            arrow[1]->setLine(QLineF(p2, p2 + vec));
        }

        label_->setPos((c1+c2)/2);
    }

private:
    QGraphicsItem *start_;
    QGraphicsItem *end_;

    QGraphicsTextItem *label_;

    QGraphicsLineItem *arrow[2];
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
                connector_->showLabel();

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

    void setLabel(const QString &string){
        nameItem_->setPlainText(string);
    }

    QString filename() const{
        return filename_;
    }

    QString label() const{
        return nameItem_->toPlainText();
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


    fileMenu->addAction(tr("Open"), this, SLOT(open()), QKeySequence::Open);
    fileMenu->addAction(tr("Save"), this, SLOT(save()), QKeySequence::Save);

    fileMenu->addSeparator();

    fileMenu->addAction(tr("Add People"), this, SLOT(addPeople()), Qt::CTRL + Qt::Key_1);
    fileMenu->addAction(tr("Crop and Add People"), this, SLOT(cropAndAddPeople()), Qt::CTRL + Qt::Key_2);


    menuBar()->addMenu(fileMenu);

    scene_ = scene;
}

void MainWindow::open(){
    QString filename = QFileDialog::getOpenFileName(this,
                                                tr("Select a file to open"),
                                                QString(),
                                                tr("XML file (*.xml)"));

    if(!filename.isEmpty()){
        scene_->clear();
        g_PeopleConnectors.clear();
        g_PeopleNodes.clear();

        QDomDocument doc;

        QFile file(filename);
        if(file.open(QIODevice::ReadOnly)){
            doc.setContent(file.readAll());

            QDomElement root = doc.firstChildElement();

            QDomNodeList peopleNodes = root.firstChildElement("people-nodes").childNodes();
            for(int i=0; i<peopleNodes.count(); i++){
                QDomElement elem = peopleNodes.at(i).toElement();
                if(elem.tagName() == "people"){
                    PeopleNode *node = new PeopleNode(elem.attribute("path"));
                    node->setPos(elem.attribute("x").toDouble(), elem.attribute("y").toDouble());
                    node->setLabel(elem.attribute("label"));
                    scene_->addItem(node);
                }
            }

            QDomNodeList peopleConnectors = root.firstChildElement("connectors").childNodes();
            for(int i=0; i<peopleConnectors.count(); i++){
                QDomElement elem = peopleConnectors.at(i).toElement();
                if(elem.tagName() == "connector"){
                    PeopleConnector *connector = new PeopleConnector;
                    connector->set(
                                g_PeopleNodes[elem.attribute("start").toInt()],
                                g_PeopleNodes[elem.attribute("end").toInt()]
                            );

                    connector->setLabel(elem.attribute("label"));

                    scene_->addItem(connector);

                    connector->showLabel();
                    connector->updatePos();
                }
            }

        }
    }
}

void MainWindow::save(){
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Select a file to save"),
                                                    QString(),
                                                    tr("XML file (*.xml)")
                                                    );

    if(!filename.isEmpty()){
        saveToFile(filename);
    }
}

void MainWindow::saveToFile(const QString &filename){
    QDomDocument doc;

    QDomElement root = doc.createElement("people-relation");

    QDomElement people = doc.createElement("people-nodes");
    for(int i=0; i<g_PeopleNodes.length(); i++){
        PeopleNode *node = g_PeopleNodes[i];

        QDomElement p = doc.createElement("people");
        p.setAttribute("id", i);
        p.setAttribute("x", node->x());
        p.setAttribute("y", node->y());
        p.setAttribute("path", node->filename());
        p.setAttribute("label", node->label());

        people.appendChild(p);
    }

    QDomElement connectors = doc.createElement("connectors");
    foreach(PeopleConnector *connector, g_PeopleConnectors){
        QDomElement n = doc.createElement("connector");

        PeopleNode *start = qgraphicsitem_cast<PeopleNode *>(connector->start());
        PeopleNode *end = qgraphicsitem_cast<PeopleNode *>(connector->end());

        n.setAttribute("start", g_PeopleNodes.indexOf(start));
        n.setAttribute("end", g_PeopleNodes.indexOf(end));
        n.setAttribute("label", connector->label());

        connectors.appendChild(n);
    }

    root.appendChild(people);
    root.appendChild(connectors);

    doc.appendChild(root);

    QDomNode xmlNode = doc.createProcessingInstruction("xml","version=\"1.0\"");
    doc.insertBefore(xmlNode, root);

    QFile file(filename);
    if(file.open(QIODevice::WriteOnly)){
        QTextStream stream(&file);
        stream << doc.toString();
    }
}

void MainWindow::addPeople(){
    QStringList filenames = QFileDialog::getOpenFileNames(this,
                                                          tr("Select a people image (cropped)"),
                                                          QString(),
                                                          tr("Image files (*.jpg *.png)"));

    for(int i=0; i<filenames.length(); i++){
        PeopleNode *node = new PeopleNode(filenames[i]);
        scene_->addItem(node);

        node->setPos(i * 50, i* 50);
    }
}

void MainWindow::addPeopleFile(const QString &filename){
    PeopleNode *people = new PeopleNode(filename);
    scene_->addItem(people);
}

void MainWindow::cropAndAddPeople(){
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Select a people image (not cropped)"),
                                                    QString(),
                                                    tr("Image files (*.jpg *.png)"));
    if(filename.isEmpty())
        return;

    ImageCropDialog *dialog = new ImageCropDialog(filename,this);
    connect(dialog, SIGNAL(imageCropped(QString)), this, SLOT(addPeopleFile(QString)));

    dialog->exec();
}

MainWindow::~MainWindow()
{

}
