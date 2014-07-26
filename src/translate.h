#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <QObject>
#include "gdmlwriter.h"

#include <AIS_InteractiveContext.hxx>
#include <TopTools_HSequenceOfShape.hxx>

class TranslateDlg;

class Translate: public QObject
{
    Q_OBJECT

public:
    enum { FormatBREP, FormatIGES, FormatSTEP, FormatCSFDB, FormatVRML, FormatSTL, FormatGDML };

    Translate( QObject* );
    ~Translate();

    bool                                  importModel( const int, const Handle(AIS_InteractiveContext)& );
    bool                                  exportModel( const int, const Handle(AIS_InteractiveContext)& );

    QString                               info() const;

    virtual Handle(TopTools_HSequenceOfShape) importModel( const int, const QString& );
    virtual bool                              exportModel( const int, const QString&,
                                                           const Handle(TopTools_HSequenceOfShape)& );
    Handle(TopTools_HSequenceOfShape)         getShapes( const Handle(AIS_InteractiveContext)& );
protected:
    virtual bool                              displayShSequence(const Handle(AIS_InteractiveContext)&,
                                                                const Handle(TopTools_HSequenceOfShape)& );

private:
    QString                                   selectFileName( const int, const bool );
    TranslateDlg*                             getDialog( const int, const bool );

    Handle(TopTools_HSequenceOfShape)         importSTEP( const QString& );

    bool exportGDML( const QString&, const Handle(TopTools_HSequenceOfShape)& );

    bool checkFacetedBrep( const Handle(TopTools_HSequenceOfShape)& );

    GdmlWriter* gdmlwriter;

protected:
    TranslateDlg*                     myDlg;
    QString                           myInfo;
};

#endif

