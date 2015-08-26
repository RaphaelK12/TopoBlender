#include "Document.h"
#include "Model.h"
#include "Viewer.h"

#include <QProgressBar>
#include <QTimer>
#include <QThread>

#include "DocumentAnalyzeWorker.h"

Document::Document(QObject *parent) : QObject(parent)
{

}

bool Document::loadModel(QString filename)
{
    if(!QFileInfo(filename).exists()) return false;

    auto model = QSharedPointer<Model>(new Model());
    bool isLoaded = model->loadFromFile(filename);
    models.push_front(model);
    return isLoaded;
}

void Document::createModel(QString modelName)
{
    auto model = QSharedPointer<Model>(new Model());
    model->loadFromFile(modelName);
    models << model;
}

void Document::saveModel(QString modelName, QString filename)
{
    auto m = getModel(modelName);
    if (m != nullptr){
        Structure::ShapeGraph * model = m;
        if(filename.size() < 3)
            filename = model->property.contains("name") ? model->property["name"].toString() : QString("%1.xml").arg(modelName);
        m->saveToFile(filename);
    }
}

void Document::clearModels()
{
    models.clear();
}

bool Document::loadDataset(QString datasetFolder)
{
    if(datasetFolder.trimmed().size() == 0) return false;

    // Store dataset path
    datasetPath = datasetFolder;

    // Load shapes in dataset
    {
        QDir datasetDir(datasetPath);
        QStringList subdirs = datasetDir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

        for(auto subdir : subdirs)
        {
            // Special folders
            if (subdir == "corr") continue;

            QDir d(datasetPath + "/" + subdir);

            // Check if no graph is in this folder
            if (d.entryList(QStringList() << "*.xml", QDir::Files).isEmpty()) continue;

            auto xml_files = d.entryList(QStringList() << "*.xml", QDir::Files);

            if(xml_files.empty()) continue;

            dataset[subdir]["Name"] = subdir;
            dataset[subdir]["graphFile"] = d.absolutePath() + "/" + (xml_files.front());
            dataset[subdir]["thumbFile"] = d.absolutePath() + "/" + d.entryList(QStringList() << "*.png", QDir::Files).join("");
            dataset[subdir]["objFile"] = d.absolutePath() + "/" + d.entryList(QStringList() << "*.obj", QDir::Files).join("");
        }
    }

    // Load categories
    if(!dataset.empty())
    {
        QFile file( datasetPath + "/categories.txt" );
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&file);
            while (!in.atEnd()){
                QStringList line = in.readLine().split("|");
                if(line.size() < 2) continue;

                QString catName = line.front().trimmed();
                QStringList catElements = line.back().split(QRegExp("[ \t]"), QString::SkipEmptyParts);

                categories[catName] = catElements;
            }

			if (!categories.empty())
				currentCategory = categories.keys().front();
        }
    }

    return !dataset.empty();
}

QString Document::categoryOf(QString modelName)
{
    QString result;

    auto m = getModel(modelName);
    if(m == nullptr) return result;

    QString modelCategoryName = QFileInfo(((Structure::ShapeGraph*)m)->property["name"].toString()).dir().dirName();

    for(auto cat : categories.keys()){
        for(auto mname : categories[cat].toStringList()){
            if(mname == modelCategoryName)
                return cat;
        }
    }

    return result;
}

void Document::analyze(QString categoryName)
{
    if(!categories.keys().contains(categoryName)) return;

    // Create progress bar
    auto bar = new QProgressBar();
    bar->setMaximum(99);
    bar->setValue(0);
    bar->show();

    auto worker = new DocumentAnalyzeWorker(this);

    QThread* thread = new QThread;
    worker->moveToThread(thread);
    connect(thread, SIGNAL (started()), worker, SLOT (process()));
    connect(worker, SIGNAL (finished()), thread, SLOT (quit()));
    connect(worker, SIGNAL (finished()), bar, SLOT (deleteLater()));
    connect(worker, SIGNAL (finished()), worker, SLOT (deleteLater()));
    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
    thread->start();

    // Connect progress bar to worker
    bar->connect(worker, SIGNAL(progress(int)), SLOT(setValue(int)));

}

void Document::saveDatasetCorr(QString filename)
{
	auto modelName = firstModelName();
	QString matching_file = datasetPath + "/" + modelName + "_matches.txt";

	QFile file(matching_file);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

	QTextStream out(&file);
	
	for (auto p : datasetCorr.keys())
		for (auto t : datasetCorr[p].keys())
			for (auto q : datasetCorr[p][t])
				out << p << " " << t << " " << q << "\n";
}

void Document::loadDatasetCorr(QString filename)
{
	auto modelName = firstModelName();
	QString matching_file = datasetPath + "/" + modelName + "_matches.txt";

	QFile file(matching_file);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	QTextStream in(&file);
	auto lines = in.readAll().split(QRegExp("[\r\n]"), QString::SkipEmptyParts);

	for (auto line : lines){
		auto item = line.split(QRegExp("[ \t]"), QString::SkipEmptyParts);
		if (item.size() != 3) continue;
		datasetCorr[item[0]][item[1]].push_back(item[2]);
	}
}

void Document::drawModel(QString name, QWidget *widget)
{
    auto glwidget = (Viewer*)widget;
    if (glwidget == nullptr) return;

    auto m = getModel(name);
    if(m != nullptr) m->draw(glwidget);
}

void Document::createCurveFromPoints(QString modelName, QVector<QVector3D> & points)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->createCurveFromPoints(points);
}

void Document::createSheetFromPoints(QString modelName, QVector<QVector3D> & points)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->createSheetFromPoints(points);
}

void Document::duplicateActiveNodeViz(QString modelName, QString duplicationOp)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->duplicateActiveNodeViz(duplicationOp);
}

void Document::duplicateActiveNode(QString modelName, QString duplicationOp)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->duplicateActiveNode(duplicationOp);
}

void Document::modifyActiveNode(QString modelName, QVector<QVector3D> &guidePoints)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->modifyLastAdded(guidePoints);
}

void Document::generateSurface(QString modelName, double offset)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->generateSurface(offset);
}

void Document::placeOnGround(QString modelName)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->placeOnGround();
}

void Document::selectPart(QString modelName, QVector3D rayOrigin, QVector3D rayDirection)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->selectPart(rayOrigin, rayDirection);
}

void Document::deselectAll(QString modelName)
{
    auto m = getModel(modelName);
    if(m != nullptr) m->deselectAll();
}

QVector3D Document::centerActiveNode(QString modelName)
{
    auto m = getModel(modelName);
    if(m == nullptr) return QVector3D(0,0,0);

    auto c = m->activeNode->center();
    return QVector3D(c[0],c[1],c[2]);
}

void Document::removeActiveNode(QString modelName)
{
    auto m = getModel(modelName);
    if(m == nullptr) return;

    QString nid = m->activeNode->id;
    m->removeNode(nid);
    m->activeNode = nullptr;
}

QString Document::firstModelName()
{
    if(models.isEmpty()) return "";
    return models.front()->name();
}

Model* Document::getModel(QString name)
{
    for(auto m : models) if(m->name() == name) return m.data();
    return nullptr;
}

Model* Document::cacheModel(QString name)
{
    if(cachedModels.contains(name)){
        return cachedModels[name].data();
    }else{
        if(!dataset.contains(name)) return nullptr;
        QString filename = dataset[name]["graphFile"].toString();

        auto model = QSharedPointer<Model>(new Model());
        if(!model->loadFromFile(filename)) return nullptr;
        cachedModels[name] = model;
        return model.data();
    }
}

Structure::ShapeGraph * Document::cloneAsShapeGraph(Model * m)
{
	return m->cloneAsShapeGraph();
}
