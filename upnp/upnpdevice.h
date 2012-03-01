/*
This file is part of MediaController.

MediaController is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MediaController is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with MediaController.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UPNPDEVICE_H
#define UPNPDEVICE_H

#include <libgupnp/gupnp.h>

#include <QObject>
#include <QUrl>
#include <QMetaType>

#include "refptrg.h"

class UPnPDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString friendlyName READ friendlyName NOTIFY friendlyNameChanged REVISION 1)
    Q_PROPERTY(QString udn READ udn REVISION 1)
    Q_PROPERTY(QUrl icon READ icon NOTIFY iconChanged REVISION 1)
    Q_PROPERTY(QString type READ type REVISION 1)
public:
    explicit UPnPDevice();
    UPnPDevice(const UPnPDevice& other);
    ~UPnPDevice();

    QString friendlyName() const;
    QUrl icon() const;
    QString udn() const;
    QString type() const;
    Q_INVOKABLE virtual void wrapDevice(const QString& udn);
    ServiceProxy getService(const char *service);

    // static helper functions
    static QUrl getIcon(GUPnPDeviceProxy *proxy);

    static const char CONNECTION_MANAGER_SERVICE[];
Q_SIGNALS:
    Q_REVISION(1) void friendlyNameChanged();
    Q_REVISION(1) void iconChanged();

public Q_SLOTS:
protected:
    DeviceProxy m_proxy;
};

#endif // UPNPDEVICE_H
