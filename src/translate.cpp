#include "translate.h"

#include "gdmlwriter.h"

#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_DisplayMode.hxx>
#include <AIS_ListOfInteractive.hxx>

#include <STEPControl_Reader.hxx>

#include <TopoDS_Shape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_HSequenceOfShape.hxx>

//
//
//                      IODIALOG
//
//

// Eventually: do lazy-loading of the dialog internally, so users need not
// define a custom slot just to create this on demand.
IODialog::IODialog(QWidget* parent, QFileDialog::AcceptMode mode, QStringList filters, QString suffix) :
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

bool Translator::displayShapes(const Handle(AIS_InteractiveContext)& theContext, const Handle(TopTools_HSequenceOfShape) & shapes)
{
    if (shapes.IsNull() || shapes->IsEmpty()) {
        return false;
    }

    for (int i = 1; i <= shapes->Length(); i++) {
        AIS_Shape* e = new AIS_Shape(shapes->Value(i));
        e->SetDisplayMode(AIS_Shaded);

        theContext->Display(e, false);
    }

    // update all viewers
    theContext->UpdateCurrentViewer();
    return true;
}

bool Translator::findAllShapes(const Handle(AIS_InteractiveContext)& ctxt, const Handle(TopTools_HSequenceOfShape) & shapes)
{
    if (shapes.IsNull()) {
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

QList<AIS_InteractiveObject*> Translator::getInteractiveObjects(const Handle(AIS_InteractiveContext)& ctxt)
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
            printf("ERROR\n");
        }
        objs.RemoveFirst();
    }
    return qll;
}

bool Translator::importSTEP(QString path)
{
    Handle(TopTools_HSequenceOfShape) shapes = new TopTools_HSequenceOfShape();
    if (!importSTEP(path, shapes)) {
        return false;
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


#include <STEPCAFControl_Reader.hxx>
#include <TDocStd_Application.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_Attribute.hxx>
#include <XSDRAW.hxx>
#include <TDataStd_Name.hxx>

bool importSTEPCAF(QString file) {
#if 1
    return !file.isEmpty();
#else
    STEPCAFControl_Reader reader;
    reader.SetColorMode(true);
    reader.SetNameMode(true);
    reader.SetMatMode(true);
    IFSelect_ReturnStatus status = reader.ReadFile((Standard_CString)file.toUtf8().constData());
    if (status != IFSelect_RetDone) {
        return false;
    }
    Handle(TDocStd_Document) doc = new TDocStd_Document("XmlXCAF");
    bool ok = reader.Transfer(doc);
    if (!ok) {
        return false;
    }

    Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    TDF_LabelSequence seq;
    shapeTool->GetFreeShapes (seq); // vs GetShapes
    qDebug("%d\n", seq.Length());
    for (int i=1;i<=seq.Length();i++) {
        TDF_Label label = seq.Value(i);
        Handle(TDataStd_Name) name = new TDataStd_Name();
        label.FindAttribute(name->GetID(), name);

        //char *chars = new char[name->Get().LengthOfCString()];
        char chars[1000];
        char* x = chars;
        name->Get().ToUTF8CString(x);
        qDebug("NAME %s", chars);
        //delete[] chars;

        TopoDS_Shape shape;
        shapeTool->GetShape(label, shape);
        if (!shape.IsNull()) {
            qDebug("HOORAY! %d", i);
        }

    }
#endif
}

bool Translator::importSTEP(QString file,
                            const Handle(TopTools_HSequenceOfShape)& shapes)
{
    if (shapes.IsNull()) {
        return false;
    }

    importSTEPCAF(file);

    STEPControl_Reader aReader;
    IFSelect_ReturnStatus status = aReader.ReadFile((Standard_CString)file.toLatin1().constData());
    if (status == IFSelect_RetDone) {
        int nbr = aReader.NbRootsForTransfer();
        for (Standard_Integer n = 1; n <= nbr; n++) {
            bool ok = aReader.TransferRoot(n);
            if (!ok) {
                continue;
            }
            int nbs = aReader.NbShapes();
            if (nbs > 0) {
                for (int i = 1; i <= nbs; i++) {
                    TopoDS_Shape shape = aReader.Shape(i);
                    for (TopExp_Explorer e(shape, TopAbs_SOLID); e.More(); e.Next()) {
                        TopoDS_Shape solid = e.Current();
                        shapes->Append(solid);
                    }
                }
            }
        }
    } else {
        return false;
    }

    return true;
}


bool Translator::exportGDML(QString path,
                            const Handle(TopTools_HSequenceOfShape)& shapes,
                            const QVector<SolidMetadata>& metadata)
{
    if (shapes.IsNull() || shapes->IsEmpty()) {
        return false;
    }

    for (int i = 1; i <= shapes->Length(); i++) {
        TopoDS_Shape shape = shapes->Value(i);
        if (shape.IsNull()) {
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
