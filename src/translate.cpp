#include "translate.h"

#include <QDir>
#include <QLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QList>
#include <QListView>
#include <QFileDialog>
#include <QApplication>
#include <QWidget>

#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_DisplayMode.hxx>

#include <STEPControl_Reader.hxx>

#include <TopoDS_Shape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_HSequenceOfShape.hxx>

// ---------------------------- TranslateDlg -----------------------------------------

class TranslateDlg : public QFileDialog
{
public:
    TranslateDlg( QWidget* = 0, Qt::WindowFlags flags = 0, bool = true );
    ~TranslateDlg();
    int                   getMode() const;
    void                  setMode( const int );
    void                  addMode( const int, const QString& );
    void                  clear();

protected:
    void                  showEvent ( QShowEvent* event );

private:
    QListView*            findListView( const QObjectList& );

private:
    QComboBox*            myBox;
    QList<int>            myList;
};

TranslateDlg::TranslateDlg( QWidget* parent, Qt::WindowFlags flags, bool modal )
    : QFileDialog( parent, flags )
{
    setModal( modal );
    QGridLayout* grid = ::qobject_cast<QGridLayout*>( layout() );

    if( grid )
    {
        QVBoxLayout *vbox = new QVBoxLayout;

        QWidget* paramGroup = new QWidget( this );
        paramGroup->setLayout( vbox );

        myBox = new QComboBox( paramGroup );
        vbox->addWidget( myBox );

        int row = grid->rowCount();
        grid->addWidget( paramGroup, row, 1, 1, 3 ); // make combobox occupy 1 row and 3 columns starting from 1
    }
}

TranslateDlg::~TranslateDlg()
{
}

int TranslateDlg::getMode() const
{
    if ( myBox->currentIndex() < 0 || myBox->currentIndex() > (int)myList.count() - 1 )
        return -1;
    else
        return myList.at( myBox->currentIndex() );
}

void TranslateDlg::setMode( const int mode )
{
    int idx = myList.indexOf( mode );
    if ( idx >= 0 )
        myBox->setCurrentIndex( idx );
}

void TranslateDlg::addMode( const int mode, const QString& name )
{
    myBox->show();
    myBox->addItem( name );
    myList.append( mode );
    myBox->updateGeometry();
    updateGeometry();
}

void TranslateDlg::clear()
{
    myList.clear();
    myBox->clear();
    myBox->hide();
    myBox->updateGeometry();
    updateGeometry();
}

QListView* TranslateDlg::findListView( const QObjectList & childList )
{
    QListView* listView = 0;
    for ( int i = 0, n = childList.count(); i < n && !listView; i++ )
    {
        listView = qobject_cast<QListView*>( childList.at( i ) );
        if ( !listView && childList.at( i ) )
        {
            listView = findListView( childList.at( i )->children() );
        }
    }
    return listView;
}

void TranslateDlg::showEvent ( QShowEvent* event )
{
    QFileDialog::showEvent ( event );
    QListView* aListView = findListView( children() );
    aListView->setViewMode( QListView::ListMode );
}


// ---------------------------- Translate -----------------------------------------

Translate::Translate( QObject* parent )
    : QObject( parent ),
      myDlg( 0 )
{
}

Translate::~Translate()
{
    if ( myDlg )
        delete myDlg;
}

QString Translate::info() const
{
    return myInfo;
}

bool Translate::importModel( const int format, const Handle(AIS_InteractiveContext)& ic )
{
    myInfo = QString();
    QString fileName = selectFileName( format, true );
    if ( fileName.isEmpty() )
        return true;

    if ( !QFileInfo( fileName ).exists() )
    {
        myInfo = QObject::tr( "INF_TRANSLATE_FILENOTFOUND" ).arg( fileName );
        return false;
    }

    QApplication::setOverrideCursor( Qt::WaitCursor );
    Handle(TopTools_HSequenceOfShape) shapes = importModel( format, fileName );
    QApplication::restoreOverrideCursor();

    return displayShSequence(ic, shapes);
}

bool Translate::displayShSequence(const Handle(AIS_InteractiveContext)& ic,
                                  const Handle(TopTools_HSequenceOfShape)& shapes )
{
    if ( shapes.IsNull() || !shapes->Length() )
        return false;

    for ( int i = 1; i <= shapes->Length(); i++ ) {
        AIS_Shape* e = new AIS_Shape( shapes->Value( i ) );
        e->SetDisplayMode(AIS_Shaded);

        ic->Display(e , false );
    }
    ic->UpdateCurrentViewer();
    return true;
}

Handle(TopTools_HSequenceOfShape) Translate::importModel( const int format, const QString& file )
{
    Handle(TopTools_HSequenceOfShape) shapes;
    try {
        switch ( format )
        {
        case FormatSTEP:
            shapes = importSTEP( file );
            break;
        }
    } catch ( Standard_Failure ) {
        shapes.Nullify();
    }
    return shapes;
}

bool Translate::exportModel( const int format, const Handle(AIS_InteractiveContext)& ic )
{
    myInfo = QString();
    QString fileName = selectFileName( format, false );
    if ( fileName.isEmpty() )
        return true;

    Handle(TopTools_HSequenceOfShape) shapes = getShapes( ic );
    if ( shapes.IsNull() || !shapes->Length() )
        return false;

    QApplication::setOverrideCursor( Qt::WaitCursor );
    bool stat = exportModel( format, fileName, shapes );
    QApplication::restoreOverrideCursor();

    return stat;
}

bool Translate::exportModel( const int format, const QString& file, const Handle(TopTools_HSequenceOfShape)& shapes )
{
    bool status = true;
    try {
        switch ( format )
        {
        case FormatGDML:
            status = exportGDML (file, shapes );
            break;
        }
    } catch ( Standard_Failure ) {
        status = false;
    }
    return status;
}

Handle(TopTools_HSequenceOfShape) Translate::getShapes( const Handle(AIS_InteractiveContext)& ic )
{
    Handle(TopTools_HSequenceOfShape) aSequence;
    Handle(AIS_InteractiveObject) picked;
    for ( ic->InitCurrent(); ic->MoreCurrent(); ic->NextCurrent() )
    {
        Handle(AIS_InteractiveObject) obj = ic->Current();
        if ( obj->IsKind( STANDARD_TYPE( AIS_Shape ) ) )
        {
            TopoDS_Shape shape = Handle(AIS_Shape)::DownCast(obj)->Shape();
            if ( aSequence.IsNull() )
                aSequence = new TopTools_HSequenceOfShape();
            aSequence->Append( shape );
        }
    }
    return aSequence;
}

/*!
    Selects a file from standard dialog acoording to selection 'filter'
*/
QString Translate::selectFileName( const int format, const bool import )
{
    TranslateDlg* theDlg = getDialog( format, import );

    int ret = theDlg->exec();
    
    qApp->processEvents();

    QString file;
    QStringList fileNames;
    if ( ret != QDialog::Accepted )
        return file;

    fileNames = theDlg->selectedFiles();
    if (!fileNames.isEmpty())
        file = fileNames[0];

    if ( !QFileInfo( file ).completeSuffix().length() )
    {
        QString selFilter = theDlg->selectedFilter();
        int idx = selFilter.indexOf( "(*." );
        if ( idx != -1 )
        {
            QString tail = selFilter.mid( idx + 3 );
            int idx = tail.indexOf( " " );
            if ( idx == -1 )
                idx = tail.indexOf( ")" );
            QString ext = tail.left( idx );
            if ( ext.length() )
                file += QString( "." ) + ext;
        }
    }

    return file;
}

TranslateDlg* Translate::getDialog( const int format, const bool import )
{
    if ( !myDlg )
        myDlg = new TranslateDlg( 0, 0, true );

    if ( format < 0 )
        return myDlg;

    QString formatFilter = QObject::tr( QString( "INF_FILTER_FORMAT_%1" ).arg( format ).toLatin1().constData() );
    QString allFilter = QObject::tr( "INF_FILTER_FORMAT_ALL" );

    QString filter;
    filter.append( formatFilter );
    filter.append( "\t" );

    if ( import )
    {
        filter.append( allFilter );
        filter.append( "\t" );
    }

    cout << filter.toLatin1().constData() << endl;
    QStringList filters = filter.split( "\t" );
    myDlg->setFilters( filters );

    if ( import )
    {
        myDlg->setWindowTitle( QObject::tr( "INF_APP_IMPORT" ) );
        ((QFileDialog*)myDlg)->setFileMode( QFileDialog::ExistingFile );
    }
    else
    {
        myDlg->setWindowTitle( QObject::tr( "INF_APP_EXPORT" ) );
        ((QFileDialog*)myDlg)->setFileMode( QFileDialog::AnyFile );
    }

    myDlg->clear();

    return myDlg;
}

// ----------------------------- Import functionality -----------------------------

Handle(TopTools_HSequenceOfShape) Translate::importSTEP( const QString& file )
{
    Handle(TopTools_HSequenceOfShape) aSequence;

    STEPControl_Reader aReader;
    IFSelect_ReturnStatus status = aReader.ReadFile( (Standard_CString)file.toLatin1().constData() );
    if ( status == IFSelect_RetDone )
    {
        //Interface_TraceFile::SetDefault();
        bool failsonly = false;
        aReader.PrintCheckLoad( failsonly, IFSelect_ItemsByEntity );

        int nbr = aReader.NbRootsForTransfer();
        aReader.PrintCheckTransfer( failsonly, IFSelect_ItemsByEntity );
        for ( Standard_Integer n = 1; n <= nbr; n++ )
        {
            aReader.TransferRoot( n );
            int nbs = aReader.NbShapes();
            if ( nbs > 0 )
            {
                aSequence = new TopTools_HSequenceOfShape();
                for ( int i = 1; i <= nbs; i++ )
                {
                    // reduce everything to the lowest level
                    TopoDS_Shape shape = aReader.Shape( i );
                    for (TopExp_Explorer e(shape, TopAbs_SOLID); e.More(); e.Next()) {
                        TopoDS_Shape solid = e.Current();
                        aSequence->Append( solid );
                    }
                }
            }
        }
    }
    return aSequence;
}

bool Translate::exportGDML( const QString& file, const Handle(TopTools_HSequenceOfShape)& shapes )
{
    printf("\n\nTRANSLATE\n");
    if ( shapes.IsNull() || shapes->IsEmpty() ) {
        printf("EMPTY\n");
        return false;
    }

    for ( int i = 1; i <= shapes->Length(); i++ )
    {
        TopoDS_Shape shape = shapes->Value( i );
        if ( shape.IsNull() )
        {
            return false;
        }
    }

    printf("SOLIDS %d\n", shapes->Length());

    // Why is this heap-allocated? Ask Cthulhu. Stuff gets corrupted otherwise. ;-(
    gdmlwriter = new GdmlWriter(file);
    gdmlwriter->writeIntro();
    for (int i=1;i<=shapes->Length();i++) {
        gdmlwriter->addSolid(shapes->Value(i), QString::number(i), QString("ALUMINUM"));
    }
    gdmlwriter->writeExtro();
    delete gdmlwriter;

    return true;
}



