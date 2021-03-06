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

#include <QDebug>
#include <QHostAddress>
#include <QStringList>
#include <QTimer>
#include <QUrl>

#include "browsemodel.h"
#include "browsemodel_p.h"
#include "upnpdevicemodel.h"
#include "glib-utils.h"
#include "serviceproxycall.h"
#include "didlliteparser.h"

const char AUDIO_PREFIX[] = "object.item.audioItem";
const char IMAGE_PREFIX[] = "object.item.imageItem";
const char VIDEO_PREFIX[] = "object.item.videoItem";
const char MUSIC_ALBUM_CLASS[] = "object.container.album.musicAlbum";
const char ARTIST_CLASS[] = "object.container.person.musicArtist";
const char GENRE_CLASS[] = "object.container.genre.musicGenre";
const char CONTAINER_PREFIX[] = "object.container";

BrowseModelPrivate::BrowseModelPrivate(ServiceProxyCall *call,
                                       const QString  &protocolInfo,
                                       BrowseModel *parent)
    : QAbstractListModel(parent)
    , m_currentOffset(0)
    , m_busy(true)
    , m_done(false)
    , m_protocolInfo(protocolInfo)
    , m_lastIndex(-1)
    , m_call(call)
    , m_settings()
    , q_ptr(parent)
{
    QHash<int, QByteArray> roles;

    roles[BrowseRoleTitle] = "title";
    roles[BrowseRoleId] = "upnpId";
    roles[BrowseRoleUPnPClass] = "upnpClass";
    roles[BrowseRoleIcon] = "icon";
    roles[BrowseRoleType] = "type";
    roles[BrowseRoleDetail] = "detail";
    roles[BrowseRoleURI] = "uri";
    roles[BrowseRoleMetaData] = "metadata";
    roles[BrowseRoleFilter] = "filter";
    setRoleNames(roles);

    qDebug() << "Created browse model";
    if (m_call != 0) {
        connect(m_call, SIGNAL(ready()), SLOT(onCallReady()));
        m_call->setParent(this);
    }

    connect(&m_settings, SIGNAL(filterInDetailsChanged()), SLOT(onFilterInDetailsChanged()));
}

BrowseModelPrivate::~BrowseModelPrivate()
{
    if (m_call != 0) {
        m_call->cancel();
    }
}

int BrowseModelPrivate::rowCount(const QModelIndex &/*parent*/) const
{
    return m_data.count();
}

static QUrl findIconForObject(GUPnPDIDLLiteObject *object)
{
    QUrl thumbnail;
    GList* resources = gupnp_didl_lite_object_get_resources(object);
    GList* it = resources;

    const char *upnp_class = gupnp_didl_lite_object_get_upnp_class (object);

    if (upnp_class == 0) {
        thumbnail.setUrl(QLatin1String("image://theme/icon-m-content-file-unknown-inverse"));

        return thumbnail;
    }

    if (strncmp(upnp_class, AUDIO_PREFIX, sizeof(AUDIO_PREFIX) - 1) == 0 ||
        strncmp(upnp_class, MUSIC_ALBUM_CLASS, sizeof(MUSIC_ALBUM_CLASS) - 1) == 0) {
        const char *album_art_uri = gupnp_didl_lite_object_get_album_art(object);
        thumbnail.setUrl(QString::fromUtf8(album_art_uri));
    } else {
        while (it) {
            GUPnPDIDLLiteResource *res = (GUPnPDIDLLiteResource*) it->data;
            GUPnPProtocolInfo *info = gupnp_didl_lite_resource_get_protocol_info(res);
            const char *profile = gupnp_protocol_info_get_dlna_profile(info);
            if (profile != 0 &&
                (strncmp(profile, "JPEG_TN", 7) == 0 ||
                 strncmp(profile, "PNG_TN", 6) == 0)) {
                thumbnail.setUrl(QString::fromUtf8(gupnp_didl_lite_resource_get_uri(res)));
                break;
            }
            it = it->next;
        }
    }

    if (thumbnail.isEmpty()) {
        if (strncmp(upnp_class, IMAGE_PREFIX, sizeof(IMAGE_PREFIX) - 1) == 0) {
            thumbnail.setUrl(QLatin1String("image://theme/icon-m-content-image-inverse"));
        } else if (strncmp(upnp_class, VIDEO_PREFIX, sizeof(VIDEO_PREFIX) - 1) == 0) {
            thumbnail.setUrl(QLatin1String("image://theme/icon-m-content-videos-inverse"));
        } else if (strncmp(upnp_class, AUDIO_PREFIX, sizeof(AUDIO_PREFIX) - 1) == 0) {
            thumbnail.setUrl(QLatin1String("image://theme/icon-m-content-audio-inverse"));
        } else if (strncmp(upnp_class, MUSIC_ALBUM_CLASS, sizeof(MUSIC_ALBUM_CLASS) - 1) == 0) {
            thumbnail.setUrl(QLatin1String("image://theme/icon-m-content-album-inverse"));
        } else if (strncmp(upnp_class, ARTIST_CLASS, sizeof(ARTIST_CLASS) - 1) == 0) {
            thumbnail.setUrl(QLatin1String("image://theme/icon-m-content-artist-inverse"));
        } else if (strncmp(upnp_class, GENRE_CLASS, sizeof(GENRE_CLASS) - 1) == 0) {
            thumbnail.setUrl(QLatin1String("image://theme/icon-m-content-genre-inverse"));
        } else if (strncmp(upnp_class, CONTAINER_PREFIX, sizeof(CONTAINER_PREFIX) - 1) == 0) {
            thumbnail.setUrl(QLatin1String("image://theme/icon-m-toolbar-directory-white"));
        }
    }

    g_list_free_full(resources, g_object_unref);

    if (thumbnail.isEmpty()) {
        thumbnail.setUrl(QLatin1String("image://theme/icon-m-content-file-unknown-inverse"));
    }

    return thumbnail;
}

static QString formatTime(long duration)
{
    QString result;

    int hours = duration / 60 / 60;
    duration %= 3600;
    int minutes = duration / 60;
    duration %= 60;
    if ((hours > 0 || minutes > 0 || duration > 0)) {
        result += QLatin1String(" ");
    }
    if (hours > 0) {
        result += QString::fromLatin1("%1:").arg(hours);
    }
    if (minutes > 0 || hours > 0 || duration > 0) {
        result += QString::fromLatin1("%1:").arg((int) minutes, hours > 0 || minutes > 9 ? 2 : 1, 10, QLatin1Char('0'));
    }
    if (duration > 0 || hours > 0 || minutes > 0) {
        result += QString::fromLatin1("%1").arg((int) duration, 2, 10, QLatin1Char('0'));
    }

    return result;
}

static QString formatSize(quint64 size)
{
    QString result;

    QStringList sizes = QStringList() << QLatin1String("GiB")
                                      << QLatin1String("MiB")
                                      << QLatin1String("KiB")
                                      << QLatin1String("bytes");

    while (size > 1024 && sizes.last() != QLatin1String("GiB")) {
        size >>= 10;
        sizes.removeLast();
    }
    result += QString::fromLatin1("%1%2").arg(QString::number(size), sizes.last());

    return result;
}

static QString createDetailsForObject(GUPnPDIDLLiteObject *object)
{
    QString result;

    if (GUPNP_IS_DIDL_LITE_CONTAINER(object)) {
        return result;
    }

    GList* resources = gupnp_didl_lite_object_get_resources(object);
    GList* it = resources;

    const char *author = gupnp_didl_lite_object_get_artist(object);
    const char *album = gupnp_didl_lite_object_get_album(object);

    if (author != 0) {
        result += QString::fromLatin1("%1").arg(QString::fromUtf8(author));
    }

    if (album != 0) {
        if (not result.isEmpty()) {
            result += QLatin1String(" | ");
        }
        result += QString::fromLatin1("%1").arg(QString::fromUtf8(album));
    }

    const QLatin1String empty = QLatin1String(" ");
    while (it) {
        GUPnPDIDLLiteResource *res = (GUPnPDIDLLiteResource*) it->data;
        GUPnPProtocolInfo *info = gupnp_didl_lite_resource_get_protocol_info(res);
        // use first non-transcoded uri; might be problematic
        if (gupnp_protocol_info_get_dlna_conversion (info) == GUPNP_DLNA_CONVERSION_NONE) {
            long duration = gupnp_didl_lite_resource_get_duration(res);
            if (duration > 0) {
                if (not result.isEmpty()) {
                    result += empty;
                }
                result += formatTime(duration);

            }

            int width = gupnp_didl_lite_resource_get_width(res);
            int height = gupnp_didl_lite_resource_get_height(res);
            if (width > 0 && height > 0) {
                if (not result.isEmpty()) {
                    result += empty;
                }
                result += QString::fromLatin1("%1x%2").arg(QString::number(width), QString::number(height));
            }

            gint64 size = gupnp_didl_lite_resource_get_size64(res);
            if (size > 0) {
                if (not result.isEmpty()) {
                    result += empty;
                }

                result += formatSize(size);
            }

            break;
        }
        it = it->next;
    }

    g_list_free_full(resources, g_object_unref);

    return result;
}

QString BrowseModelPrivate::getCompatibleUri(int index, const QString &protocolInfo) const
{
    RefPtrG<GUPnPDIDLLiteResource> resource;
    DIDLLiteObject object = m_data.at(index);
    QString urlString;

    if (object.isEmpty()) {
        return QString();
    }

    // work-around GNOME bug 671246
    QStringList protocolInfoList = protocolInfo.split(QLatin1Char(','));
    while (protocolInfoList.size() > 0) {
        int split = MIN(255, protocolInfoList.size());
        QString protocolInfoStrip = QStringList(protocolInfoList.mid(0, split)).join(QLatin1String(","));
        protocolInfoList = protocolInfoList.mid(split);

        resource = wrap(gupnp_didl_lite_object_get_compat_resource(object,
                                                                   protocolInfoStrip.toUtf8().constData(),
                                                                   FALSE));
        if (not resource.isEmpty()) {
            urlString = QString::fromUtf8(gupnp_didl_lite_resource_get_uri(resource));
            QUrl url(urlString);

            // check that uri has a host and that the host is an IP address
            if (not (url.host().isEmpty()) &&
                not QHostAddress(url.host()).isNull()) {
                break;
            }
        }
    }

    if (resource.isEmpty()) {
        return QString();
    }

    return urlString;
}

static QString generateMetaData(GUPnPDIDLLiteObject *object)
{
    GList *resources, *it;

    if (GUPNP_IS_DIDL_LITE_CONTAINER(object)) {
        return QString();
    }

    // produce minimal DIDL
    auto writer = wrap(gupnp_didl_lite_writer_new (NULL));
    GUPnPDIDLLiteObject *item = GUPNP_DIDL_LITE_OBJECT(gupnp_didl_lite_writer_add_item(writer));
    const char *title = gupnp_didl_lite_object_get_title(object);

    // maximum title length is 256 bytes
    if (strlen(title) > 256) {
        char *tmp = 0;
        const char *end_ptr = 0;

        g_utf8_validate(title, 256, &end_ptr);
        tmp = g_strndup(title, end_ptr - title);
        gupnp_didl_lite_object_set_title(item, tmp);
        g_free(tmp);
    } else {
        gupnp_didl_lite_object_set_title(item, title);
    }

    gupnp_didl_lite_object_set_upnp_class(item, gupnp_didl_lite_object_get_upnp_class(object));
    gupnp_didl_lite_object_set_parent_id(item, gupnp_didl_lite_object_get_parent_id(object));
    gupnp_didl_lite_object_set_id(item, gupnp_didl_lite_object_get_id(object));
    gupnp_didl_lite_object_set_restricted(item, gupnp_didl_lite_object_get_restricted(object));
    it = resources = gupnp_didl_lite_object_get_resources(object);
    while (it != 0) {
        GUPnPDIDLLiteResource *orig_resource = GUPNP_DIDL_LITE_RESOURCE(it->data);
        GUPnPDIDLLiteResource *resource = gupnp_didl_lite_object_add_resource(item);
        gupnp_didl_lite_resource_set_uri(resource,
                                         gupnp_didl_lite_resource_get_uri (orig_resource));
        gupnp_didl_lite_resource_set_protocol_info(resource,
                                                   gupnp_didl_lite_resource_get_protocol_info(orig_resource));

        it = it->next;
    }
    g_list_free_full(resources, (GDestroyNotify)g_object_unref);

    return QString::fromUtf8(gupnp_didl_lite_writer_get_string(writer));
}

QVariant BrowseModelPrivate::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    DIDLLiteObject object = m_data.at(index.row());
    if (object.isEmpty()) {
        return QVariant();
    }

    switch (role) {
    case BrowseRoleTitle:
        return QString::fromUtf8(gupnp_didl_lite_object_get_title(object));
    case BrowseRoleId:
        return QString::fromUtf8(gupnp_didl_lite_object_get_id(object));
    case BrowseRoleUPnPClass:
        return QString::fromUtf8(gupnp_didl_lite_object_get_upnp_class(object));
    case BrowseRoleIcon:
        return findIconForObject(object);
    case BrowseRoleURI:
        return getCompatibleUri(index.row(), m_protocolInfo);
    case BrowseRoleType:
        if (GUPNP_IS_DIDL_LITE_CONTAINER(object)) {
            return QLatin1String("container");
        } else {
            return QLatin1String("object");
        }
    case BrowseRoleDetail:
        return createDetailsForObject(object);
    case BrowseRoleMetaData:
        return generateMetaData(object);
    case BrowseRoleFilter:
        return QString::fromUtf8(gupnp_didl_lite_object_get_title(object)) + createDetailsForObject(object);
    default:
        return QVariant();
    }
}

void BrowseModelPrivate::onCallReady()
{
    auto call = qobject_cast<ServiceProxyCall *>(sender());
    if (call == 0) {
        return;
    }

    if (call->cancelled()) {
        return;
    }

    call->finalize(QStringList() << QLatin1String("Result")
                                 << QLatin1String("NumberReturned")
                                 << QLatin1String("TotalMatches"));
    setBusy(false);

    if (call->hasError()) {
        Q_EMIT error(call->errorCode(), call->errorMessage());

        return;
    }

    unsigned int numberReturned = call->get(QLatin1String("NumberReturned")).toUInt();
    if (numberReturned == 0) {
        setDone(true);

        return;
    }

    auto objects = DIDLLiteParser().parse(call->get(QLatin1String("Result")).toString());

    beginInsertRows(QModelIndex(),
                    m_data.count(),
                    m_data.count() + numberReturned - 1);
    m_data << objects;
    endInsertRows();

    m_currentOffset += numberReturned;

    unsigned int totalMatches = call->get(QLatin1String("TotalMatches")).toUInt();
    if (totalMatches > 0 && m_currentOffset < totalMatches) {
        m_call->setArg(QLatin1String("StartingIndex"), m_currentOffset);
        m_call->run();
    } else {
        setDone(true);
    }
}

void BrowseModelPrivate::refresh() {
    beginResetModel();
    setDone(false);
    setBusy(true);
    m_currentOffset = 0;
    m_data.clear();
    qDebug () << "Starting to browse" << m_call->arg(QLatin1String("ObjectID"));
    m_call->setArg(QLatin1String("StartingIndex"), m_currentOffset);
    m_call->run();
    endResetModel();
}

QString BrowseModelPrivate::formatTime(long duration)
{
    return ::formatTime(duration);
}

void BrowseModelPrivate::setProtocolInfo(const QString& protocolInfo)
{
    if (protocolInfo != m_protocolInfo) {
        m_protocolInfo = protocolInfo;
        Q_EMIT protocolInfoChanged();
        Q_EMIT dataChanged(index(0), index(m_data.count() - 1));
    }
}

void BrowseModelPrivate::setLastIndex(int index)
{
    if (index != m_lastIndex) {
        m_lastIndex = index;
        Q_EMIT lastIndexChanged();
    }
}

void BrowseModelPrivate::onFilterInDetailsChanged()
{
    Q_Q(BrowseModel);
    q->setFilterRole(m_settings.filterInDetails() ? BrowseModelPrivate::BrowseRoleFilter : BrowseModelPrivate::BrowseRoleTitle);
}
