/*
This file is part of Helium.

Helium is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Helium is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Helium.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UPNPRENDERERMODEL_H
#define UPNPRENDERERMODEL_H

#include <QSortFilterProxyModel>

class UPnPRenderer;

class UPnPRendererModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    UPnPRendererModel(QObject *parent = 0);
public Q_SLOTS:
    void refresh() const;
    QString get(int index) const;
    UPnPRenderer *getDevice(int row) const;
};

#endif // UPNPRENDERERMODEL_H
