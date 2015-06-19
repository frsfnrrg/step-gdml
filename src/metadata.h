#ifndef METADATA_H
#define METADATA_H

#include <QString>

#include <Quantity_Color.hxx>
#include <Standard_Real.hxx>

class AIS_InteractiveObject;
class QListWidgetItem;

typedef struct {
    AIS_InteractiveObject* object;
    QListWidgetItem* item;
    QString name;
    QString material;
    Quantity_Color color;
    Standard_Real transp;
} SolidMetadata;

#endif // METADATA_H
