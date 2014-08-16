#include "imagecropdialog.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsEllipseItem>
#include <QPushButton>
#include <QGraphicsSceneMouseEvent>
#include <QFileInfo>
#include <QDir>
#include <qmath.h>

#include <QtDebug>

static const qreal kCropItemRadius = 100;
static const qreal kCropItemControlPointRadius = 8;

static const QRectF MakeCircleRect(const qreal radius){
    return QRectF(-radius, -radius, radius *2, radius *2);
}

class CropItemControlPoint: public QGraphicsEllipseItem{
public:
    CropItemControlPoint():
        QGraphicsEllipseItem(MakeCircleRect(kCropItemControlPointRadius))
    {
        setBrush(Qt::white);
        setFlag(ItemIsMovable);
        setFlag(ItemSendsGeometryChanges);
    }

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value){
        if(change == ItemPositionHasChanged){
            QGraphicsEllipseItem *item = qgraphicsitem_cast<QGraphicsEllipseItem *>(parentItem());

            qreal diff = qSqrt(x()*x() + y()*y());
            item->setRect(MakeCircleRect(diff - kCropItemControlPointRadius));
        }

        return QGraphicsEllipseItem::itemChange(change, value);
    }
};

class CropItem: public QGraphicsEllipseItem{
public:
    CropItem():
        QGraphicsEllipseItem(MakeCircleRect(kCropItemRadius))
    {
        setBrush(Qt::blue);

        setOpacity(0.4);

        setPen(QPen(Qt::NoPen));
        setFlag(ItemIsMovable);

        controlPoint_ = new CropItemControlPoint;
        controlPoint_->setParentItem(this);
        controlPoint_->setPos(0, kCropItemRadius - kCropItemControlPointRadius);
    }

private:
    CropItemControlPoint *controlPoint_;
};


ImageCropDialog::ImageCropDialog(const QString &filename, QWidget *parent):
    QDialog(parent)
{
    QGraphicsScene *scene = new QGraphicsScene(QRectF(0, 0, 800, 600));

    QGraphicsPixmapItem *imgItem = new QGraphicsPixmapItem(QPixmap(filename));
    scene->addItem(imgItem);
    imgItem->setFlag(QGraphicsItem::ItemIsMovable);

    CropItem *cropItem = new CropItem;
    scene->addItem(cropItem);
    cropItem->setPos(350, 250);

    QVBoxLayout *vlayout = new QVBoxLayout;

    QGraphicsView *view = new QGraphicsView;
    view->setScene(scene);

    QHBoxLayout *hlayout = new QHBoxLayout;

    hlayout->addStretch();

    QPushButton *okButton = new QPushButton(tr("OK"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));

    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    hlayout->addWidget(okButton);
    hlayout->addWidget(cancelButton);

    vlayout->addWidget(view);
    vlayout->addLayout(hlayout);

    setLayout(vlayout);

    filename_ = filename;
    imgItem_ = imgItem;
    cropItem_ = cropItem;
}

void ImageCropDialog::accept(){
    QFileInfo info(filename_);
    QString croppedFileName = info.dir().filePath(info.baseName() + "_cropped.png");

    QRect cropRect;

    int x = -imgItem_->x() + cropItem_->x() - cropItem_->rect().width()/2;
    int y = -imgItem_->y() + cropItem_->y() - cropItem_->rect().height()/2;

    cropRect.setTopLeft(QPoint(x,y));
    cropRect.setSize(cropItem_->rect().size().toSize());

    QImage toSave(cropRect.size(), QImage::Format_ARGB32);

    toSave.fill(Qt::transparent);

    QPainter p(&toSave);

    QPainterPath path;
    path.addEllipse(QRectF(0, 0, cropRect.width(), cropRect.height()));
    p.setClipPath(path);
    p.setClipping(true);

    p.drawPixmap(0, 0, imgItem_->pixmap().copy(cropRect));

    if(toSave.save(croppedFileName))
        emit imageCropped(croppedFileName);

    QDialog::accept();
}
