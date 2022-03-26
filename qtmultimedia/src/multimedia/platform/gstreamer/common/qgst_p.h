/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGST_P_H
#define QGST_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtmultimediaglobal_p.h>

#include <QSemaphore>
#include <QtCore/qlist.h>

#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qvideoframe.h>

#include <gst/gst.h>
#include <gst/video/video-info.h>

#include <functional>

#if QT_CONFIG(gstreamer_photography)
#define GST_USE_UNSTABLE_API
#include <gst/interfaces/photography.h>
#undef GST_USE_UNSTABLE_API
#endif
#ifndef QT_NO_DEBUG
#include <qdebug.h>
#endif

QT_BEGIN_NAMESPACE

class QSize;
class QGstStructure;
class QGstCaps;
class QGstPipelinePrivate;
class QCameraFormat;

template <typename T> struct QGRange
{
    T min;
    T max;
};

class QGString
{
    char *str;
public:
    QGString(char *string) : str(string) {}
    ~QGString() { g_free(str); }
    operator QByteArray() { return QByteArray(str); }
    operator const char *() { return str; }
};

class QGValue
{
public:
    QGValue(const GValue *v) : value(v) {}
    const GValue *value;

    bool isNull() const { return !value; }

    std::optional<bool> toBool() const
    {
        if (!G_VALUE_HOLDS_BOOLEAN(value))
            return std::nullopt;
        return g_value_get_boolean(value);
    }
    std::optional<int> toInt() const
    {
        if (!G_VALUE_HOLDS_INT(value))
            return std::nullopt;
        return g_value_get_int(value);
    }
    std::optional<int> toInt64() const
    {
        if (!G_VALUE_HOLDS_INT64(value))
            return std::nullopt;
        return g_value_get_int64(value);
    }
    template<typename T>
    T *getPointer() const
    {
        return value ? static_cast<T *>(g_value_get_pointer(value)) : nullptr;
    }

    const char *toString() const
    {
        return value ? g_value_get_string(value) : nullptr;
    }
    std::optional<float> getFraction() const
    {
        if (!GST_VALUE_HOLDS_FRACTION(value))
            return std::nullopt;
        return (float)gst_value_get_fraction_numerator(value)/(float)gst_value_get_fraction_denominator(value);
    }

    std::optional<QGRange<float>> getFractionRange() const
    {
        if (!GST_VALUE_HOLDS_FRACTION_RANGE(value))
            return std::nullopt;
        QGValue min = gst_value_get_fraction_range_min(value);
        QGValue max = gst_value_get_fraction_range_max(value);
        return QGRange<float>{ *min.getFraction(), *max.getFraction() };
    }

    std::optional<QGRange<int>> toIntRange() const
    {
        if (!GST_VALUE_HOLDS_INT_RANGE(value))
            return std::nullopt;
        return QGRange<int>{ gst_value_get_int_range_min(value), gst_value_get_int_range_max(value) };
    }

    inline QGstStructure toStructure() const;
    inline QGstCaps toCaps() const;

    inline bool isList() const { return value && GST_VALUE_HOLDS_LIST(value); }
    inline int listSize() const { return gst_value_list_get_size(value); }
    inline QGValue at(int index) const { return gst_value_list_get_value(value, index); }

    Q_MULTIMEDIA_EXPORT QList<QAudioFormat::SampleFormat> getSampleFormats() const;
};

class QGstStructure {
public:
    const GstStructure *structure = nullptr;
    QGstStructure() = default;
    QGstStructure(const GstStructure *s) : structure(s) {}
    void free() { if (structure) gst_structure_free(const_cast<GstStructure *>(structure)); structure = nullptr; }

    bool isNull() const { return !structure; }

    QByteArrayView name() const { return gst_structure_get_name(structure); }

    QGValue operator[](const char *name) const { return gst_structure_get_value(structure, name); }

    Q_MULTIMEDIA_EXPORT QSize resolution() const;
    Q_MULTIMEDIA_EXPORT QVideoFrameFormat::PixelFormat pixelFormat() const;
    Q_MULTIMEDIA_EXPORT QGRange<float> frameRateRange() const;

    QByteArray toString() const
    {
        char *s = gst_structure_to_string(structure);
        QByteArray str(s);
        g_free(s);
        return str;
    }
    QGstStructure copy() const { return gst_structure_copy(structure); }
};

class QGstCaps {
    const GstCaps *caps = nullptr;
public:
    enum MemoryFormat {
        CpuMemory,
        GLTexture,
        DMABuf
    };

    QGstCaps() = default;
    QGstCaps(const GstCaps *c) : caps(c) {}

    bool isNull() const { return !caps; }

    int size() const { return gst_caps_get_size(caps); }
    QGstStructure at(int index) { return gst_caps_get_structure(caps, index); }
    const GstCaps *get() const { return caps; }
    QByteArray toString() const
    {
        gchar *c = gst_caps_to_string(caps);
        QByteArray b(c);
        g_free(c);
        return b;
    }
    MemoryFormat memoryFormat() const {
        auto *features = gst_caps_get_features(caps, 0);
        if (gst_caps_features_contains(features, "memory:GLMemory"))
            return GLTexture;
        else if (gst_caps_features_contains(features, "memory:DMABuf"))
            return DMABuf;
        return CpuMemory;
    }
    QVideoFrameFormat formatForCaps(GstVideoInfo *info) const;
};

class QGstMutableCaps : public QGstCaps {
    GstCaps *caps = nullptr;
public:
    enum RefMode { HasRef, NeedsRef };
    QGstMutableCaps() = default;
    QGstMutableCaps(GstCaps *c, RefMode mode = HasRef)
        : QGstCaps(c), caps(c)
    {
        Q_ASSERT(QGstCaps::get() == caps);
        if (mode == NeedsRef)
            gst_caps_ref(caps);
    }
    QGstMutableCaps(const QGstMutableCaps &other)
        : QGstCaps(other), caps(other.caps)
    {
        Q_ASSERT(QGstCaps::get() == caps);
        if (caps)
            gst_caps_ref(caps);
    }
    QGstMutableCaps &operator=(const QGstMutableCaps &other)
    {
        QGstCaps::operator=(other);
        if (other.caps)
            gst_caps_ref(other.caps);
        if (caps)
            gst_caps_unref(caps);
        caps = other.caps;
        Q_ASSERT(QGstCaps::get() == caps);
        return *this;
    }
    ~QGstMutableCaps() {
        Q_ASSERT(QGstCaps::get() == caps);
        if (caps)
            gst_caps_unref(caps);
    }

    void create() {
        caps = gst_caps_new_empty();
        QGstCaps::operator=(QGstCaps(caps));
    }

    void addPixelFormats(const QList<QVideoFrameFormat::PixelFormat> &formats, const char *modifier = nullptr);
    static QGstMutableCaps fromCameraFormat(const QCameraFormat &format);

    GstCaps *get() const { return caps; }
};

class QGstObject
{
protected:
    GstObject *m_object = nullptr;
public:
    enum RefMode { HasRef, NeedsRef };

    QGstObject() = default;
    explicit QGstObject(GstObject *o, RefMode mode = HasRef)
        : m_object(o)
    {
        if (o && mode == NeedsRef)
            // Use ref_sink to remove any floating references
            gst_object_ref_sink(m_object);
    }
    QGstObject(const QGstObject &other)
        : m_object(other.m_object)
    {
        if (m_object)
            gst_object_ref(m_object);
    }
    QGstObject &operator=(const QGstObject &other)
    {
        if (this == &other)
            return *this;
        if (other.m_object)
            gst_object_ref(other.m_object);
        if (m_object)
            gst_object_unref(m_object);
        m_object = other.m_object;
        return *this;
    }
    virtual ~QGstObject() {
        if (m_object)
            gst_object_unref(m_object);
    }

    friend bool operator==(const QGstObject &a, const QGstObject &b)
    { return a.m_object == b.m_object; }
    friend bool operator!=(const QGstObject &a, const QGstObject &b)
    { return a.m_object != b.m_object; }

    bool isNull() const { return !m_object; }

    void set(const char *property, const char *str) { g_object_set(m_object, property, str, nullptr); }
    void set(const char *property, bool b) { g_object_set(m_object, property, gboolean(b), nullptr); }
    void set(const char *property, uint i) { g_object_set(m_object, property, guint(i), nullptr); }
    void set(const char *property, int i) { g_object_set(m_object, property, gint(i), nullptr); }
    void set(const char *property, qint64 i) { g_object_set(m_object, property, gint64(i), nullptr); }
    void set(const char *property, quint64 i) { g_object_set(m_object, property, guint64(i), nullptr); }
    void set(const char *property, double d) { g_object_set(m_object, property, gdouble(d), nullptr); }
    void set(const char *property, const QGstObject &o) { g_object_set(m_object, property, o.object(), nullptr); }
    void set(const char *property, const QGstMutableCaps &c) { g_object_set(m_object, property, c.get(), nullptr); }

    QGString getString(const char *property) const
    { char *s = nullptr; g_object_get(m_object, property, &s, nullptr); return s; }
    QGstStructure getStructure(const char *property) const
    { GstStructure *s = nullptr; g_object_get(m_object, property, &s, nullptr); return QGstStructure(s); }
    bool getBool(const char *property) const { gboolean b = false; g_object_get(m_object, property, &b, nullptr); return b; }
    uint getUInt(const char *property) const { guint i = 0; g_object_get(m_object, property, &i, nullptr); return i; }
    int getInt(const char *property) const { gint i = 0; g_object_get(m_object, property, &i, nullptr); return i; }
    quint64 getUInt64(const char *property) const { guint64 i = 0; g_object_get(m_object, property, &i, nullptr); return i; }
    qint64 getInt64(const char *property) const { gint64 i = 0; g_object_get(m_object, property, &i, nullptr); return i; }
    float getFloat(const char *property) const { gfloat d = 0; g_object_get(m_object, property, &d, nullptr); return d; }
    double getDouble(const char *property) const { gdouble d = 0; g_object_get(m_object, property, &d, nullptr); return d; }
    QGstObject getObject(const char *property) const { GstObject *o = nullptr; g_object_get(m_object, property, &o, nullptr); return QGstObject(o, HasRef); }

    void connect(const char *name, GCallback callback, gpointer userData) { g_signal_connect(m_object, name, callback, userData); }

    GstObject *object() const { return m_object; }
    const char *name() const { return m_object ? GST_OBJECT_NAME(m_object) : "(null)"; }
};

class QGstElement;

class QGstPad : public QGstObject
{
public:
    QGstPad() = default;
    QGstPad(const QGstObject &o)
        : QGstPad(GST_PAD(o.object()), NeedsRef)
    {}
    QGstPad(GstPad *pad, RefMode mode = NeedsRef)
        : QGstObject(&pad->object, mode)
    {}

    QGstMutableCaps currentCaps() const
    { return QGstMutableCaps(gst_pad_get_current_caps(pad())); }
    QGstCaps queryCaps() const
    { return QGstCaps(gst_pad_query_caps(pad(), nullptr)); }

    bool isLinked() const { return gst_pad_is_linked(pad()); }
    bool link(const QGstPad &sink) const { return gst_pad_link(pad(), sink.pad()) == GST_PAD_LINK_OK; }
    bool unlink(const QGstPad &sink) const { return gst_pad_unlink(pad(), sink.pad()); }
    bool unlinkPeer() const { return unlink(peer()); }
    QGstPad peer() const { return QGstPad(gst_pad_get_peer(pad()), HasRef); }
    inline QGstElement parent() const;

    GstPad *pad() const { return GST_PAD_CAST(object()); }

    GstEvent *stickyEvent(GstEventType type) { return gst_pad_get_sticky_event(pad(), type, 0); }
    bool sendEvent(GstEvent *event) { return gst_pad_send_event (pad(), event); }

    template<auto Member, typename T>
    void addProbe(T *instance, GstPadProbeType type) {
        struct Impl {
            static GstPadProbeReturn callback(GstPad *pad, GstPadProbeInfo *info, gpointer userData) {
                return (static_cast<T *>(userData)->*Member)(QGstPad(pad, NeedsRef), info);
            };
        };

        gst_pad_add_probe (pad(), type, Impl::callback, instance, nullptr);
    }

    void doInIdleProbe(std::function<void()> work) {
        struct CallbackData {
            QSemaphore waitDone;
            std::function<void()> work;
        } cd;
        cd.work = work;

        auto callback= [](GstPad *, GstPadProbeInfo *, gpointer p) {
            auto cd = reinterpret_cast<CallbackData*>(p);
            cd->work();
            cd->waitDone.release();
            return GST_PAD_PROBE_REMOVE;
        };

        gst_pad_add_probe(pad(), GST_PAD_PROBE_TYPE_IDLE, callback, &cd, nullptr);
        cd.waitDone.acquire();
    }

    template<auto Member, typename T>
    void addEosProbe(T *instance) {
        struct Impl {
            static GstPadProbeReturn callback(GstPad */*pad*/, GstPadProbeInfo *info, gpointer userData) {
                if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) != GST_EVENT_EOS)
                    return GST_PAD_PROBE_PASS;
                (static_cast<T *>(userData)->*Member)();
                return GST_PAD_PROBE_REMOVE;
            };
        };

        gst_pad_add_probe (pad(), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, Impl::callback, instance, nullptr);
    }
};

class QGstClock : public QGstObject
{
public:
    QGstClock() = default;
    QGstClock(const QGstObject &o)
        : QGstClock(GST_CLOCK(o.object()))
    {}
    QGstClock(GstClock *clock, RefMode mode = NeedsRef)
        : QGstObject(&clock->object, mode)
    {}

    GstClock *clock() const { return GST_CLOCK_CAST(object()); }

    GstClockTime time() const { return gst_clock_get_time(clock()); }
};

class QGstElement : public QGstObject
{
public:
    QGstElement() = default;
    QGstElement(const QGstObject &o)
        : QGstElement(GST_ELEMENT(o.object()), NeedsRef)
    {}
    QGstElement(GstElement *element, RefMode mode = NeedsRef)
        : QGstObject(&element->object, mode)
    {}

    QGstElement(const char *factory, const char *name = nullptr)
        : QGstElement(gst_element_factory_make(factory, name), NeedsRef)
    {
    }

    bool linkFiltered(const QGstElement &next, const QGstMutableCaps &caps)
    { return gst_element_link_filtered(element(), next.element(), caps.get()); }
    bool link(const QGstElement &next)
    { return gst_element_link(element(), next.element()); }
    bool link(const QGstElement &n1, const QGstElement &n2)
    { return gst_element_link_many(element(), n1.element(), n2.element(), nullptr); }
    bool link(const QGstElement &n1, const QGstElement &n2, const QGstElement &n3)
    { return gst_element_link_many(element(), n1.element(), n2.element(), n3.element(), nullptr); }
    bool link(const QGstElement &n1, const QGstElement &n2, const QGstElement &n3, const QGstElement &n4)
    { return gst_element_link_many(element(), n1.element(), n2.element(), n3.element(), n4.element(), nullptr); }
    bool link(const QGstElement &n1, const QGstElement &n2, const QGstElement &n3, const QGstElement &n4, const QGstElement &n5)
    { return gst_element_link_many(element(), n1.element(), n2.element(), n3.element(), n4.element(), n5.element(), nullptr); }

    void unlink(const QGstElement &next)
    { gst_element_unlink(element(), next.element()); }

    QGstPad staticPad(const char *name) const { return QGstPad(gst_element_get_static_pad(element(), name), HasRef); }
    QGstPad src() const { return staticPad("src"); }
    QGstPad sink() const { return staticPad("sink"); }
    QGstPad getRequestPad(const char *name) const { return QGstPad(gst_element_get_request_pad(element(), name), HasRef); }
    void releaseRequestPad(const QGstPad &pad) const { return gst_element_release_request_pad(element(), pad.pad()); }

    GstState state() const
    {
        GstState state;
        gst_element_get_state(element(), &state, nullptr, 0);
        return state;
    }
    GstStateChangeReturn setState(GstState state) { return gst_element_set_state(element(), state); }
    bool setStateSync(GstState state)
    {
        auto change = gst_element_set_state(element(), state);
        if (change == GST_STATE_CHANGE_ASYNC) {
            change = gst_element_get_state(element(), nullptr, &state, 1000*1e6 /*nano seconds*/);
        }
#ifndef QT_NO_DEBUG
        if (change != GST_STATE_CHANGE_SUCCESS && change != GST_STATE_CHANGE_NO_PREROLL)
            qWarning() << "Could not change state of" << name() << "to" << state << change;
#endif
        return change == GST_STATE_CHANGE_SUCCESS;
    }
    bool syncStateWithParent() { return gst_element_sync_state_with_parent(element()) == TRUE; }
    bool finishStateChange()
    {
        auto change = gst_element_get_state(element(), nullptr, nullptr, 1000*1e6 /*nano seconds*/);
#ifndef QT_NO_DEBUG
        if (change != GST_STATE_CHANGE_SUCCESS && change != GST_STATE_CHANGE_NO_PREROLL)
            qWarning() << "Could finish change state of" << name();
#endif
        return change == GST_STATE_CHANGE_SUCCESS;
    }

    void lockState(bool locked) { gst_element_set_locked_state(element(), locked); }
    bool isStateLocked() const { return gst_element_is_locked_state(element()); }

    void sendEvent(GstEvent *event) const { gst_element_send_event(element(), event); }
    void sendEos() const { sendEvent(gst_event_new_eos()); }

    template<auto Member, typename T>
    void onPadAdded(T *instance) {
        struct Impl {
            static void callback(GstElement *e, GstPad *pad, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstElement(e), QGstPad(pad, NeedsRef));
            };
        };

        connect("pad-added", G_CALLBACK(Impl::callback), instance);
    }
    template<auto Member, typename T>
    void onPadRemoved(T *instance) {
        struct Impl {
            static void callback(GstElement *e, GstPad *pad, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstElement(e), QGstPad(pad, NeedsRef));
            };
        };

        connect("pad-removed", G_CALLBACK(Impl::callback), instance);
    }
    template<auto Member, typename T>
    void onNoMorePads(T *instance) {
        struct Impl {
            static void callback(GstElement *e, gpointer userData) {
                (static_cast<T *>(userData)->*Member)(QGstElement(e));
            };
        };

        connect("no-more-pads", G_CALLBACK(Impl::callback), instance);
    }

    GstClockTime baseTime() const { return gst_element_get_base_time(element()); }
    void setBaseTime(GstClockTime time) const { gst_element_set_base_time(element(), time); }

    GstElement *element() const { return GST_ELEMENT_CAST(m_object); }
};

inline QGstElement QGstPad::parent() const
{
    return QGstElement(gst_pad_get_parent_element(pad()), HasRef);
}

class QGstBin : public QGstElement
{
public:
    QGstBin() = default;
    QGstBin(const QGstObject &o)
        : QGstBin(GST_BIN(o.object()), NeedsRef)
    {}
    QGstBin(const char *name)
        : QGstElement(gst_bin_new(name), NeedsRef)
    {
    }
    QGstBin(GstBin *bin, RefMode mode = NeedsRef)
        : QGstElement(&bin->element, mode)
    {}

    void add(const QGstElement &element)
    { gst_bin_add(bin(), element.element()); }
    void add(const QGstElement &e1, const QGstElement &e2)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), nullptr); }
    void add(const QGstElement &e1, const QGstElement &e2, const QGstElement &e3)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), e3.element(), nullptr); }
    void add(const QGstElement &e1, const QGstElement &e2, const QGstElement &e3, const QGstElement &e4)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), e3.element(), e4.element(), nullptr); }
    void add(const QGstElement &e1, const QGstElement &e2, const QGstElement &e3, const QGstElement &e4, const QGstElement &e5)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), e3.element(), e4.element(), e5.element(), nullptr); }
    void add(const QGstElement &e1, const QGstElement &e2, const QGstElement &e3, const QGstElement &e4, const QGstElement &e5, const QGstElement &e6)
    { gst_bin_add_many(bin(), e1.element(), e2.element(), e3.element(), e4.element(), e5.element(), e6.element(), nullptr); }
    void remove(const QGstElement &element)
    { gst_bin_remove(bin(), element.element()); }

    GstBin *bin() const { return GST_BIN_CAST(m_object); }

    void addGhostPad(const QGstElement &child, const char *name)
    {
        addGhostPad(name, child.staticPad(name));
    }
    void addGhostPad(const char *name, const QGstPad &pad)
    {
        gst_element_add_pad(element(), gst_ghost_pad_new(name, pad.pad()));
    }
};

inline QGstStructure QGValue::toStructure() const
{
    if (!value || !GST_VALUE_HOLDS_STRUCTURE(value))
        return QGstStructure();
    return QGstStructure(gst_value_get_structure(value));
}

inline QGstCaps QGValue::toCaps() const
{
    if (!value || !GST_VALUE_HOLDS_CAPS(value))
        return QGstCaps();
    return QGstCaps(gst_value_get_caps(value));
}

QT_END_NAMESPACE

#endif
