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

#ifndef SCOPEREF_H
#define SCOPEREF_H

#include <glib-object.h>

#include <libgupnp/gupnp.h>
#include <gupnp-av/gupnp-av.h>

#include <QDebug>
#include <QScopedPointer>

template<typename C>
struct ScopedPointerDestroyHelperGFree
{
    static inline void cleanup(C *pointer)
    {
        if (pointer != 0) {
            g_free ((gpointer)pointer);
        }
    }
};
typedef QScopedPointer<char, ScopedPointerDestroyHelperGFree<char> > ScopedGPointer;

template<typename C>
struct ScopedPointerLater
{
    static inline void cleanup(C *pointer) {
        if (pointer != 0) {
            pointer->deleteLater();
        }
    }
};
typedef QScopedPointer<QObject, ScopedPointerLater<QObject> > ScopedLaterPointer;

template<typename C>
struct ScopedPointerDestroyHelperGList
{
    static inline void cleanup(C *pointer)
    {
        if (pointer != 0) {
            g_list_free(pointer);
        }
    }
};
typedef QScopedPointer<GList, ScopedPointerDestroyHelperGList<GList> > QGListScopedPointer;

template<typename C>
struct ScopedPointerDestroyHelperGListFull
{
    static inline void cleanup(C *pointer)
    {
        if (pointer != 0) {
            g_list_free_full(pointer, g_free);
        }
    }
};
typedef QScopedPointer<GList, ScopedPointerDestroyHelperGListFull<GList> > QGListFullScopedPointer;

/**
 * Keep a reference on a GObject in the current scope using RAII
 */
template <typename C>
class RefPtrG {
public:
    static RefPtrG<C> wrap(C* type)
    {
        RefPtrG<C> ref;
        ref.m_type = type;

        return ref;
    }

    explicit RefPtrG(C *type = 0)
        : m_type(type == 0 ? 0 : (C *)g_object_ref (type))
    {
    }

    RefPtrG(RefPtrG const& other)
        : m_type(other.m_type == 0 ? 0 : (C *)g_object_ref (other.m_type))
    {
    }

    ~RefPtrG()
    {
        if (m_type) {
            g_object_unref (m_type);
        }
    }

    operator C*() const
    {
        return m_type;
    }

    /// Helper cast operator for so we can use the GObject casting macros on
    /// an instance of this class
    operator GTypeInstance *() const
    {
        return reinterpret_cast<GTypeInstance*>(m_type);
    }

    RefPtrG& operator=(RefPtrG const& other)
    {
        if (this != &other) {
            RefPtrG(other).swap(*this);
        }

        return *this;
    }

    bool isEmpty() const
    {
        return m_type == 0;
    }

    void clear()
    {
        if (m_type) {
            g_object_unref(m_type);
            m_type = 0;
        }
    }

    C *data() { return m_type; }

private:
    void swap (RefPtrG &ref) throw ()
    {
        std::swap(this->m_type, ref.m_type);
    }

    C *m_type;
};

typedef RefPtrG<GUPnPServiceProxy> GServiceProxy;
typedef RefPtrG<GUPnPDeviceProxy> DeviceProxy;
typedef RefPtrG<GUPnPDIDLLiteParser> DIDLLiteParser;

#endif // SCOPEREF_H
