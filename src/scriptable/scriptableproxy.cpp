/*
    Copyright (c) 2014, Lukas Holecek <hluk@email.cz>

    This file is part of CopyQ.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "scriptable.h"

#include "common/log.h"
#include "gui/mainwindow.h"
#include "item/clipboarditem.h"
#include "platform/platformnativeinterface.h"

#include <QDialog>

#define BROWSER(call) \
    ClipboardBrowser *c = fetchBrowser(); \
    if (c) \
        c->call

#define BROWSER_RESULT(call) \
    ClipboardBrowser *c = fetchBrowser(); \
    v = c ? QVariant(c->call) : QVariant()

namespace detail {

QByteArray serializeWindow(WId winId)
{
    QByteArray data;
    return createPlatformNativeInterface()->serialize(winId, &data) ? data : QByteArray();
}

ScriptableProxyHelper::ScriptableProxyHelper(MainWindow *mainWindow)
    : QObject(NULL)
    , m_wnd(mainWindow)
    , m_tabName()
    , m_lock()
{
    qRegisterMetaType< QPointer<QWidget> >("QPointer<QWidget>");
    moveToThread(m_wnd->thread());
}

const QVariant &ScriptableProxyHelper::value() const
{
    return v;
}

QString ScriptableProxyHelper::tabNotFoundError()
{
    return ScriptableProxyHelper::tr("Tab with given name doesn't exist!");
}

QString ScriptableProxyHelper::tabNameEmptyError()
{
    return ScriptableProxyHelper::tr("Tab name cannot be empty!");
}

void ScriptableProxyHelper::close()
{
    m_wnd->close();
}

void ScriptableProxyHelper::showWindow()
{
    m_wnd->showWindow();
}

void ScriptableProxyHelper::pasteToCurrentWindow()
{
    PlatformWindowPtr window = createPlatformNativeInterface()->getCurrentWindow();
    v = !window.isNull();
    if (window)
        window->pasteClipboard();
}

void ScriptableProxyHelper::copyFromCurrentWindow()
{
    PlatformWindowPtr window = createPlatformNativeInterface()->getCurrentWindow();
    v = !window.isNull();
    if (window)
        window->copy();
}

void ScriptableProxyHelper::ignoreCurrentClipboard()
{
    m_wnd->ignoreCurrentClipboard();
}

void ScriptableProxyHelper::isMonitoringEnabled()
{
    v = m_wnd->isMonitoringEnabled();
}

void ScriptableProxyHelper::disableMonitoring(bool arg1)
{
    m_wnd->disableClipboardStoring(arg1);
}

void ScriptableProxyHelper::setClipboard(const QVariantMap &arg1)
{
    m_wnd->setClipboard(arg1);
}

void ScriptableProxyHelper::renameTab(const QString &arg1, const QString &arg2)
{
    v = QString();

    if ( arg1.isEmpty() || arg2.isEmpty() ) {
        v = tabNameEmptyError();
        return;
    }

    const int i = m_wnd->findTabIndex(arg2);
    if (i == -1) {
        v = tabNotFoundError();
        return;
    }

    if ( m_wnd->findTabIndex(arg1) != -1 ) {
        v = tr("Tab with given name already exists!");
        return;
    }

    m_wnd->renameTab(arg1, i);
}

void ScriptableProxyHelper::removeTab(const QString &arg1)
{
    v = QString();

    if ( arg1.isEmpty() ) {
        v = tabNameEmptyError();
        return;
    }

    const int i = m_wnd->findTabIndex(arg1);
    if (i == -1) {
        v = tabNotFoundError();
        return;
    }

    m_wnd->removeTab(false, i);
}

void ScriptableProxyHelper::showBrowser(const QString &tabName)
{
    ClipboardBrowser *c = fetchBrowser(tabName);
    if (c)
        m_wnd->showBrowser(c);
}

void ScriptableProxyHelper::showBrowser()
{
    showBrowser(m_tabName);
}

void ScriptableProxyHelper::action(const QVariantMap &arg1, const Command &arg2)
{
    m_wnd->action(arg1, arg2);
}

void ScriptableProxyHelper::showMessage(const QString &arg1, const QString &arg2, QSystemTrayIcon::MessageIcon arg3, int arg4)
{
    m_wnd->showMessage(arg1, arg2, arg3, arg4);
}

void ScriptableProxyHelper::browserLock()
{
    Q_ASSERT(m_lock.isNull());
    ClipboardBrowser *c = fetchBrowser();
    if (c)
        m_lock.reset( new ClipboardBrowser::Lock(c) );
}

void ScriptableProxyHelper::browserUnlock()
{
    Q_ASSERT(!m_lock.isNull());
    m_lock.reset();
}

void ScriptableProxyHelper::browserCopyNextItemToClipboard()
{
    BROWSER(copyNextItemToClipboard());
}

void ScriptableProxyHelper::browserCopyPreviousItemToClipboard()
{
    BROWSER(copyPreviousItemToClipboard());
}

void ScriptableProxyHelper::browserMoveToClipboard(int arg1)
{
    BROWSER(moveToClipboard(arg1));
}

void ScriptableProxyHelper::browserSetCurrent(int arg1)
{
    BROWSER(setCurrent(arg1));
}

void ScriptableProxyHelper::browserRemoveRows(QList<int> rows)
{
    ClipboardBrowser *c = fetchBrowser();
    if (!c)
        return;

    qSort( rows.begin(), rows.end(), qGreater<int>() );

    ClipboardBrowser::Lock lock(c);
    foreach (int row, rows)
        c->removeRow(row);
}

void ScriptableProxyHelper::browserEditRow(int arg1)
{
    BROWSER(editRow(arg1));
}

void ScriptableProxyHelper::browserEditNew(const QString &arg1)
{
    BROWSER(editNew(arg1));
}

void ScriptableProxyHelper::tabs()
{
    v = m_wnd->tabs();
}

void ScriptableProxyHelper::toggleVisible()
{
    v = m_wnd->toggleVisible();
}

void ScriptableProxyHelper::toggleMenu(const QString &tabName)
{
    v = m_wnd->toggleMenu(fetchBrowser(tabName));
}

void ScriptableProxyHelper::toggleMenu()
{
    v = m_wnd->toggleMenu();
}

void ScriptableProxyHelper::mainWinId()
{
    v = serializeWindow(m_wnd->winId());
}

void ScriptableProxyHelper::trayMenuWinId()
{
    v = serializeWindow(m_wnd->trayMenu()->winId());
}

void ScriptableProxyHelper::findTabIndex(const QString &arg1)
{
    v = m_wnd->findTabIndex(arg1);
}

void ScriptableProxyHelper::openActionDialog(const QVariantMap &arg1)
{
    v = (qulonglong)m_wnd->openActionDialog(arg1);
}

void ScriptableProxyHelper::loadTab(const QString &arg1)
{
    v = m_wnd->loadTab(arg1);
}

void ScriptableProxyHelper::saveTab(const QString &arg1)
{
    v = QString();
    ClipboardBrowser *c = fetchBrowser();
    if (c) {
        const int i = m_wnd->findTabIndex( c->tabName() );
        v = m_wnd->saveTab(arg1, i);
    }
}

void ScriptableProxyHelper::config(const QString &arg1, const QString &arg2)
{
    v = m_wnd->config(arg1, arg2);
}

void ScriptableProxyHelper::getClipboardData(const QString &arg1)
{
    v = m_wnd->getClipboardData(arg1);
}

void ScriptableProxyHelper::getClipboardData(const QString &arg1, QClipboard::Mode arg2)
{
    v = m_wnd->getClipboardData(arg1, arg2);
}

void ScriptableProxyHelper::getActionData(const QByteArray &arg1, const QString &arg2)
{
    v = m_wnd->getActionData(arg1, arg2);
}

void ScriptableProxyHelper::browserLength()
{
    BROWSER_RESULT(length());
}

void ScriptableProxyHelper::browserOpenEditor(const QByteArray &arg1)
{
    BROWSER_RESULT(openEditor(arg1));
}

void ScriptableProxyHelper::browserAdd(const QString &arg1)
{
    BROWSER_RESULT(add(arg1));
}

void ScriptableProxyHelper::browserAdd(const QVariantMap &arg1, int arg2)
{
    BROWSER_RESULT(add(arg1, arg2));
}

void ScriptableProxyHelper::browserAdd(const QStringList &texts) {
    ClipboardBrowser *c = fetchBrowser();
    if (!c) {
        v = false;
        return;
    }

    v = true;

    ClipboardBrowser::Lock lock(c);
    foreach (const QString &text, texts) {
        if ( !c->add(text) ) {
            v = false;
            return;
        }
    }

    return;
}

void ScriptableProxyHelper::browserItemData(int arg1, const QString &arg2)
{
    BROWSER_RESULT(itemData(arg1, arg2));
}

void ScriptableProxyHelper::browserItemData(int arg1)
{
    BROWSER_RESULT(itemData(arg1));
}

void ScriptableProxyHelper::setCurrentTab(const QString &tabName)
{
    m_tabName = tabName;
}

void ScriptableProxyHelper::currentTab()
{
    BROWSER_RESULT(tabName());
}

void ScriptableProxyHelper::currentItem()
{
    BROWSER_RESULT(currentIndex().row());
}

void ScriptableProxyHelper::selectItems(const QList<int> &items)
{
    v = false;

    ClipboardBrowser *c = fetchBrowser();
    if (!c)
        return;

    v = true;

    c->clearSelection();

    if ( !items.isEmpty() ) {
        c->setCurrent(items.last());

        foreach (int i, items) {
            const QModelIndex index = c->index(i);
            if (index.isValid())
                c->selectionModel()->select(index, QItemSelectionModel::Select);
        }
    }
}

void ScriptableProxyHelper::selectedTab()
{
    v = m_wnd->selectedTab();
}

void ScriptableProxyHelper::selectedItems()
{
    v = QVariant::fromValue(m_wnd->selectedItems());
}

void ScriptableProxyHelper::sendKeys(const QString &keys)
{
#ifdef HAS_TESTS
    v = QString();

    if (keys == "FLUSH_KEYS")
        return;

    QWidget *w = m_wnd->trayMenu();

    if ( !w->isVisible() ) {
        w = QApplication::focusWidget();
        if (!w) {
            COPYQ_LOG("No focused widget -> using main window");
            w = m_wnd;
        }
    }

    if (keys.startsWith(":")) {
        const QString widgetName = QString("%1 in %2")
                .arg(w->metaObject()->className())
                .arg(w->window()->metaObject()->className());

        COPYQ_LOG( QString("Sending keys \"%1\" to \"%2\".")
                   .arg(keys.mid(1))
                   .arg(widgetName) );

        QTest::keyClicks(w, keys.mid(1), Qt::NoModifier, 50);
    } else {
        const QKeySequence shortcut(keys, QKeySequence::PortableText);

        if ( shortcut.isEmpty() ) {
            v = QString("Cannot parse key \"%1\"!").arg(keys);
            return;
        }

        // Don't stop when modal window is open.
        QMetaObject::invokeMethod( this, "keyClick", Qt::QueuedConnection,
                                   Q_ARG(const QKeySequence &, shortcut),
                                   Q_ARG(const QPointer<QWidget> &, w)
                                   );
    }
#else
    Q_UNUSED(keys);
    v = QString("This is only available if tests are compiled!");
#endif
}

void ScriptableProxyHelper::keyClick(const QKeySequence &shortcut, const QPointer<QWidget> &widget)
{
#ifdef HAS_TESTS
    const QString keys = shortcut.toString();

    if (widget.isNull()) {
        COPYQ_LOG( QString("Failed to send key \"%1\".").arg(keys) );
        return;
    }

    const QString widgetName = QString("%1 in %2")
            .arg(widget->metaObject()->className())
            .arg(widget->window()->metaObject()->className());

    m_wnd->showMessage( widgetName, shortcut.toString(),
                        QSystemTrayIcon::Information, 4000 );

    COPYQ_LOG( QString("Sending key \"%1\" to \"%2\".")
               .arg(keys)
               .arg(widgetName) );

    QTest::keyClick( widget.data(),
                     Qt::Key(shortcut[0] & ~Qt::KeyboardModifierMask),
            Qt::KeyboardModifiers(shortcut[0] & Qt::KeyboardModifierMask), 0 );

    COPYQ_LOG( QString("Key \"%1\" sent to \"%2\".")
               .arg(keys)
               .arg(widgetName) );
#else
    Q_UNUSED(shortcut);
    Q_UNUSED(widget);
#endif
}

void ScriptableProxyHelper::currentWindowTitle()
{
    PlatformWindowPtr window = createPlatformNativeInterface()->getCurrentWindow();
    v = window ? window->getTitle() : QString();
}

} // namespace detail

ClipboardBrowser *detail::ScriptableProxyHelper::fetchBrowser(const QString &tabName)
{
    ClipboardBrowser *c = tabName.isEmpty() ? m_wnd->browser(0) : m_wnd->createTab(tabName);
    if (!c)
        return NULL;

    c->loadItems();
    return c->isLoaded() ? c : NULL;
}

ClipboardBrowser *detail::ScriptableProxyHelper::fetchBrowser() { return fetchBrowser(m_tabName); }

ScriptableProxy::ScriptableProxy(MainWindow *mainWindow)
    : m_helper(new detail::ScriptableProxyHelper(mainWindow))
{
    qRegisterMetaType<QSystemTrayIcon::MessageIcon>("SystemTrayIcon::MessageIcon");
}

ScriptableProxy::~ScriptableProxy() { delete m_helper; }
