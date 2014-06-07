#ifndef MAGPROFILE_H
#define MAGPROFILE_H

#include "stbprofile.h"


#include <QObject>

namespace yasem
{
class StbPlugin;
class MagProfile : public QObject, public Profile
{
    Q_OBJECT
public:
    explicit MagProfile(StbPlugin *profilePlugin, const QString &id);

    enum DeviceModel {
        MAG_100,
        MAG_200,
        MAG_245,
        MAG_250,
        AURA_HD,
        MAG_254,
        MAG_255,
        MAG_260,
        MAG_270,
        MAG_275
    };

signals:

public slots:


public:
    void start();
    void stop();
    void initDefaults();

    bool isInternalPortal();
    void configureKeyMap();

protected:
    bool internalPortal;

    // Profile interface
public:
    QString portal();
};

}

#endif // MAGPROFILE_H
