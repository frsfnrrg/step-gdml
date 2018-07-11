#include "window.h"
#include "viewer.h"
#include "translate.h"
#include "gdmlwriter.h"
#include "util.h"
#include "helpdialog.h"

#include <QLabel>
#include <QMenu>
#include <QGridLayout>
#include <QMenuBar>
#include <QColorDialog>
#include <QSignalMapper>
#include <QStandardItemModel>
#include <QFileDialog>

#include <AIS_InteractiveObject.hxx>

#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>

#include <cstdio>

QIcon makeIcon(QColor color)
{
    QPixmap pixmap(QSize(30, 30));
    pixmap.fill(color);
    return QIcon(pixmap);
}

QIcon makeIcon(Quantity_Color color)
{
    return makeIcon(QColor::fromRgbF(color.Red(), color.Green(), color.Blue()));
}



GDMLNameValidator::GDMLNameValidator(QObject* parent,
                                     const QSet<QString>& enames) :
    QValidator(parent), names(enames)
{
}
QValidator::State GDMLNameValidator::validate(QString& text, int&) const
{
    // no quotes (breaks the naive exporter). Empty names are not liked
    if (text.contains('"')) {
        return Invalid;
    }

    if (names.contains(text) || text.isEmpty()) {
        emit((GDMLNameValidator*)this)->textIntermediate();
        return Intermediate;
    } else {
        emit((GDMLNameValidator*)this)->textAcceptable();
        return Acceptable;
    }
}

void GDMLNameValidator::fixup(QString&) const
{
}

MainWindow::MainWindow(QString openFile) :
    QMainWindow(), helpdialog(NULL)
{
    setWindowTitle("STEP to GDML");

    HelpDialog::loadConfig();

    V3d_Viewer* v = Viewer::makeViewer();
    context = new AIS_InteractiveContext(v);
    // Lights must be configured after the context is set. Why?
    v->SetLightOn(new V3d_DirectionalLight(v, V3d_Zneg, Quantity_NOC_WHITE,
                                           Standard_True));
    v->SetLightOn(new V3d_AmbientLight(v, Quantity_NOC_WHITE));


    view = new Viewer(context, this);
    connect(view, SIGNAL(selectionMightBeChanged()),
            SLOT(onViewSelectionChanged()));
    if (!openFile.isEmpty()) {
        QSignalMapper* sigmap = new QSignalMapper(this);
        sigmap->setMapping(view, openFile);
        connect(view, SIGNAL(readyToUse()), sigmap, SLOT(map()));
        connect(sigmap, SIGNAL(mapped(QString)), this, SLOT(importSTEP(QString)));
    }

    translate = new Translator(context);

    createMenus();
    createInterface();

    loadSettings();

    current_object = -1;
}

void MainWindow::loadSettings()
{
    QSettings settings;

    this->restoreGeometry(settings.value("this-geom").toByteArray());
    splitter->restoreState(settings.value("splitter-state").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent* evt)
{
    QSettings settings;
    settings.setValue("this-geom", this->saveGeometry());
    settings.setValue("splitter-state", splitter->saveState());
    HelpDialog::saveConfig();

    QMainWindow::closeEvent(evt);
}

void MainWindow::createInterface()
{
    namesList = new QListWidget();
    namesList->setAlternatingRowColors(true);
    namesList->setSortingEnabled(true);
    namesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    namesList->setSpacing(1);
    connect(namesList, SIGNAL(itemSelectionChanged()),
            SLOT(onListSelectionChanged()));
    connect(namesList, SIGNAL(currentRowChanged(int)),
            SLOT(changeCurrentObject(int)));

    objMaterial = new QComboBox();
    objMaterial->addItems(QStringList() << GdmlWriter::defaultMaterial());
    objMaterial->setEditable(true);
    connect(objMaterial, SIGNAL(currentIndexChanged(int)),
            SLOT(currentObjectUpdated()));
    QLabel* objMaterialLabel = new QLabel("Material");

    objName = new QLineEdit();
    validator = new GDMLNameValidator(this, names);
    objName->setValidator(validator);
    QSignalMapper* sigmap = new QSignalMapper(this);
    sigmap->setMapping(validator, "QLineEdit{color: red;}");
    connect(validator, SIGNAL(textIntermediate()), sigmap, SLOT(map()));
    connect(sigmap, SIGNAL(mapped(QString)), objName, SLOT(setStyleSheet(QString)));
    connect(validator, SIGNAL(textAcceptable()), SLOT(currentObjectUpdated()));
    QLabel* objNameLabel = new QLabel("Name");

    objTransparency = new QSlider(Qt::Horizontal);
    objTransparency->setRange(0, 100);
    objTransparency->setPageStep(20);
    objTransparency->setSingleStep(5);
    connect(objTransparency, SIGNAL(sliderMoved(int)),
            SLOT(currentObjectUpdated()));
    QLabel* objTransparencyLabel = new QLabel("Alpha");

    Quantity_Color qcol;
#if OCC_VERSION_HEX >= 0x070000
    qcol = context->DefaultDrawer()->Color();
#else
    qcol = Quantity_Color(context->DefaultDrawer()->Color());
#endif
    objColor = new QPushButton(makeIcon(qcol), "");
    connect(objColor, SIGNAL(clicked()), this, SLOT(getColor()));
    QLabel* objColorLabel = new QLabel("Color");

    connect(this, SIGNAL(enableObjectEditor(bool)), objMaterial,
            SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objMaterialLabel,
            SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objName,
            SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objNameLabel,
            SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objTransparency,
            SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objTransparencyLabel,
            SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objColor,
            SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objColorLabel,
            SLOT(setEnabled(bool)));

    emit enableObjectEditor(false);

    QStandardItemModel* smi = new QStandardItemModel(2, 1);
    smi->setItem(0, new QStandardItem("TEXT"));
    smi->setItem(1, new QStandardItem("LOG"));
    smi->sort(0, Qt::AscendingOrder);
    smi->setColumnCount(1);

    // Layouts

    QGridLayout* rtlayout = new QGridLayout();
    rtlayout->setContentsMargins(3, 3, 3, 3);
    rtlayout->addWidget(objNameLabel, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    rtlayout->addWidget(objName, 0, 2);

    rtlayout->addWidget(objMaterialLabel, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    rtlayout->addWidget(objMaterial, 2, 2);

    rtlayout->addWidget(objTransparencyLabel, 4, 0,
                        Qt::AlignRight | Qt::AlignVCenter);
    rtlayout->addWidget(objTransparency, 4, 2);

    rtlayout->addWidget(objColorLabel, 6, 0, Qt::AlignRight | Qt::AlignVCenter);
    rtlayout->addWidget(objColor, 6, 2);

    rtlayout->setRowMinimumHeight(1, 5);
    rtlayout->setRowMinimumHeight(3, 5);
    rtlayout->setRowMinimumHeight(5, 5);
    rtlayout->setColumnMinimumWidth(1, 5);

    QVBoxLayout* rlayout = new QVBoxLayout();
    rlayout->addLayout(rtlayout, 1);
    rlayout->addSpacing(5);
    rlayout->addWidget(new QLabel("Materials go here!"), 0);
    rlayout->addStretch(3);

    splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);

    splitter->addWidget(namesList);
    splitter->setStretchFactor(0, 1);

    splitter->addWidget(view);
    splitter->setStretchFactor(1, 20);

    QWidget* m = new QWidget();
    m->setLayout(rlayout);
    splitter->addWidget(m);
    splitter->setStretchFactor(2, 1);

    this->setCentralWidget(splitter);
}


void MainWindow::createMenus()
{
    QAction* quit = mkAction(this, "Quit", "Ctrl+Q", SLOT(close()));
    QAction* load = mkAction(this, "Load STEP file...", "Ctrl+O",
                             SLOT(raiseSTEP()));
    QAction* expo = mkAction(this, "Export GDML file", "Ctrl+E", SLOT(raiseGDML()));
    QAction* help = mkAction(this, "Help", "", SLOT(raiseHelp()));

    QMenu* fileMenu = new QMenu("File", this);
    fileMenu->addAction(load);
    fileMenu->addAction(expo);
    fileMenu->addSeparator();
    fileMenu->addAction(quit);
    this->menuBar()->addMenu(fileMenu);

    QMenu* helpMenu = new QMenu("Help", this);
    helpMenu->addAction(help);
    this->menuBar()->addMenu(helpMenu);
}

QList<QString> ensureUniqueness(const QList<QString>& input)
{
    QMap<QString, int> nameIndex;
    QVector<int> nameFreqs;
    for (int i = 0; i < input.length(); i++) {
        QString name = input[i];
        int index = nameIndex.value(name, -1);
        if (index == -1) {
            nameFreqs.append(1);
            nameIndex[name] = nameFreqs.count() - 1;
        } else {
            nameFreqs[index]++;
        }
    }

    QVector<int> nameCounter(nameFreqs);

    QList<QString> output;
    for (int i = 0; i < input.length(); i++) {
        QString name = input[i];
        int index = nameIndex[name];
        if (nameFreqs[index] > 1) {
            int len = QString::number(nameFreqs[index]).length();
            QString num = QString::number(nameCounter[index]);
            name.append(QString("_") + QString("0").repeated(len - num.length()) + num);
            nameCounter[index]--;
        }
        output.append(name);
    }
    return output;
}

void MainWindow::importSTEP(QString path)
{
    qDebug("Importing file %s", path.toUtf8().data());

    context->RemoveAll(true);
    metadata.clear();
    itemsToIndices.clear();
    objectsToIndices.clear();
    namesList->clear();
    names.clear();

    QList<QPair<QString, QColor> > objectData;
    QList<AIS_InteractiveObject*>  objects = translate->importSTEP(path,
            objectData);
    if (objects.isEmpty()) {
        qDebug("Failure");
    } else {
        qDebug("Success");
        importedName = path;
    }
    QList<QString> objectNames;
    QList<QColor> objectColors;
    for (int i = 0; i < objectData.length(); i++) {
        objectNames.append(objectData[i].first);
        objectColors.append(objectData[i].second);
    }
    objectNames = ensureUniqueness(objectNames);


    for (int i = 0; i < objects.length(); i++) {
        SolidMetadata sm;

        sm.name = objectNames[i];
        sm.item = new QListWidgetItem(sm.name);
        sm.object = objects[i];
        sm.material = "ALUMINUM";
        QColor c = objectColors[i];
        sm.color = Quantity_Color(c.redF(), c.greenF(), c.blueF(), Quantity_TOC_RGB);
        sm.transp = 0.0;

        metadata.append(sm);
        itemsToIndices[sm.item] = i;
        objectsToIndices[sm.object] = i;
        namesList->blockSignals(true);
        namesList->addItem(sm.item);
        namesList->blockSignals(false);
        names.insert(sm.name);
    }

    for (int i = 0; i < metadata.size(); i++) {
        SolidMetadata& m = metadata[i];
        context->SetColor(m.object, m.color, false);
        context->SetTransparency(m.object, m.transp, false);
    }

    view->resetView();
}

void MainWindow::exportGDML(QString path)
{
    qDebug("Exporting file %s", path.toUtf8().data());
    bool success = translate->exportGDML(path, metadata);
    qDebug("Success %c", success ? 'Y' : 'N');
}

void MainWindow::raiseSTEP()
{
    QString filters = "All Files (*.*);;Step Files (*.stp *.step)";
    QString name = QFileDialog::getOpenFileName(this, "Import STEP file",
                   QDir::currentPath(), filters);
    if (!name.isEmpty()) {
        importSTEP(name);
    }
}

void MainWindow::raiseGDML()
{
    QString filters = "GDML Files (*.gdml);;All Files (*.*)";
    QString adv_name = importedName.isEmpty() ? "output.gdml" : importedName.split(QDir::separator()).last();
    QString name = QFileDialog::getSaveFileName(this, "Export GDML file",
                                 QDir::currentPath() + QDir::separator() + adv_name, filters);
    if (!name.isEmpty()) {
        exportGDML(name);
    }
}

void MainWindow::raiseHelp()
{
    if (!helpdialog) {
        helpdialog = new HelpDialog(this);
    }
    helpdialog->show();
    helpdialog->raise();
}

void MainWindow::onViewSelectionChanged()
{
    namesList->blockSignals(true);

    namesList->clearSelection();

    QListWidgetItem* current = 0;//namesList->currentItem();
    for (context->InitSelected(); context->MoreSelected();
         context->NextSelected()) {
        int idx = objectsToIndices[&(*context->Current())];
        if (!metadata[idx].item->isSelected()) {
            current = metadata[idx].item;
        }
        metadata[idx].item->setSelected(true);
    }
    namesList->blockSignals(false);

    if (current != 0) {
        namesList->setCurrentItem(current);
    } else {
        int row = namesList->currentRow();
        if (row >= 0) {
            int idx = itemsToIndices[namesList->item(row)];
            if (!context->IsSelected(metadata[idx].object)) {
                namesList->setCurrentRow(-1);
            }
        }
    }
}
void MainWindow::onListSelectionChanged()
{
    context->ClearSelected(false);

    QList<QListWidgetItem*> items = namesList->selectedItems();
    for (int i = 0; i < items.length(); i++) {
        Handle(AIS_InteractiveObject) obj = metadata[itemsToIndices[items[i]]].object;
        context->AddOrRemoveSelected(obj, false);
    }
    context->UpdateCurrentViewer();

    if (items.length() == 0) {
        namesList->setCurrentRow(-1);
    }
}

void MainWindow::changeCurrentObject(int row)
{
    if (row < 0) {
        if (current_object >= 0) {
            emit enableObjectEditor(false);
            current_object = -1;
        }
        return;
    }

    if (current_object < 0)  {
        emit enableObjectEditor(true);
    }

    int idx = itemsToIndices[namesList->item(row)];
    if (current_object == idx) {
        return;
    }

    current_object = idx;

    SolidMetadata& meta = currentMetadata();

    names.insert(objName->text());
    names.remove(meta.name);
    validator->blockSignals(true);
    objName->setText(meta.name);
    validator->blockSignals(false);

    objTransparency->blockSignals(true);
    objTransparency->setValue((1.0 - meta.transp) * objTransparency->maximum());
    objTransparency->blockSignals(false);

    objColor->setIcon(makeIcon(meta.color));

    QString mat = meta.material;
    int mats = objMaterial->count();
    int found = 0;
    for (int i = 0; i < mats; i++) {
        if (mat == objMaterial->itemText(i)) {
            found = i;
            break;
        }
    }
    objMaterial->blockSignals(true);
    objMaterial->setCurrentIndex(found);
    objMaterial->blockSignals(false);
}

bool sortPairs(const QPair<int, QString*>& a, const QPair<int, QString*>& b)
{
    return *a.second < *b.second;
}

void MainWindow::currentObjectUpdated()
{
    objName->setStyleSheet("");
    QString next = objName->text();
    SolidMetadata& meta = currentMetadata();
    QListWidgetItem* item = meta.item;
    meta.name = next;
    item->setText(next);

    meta.material = objMaterial->currentText();

    double transp = 1.0 - ((double)objTransparency->value() /
                           (double)objTransparency->maximum());
    if (transp != meta.transp) {
        meta.transp = transp;
        context->SetTransparency(meta.object, meta.transp, true);
    }
}

void MainWindow::getColor()
{
    SolidMetadata& meta = currentMetadata();

    Quantity_Color& col = meta.color;
    QColor initial = QColor::fromRgbF(col.Red(), col.Green(), col.Blue());

    QColor next = QColorDialog::getColor(initial, this, "Select Color");
    if (!next.isValid()) {
        return;
    }
    objColor->setIcon(makeIcon(next));

    Quantity_Color nco(next.redF(), next.greenF(), next.blueF(), Quantity_TOC_RGB);
    meta.color = nco;
    context->SetColor(meta.object, nco, true);
}

SolidMetadata& MainWindow::currentMetadata()
{
    int row = namesList->currentRow();
    int idx = itemsToIndices[namesList->item(row)];
    return metadata[idx];
}

