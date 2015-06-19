#include "translate.h"
#include "gdmlwriter.h"

#include <QSet>
#include <QColor>

#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_ListOfInteractive.hxx>

#include <TopoDS_Shape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_HSequenceOfShape.hxx>

#include <STEPCAFControl_Reader.hxx>

#include <TDocStd_Document.hxx>

#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>

#include <TDF_LabelSequence.hxx>
#include <TDataStd_Name.hxx>


//
//
//                      IODIALOG
//
//

// Eventually: do lazy-loading of the dialog internally, so users need not
// define a custom slot just to create this on demand.
IODialog::IODialog(QWidget* parent, QFileDialog::AcceptMode mode,
                   QStringList filters, QString suffix) :
    QObject(parent), fd(new QFileDialog(parent))
{
    fd->setDirectory(QDir::current());
    fd->setAcceptMode(mode);
    fd->setModal(true);
    fd->setConfirmOverwrite(true);
    // setDefaultSuffix(); // based on filter
    if (mode == QFileDialog::AcceptOpen) {
        fd->setFileMode(QFileDialog::ExistingFile);
        fd->setWindowTitle("Import File");
        filters.prepend("Any files (*)");
    } else {
        fd->selectFile(QDir::currentPath() + QDir::separator() + "output." + suffix);
        fd->setFileMode(QFileDialog::AnyFile);
        fd->setWindowTitle("Export File");
        filters.append("Any files (*)");
    }
    fd->setNameFilters(filters);
    fd->setDefaultSuffix(suffix);
    fd->setNameFilterDetailsVisible(true);

    connect(fd, SIGNAL(accepted()), SLOT(onComplete()));
}

void IODialog::hook(QObject* target, const char* slot)
{
    connect(this, SIGNAL(chosen(QString)), target, slot);
}

void IODialog::onComplete()
{
    QStringList files = fd->selectedFiles();
    if (files.length() != 1) {
        return;
    }
    QString filename = QDir(files[0]).absolutePath();

    emit chosen(filename);
}

void IODialog::display()
{
    fd->show();
    fd->raise();
}

//
//
//                      TRANSLATOR
//
//

Translator::Translator(const Handle(AIS_InteractiveContext) theContext) :
    context(theContext)
{
}

QList<AIS_InteractiveObject*> Translator::displayShapes(const Handle(
            AIS_InteractiveContext)& theContext,
        const Handle(TopTools_HSequenceOfShape) & shapes)
{
    QList<AIS_InteractiveObject*> objs;
    if (shapes.IsNull() || shapes->IsEmpty()) {
        qWarning("Incoming list of shapes is empty");
        return objs;
    }

    for (int i = 1; i <= shapes->Length(); i++) {
        AIS_Shape* e = new AIS_Shape(shapes->Value(i));
        e->SetDisplayMode(AIS_Shaded);

        Graphic3d_MaterialAspect mat;
        mat.SetTransparency(0.0);
        mat.SetShininess(0.003);
        mat.SetEnvReflexion(0.003);

        mat.SetAmbient(0.5);
        mat.SetAmbientColor(Quantity_NOC_WHITE);
        mat.SetReflectionModeOn(Graphic3d_TOR_AMBIENT);

        mat.SetDiffuse(0.65);
        mat.SetDiffuseColor(Quantity_NOC_WHITE);
        mat.SetReflectionModeOn(Graphic3d_TOR_DIFFUSE);

        mat.SetSpecular(0.01);
        mat.SetSpecularColor(Quantity_NOC_WHITE);
        mat.SetReflectionModeOn(Graphic3d_TOR_SPECULAR);

        mat.SetEmissive(0.0);
        mat.SetEmissiveColor(Quantity_NOC_WHITE);
        mat.SetReflectionModeOn(Graphic3d_TOR_EMISSION);

        mat.SetMaterialName("Magic Material");
        mat.SetMaterialType(Graphic3d_MATERIAL_ASPECT);

        e->SetMaterial(mat);

        theContext->Display(e, false);
        objs.append(e);
    }

    // update all viewers
    theContext->UpdateCurrentViewer();
    return objs;
}

bool Translator::findAllShapes(const Handle(AIS_InteractiveContext)& ctxt,
                               const Handle(TopTools_HSequenceOfShape) & shapes)
{
    if (shapes.IsNull()) {
        qWarning("Incoming shape list was null.");
        return false;
    }

    AIS_ListOfInteractive objs;
    ctxt->EraseAll(false);
    ctxt->ErasedObjects(objs);
    ctxt->DisplayAll(false);
    while (!objs.IsEmpty()) {
        Handle(AIS_InteractiveObject) obj = objs.First();
        if (obj->IsKind(STANDARD_TYPE(AIS_Shape))) {
            TopoDS_Shape shape = Handle(AIS_Shape)::DownCast(obj)->Shape();
            shapes->Append(shape);
        }
        objs.RemoveFirst();
    }

    return !shapes->IsEmpty();
}

QList<AIS_InteractiveObject*> Translator::getInteractiveObjects(const Handle(
            AIS_InteractiveContext)& ctxt)
{
    QList<AIS_InteractiveObject*> qll;
    AIS_ListOfInteractive objs;
    ctxt->EraseAll(false);
    ctxt->ErasedObjects(objs);
    ctxt->DisplayAll(false);
    while (!objs.IsEmpty()) {
        Handle(AIS_InteractiveObject) obj = objs.First();
        if (obj->IsKind(STANDARD_TYPE(AIS_Shape))) {
            qll.append(&(*obj));
        } else {
            qDebug("ERROR\n");
        }
        objs.RemoveFirst();
    }
    return qll;
}

QList<AIS_InteractiveObject*> Translator::importSTEP(QString path,
        QList<QPair<QString, QColor> >& objNames)
{
    Handle(TopTools_HSequenceOfShape) shapes = new TopTools_HSequenceOfShape();
    if (!importSTEP(path, shapes, objNames)) {
        qWarning("STEP Import failed\n");
        return QList<AIS_InteractiveObject*>();
    }
    return displayShapes(context, shapes);
}

bool Translator::exportGDML(QString path,
                            const QVector<SolidMetadata>& metadata)
{
    Handle(TopTools_HSequenceOfShape) shapes = new TopTools_HSequenceOfShape();
    // add all dem shapes

    if (!findAllShapes(context, shapes)) {
        return false;
    }

    return exportGDML(path, shapes, metadata);
}


QString getName(const TDF_Label& label)
{
    Handle(TDataStd_Name) name = new TDataStd_Name();
    bool found = label.FindAttribute(name->GetID(), name);
    if (!found) {
        return QString("???");
    } else {
        char chars[name->Get().LengthOfCString()];
        char* x = chars;
        name->Get().ToUTF8CString(x);
        return QString(x);
    }
}

uint qHash(const QColor& color)
{
    return qHash(color.name());
}

QColor getColor(const TopoDS_Shape& shape,
                const Handle(XCAFDoc_ColorTool)& tool)
{
    bool success;
    Quantity_Color color;
    // From observation (no research yet)
    // ColorSurf/ColorCurv can apply to both faces and objects.
    // ColorGen has yet to appear.
    // When ColorCurv is present, so is ColorSurf.
    //

    success = tool->GetColor(shape, XCAFDoc_ColorSurf, color);
    if (success) {
        return QColor::fromRgbF(color.Red(), color.Green(), color.Blue());
    }
    success = tool->GetColor(shape, XCAFDoc_ColorCurv, color);
    if (success) {
        return QColor::fromRgbF(color.Red(), color.Green(), color.Blue());
    }
    success = tool->GetColor(shape, XCAFDoc_ColorGen, color);
    if (success) {
        return QColor::fromRgbF(color.Red(), color.Green(), color.Blue());
    }
    return QColor();
}

int countSubshapes(const TopoDS_Shape& s, TopAbs_ShapeEnum type)
{
    TopExp_Explorer exp(s, type);
    int i = 0;
    while (exp.More()) {
        exp.Next();
        i++;
    }
    return i;
}

QPair<QString, QColor> handleShapeMetadata(const TopoDS_Shape& shape,
        const Handle(XCAFDoc_ColorTool)& colorTool,
        const Handle(XCAFDoc_ShapeTool)& shapeTool)
{
    QColor result = getColor(shape, colorTool);

    if (!result.isValid()) {
        // If there is no applied color, grab the color from the object.
        QSet<QColor> cols;
        for (TopExp_Explorer fcxp(shape, TopAbs_FACE); fcxp.More(); fcxp.Next())  {
            TopoDS_Shape face = fcxp.Current();
            QColor faceColor = getColor(face, colorTool);
            if (faceColor.isValid()) {
                cols.insert(faceColor);
            }
        }
        if (cols.size()) {
            int r = 0, g = 0, b = 0;
            for (QSet<QColor>::const_iterator c = cols.begin(); c != cols.end(); c++) {
                const QColor& t = *c;
                r += t.red(), g += t.green(), b += t.blue();
            }

            result = QColor(r / cols.size(), g / cols.size(), b / cols.size());

            if (cols.size() > 1) {
                qDebug("Note: Multiple colors merged");
            }
        }
    }

    if (!result.isValid()) {
        result = QColor::fromRgbF(0.5, 0.5, 0.5);
    }

    TDF_Label loc;
    bool found = shapeTool->Search(shape, loc);
    if (found) {
        return QPair<QString, QColor>(getName(loc), result);
    } else {
        return QPair<QString, QColor>("?", result);
    }
}

bool Translator::importSTEP(QString file,
                            const Handle(TopTools_HSequenceOfShape)& shapes,
                            QList<QPair<QString, QColor> >& objData)
{
    if (shapes.IsNull()) {
        qWarning("Shape list was null. Aborting export.");
        return false;
    }

    STEPCAFControl_Reader reader;
    reader.SetColorMode(true);
    reader.SetNameMode(true);
    reader.SetMatMode(true);

    qDebug("Reading begun.");
    IFSelect_ReturnStatus status = reader.ReadFile((Standard_CString)
                                   file.toUtf8().constData());
    qDebug("Reading complete.");
    switch (status) {
    case IFSelect_RetVoid:
        qWarning("Read status was VOID.");
        return false;
    case IFSelect_RetError:
        qWarning("Read status was ERROR.");
        return false;
    case IFSelect_RetFail:
        qWarning("Read status was FAIL.");
        return false;
    case IFSelect_RetStop:
        qWarning("Read status was STOP.");
        return false;
    case IFSelect_RetDone:
        break;
    }

    Handle(TDocStd_Document) doc = new TDocStd_Document("XmlXCAF");
    qDebug("Transfer begun.");
    bool ok = reader.Transfer(doc);
    qDebug("Transfer complete.");
    if (!ok) {
        qWarning("Transfer wasn't ok. Aborting.");
        return false;
    }

    TDF_Label mainLabel = doc->Main();
    Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(
            mainLabel);
    Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(
            mainLabel);

    TDF_LabelSequence labels;
    shapeTool->GetFreeShapes(labels);
    if (labels.IsEmpty()) {
        qWarning("There are no free shapes");
        return false;
    }

    for (int i = 1; i <= labels.Length(); i++) {
        TopoDS_Shape tds = shapeTool->GetShape(labels.Value(i));
        bool found = false;
        for (TopExp_Explorer exp(tds, TopAbs_SOLID); exp.More(); exp.Next()) {
            TopoDS_Shape solid = exp.Current();
            shapes->Append(solid);
            objData.append(handleShapeMetadata(solid, colorTool, shapeTool));
            found = true;
        }
        if (!found) {
            for (TopExp_Explorer exp(tds, TopAbs_SHELL); exp.More(); exp.Next()) {
                TopoDS_Shape solid = exp.Current();
                objData.append(handleShapeMetadata(solid, colorTool, shapeTool));
                shapes->Append(solid);
                found = true;
            }
            if (found) {
                // TODO: create a "WARNING" field/list, that can be checked postop,
                // and raised by the window.
                // Should we even allow standalone shells?
                qCritical("Could not find any dependent solids. Instead, added shells. Output geometry may not be closed.");
            } else {
                qWarning("No dependent solids or shells found for shape.");
            }
        }
    }

    return true;
}


bool Translator::exportGDML(QString path,
                            const Handle(TopTools_HSequenceOfShape)& shapes,
                            const QVector<SolidMetadata>& metadata)
{
    if (shapes.IsNull() || shapes->IsEmpty()) {
        qWarning("Shape list was null or empty. Aborting export.");
        return false;
    }

    for (int i = 1; i <= shapes->Length(); i++) {
        TopoDS_Shape shape = shapes->Value(i);
        if (shape.IsNull()) {
            qWarning("Shape was null. Aborting export.");
            return false;
        }
    }

#if HEAP_ALLOC_ALL_THE_THINGS
    // Why is this heap-allocated? Ask Cthulhu. Stuff gets corrupted otherwise. ;-(
    gdmlWriter = new GdmlWriter(path);
    gdmlWriter->writeIntro();
    for (int i = 1; i <= shapes->Length(); i++) {
        SolidMetadata& meta = metadata[i - 1];
        gdmlWriter->addSolid(shapes->Value(i), meta.name, meta.material);
    }
    gdmlWriter->writeExtro();
    delete gdmlWriter;
#else
    {
        GdmlWriter writer(path);
        writer.writeIntro();
        for (int i = 1; i <= shapes->Length(); i++) {
            const SolidMetadata& meta = metadata[i - 1];
            writer.addSolid(shapes->Value(i), meta.name, meta.material);
        }
        writer.writeExtro();
    }
#endif
    return true;
}
