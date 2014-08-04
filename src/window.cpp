#include "window.h"
#include "view.h"
#include "translate.h"
#include "util.h"
#include <cstdio>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveObject.hxx>

QIcon makeIcon(QColor color) {
    QPixmap pixmap(QSize(30,30));
    pixmap.fill(color);
    return QIcon(pixmap);
}

QIcon makeIcon(Quantity_Color color) {
    return makeIcon(QColor::fromRgbF(color.Red(), color.Green(), color.Blue()));
}



GDMLNameValidator::GDMLNameValidator(QObject* parent, const QSet<QString>& enames) :
    QValidator(parent), names(enames) {
}
QValidator::State GDMLNameValidator::validate(QString & text, int &) const {
    // no quotes (breaks the naive exporter). Empty names are not liked
    if (text.contains('"')) {
        return Invalid;
    }

    if (names.contains(text) || text.isEmpty()) {
        emit ((GDMLNameValidator*)this)->textIntermediate();
        return Intermediate;
    } else {
        emit ((GDMLNameValidator*)this)->textAcceptable();
        return Acceptable;
    }
}

void GDMLNameValidator::fixup(QString &) const {
}

MainWindow::MainWindow(QString openFile) :
    QMainWindow(), gdmldialog(NULL), stepdialog(NULL)
{
    setWindowTitle("STEP to GDML");

    Handle(V3d_Viewer) myViewer = View::makeViewer();

    myViewer->SetDefaultLights();
    myViewer->SetLightOn();

    context = new AIS_InteractiveContext(myViewer);
    view = new View(context, this);
    connect(view, SIGNAL(selectionChanged()), SLOT(onViewSelectionChanged()));
    if (!openFile.isEmpty()) {
        QSignalMapper* sigmap = new QSignalMapper(this);
        sigmap->setMapping(view, openFile);
        connect(view, SIGNAL(readyToUse()), sigmap, SLOT(map()));
        connect(sigmap, SIGNAL(mapped(QString)), this, SLOT(importSTEP(QString)));
    }
    view->setMinimumSize(QSize(150,150));
    view->setFocusPolicy(Qt::NoFocus);

    translate = new Translator(context);

    createMenus();
    createInterface();

    loadSettings();

    current_object = -1;
}

void MainWindow::loadSettings() {
    QSettings settings;

    this->restoreGeometry(settings.value("this-geom").toByteArray());
    splitter->restoreState(settings.value("splitter-state").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *evt) {
    QSettings settings;

    settings.setValue("this-geom", this->saveGeometry());
    settings.setValue("splitter-state", splitter->saveState());

    QMainWindow::closeEvent(evt);
}

void MainWindow::createInterface() {
    namesList = new QListWidget();
    namesList->setAlternatingRowColors(true);
    namesList->setSortingEnabled(true);
    namesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    namesList->setSpacing(1);
    connect(namesList, SIGNAL(itemSelectionChanged()), SLOT(onListSelectionChanged()));
    connect(namesList, SIGNAL(currentRowChanged(int)), SLOT(changeCurrentObject(int)));

    objMaterial = new QComboBox();
    objMaterial->addItems(QStringList() << "ALUMINUM" << "STEEL");
    connect(objMaterial, SIGNAL(currentIndexChanged(int)), SLOT(currentObjectUpdated()));
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
    connect(objTransparency, SIGNAL(sliderMoved(int)), SLOT(currentObjectUpdated()));
    QLabel* objTransparencyLabel = new QLabel("Alpha");

    objColor = new QPushButton(makeIcon(Quantity_Color(context->DefaultColor())), "");
    connect(objColor, SIGNAL(clicked()), this, SLOT(getColor()));
    QLabel* objColorLabel = new QLabel("Color");

    connect(this, SIGNAL(enableObjectEditor(bool)), objMaterial, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objMaterialLabel, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objName, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objNameLabel, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objTransparency, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objTransparencyLabel, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objColor, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(enableObjectEditor(bool)), objColorLabel, SLOT(setEnabled(bool)));

    emit enableObjectEditor(false);

    QStandardItemModel* smi = new QStandardItemModel(2,1);
    smi->setItem(0, new QStandardItem("TEXT"));
    smi->setItem(1, new QStandardItem("LOG"));
    smi->sort(0, Qt::AscendingOrder);
    smi->setColumnCount(1);

    // Layouts

    QGridLayout* rtlayout = new QGridLayout();
    rtlayout->setContentsMargins(3,3,3,3);
    rtlayout->addWidget(objNameLabel, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
    rtlayout->addWidget(objName, 0, 2);

    rtlayout->addWidget(objMaterialLabel, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    rtlayout->addWidget(objMaterial, 2, 2);

    rtlayout->addWidget(objTransparencyLabel, 4, 0, Qt::AlignRight | Qt::AlignVCenter);
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


void MainWindow::createMenus() {
    QAction* quit = mkAction(this, "Quit", "Ctrl+Q", SLOT(close()));

    QAction* load = mkAction(this, "Load STEP file...", "Ctrl+O", SLOT(raiseSTEP()));

    QAction* expo = mkAction(this, "Export GDML file", "Ctrl+E", SLOT(raiseGDML()));

    QMenu* fileMenu = new QMenu("File", this);
    fileMenu->addAction(load);
    fileMenu->addAction(expo);
    fileMenu->addSeparator();
    fileMenu->addAction(quit);

    this->menuBar()->addMenu(fileMenu);
}

void MainWindow::importSTEP(QString path) {
    printf("Importing file %s\n", path.toUtf8().data());

    context->RemoveAll(true);
    metadata.clear();
    itemsToIndices.clear();
    objectsToIndices.clear();
    namesList->clear();
    names.clear();

    bool success = translate->importSTEP(path);
    printf("Success %c\n", success ? 'Y' : 'N');
    QList<AIS_InteractiveObject*> objects = Translator::getInteractiveObjects(context);

    Quantity_NameOfColor color = context->DefaultColor();

    int len = QString::number(objects.length()).length();
    for (int i=0;i<objects.length();i++) {
        SolidMetadata sm;
        QString num = QString::number(i);
        sm.name = QString("0").repeated(len - num.length()) + num;
        sm.item = new QListWidgetItem(sm.name);
        sm.object = objects[i];
        sm.material = "ALUMINUM";
        sm.color = Quantity_Color(color);
        sm.transp = 0.0;

        metadata.append(sm);
        itemsToIndices[sm.item] = i;
        objectsToIndices[sm.object] = i;
        namesList->addItem(sm.item);
        names.insert(sm.name);
    }

    for (int i=0;i<metadata.size();i++) {
        SolidMetadata& m = metadata[i];
        context->SetColor(m.object, m.color, false);
        context->SetTransparency(m.object, m.transp, false);
    }

    view->fitAll();
}


void MainWindow::exportGDML(QString path) {
    printf("Exporting file %s\n", path.toUtf8().data());
    bool success = translate->exportGDML(path, metadata);
    printf("Success %c\n", success ? 'Y' : 'N');
}

void MainWindow::raiseSTEP() {
    if (!stepdialog) {
        stepdialog = new IODialog(this, QFileDialog::AcceptOpen, QStringList("Step Files (*.stp *.step)"), "stp");
        stepdialog->hook(this, SLOT(importSTEP(QString)));
    }
    stepdialog->display();
}

void MainWindow::raiseGDML() {
    if (!gdmldialog) {
        gdmldialog = new IODialog(this, QFileDialog::AcceptSave, QStringList("GDML Files (*.gdml)"), "gdml");
        gdmldialog->hook(this, SLOT(exportGDML(QString)));
    }
    gdmldialog->display();
}


void MainWindow::onViewSelectionChanged() {
    namesList->blockSignals(true);

    namesList->clearSelection();

    int current = namesList->currentRow();
    for (context->InitSelected();context->MoreSelected();context->NextSelected()) {
        int idx = objectsToIndices[&(*context->Current())];
        if (!metadata[idx].item->isSelected()) {
            current = idx;
        }
        metadata[idx].item->setSelected(true);
    }
    namesList->blockSignals(false);

    namesList->setCurrentRow(current);
}
void MainWindow::onListSelectionChanged() {
    context->ClearSelected(false);

    QList<QListWidgetItem*> items = namesList->selectedItems();
    for (int i=0;i<items.length();i++) {
        Handle(AIS_InteractiveObject) obj = metadata[itemsToIndices[items[i]]].object;
        context->AddOrRemoveSelected(obj, false);
    }
    context->UpdateCurrentViewer();
}

void MainWindow::changeCurrentObject(int row) {
    if (row < 0) {
        if (current_object >= 0) {
            emit enableObjectEditor(false);
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

    SolidMetadata &meta = currentMetadata();

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
    for (int i=0;i<mats;i++) {
        if (mat == objMaterial->itemText(i)) {
            found = i;
            break;
        }
    }
    objMaterial->blockSignals(true);
    objMaterial->setCurrentIndex(found);
    objMaterial->blockSignals(false);
}

bool sortPairs(const QPair<int, QString*>& a, const QPair<int, QString*>& b) {
    return *a.second < *b.second;
}

void MainWindow::currentObjectUpdated() {
    objName->setStyleSheet("");
    QString next = objName->text();
    SolidMetadata &meta = currentMetadata();
    QListWidgetItem* item = meta.item;
    meta.name = next;
    item->setText(next);

    meta.material = objMaterial->currentText();

    double transp = 1.0 - ((double)objTransparency->value() / (double)objTransparency->maximum());
    if (transp != meta.transp) {
        meta.transp = transp;
        context->SetTransparency(meta.object, meta.transp, true);
    }
}

void MainWindow::getColor() {
    SolidMetadata &meta = currentMetadata();

    Quantity_Color& col = meta.color;
    QColor initial = QColor::fromRgbF(col.Red(),col.Green(),col.Blue());

    QColor next = QColorDialog::getColor(initial, this, "Select Color");
    objColor->setIcon(makeIcon(next));

    Quantity_Color nco(next.redF(), next.greenF(), next.blueF(), Quantity_TOC_RGB);
    meta.color = nco;
    context->SetColor(meta.object, nco, true);
}

SolidMetadata& MainWindow::currentMetadata() {
    int row = namesList->currentRow();
    int idx = itemsToIndices[namesList->item(row)];
    return metadata[idx];
}

