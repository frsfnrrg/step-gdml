#include "window.h"
#include "view.h"
#include "translate.h"
#include "util.h"
#include <cstdio>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveObject.hxx>


MainWindow::MainWindow(QString openFile) :
    QMainWindow(), gdmldialog(NULL), stepdialog(NULL)
{
    setWindowTitle("STEP to GDML");
    createMenus();

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

    namesList = new QListWidget();
    namesList->setAlternatingRowColors(true);
    namesList->setSortingEnabled(true);
    namesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(namesList, SIGNAL(itemSelectionChanged()), SLOT(onListSelectionChanged()));
    connect(namesList, SIGNAL(currentRowChanged(int)), SLOT(changeCurrentObject(int)));
    namesList->setSpacing(1);

    objMaterial = new QComboBox();
    objMaterial->addItem("ALUMINUM");
    connect(objMaterial, SIGNAL(currentIndexChanged(int)), SLOT(currentObjectUpdated()));
    objMaterialLabel = new QLabel("Material");

    objName = new QLineEdit();
    connect(objName, SIGNAL(textChanged(QString)), SLOT(currentObjectUpdated()));
    objNameLabel = new QLabel("Name");

    objMaterial->setDisabled(true);
    objName->setDisabled(true);
    objMaterialLabel->setDisabled(true);
    objNameLabel->setDisabled(true);


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

    rtlayout->setRowMinimumHeight(1, 5);
    rtlayout->setColumnMinimumWidth(1, 5);

    QVBoxLayout* rlayout = new QVBoxLayout();
    rlayout->addLayout(rtlayout, 1);
    rlayout->addSpacing(5);
    rlayout->addWidget(new QLabel("Materials go here!"), 0);
    rlayout->addStretch(3);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);
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

void MainWindow::importSTEP(QString path) {
    printf("Importing file %s\n", path.toLocal8Bit().data());

    context->RemoveAll(true);
    metadata.clear();
    itemsToIndices.clear();
    objectsToIndices.clear();
    namesList->clear();

    bool success = translate->importSTEP(path);
    printf("Success %c\n", success ? 'Y' : 'N');
    QList<AIS_InteractiveObject*> objects = Translator::getInteractiveObjects(context);

    int len = QString::number(objects.length()).length();
    for (int i=0;i<objects.length();i++) {
        SolidMetadata sm;
        QString num = QString::number(i);
        sm.name = QString("0").repeated(len - num.length()) + num;
        sm.item = new QListWidgetItem(sm.name);
        sm.object = objects[i];
        sm.material = "ALUMINUM";
        metadata.append(sm);

        itemsToIndices[sm.item] = i;
        objectsToIndices[sm.object] = i;
        namesList->addItem(sm.item);
    }

    view->fitAll();
}


void MainWindow::exportGDML(QString path) {
    printf("Exporting file %s\n", path.toLocal8Bit().data());
    bool success = translate->exportGDML(path);
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
    bool enabled = (row >= 0);
    objMaterial->setEnabled(enabled);
    objMaterialLabel->setEnabled(enabled);
    objName->setEnabled(enabled);
    objNameLabel->setEnabled(enabled);
    if (enabled) {
        int idx = itemsToIndices[namesList->item(row)];
        objName->blockSignals(true);
        objName->setText(metadata[idx].name);
        objName->blockSignals(false);

        QString mat = metadata[idx].material;
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
    } else {
        objName->setText("Solid Name");
        objMaterial->blockSignals(true);
        objMaterial->setCurrentIndex(0);
        objMaterial->blockSignals(false);
    }

}

bool sortPairs(const QPair<int, QString*>& a, const QPair<int, QString*>& b) {
    return *a.second < *b.second;
}

void MainWindow::currentObjectUpdated() {
    int row = namesList->currentRow();
    if (row == -1) {
        return;
    }

    int idx = itemsToIndices[namesList->item(row)];

    metadata[idx].name = objName->text();
    metadata[idx].material = objMaterial->currentText();

    QListWidgetItem* item = metadata[idx].item;
    item->setText(objName->text());
}
