/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QITERABLE_H
#define QITERABLE_H

#include <QtCore/qglobal.h>
#include <QtCore/qtypeinfo.h>
#include <QtCore/qmetacontainer.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qtaggedpointer.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
    template<typename Type, typename Storage = Type>
    class QConstPreservingPointer
    {
        enum Tag : bool { Const, Mutable };
        QTaggedPointer<Storage, Tag> m_pointer;

    public:
        QConstPreservingPointer(std::nullptr_t) : m_pointer(nullptr, Const) {}

        QConstPreservingPointer(const void *pointer, qsizetype alignment)
            : m_pointer(reinterpret_cast<Storage *>(const_cast<void *>(pointer)), Const)
        {
            Q_UNUSED(alignment);
            Q_ASSERT(alignment > qsizetype(alignof(Storage)));
        }

        QConstPreservingPointer(void *pointer, qsizetype alignment)
            : m_pointer(reinterpret_cast<Storage *>(pointer), Mutable)
        {
            Q_UNUSED(alignment);
            Q_ASSERT(alignment > qsizetype(alignof(Storage)));
        }

        template<typename InputType>
        QConstPreservingPointer(const InputType *pointer)
            : m_pointer(reinterpret_cast<Storage *>(const_cast<InputType *>(pointer)), Const)
        {
            static_assert(alignof(InputType) >= alignof(Storage));
        }

        template<typename InputType>
        QConstPreservingPointer(InputType *pointer)
            : m_pointer(reinterpret_cast<Storage *>(pointer), Mutable)
        {
            static_assert(alignof(InputType) >= alignof(Storage));
        }

        QConstPreservingPointer() = default;

        const Type *constPointer() const
        {
            return reinterpret_cast<const Type *>(m_pointer.data());
        }

        Type *mutablePointer() const
        {
            return m_pointer.tag() == Mutable ? reinterpret_cast<Type *>(m_pointer.data()) : nullptr;
        }
    };
}

class Q_CORE_EXPORT QSequentialIterable
{
    uint m_revision = 0;
    QtPrivate::QConstPreservingPointer<void, quint16> m_iterable;
    QMetaSequence m_metaSequence;

public:
    struct Q_CORE_EXPORT const_iterator
    {
    private:
        const QSequentialIterable *m_iterable = nullptr;
        void *m_iterator = nullptr;
        QAtomicInt *m_ref = nullptr;

        friend class QSequentialIterable;
        explicit const_iterator(const QSequentialIterable *iterable, void *iterator);

    public:
        ~const_iterator();

        const_iterator(const const_iterator &other);

        const_iterator& operator=(const const_iterator &other);

        const QVariant operator*() const;
        bool operator==(const const_iterator &o) const;
        bool operator!=(const const_iterator &o) const;
        const_iterator &operator++();
        const_iterator operator++(int);
        const_iterator &operator--();
        const_iterator operator--(int);
        const_iterator &operator+=(int j);
        const_iterator &operator-=(int j);
        const_iterator operator+(int j) const;
        const_iterator operator-(int j) const;
        friend inline const_iterator operator+(int j, const const_iterator &k) { return k + j; }
    };

    friend struct const_iterator;

    template<class T>
    QSequentialIterable(const T *p)
      : m_iterable(p)
      , m_metaSequence(QMetaSequence::fromContainer<T>())
    {
        Q_UNUSED(m_revision);
    }

    QSequentialIterable() = default;

    template<typename Pointer>
    QSequentialIterable(const QMetaSequence &metaSequence, Pointer iterable)
        : m_iterable(iterable)
        , m_metaSequence(metaSequence)
    {
    }

    const_iterator begin() const;
    const_iterator end() const;

    QVariant at(qsizetype idx) const;
    qsizetype size() const;

    enum Position { Unspecified, AtBegin, AtEnd };
    void addValue(const QVariant &value, Position position = Unspecified);
    void removeValue(Position position = Unspecified);

    bool canForwardIterate() const;
    bool canReverseIterate() const;

    const void *constIterable() const { return m_iterable.constPointer(); }

    // TODO: fix this when introducing mutable iterables
    void *mutableIterable() { return const_cast<void *>(m_iterable.constPointer()); }

    QMetaSequence metaSequence() const { return m_metaSequence; }
    QMetaType valueMetaType() const { return m_metaSequence.valueMetaType(); }
};

namespace QtPrivate {

template<typename From>
struct QSequentialIterableConvertFunctor
{
    QSequentialIterable operator()(const From &f) const
    {
        return QSequentialIterable(&f);
    }
};

template<typename T>
struct SequentialValueTypeIsMetaType<T, true>
{
    static bool registerConverter(int id)
    {
        const int toId = qMetaTypeId<QSequentialIterable>();
        if (!QMetaType::hasRegisteredConverterFunction(id, toId)) {
            QSequentialIterableConvertFunctor<T> o;
            return QMetaType::registerConverter<T, QSequentialIterable>(o);
        }
        return true;
    }
};

}

Q_DECLARE_TYPEINFO(QSequentialIterable, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QSequentialIterable::const_iterator, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // QITERABLE_H
