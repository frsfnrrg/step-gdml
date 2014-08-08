#include <QtCore>
#include <QtGui>

#include "window.h"
#include "translate.h"
#include "stdio.h"

#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <Message_SequenceOfPrinters.hxx>

#include <iostream>

#include <Message_Printer.hxx>

class CustomPrinter : public Message_Printer {
public:
    CustomPrinter() {}
    virtual void Send(const TCollection_ExtendedString& theString,const Message_Gravity,const Standard_Boolean putEndl) const {
        char buf[theString.LengthOfCString()];
        char* alias = buf;
        theString.ToUTF8CString(alias);
        if (putEndl) {
            printf("%s\n", alias);
        } else {
            printf(alias);
        }
    }
};

void setOpenCASCADEPrinters() {
    // OSD_Timer writes directly to std::cout. We disable...
    // This does kill all of its users, but this application
    // doesn't have any worth keeping. The "proper" solution
    // is to enable/disable it exactly around the operation
    // to be excised.
    std::cout.setstate(std::ios_base::failbit);

    // Update standard OpenCASCADE output method
    const Message_SequenceOfPrinters& p = Message::DefaultMessenger()->Printers();
    for (int i=1;i<=p.Length();i++) {
        Message::DefaultMessenger()->RemovePrinter(p.Value(i));
    }
    //Message::DefaultMessenger()->AddPrinter(new CustomPrinter());
}


int main(int argc, char** argv)
{
    setOpenCASCADEPrinters();

    QApplication app(argc, argv);
    app.setApplicationName("STEP-GDML");
    QStringList args = app.arguments();

    if (args.length() <= 2) {
        QString ifile;
        if (args.length() == 2) {
            ifile = args[1];
        }
        MainWindow w(ifile);
        w.show();
        return app.exec();
    } else if (args.length() == 3) {
        QString ifile = args[1];
        QString ofile = args[2];
        Handle(TopTools_HSequenceOfShape) shapes = new TopTools_HSequenceOfShape();
        QList<QPair<QString, QColor> > li;
        if (!Translator::importSTEP(ifile, shapes, li)) {
            printf("Import failed. :-(\n");
            return -1;
        }
        QVector<SolidMetadata> metadata(shapes->Length());
        for (int i = 0; i < metadata.size(); i++) {
            metadata[i].color = Quantity_Color();
            metadata[i].item = 0;
            metadata[i].object = 0;
            metadata[i].name = QString::number(i);
            metadata[i].transp = 0.0;
            metadata[i].material = "ALUMINUM";
        }

        if (!Translator::exportGDML(ofile, shapes, metadata)) {
            printf("Export failed. :-(\n");
            return -1;
        }
        return 0;
    } else {
        printf("Usage: step-gdml [INPUT_STEP_FILE] [OUTPUT_GDML_FILE]\n");
        return -1;
    }
}
