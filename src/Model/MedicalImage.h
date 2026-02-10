#pragma once

#include <QString>
#include <QPixmap>

class MedicalImage 
{

public:

    virtual ~MedicalImage() = default;
    virtual const QPixmap& pixmap() const = 0;
    virtual bool isValid() const = 0;

};
