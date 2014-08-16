#ifndef IMAGECROPDIALOG_H
#define IMAGECROPDIALOG_H

#include <QDialog>

class QGraphicsPixmapItem;
class CropItem;

class ImageCropDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImageCropDialog(const QString &filename, QWidget *parent);

    virtual void accept();

signals:
    void imageCropped(const QString &cropped);

private:
    QString filename_;
    QGraphicsPixmapItem *imgItem_;
    CropItem *cropItem_;
};

#endif // IMAGECROPDIALOG_H
