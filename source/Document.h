#pragma once

#include <QObject>
#include <QVector>
#include <QSharedPointer>

class Model;

class Document : public QObject
{
    Q_OBJECT
public:
    explicit Document(QObject *parent = 0);

    // IO:
    bool loadModel(QString filename);
    void createModel(QString modelName);
    void saveModel(QString modelName, QString filename = "");
    void clearModels();

    // Visualization:
    void drawModel(QString modelName, QWidget * widget);

    // Append to models:
    void createCurveFromPoints(QString modelName, QVector<QVector3D> &points);
    void createSheetFromPoints(QString modelName, QVector<QVector3D> &points);

    void duplicateActiveNodeViz(QString modelName, QString duplicationOp);
    void duplicateActiveNode(QString modelName, QString duplicationOp);

    // Modify models:
    void modifyActiveNode(QString modelName, QVector<QVector3D> &guidePoints);
    void generateSurface(QString modelName, double offset);
    void placeOnGround(QString modelName);
    void selectPart(QString modelName, QVector3D rayOrigin, QVector3D rayDirection);

    // Stats:
    QString firstModelName();

	// Direct access to models
	Model * getModel(QString name);

protected:
    QVector< QSharedPointer<Model> > models;

signals:

public slots:
};

