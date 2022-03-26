/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "testcpp.h"

namespace TestQDoc {

/*
//! [random tag]
\note This is just a test.
//! [random tag]
*/

/*!
    \module TestCPP
    \qtvariable testcpp
    \qtcmakepackage QDocTest
    \title QDoc Test C++ Classes
    \brief A test module page.

    \testnoautolist

    \include testcpp.cpp random tag

\if defined(test_nestedmacro)
    \versionnote {module} {\ver}
\endif

    \section1 Linking to function-like things

    \list
        \li \l {TestQDoc::Test::someFunctionDefaultArg}
               {someFunctionDefaultArg()}
        \li QDOCTEST_MACRO2()
        \li \l {TestQDoc::Test::}{QDOCTEST_MACRO2(int &x)}
        \li \l {section()}
        \li \l {section()} {section() is a section title}
        \li \l {TestQDoc::Test::Test()} {open( parenthesis}
        \li \l {https://en.cppreference.com/w/cpp/utility/move}
               {C++11 added std::move(T&& t)}
    \endlist

    \section2 section()
*/

/*!
    \namespace TestQDoc
    \inheaderfile TestCPP
    \inmodule TestCPP
    \brief A namespace.

    \section1 Usage
    This namespace is for testing QDoc output.
*/

/*!
    \class TestQDoc::Test
    \inmodule TestCPP
    \brief A class in a namespace.

\if defined(test_ignoresince)
    //! omitted by ignoresince
    \since 1.1
\endif
    \ingroup testgroup
*/

/*!
    \fn TestQDoc::Test::Test()

    Default constructor.
*/

/*!
    \fn Test &Test::operator=(Test &&other)

    Move-assigns \a other.
*/

/*!
    \class TestQDoc::TestDerived
    \inmodule TestCPP
    \brief A derived class in a namespace.
*/

/*!
    \macro QDOCTEST_MACRO
    \relates TestQDoc
\if defined(test_ignoresince)
    //! omitted by ignoresince.Test
    \since Test 0.9
\endif
*/

/*!
    \macro QDOCTEST_MACRO2(int &x)
    \relates TestQDoc::Test
    \since Test 1.1
    \brief A macro with argument \a x.
    \ingroup testgroup
*/

/*!
\if defined(test_properties)
    \property Test::id
\else
    \nothing
\endif
*/

/*!
    \deprecated [6.0] Use someFunction() instead.
*/
void Test::deprecatedMember()
{
    return;
}

/*!
    \obsolete

    Use someFunction() instead.
*/
void Test::obsoleteMember()
{
    return;
}

/*!
    \obsolete Use obsoleteMember() instead.
*/
void Test::anotherObsoleteMember()
{
    return;
}

/*!
    Function that takes a parameter \a i and \a b.
\if defined(test_ignoresince)
    \since 2.0
\endif
    \ingroup testgroup
*/
void Test::someFunctionDefaultArg(int i, bool b = false)
{
    return;
}

/*!
    \fn void Test::func(bool)
    \internal
*/

/*!
    \fn [funcPtr] void (*funcPtr(bool b, const char *s))(bool)

    Returns a pointer to a function that takes a boolean. Uses \a b and \a s.
*/

/*!
    \fn [op-inc] Test::operator++()
    \fn [op-dec] Test::operator--()
    \deprecated
*/

// Documented below with an \fn command. Unnecessary but we support it, and it's used.
int Test::someFunction(int, int v)
{
    return v;
}

/*!
    \fn void TestQDoc::Test::inlineFunction()

    \brief An inline function, documented using the \CMDFN QDoc command.
*/

/*!
    \fn int Test::someFunction(int, int v = 0)

    Function that takes a parameter \a v.
    Also returns the value of \a v.
\if defined(test_ignoresince)
    \since Test 1.0
\endif
*/

/*!
    Function that must be reimplemented.
*/
void Test::virtualFun()
{
    return;
}

/*!
    \fn bool Test::operator==(const Test &lhs, const Test &rhs)

    Returns true if \a lhs and \a rhs are equal.
*/

/*!
    \typedef Test::SomeType
    \brief A typedef.
*/

/*!
    \reimp
*/
void TestDerived::virtualFun()
{
    return;
}

/*!
    \fn TestQDoc::Test::overload()
    \fn Test::overload(bool b)
     //! The second overload should match even without the fully qualified path

    Overloads that share a documentation comment, optionally taking
    a parameter \a b.
*/

/*!
    \fn Test::overload(bool b)
    \since Test 1.2
*/

/*!
    \typealias TestDerived::DerivedType
    An aliased typedef.
*/

/*!
    \typedef TestDerived::NotTypedef
    I'm an alias, not a typedef.
*/

/*!
    \obsolete

    Static obsolete method.
*/
void TestDerived::staticObsoleteMember()
{
    return;
}

/*!
\if defined(test_properties)
    \fn void TestDerived::emitSomething()
    Emitted when things happen.
\else
    \nothing
\endif
*/

/*!
\if defined(test_properties)
    \reimp
\else
    \nothing
\endif
*/
int TestDerived::id()
{
    return 1;
}

/*!
\if defined(test_template)
    \fn template <typename T1, typename T2> void TestQDoc::Test::funcTemplate(T1 a, T2 b)
    \brief Function template with two parameters, \a a and \a b.
\else
    \nothing
\endif
*/

/*!
\if defined(test_template)
    \struct TestQDoc::Test::Struct
    \inmodule TestCPP
    \brief Templated struct.
\else
    \nothing
\endif
*/

/*!
\if defined(test_template)
    \typealias TestQDoc::Test::Specialized
\else
    \nothing
\endif
*/

/*!
\if defined(test_template)
    \class TestQDoc::Vec
    \inmodule TestCPP
    \brief Type alias that has its own reference.
\else
    \nothing
\endif
*/

/*!
\if defined(test_template)
    \macro Q_INVOKABLE
    \relates TestQDoc::Test

    This is a mock Q_INVOKABLE for the purpose of ensuring QDoc autolink to it
    as expected.
\else
    \nothing
\endif
*/

} // namespace TestQDoc


/*!
    \namespace CrossModuleRef
    \inmodule TestCPP
    \brief Namespace that has documented functions in multiple modules.
*/
namespace CrossModuleRef {

/*!
    Document me!
*/
void documentMe()
{
}

} // namespace CrossModuleRef

/*!
    \class DontLinkToMe
    \inmodule TestCPP
    \brief Class that does not generate documentation.
*/

/*!
    \dontdocument (DontLinkToMe)
*/
