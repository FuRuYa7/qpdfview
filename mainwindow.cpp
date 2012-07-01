/*

Copyright 2012 Adam Reichold

This file is part of qpdfview.

qpdfview is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

qpdfview is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with qpdfview.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    {
        // settings

        m_settings = new QSettings(this);

        PageItem::setCacheSize(m_settings->value("pageItem/cacheSize", 32 * 1024 * 1024).toInt());

        PageItem::setDecoratePages(m_settings->value("pageItem/decoratePages", true).toBool());
        PageItem::setDecorateLinks(m_settings->value("pageItem/decorateLinks", true).toBool());

        DocumentView::setPageSpacing(m_settings->value("documentView/pageSpacing", 5.0).toReal());
        DocumentView::setThumbnailSpacing(m_settings->value("documentView/thumbnailSpacing", 3.0).toReal());

        DocumentView::setThumbnailSize(m_settings->value("documentView/thumbnailSize", 150.0).toReal());

        if(m_settings->contains("mainWindow/iconTheme"))
        {
            QIcon::setThemeName(m_settings->value("mainWindow/iconTheme").toString());
        }
    }

    setAcceptDrops(true);

    createWidgets();
    createActions();
    createToolBars();
    createDocks();
    createMenus();

    restoreGeometry(m_settings->value("mainWindow/geometry").toByteArray());
    restoreState(m_settings->value("mainWindow/state").toByteArray());

    restoreTabs();
    restoreBookmarks();

    on_tabWidget_currentChanged(m_tabWidget->currentIndex());
}

QSize MainWindow::sizeHint() const
{
    return QSize(600, 800);
}

QMenu* MainWindow::createPopupMenu()
{
    QMenu* menu = new QMenu();

    menu->addAction(m_fileToolBar->toggleViewAction());
    menu->addAction(m_editToolBar->toggleViewAction());
    menu->addAction(m_viewToolBar->toggleViewAction());
    menu->addSeparator();
    menu->addAction(m_outlineDock->toggleViewAction());
    menu->addAction(m_propertiesDock->toggleViewAction());
    menu->addAction(m_thumbnailsDock->toggleViewAction());

    return menu;
}

DocumentView* MainWindow::currentTab() const
{
    return qobject_cast< DocumentView* >(m_tabWidget->currentWidget());
}

DocumentView* MainWindow::tab(int index) const
{
    return qobject_cast< DocumentView* >(m_tabWidget->widget(index));
}

bool MainWindow::open(const QString &filePath, int page)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(currentTab()->open(filePath))
        {
            QFileInfo fileInfo(filePath);

            m_tabWidget->setTabText(m_tabWidget->currentIndex(), fileInfo.completeBaseName());
            m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), fileInfo.absoluteFilePath());

            m_settings->setValue("mainWindow/path", fileInfo.absolutePath());

            currentTab()->jumpToPage(page);
            currentTab()->setFocus();

            return true;
        }
        else
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not open '%1'.").arg(filePath));
        }
    }

    return false;
}

bool MainWindow::openInNewTab(const QString& filePath, int page)
{
    DocumentView* newTab = new DocumentView();

    if(newTab->open(filePath))
    {
        QFileInfo fileInfo(filePath);

        int index = m_tabWidget->addTab(newTab, fileInfo.completeBaseName());
        m_tabWidget->setTabToolTip(index, fileInfo.absoluteFilePath());
        m_tabWidget->setCurrentIndex(index);

        m_settings->setValue("mainWindow/path", fileInfo.absolutePath());

        connect(newTab, SIGNAL(filePathChanged(QString)), SLOT(on_currentTab_filePathChanged(QString)));
        connect(newTab, SIGNAL(numberOfPagesChanged(int)), SLOT(on_currentTab_numberOfPagesChaned(int)));
        connect(newTab, SIGNAL(currentPageChanged(int)), SLOT(on_currentTab_currentPageChanged(int)));

        connect(newTab, SIGNAL(continousModeChanged(bool)), SLOT(on_currentTab_continuousModeChanged(bool)));
        connect(newTab, SIGNAL(scaleModeChanged(DocumentView::ScaleMode)), SLOT(on_currentTab_scaleModeChanged(DocumentView::ScaleMode)));
        connect(newTab, SIGNAL(scaleFactorChanged(qreal)), SLOT(on_currentTab_scaleFactorChanged(qreal)));

        connect(newTab, SIGNAL(highlightAllChanged(bool)), SLOT(on_currentTab_highlightAllChanged(bool)));

        connect(newTab, SIGNAL(searchProgressed(int)), SLOT(on_currentTab_searchProgressed(int)));
        connect(newTab, SIGNAL(searchFinished()), SLOT(on_currentTab_searchFinished()));
        connect(newTab, SIGNAL(searchCanceled()), SLOT(on_currentTab_searchCanceled()));

        QAction* tabAction = new QAction(m_tabWidget->tabText(index), newTab);
        connect(tabAction, SIGNAL(triggered()), SLOT(on_tab_triggered()));

        m_tabsMenu->addAction(tabAction);

        newTab->jumpToPage(page);
        newTab->setFocus();

        return true;
    }
    else
    {
        delete newTab;

        QMessageBox::warning(this, tr("Warning"), tr("Could not open '%1'.").arg(filePath));
    }

    return false;
}

bool MainWindow::refreshOrOpenInNewTab(const QString& filePath, int page)
{
    for(int index = 0; index < m_tabWidget->count(); index++)
    {
        if(QFileInfo(tab(index)->filePath()).absoluteFilePath() == QFileInfo(filePath).absoluteFilePath())
        {
            m_tabWidget->setCurrentIndex(index);

            if(currentTab()->refresh())
            {
                currentTab()->jumpToPage(page);
                currentTab()->setFocus();

                return true;
            }
        }
    }

    return openInNewTab(filePath, page);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if(index != -1)
    {
        m_refreshAction->setEnabled(true);
        m_saveCopyAction->setEnabled(true);
        m_printAction->setEnabled(true);

        m_previousPageAction->setEnabled(true);
        m_nextPageAction->setEnabled(true);
        m_firstPageAction->setEnabled(true);
        m_lastPageAction->setEnabled(true);

        m_jumpToPageAction->setEnabled(true);

        m_searchAction->setEnabled(true);
        m_findPreviousAction->setEnabled(true);
        m_findNextAction->setEnabled(true);
        m_cancelSearchAction->setEnabled(true);

        m_continuousModeAction->setEnabled(true);
        m_twoPagesModeAction->setEnabled(true);

        m_zoomInAction->setEnabled(true);
        m_zoomOutAction->setEnabled(true);
        m_originalSizeAction->setEnabled(true);
        m_fitToPageWidthAction->setEnabled(true);
        m_fitToPageSizeAction->setEnabled(true);

        m_rotateLeftAction->setEnabled(true);
        m_rotateRightAction->setEnabled(true);

        m_fontsAction->setEnabled(true);

        m_presentationAction->setEnabled(true);

        m_previousTabAction->setEnabled(true);
        m_nextTabAction->setEnabled(true);
        m_closeTabAction->setEnabled(true);
        m_closeAllTabsAction->setEnabled(true);

        m_addBookmarkAction->setEnabled(true);

        m_currentPageLineEdit->setEnabled(true);
        m_scaleFactorComboBox->setEnabled(true);
        m_searchLineEdit->setEnabled(true);
        m_matchCaseCheckBox->setEnabled(true);
        m_highlightAllCheckBox->setEnabled(true);

        if(m_searchToolBar->isVisible())
        {
            m_searchTimer->stop();

            m_searchLineEdit->setText(QString());
            m_searchLineEdit->setProgress(0);

            for(int index = 0; index < m_tabWidget->count(); index++)
            {
                tab(index)->cancelSearch();
            }
        }

        on_currentTab_filePathChanged(currentTab()->filePath());
        on_currentTab_numberOfPagesChaned(currentTab()->numberOfPages());
        on_currentTab_currentPageChanged(currentTab()->currentPage());

        on_currentTab_continuousModeChanged(currentTab()->continousMode());
        on_currentTab_twoPagesModeChanged(currentTab()->twoPagesMode());
        on_currentTab_scaleModeChanged(currentTab()->scaleMode());
        on_currentTab_scaleFactorChanged(currentTab()->scaleFactor());

        on_currentTab_highlightAllChanged(currentTab()->highlightAll());

        m_outlineView->setModel(currentTab()->outlineModel());
        m_propertiesView->setModel(currentTab()->propertiesModel());

        m_thumbnailsView->setScene(currentTab()->thumbnailsScene());
    }
    else
    {
        m_refreshAction->setEnabled(false);
        m_saveCopyAction->setEnabled(false);
        m_printAction->setEnabled(false);

        m_previousPageAction->setEnabled(false);
        m_nextPageAction->setEnabled(false);
        m_firstPageAction->setEnabled(false);
        m_lastPageAction->setEnabled(false);

        m_jumpToPageAction->setEnabled(false);

        m_searchAction->setEnabled(false);
        m_findPreviousAction->setEnabled(false);
        m_findNextAction->setEnabled(false);
        m_cancelSearchAction->setEnabled(false);

        m_continuousModeAction->setEnabled(false);
        m_twoPagesModeAction->setEnabled(false);

        m_zoomInAction->setEnabled(false);
        m_zoomOutAction->setEnabled(false);
        m_originalSizeAction->setEnabled(false);
        m_fitToPageWidthAction->setEnabled(false);
        m_fitToPageSizeAction->setEnabled(false);

        m_rotateLeftAction->setEnabled(false);
        m_rotateRightAction->setEnabled(false);

        m_fontsAction->setEnabled(false);

        m_presentationAction->setEnabled(false);

        m_previousTabAction->setEnabled(false);
        m_nextTabAction->setEnabled(false);
        m_closeTabAction->setEnabled(false);
        m_closeAllTabsAction->setEnabled(false);

        m_addBookmarkAction->setEnabled(false);

        m_currentPageLineEdit->setEnabled(false);
        m_scaleFactorComboBox->setEnabled(false);
        m_searchLineEdit->setEnabled(false);
        m_matchCaseCheckBox->setEnabled(false);
        m_highlightAllCheckBox->setEnabled(false);

        if(m_searchToolBar->isVisible())
        {
            m_searchTimer->stop();

            m_searchToolBar->setVisible(false);

            m_searchLineEdit->setText(QString());
            m_searchLineEdit->setProgress(0);
        }

        setWindowTitle("qpdfview");

        m_currentPageLineEdit->setText(QString());
        m_numberOfPagesLabel->setText(QString());
        m_scaleFactorComboBox->setCurrentIndex(4);

        m_continuousModeAction->setChecked(false);
        m_twoPagesModeAction->setChecked(false);

        m_fitToPageSizeAction->setChecked(false);
        m_fitToPageWidthAction->setChecked(false);

        m_outlineView->setModel(0);
        m_propertiesView->setModel(0);

        m_thumbnailsView->setScene(0);
    }
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    delete m_tabWidget->widget(index);
}

void MainWindow::on_currentTab_filePathChanged(const QString& filePath)
{
    for(int index = 0; index < m_tabWidget->count(); index++)
    {
        if(sender() == m_tabWidget->widget(index))
        {
            QFileInfo fileInfo(filePath);

            m_tabWidget->setTabText(m_tabWidget->currentIndex(), fileInfo.completeBaseName());
            m_tabWidget->setTabToolTip(m_tabWidget->currentIndex(), fileInfo.absoluteFilePath());

            foreach(QAction* tabAction, m_tabsMenu->actions())
            {
                if(tabAction->parent() == m_tabWidget->widget(index))
                {
                    tabAction->setText(m_tabWidget->tabText(index));

                    break;
                }
            }

            break;
        }
    }

    if(senderIsCurrentTab())
    {
        setWindowTitle(m_tabWidget->tabText(m_tabWidget->currentIndex()) + " - qpdfview");
    }
}

void MainWindow::on_currentTab_numberOfPagesChaned(int numberOfPages)
{
    if(senderIsCurrentTab())
    {
        m_currentPageValidator->setRange(1, numberOfPages);
        m_numberOfPagesLabel->setText(tr("of %1").arg(numberOfPages));
    }
}

void MainWindow::on_currentTab_currentPageChanged(int currentPage)
{
    if(senderIsCurrentTab())
    {
        m_currentPageLineEdit->setText(QString::number(currentPage));

        m_thumbnailsView->ensureVisible(currentTab()->thumbnailsItem(currentPage));
    }
}

void MainWindow::on_currentTab_continuousModeChanged(bool continuousMode)
{
    if(senderIsCurrentTab())
    {
        m_continuousModeAction->setChecked(continuousMode);
    }
}

void MainWindow::on_currentTab_twoPagesModeChanged(bool twoPagesMode)
{
    if(senderIsCurrentTab())
    {
        m_twoPagesModeAction->setChecked(twoPagesMode);
    }
}

void MainWindow::on_currentTab_scaleModeChanged(DocumentView::ScaleMode scaleMode)
{
    if(senderIsCurrentTab())
    {
        switch(scaleMode)
        {
        case DocumentView::ScaleFactor:
            m_fitToPageWidthAction->setChecked(false);
            m_fitToPageSizeAction->setChecked(false);

            m_scaleFactorComboBox->setCurrentIndex(-1);

            on_currentTab_scaleFactorChanged(currentTab()->scaleFactor());
            break;
        case DocumentView::FitToPageWidth:
            m_fitToPageWidthAction->setChecked(true);
            m_fitToPageSizeAction->setChecked(false);

            m_scaleFactorComboBox->setCurrentIndex(0);
            break;
        case DocumentView::FitToPageSize:
            m_fitToPageWidthAction->setChecked(false);
            m_fitToPageSizeAction->setChecked(true);

            m_scaleFactorComboBox->setCurrentIndex(1);
            break;
        }
    }
}

void MainWindow::on_currentTab_scaleFactorChanged(qreal scaleFactor)
{
    if(senderIsCurrentTab())
    {
        if(currentTab()->scaleMode() == DocumentView::ScaleFactor)
        {
            m_scaleFactorComboBox->lineEdit()->setText(QString("%1 %").arg(qRound(scaleFactor * 100.0)));
        }
    }
}

void MainWindow::on_currentTab_highlightAllChanged(bool highlightAll)
{
    if(senderIsCurrentTab())
    {
        m_highlightAllCheckBox->setChecked(highlightAll);
    }
}

void MainWindow::on_currentTab_searchProgressed(int progress)
{
    m_searchLineEdit->setProgress(progress);
}

void MainWindow::on_currentTab_searchFinished()
{
    m_searchLineEdit->setProgress(0);
}

void MainWindow::on_currentTab_searchCanceled()
{
    m_searchLineEdit->setProgress(0);
}

void MainWindow::on_currentPage_editingFinished()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        currentTab()->jumpToPage(m_currentPageLineEdit->text().toInt());
    }
}

void MainWindow::on_currentPage_returnPressed()
{
    currentTab()->setFocus();
}

void MainWindow::on_scaleFactor_currentIndexChanged(int index)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        if(index == 0)
        {
            currentTab()->setScaleMode(DocumentView::FitToPageWidth);
        }
        else if(index == 1)
        {
            currentTab()->setScaleMode(DocumentView::FitToPageSize);
        }
        else
        {
            bool ok = false;
            qreal scaleFactor = m_scaleFactorComboBox->itemData(index).toReal(&ok);

            if(ok)
            {
                currentTab()->setScaleFactor(scaleFactor);
                currentTab()->setScaleMode(DocumentView::ScaleFactor);
            }
        }
    }
}

void MainWindow::on_scaleFactor_editingFinished()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        bool ok = false;
        qreal scaleFactor = m_scaleFactorComboBox->lineEdit()->text().toInt(&ok) / 100.0;

        scaleFactor = scaleFactor >= DocumentView::minimumScaleFactor() ? scaleFactor : DocumentView::minimumScaleFactor();
        scaleFactor = scaleFactor <= DocumentView::maximumScaleFactor() ? scaleFactor : DocumentView::maximumScaleFactor();

        if(ok)
        {
            currentTab()->setScaleFactor(scaleFactor);
            currentTab()->setScaleMode(DocumentView::ScaleFactor);
        }

        on_currentTab_scaleFactorChanged(currentTab()->scaleFactor());
        on_currentTab_scaleModeChanged(currentTab()->scaleMode());
    }
}

void MainWindow::on_scaleFactor_returnPressed()
{
    currentTab()->setFocus();
}

void MainWindow::on_open_triggered()
{
    if(m_tabWidget->currentIndex() != -1)
    {
        QString path = m_settings->value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString();
        QString filePath = QFileDialog::getOpenFileName(this, tr("Open"), path, "Portable document format (*.pdf)");

        if(!filePath.isEmpty())
        {
            open(filePath);
        }
    }
    else
    {
        on_openInNewTab_triggered();
    }
}

void MainWindow::on_openInNewTab_triggered()
{
    QString path = m_settings->value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString();
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open in new tab"), path, "Portable document format (*.pdf)");

    if(!filePaths.isEmpty())
    {
        disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        foreach(QString filePath, filePaths)
        {
            openInNewTab(filePath);
        }

        connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        on_tabWidget_currentChanged(m_tabWidget->currentIndex());
    }
}

void MainWindow::on_refresh_triggered()
{
    if(!currentTab()->refresh())
    {
        QMessageBox::warning(this, tr("Warning"), tr("Could not refresh '%1'.").arg(currentTab()->filePath()));
    }
}

void MainWindow::on_saveCopy_triggered()
{
    QString path = m_settings->value("mainWindow/path", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation)).toString();
    QString fileName = QFileInfo(currentTab()->filePath()).fileName();
    QString filePath = QFileInfo(QDir(path), fileName).filePath();

    filePath = QFileDialog::getSaveFileName(this, tr("Save copy"), filePath, "Portable document format (*.pdf)");

    if(!filePath.isEmpty())
    {
        if(!currentTab()->saveCopy(filePath))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not save copy at '%1'.").arg(filePath));
        }
    }
}

void MainWindow::on_print_triggered()
{
    QPrinter* printer = new QPrinter();
    QPrintDialog* printDialog = new QPrintDialog(printer, this);

    printer->setDocName(QFileInfo(currentTab()->filePath()).completeBaseName());
    printer->setFullPage(true);

    printDialog->setMinMax(1, currentTab()->numberOfPages());
    printDialog->setOption(QPrintDialog::PrintToFile, false);

    if(printDialog->exec() == QDialog::Accepted)
    {
        if(!currentTab()->print(printer))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Could not print '%1'.").arg(currentTab()->filePath()));
        }
    }

    delete printer;
    delete printDialog;
}

void MainWindow::on_previousPage_triggered()
{
    currentTab()->previousPage();
}

void MainWindow::on_nextPage_triggered()
{
    currentTab()->nextPage();
}

void MainWindow::on_firstPage_triggered()
{
    currentTab()->firstPage();
}

void MainWindow::on_lastPage_triggered()
{
    currentTab()->lastPage();
}

void MainWindow::on_jumpToPage_triggered()
{
    bool ok = false;
    int page = QInputDialog::getInt(this, tr("Jump to page"), tr("Page:"), currentTab()->currentPage(), 1, currentTab()->numberOfPages(), 1, &ok);

    if(ok)
    {
        currentTab()->jumpToPage(page);
    }
}

void MainWindow::on_search_triggered()
{
    if(!m_searchToolBar->isVisible())
    {
        m_searchToolBar->setVisible(true);
    }
    else
    {
        m_searchLineEdit->selectAll();
    }

    m_searchLineEdit->setFocus();
}

void MainWindow::on_findPrevious_triggered()
{
    if(!m_searchToolBar->isVisible())
    {
        m_searchToolBar->setVisible(true);
        m_searchLineEdit->setFocus();
    }
    else
    {
        if(!m_searchLineEdit->text().isEmpty())
        {
            currentTab()->findPrevious();
        }
    }
}

void MainWindow::on_findNext_triggered()
{
    if(!m_searchToolBar->isVisible())
    {
        m_searchToolBar->setVisible(true);
        m_searchLineEdit->setFocus();
    }
    else
    {
        if(!m_searchLineEdit->text().isEmpty())
        {
            currentTab()->findNext();
        }
    }
}

void MainWindow::on_cancelSearch_triggered()
{
    m_searchTimer->stop();

    m_searchToolBar->setVisible(false);

    m_searchLineEdit->setText(QString());
    m_searchLineEdit->setProgress(0);

    currentTab()->cancelSearch();
}

void MainWindow::on_search_timeout()
{
    m_searchTimer->stop();

    if(!m_searchLineEdit->text().isEmpty())
    {
        currentTab()->startSearch(m_searchLineEdit->text(), m_matchCaseCheckBox->isChecked());
    }
}

void MainWindow::on_settings_triggered()
{
    SettingsDialog* settingsDialog = new SettingsDialog(this);

    if(settingsDialog->exec() == QDialog::Accepted)
    {
        m_tabWidget->setTabPosition(static_cast< QTabWidget::TabPosition >(m_settings->value("mainWindow/tabPosition", 0).toUInt()));
        m_tabWidget->setTabBarPolicy(static_cast< TabWidget::TabBarPolicy >(m_settings->value("mainWindow/tabVisibility", 0).toUInt()));

        PageItem::setCacheSize(m_settings->value("pageItem/cacheSize", 32 * 1024 * 1024).toInt());

        PageItem::setDecoratePages(m_settings->value("pageItem/decoratePages", true).toBool());
        PageItem::setDecorateLinks(m_settings->value("pageItem/decorateLinks", true).toBool());

        for(int index = 0; index < m_tabWidget->count(); index++)
        {
            if(!tab(index)->refresh())
            {
                QMessageBox::warning(this, tr("Warning"), tr("Could not refresh '%1'.").arg(currentTab()->filePath()));
            }
        }
    }
}

void MainWindow::on_continuousMode_triggered(bool checked)
{
    currentTab()->setContinousMode(checked);
}

void MainWindow::on_twoPagesMode_triggered(bool checked)
{
    currentTab()->setTwoPagesMode(checked);
}

void MainWindow::on_zoomIn_triggered()
{
    currentTab()->zoomIn();
}

void MainWindow::on_zoomOut_triggered()
{
    currentTab()->zoomOut();
}

void MainWindow::on_originalSize_triggered()
{
    currentTab()->originalSize();
}

void MainWindow::on_fitToPageWidth_triggered(bool checked)
{
    if(checked)
    {
        currentTab()->setScaleMode(DocumentView::FitToPageWidth);
    }
    else
    {
        currentTab()->setScaleMode(DocumentView::ScaleFactor);
    }
}

void MainWindow::on_fitToPageSize_triggered(bool checked)
{
    if(checked)
    {
        currentTab()->setScaleMode(DocumentView::FitToPageSize);
    }
    else
    {
        currentTab()->setScaleMode(DocumentView::ScaleFactor);
    }
}

void MainWindow::on_rotateLeft_triggered()
{
    currentTab()->rotateLeft();
}

void MainWindow::on_rotateRight_triggered()
{
    currentTab()->rotateRight();
}

void MainWindow::on_fonts_triggered()
{
    QStandardItemModel* fontsModel = currentTab()->fontsModel();
    QDialog* dialog = new QDialog(this);

    QTableView* tableView = new QTableView(dialog);

    tableView->setAlternatingRowColors(true);
    tableView->setSortingEnabled(true);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    tableView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    tableView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    tableView->verticalHeader()->setVisible(false);

    tableView->setModel(fontsModel);

    QDialogButtonBox* dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, dialog);
    connect(dialogButtonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(dialogButtonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

    dialog->setLayout(new QVBoxLayout());
    dialog->layout()->addWidget(tableView);
    dialog->layout()->addWidget(dialogButtonBox);

    dialog->resize(m_settings->value("mainWindow/fontsDialogSize", dialog->sizeHint()).toSize());
    dialog->exec();
    m_settings->setValue("mainWindow/fontsDialogSize", dialog->size());

    delete fontsModel;
    delete dialog;
}

void MainWindow::on_fullscreen_triggered(bool checked)
{
    if(checked)
    {
        m_fullscreenAction->setData(saveGeometry());

        showFullScreen();
    }
    else
    {
        restoreGeometry(m_fullscreenAction->data().toByteArray());

        showNormal();

        restoreGeometry(m_fullscreenAction->data().toByteArray());
    }
}

void MainWindow::on_presentation_triggered()
{
    currentTab()->presentation();
}

void MainWindow::on_previousTab_triggered()
{
    if(m_tabWidget->currentIndex() > 0)
    {
        m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex() - 1);
    }
    else
    {
        m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
    }
}

void MainWindow::on_nextTab_triggered()
{
    if(m_tabWidget->currentIndex() < m_tabWidget->count() - 1)
    {
        m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex() + 1);
    }
    else
    {
        m_tabWidget->setCurrentIndex(0);
    }
}

void MainWindow::on_closeTab_triggered()
{
    delete m_tabWidget->currentWidget();
}

void MainWindow::on_closeAllTabs_triggered()
{
    disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

    while(m_tabWidget->count() > 0)
    {
        delete m_tabWidget->widget(0);
    }

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

    on_tabWidget_currentChanged(-1);
}

void MainWindow::on_tab_triggered()
{
    for(int index = 0; index < m_tabWidget->count(); index++)
    {
        if(sender()->parent() == m_tabWidget->widget(index))
        {
            m_tabWidget->setCurrentIndex(index);

            break;
        }
    }
}

void MainWindow::on_addBookmark_triggered()
{
    foreach(QAction* action, m_bookmarksMenu->actions())
    {
        Bookmark* bookmark = qobject_cast< Bookmark* >(action->menu());

        if(bookmark != 0)
        {
            if(QFileInfo(bookmark->filePath()).absoluteFilePath() == QFileInfo(currentTab()->filePath()).absoluteFilePath())
            {
                if(currentTab()->currentPage() != 1)
                {
                    bookmark->addJumpToPage(currentTab()->currentPage());
                }

                return;
            }
        }
    }

    Bookmark* bookmark = new Bookmark(currentTab()->filePath(), this);

    if(currentTab()->currentPage() != 1)
    {
        bookmark->addJumpToPage(currentTab()->currentPage());
    }

    connect(bookmark, SIGNAL(openTriggered(QString)), SLOT(on_bookmark_openTriggered(QString)));
    connect(bookmark, SIGNAL(openInNewTabTriggered(QString)), SLOT(on_bookmark_openInNewTabTriggered(QString)));
    connect(bookmark, SIGNAL(jumpToPageTriggered(QString,int)), SLOT(on_bookmark_jumpToPageTriggered(QString,int)));

    m_bookmarksMenu->addMenu(bookmark);
}

void MainWindow::on_removeAllBookmarks_triggered()
{
    foreach(QAction* action, m_bookmarksMenu->actions())
    {
        Bookmark* bookmark = qobject_cast< Bookmark* >(action->menu());

        if(bookmark != 0)
        {
            bookmark->deleteLater();
        }
    }
}

void MainWindow::on_bookmark_openTriggered(const QString& filePath)
{
    if(m_tabWidget->currentIndex() != -1)
    {
        open(filePath);
    }
    else
    {
        openInNewTab(filePath);
    }
}

void MainWindow::on_bookmark_openInNewTabTriggered(const QString& filePath)
{
    openInNewTab(filePath);
}

void MainWindow::on_bookmark_jumpToPageTriggered(const QString& filePath, int page)
{
    refreshOrOpenInNewTab(filePath, page);
}

void MainWindow::on_about_triggered()
{
    QMessageBox::about(this, tr("About qpdfview"), tr("<p><b>qpdfview %1</b></p><p>qpdfview is a tabbed PDF viewer using the poppler library. See <a href=\"https://launchpad.net/qpdfview\">launchpad.net/qpdfview</a> for more information.</p><p>&copy; 2012 Adam Reichold</p>").arg(QApplication::applicationVersion()));
}

void MainWindow::on_highlightAll_clicked(bool checked)
{
    currentTab()->setHighlightAll(checked);
}

void MainWindow::on_outline_clicked(const QModelIndex& index)
{
    bool ok = false;
    int page = m_outlineView->model()->data(index, Qt::UserRole + 1).toInt(&ok);
    qreal left = m_outlineView->model()->data(index, Qt::UserRole + 2).toReal();
    qreal top = m_outlineView->model()->data(index, Qt::UserRole + 3).toReal();

    if(ok)
    {
        currentTab()->jumpToPage(page, left, top);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();

        disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        foreach(QUrl url, event->mimeData()->urls())
        {
            if(url.scheme() == "file")
            {
                openInNewTab(url.path());
            }
        }

        connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

        on_tabWidget_currentChanged(m_tabWidget->currentIndex());
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveTabs();
    saveBookmarks();

    removeToolBar(m_searchToolBar);

    m_settings->setValue("mainWindow/geometry", m_fullscreenAction->isChecked() ? m_fullscreenAction->data() : saveGeometry());
    m_settings->setValue("mainWindow/state", saveState());

    QMainWindow::closeEvent(event);
}

bool MainWindow::senderIsCurrentTab() const
{
    return sender() == m_tabWidget->currentWidget() || qobject_cast< DocumentView* >(sender()) == 0;
}

void MainWindow::createWidgets()
{
    m_tabWidget = new TabWidget(this);

    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setTabsClosable(true);

    m_tabWidget->setTabPosition(static_cast< QTabWidget::TabPosition >(m_settings->value("mainWindow/tabPosition", 0).toUInt()));
    m_tabWidget->setTabBarPolicy(static_cast< TabWidget::TabBarPolicy >(m_settings->value("mainWindow/tabVisibility", 0).toUInt()));

    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)), SLOT(on_tabWidget_currentChanged(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), SLOT(on_tabWidget_tabCloseRequested(int)));

    // current page

    m_currentPageLineEdit = new LineEdit(this);
    m_currentPageLineEdit->setAlignment(Qt::AlignCenter);
    m_currentPageLineEdit->setFixedWidth(40);

    m_currentPageValidator = new QIntValidator(this);
    m_currentPageLineEdit->setValidator(m_currentPageValidator);

    connect(m_currentPageLineEdit, SIGNAL(editingFinished()), SLOT(on_currentPage_editingFinished()));
    connect(m_currentPageLineEdit, SIGNAL(returnPressed()), SLOT(on_currentPage_returnPressed()));

    // number of pages

    m_numberOfPagesLabel = new QLabel(this);
    m_numberOfPagesLabel->setAlignment(Qt::AlignCenter);
    m_numberOfPagesLabel->setFixedWidth(60);

    // scale factor

    m_scaleFactorComboBox = new ComboBox(this);

    m_scaleFactorComboBox->setEditable(true);
    m_scaleFactorComboBox->setInsertPolicy(QComboBox::NoInsert);

    m_scaleFactorComboBox->addItem(tr("Page width"));
    m_scaleFactorComboBox->addItem(tr("Page size"));
    m_scaleFactorComboBox->addItem("50 %", 0.5);
    m_scaleFactorComboBox->addItem("75 %", 0.75);
    m_scaleFactorComboBox->addItem("100 %", 1.0);
    m_scaleFactorComboBox->addItem("125 %", 1.25);
    m_scaleFactorComboBox->addItem("150 %", 1.5);
    m_scaleFactorComboBox->addItem("200 %", 2.0);
    m_scaleFactorComboBox->addItem("400 %", 4.0);

    connect(m_scaleFactorComboBox, SIGNAL(currentIndexChanged(int)), SLOT(on_scaleFactor_currentIndexChanged(int)));
    connect(m_scaleFactorComboBox->lineEdit(), SIGNAL(editingFinished()), SLOT(on_scaleFactor_editingFinished()));
    connect(m_scaleFactorComboBox->lineEdit(), SIGNAL(returnPressed()), SLOT(on_scaleFactor_returnPressed()));

    // search

    m_searchLineEdit = new ProgressLineEdit(this);
    m_searchTimer = new QTimer(this);

    m_searchTimer->setInterval(2000);
    m_searchTimer->setSingleShot(true);

    connect(m_searchLineEdit, SIGNAL(textEdited(QString)), m_searchTimer, SLOT(start()));
    connect(m_searchLineEdit, SIGNAL(returnPressed()), SLOT(on_search_timeout()));
    connect(m_searchTimer, SIGNAL(timeout()), SLOT(on_search_timeout()));

    m_matchCaseCheckBox = new QCheckBox(tr("Match &case"), this);
    m_highlightAllCheckBox = new QCheckBox(tr("Hightlight &all"), this);

    connect(m_highlightAllCheckBox, SIGNAL(clicked(bool)), SLOT(on_highlightAll_clicked(bool)));
}

void MainWindow::createActions()
{
    // open

    m_openAction = new QAction(tr("&Open..."), this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setIcon(QIcon::fromTheme("document-open", QIcon(":icons/document-open.svg")));
    m_openAction->setIconVisibleInMenu(true);
    connect(m_openAction, SIGNAL(triggered()), SLOT(on_open_triggered()));

    // open in new tab

    m_openInNewTabAction = new QAction(tr("Open in new &tab..."), this);
    m_openInNewTabAction->setShortcut(QKeySequence::AddTab);
    m_openInNewTabAction->setIcon(QIcon::fromTheme("tab-new", QIcon(":icons/tab-new.svg")));
    m_openInNewTabAction->setIconVisibleInMenu(true);
    connect(m_openInNewTabAction, SIGNAL(triggered()), SLOT(on_openInNewTab_triggered()));

    // refresh

    m_refreshAction = new QAction(tr("&Refresh"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setIcon(QIcon::fromTheme("view-refresh", QIcon(":icons/view-refresh.svg")));
    m_refreshAction->setIconVisibleInMenu(true);
    connect(m_refreshAction, SIGNAL(triggered()), SLOT(on_refresh_triggered()));

    // save copy

    m_saveCopyAction = new QAction(tr("&Save copy..."), this);
    m_saveCopyAction->setShortcut(QKeySequence::Save);
    m_saveCopyAction->setIcon(QIcon::fromTheme("document-save", QIcon(":icons/document-save.svg")));
    m_saveCopyAction->setIconVisibleInMenu(true);
    connect(m_saveCopyAction, SIGNAL(triggered()), SLOT(on_saveCopy_triggered()));

    // print

    m_printAction = new QAction(tr("&Print..."), this);
    m_printAction->setShortcut(QKeySequence::Print);
    m_printAction->setIcon(QIcon::fromTheme("document-print", QIcon(":icons/document-print.svg")));
    m_printAction->setIconVisibleInMenu(true);
    connect(m_printAction, SIGNAL(triggered()), SLOT(on_print_triggered()));

    // exit

    m_exitAction = new QAction(tr("&Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setIcon(QIcon::fromTheme("application-exit"));
    m_exitAction->setIconVisibleInMenu(true);
    connect(m_exitAction, SIGNAL(triggered()), SLOT(close()));

    // previous page

    m_previousPageAction = new QAction(tr("&Previous page"), this);
    m_previousPageAction->setShortcut(QKeySequence(Qt::Key_Backspace));
    m_previousPageAction->setIcon(QIcon::fromTheme("go-previous", QIcon(":icons/go-previous.svg")));
    m_previousPageAction->setIconVisibleInMenu(true);
    connect(m_previousPageAction, SIGNAL(triggered()), SLOT(on_previousPage_triggered()));

    // next page

    m_nextPageAction = new QAction(tr("&Next page"), this);
    m_nextPageAction->setShortcut(QKeySequence(Qt::Key_Space));
    m_nextPageAction->setIcon(QIcon::fromTheme("go-next", QIcon(":icons/go-next.svg")));
    m_nextPageAction->setIconVisibleInMenu(true);
    connect(m_nextPageAction, SIGNAL(triggered()), SLOT(on_nextPage_triggered()));

    // first page

    m_firstPageAction = new QAction(tr("&First page"), this);
    m_firstPageAction->setShortcut(QKeySequence(Qt::Key_Home));
    m_firstPageAction->setIcon(QIcon::fromTheme("go-first", QIcon(":icons/go-first.svg")));
    m_firstPageAction->setIconVisibleInMenu(true);
    connect(m_firstPageAction, SIGNAL(triggered()), SLOT(on_firstPage_triggered()));

    // last page

    m_lastPageAction = new QAction(tr("&Last page"), this);
    m_lastPageAction->setShortcut(QKeySequence(Qt::Key_End));
    m_lastPageAction->setIcon(QIcon::fromTheme("go-last", QIcon(":icons/go-last.svg")));
    m_lastPageAction->setIconVisibleInMenu(true);
    connect(m_lastPageAction, SIGNAL(triggered()), SLOT(on_lastPage_triggered()));

    // jump to page

    m_jumpToPageAction = new QAction(tr("&Jump to page..."), this);
    m_jumpToPageAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
    m_jumpToPageAction->setIcon(QIcon::fromTheme("go-jump", QIcon(":icons/go-jump.svg")));
    m_jumpToPageAction->setIconVisibleInMenu(true);
    connect(m_jumpToPageAction, SIGNAL(triggered()), SLOT(on_jumpToPage_triggered()));

    // search

    m_searchAction = new QAction(tr("&Search..."), this);
    m_searchAction->setShortcut(QKeySequence::Find);
    m_searchAction->setIcon(QIcon::fromTheme("edit-find", QIcon(":icons/edit-find.svg")));
    m_searchAction->setIconVisibleInMenu(true);
    connect(m_searchAction, SIGNAL(triggered()), SLOT(on_search_triggered()));

    // find previous

    m_findPreviousAction = new QAction(tr("Find previous"), this);
    m_findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    m_findPreviousAction->setIcon(QIcon::fromTheme("go-up", QIcon(":icons/go-up.svg")));
    m_findPreviousAction->setIconVisibleInMenu(true);
    connect(m_findPreviousAction, SIGNAL(triggered()), SLOT(on_findPrevious_triggered()));

    // find next

    m_findNextAction = new QAction(tr("Find next"), this);
    m_findNextAction->setShortcut(QKeySequence::FindNext);
    m_findNextAction->setIcon(QIcon::fromTheme("go-down", QIcon(":icons/go-down.svg")));
    m_findNextAction->setIconVisibleInMenu(true);
    connect(m_findNextAction, SIGNAL(triggered()), SLOT(on_findNext_triggered()));

    // cancel search

    m_cancelSearchAction = new QAction(tr("Cancel search"), this);
    m_cancelSearchAction->setShortcut(QKeySequence(Qt::Key_Escape));
    m_cancelSearchAction->setIcon(QIcon::fromTheme("process-stop", QIcon(":icons/process-stop.svg")));
    m_cancelSearchAction->setIconVisibleInMenu(true);
    connect(m_cancelSearchAction, SIGNAL(triggered()), SLOT(on_cancelSearch_triggered()));

    // settings

    m_settingsAction = new QAction(tr("Settings..."), this);
    connect(m_settingsAction, SIGNAL(triggered()), SLOT(on_settings_triggered()));

    // continuous mode

    m_continuousModeAction = new QAction(tr("&Continuous"), this);
    m_continuousModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_7));
    m_continuousModeAction->setCheckable(true);
    m_continuousModeAction->setIcon(QIcon(":icons/continuous.svg"));
    connect(m_continuousModeAction, SIGNAL(triggered(bool)), SLOT(on_continuousMode_triggered(bool)));

    // two pages mode

    m_twoPagesModeAction = new QAction(tr("&Two pages"), this);
    m_twoPagesModeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_6));
    m_twoPagesModeAction->setCheckable(true);
    m_twoPagesModeAction->setIcon(QIcon(":icons/two-pages.svg"));
    connect(m_twoPagesModeAction, SIGNAL(triggered(bool)), SLOT(on_twoPagesMode_triggered(bool)));

    // zoom in

    m_zoomInAction = new QAction(tr("Zoom &in"), this);
    m_zoomInAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    m_zoomInAction->setIcon(QIcon::fromTheme("zoom-in", QIcon(":icons/zoom-in.svg")));
    m_zoomInAction->setIconVisibleInMenu(true);
    connect(m_zoomInAction, SIGNAL(triggered()), SLOT(on_zoomIn_triggered()));

    // zoom out
    m_zoomOutAction = new QAction(tr("Zoom &out"), this);
    m_zoomOutAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    m_zoomOutAction->setIcon(QIcon::fromTheme("zoom-out", QIcon(":icons/zoom-out.svg")));
    m_zoomOutAction->setIconVisibleInMenu(true);
    connect(m_zoomOutAction, SIGNAL(triggered()), SLOT(on_zoomOut_triggered()));

    // original size

    m_originalSizeAction = new QAction(tr("Original &size"), this);
    m_originalSizeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    m_originalSizeAction->setIcon(QIcon::fromTheme("zoom-original"));
    m_originalSizeAction->setIconVisibleInMenu(true);
    connect(m_originalSizeAction, SIGNAL(triggered()), SLOT(on_originalSize_triggered()));

    // fit to page width

    m_fitToPageWidthAction = new QAction(tr("Fit to page width"), this);
    m_fitToPageWidthAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_9));
    m_fitToPageWidthAction->setCheckable(true);
    m_fitToPageWidthAction->setIcon(QIcon(":icons/fit-to-page-width.svg"));
    connect(m_fitToPageWidthAction, SIGNAL(triggered(bool)), SLOT(on_fitToPageWidth_triggered(bool)));

    // fit to page size

    m_fitToPageSizeAction = new QAction(tr("Fit to page size"), this);
    m_fitToPageSizeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_8));
    m_fitToPageSizeAction->setCheckable(true);
    m_fitToPageSizeAction->setIcon(QIcon(":icons/fit-to-page-size.svg"));
    connect(m_fitToPageSizeAction, SIGNAL(triggered(bool)), SLOT(on_fitToPageSize_triggered(bool)));

    // rotate left

    m_rotateLeftAction = new QAction(tr("Rotate &left"), this);
    m_rotateLeftAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    m_rotateLeftAction->setIcon(QIcon::fromTheme("object-rotate-left"));
    m_rotateLeftAction->setIconVisibleInMenu(true);
    connect(m_rotateLeftAction, SIGNAL(triggered()), SLOT(on_rotateLeft_triggered()));

    // rotate right

    m_rotateRightAction = new QAction(tr("Rotate &right"), this);
    m_rotateRightAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    m_rotateRightAction->setIcon(QIcon::fromTheme("object-rotate-right"));
    m_rotateRightAction->setIconVisibleInMenu(true);
    connect(m_rotateRightAction, SIGNAL(triggered()), SLOT(on_rotateRight_triggered()));

    // fonts

    m_fontsAction = new QAction(tr("Fonts..."), this);
    connect(m_fontsAction, SIGNAL(triggered()), SLOT(on_fonts_triggered()));

    // fullscreen

    m_fullscreenAction = new QAction(tr("&Fullscreen"), this);
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    m_fullscreenAction->setIcon(QIcon::fromTheme("view-fullscreen", QIcon(":icons/view-fullscreen.svg")));
    connect(m_fullscreenAction, SIGNAL(triggered(bool)), SLOT(on_fullscreen_triggered(bool)));

    // presentation

    m_presentationAction = new QAction(tr("&Presentation..."), this);
    m_presentationAction->setShortcut(QKeySequence(Qt::Key_F12));
    m_presentationAction->setIcon(QIcon::fromTheme("x-office-presentation", QIcon(":icons/x-office-presentation.svg")));
    m_presentationAction->setIconVisibleInMenu(true);
    connect(m_presentationAction, SIGNAL(triggered()), SLOT(on_presentation_triggered()));

    // previous tab

    m_previousTabAction = new QAction(tr("&Previous tab"), this);
    m_previousTabAction->setShortcut(QKeySequence::PreviousChild);
    connect(m_previousTabAction, SIGNAL(triggered()), SLOT(on_previousTab_triggered()));

    // next tab

    m_nextTabAction = new QAction(tr("&Next tab"), this);
    m_nextTabAction->setShortcut(QKeySequence::NextChild);
    connect(m_nextTabAction, SIGNAL(triggered()), SLOT(on_nextTab_triggered()));

    // close tab

    m_closeTabAction = new QAction(tr("&Close tab"), this);
    m_closeTabAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    m_closeTabAction->setIcon(QIcon::fromTheme("window-close"));
    m_closeTabAction->setIconVisibleInMenu(true);
    connect(m_closeTabAction, SIGNAL(triggered()), SLOT(on_closeTab_triggered()));

    // close all tabs

    m_closeAllTabsAction = new QAction(tr("Close &all tabs"), this);
    m_closeAllTabsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_W));
    connect(m_closeAllTabsAction, SIGNAL(triggered()), SLOT(on_closeAllTabs_triggered()));

    // add bookmark

    m_addBookmarkAction = new QAction(tr("&Add bookmark"), this);
    m_addBookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    connect(m_addBookmarkAction, SIGNAL(triggered()), SLOT(on_addBookmark_triggered()));

    // remove all bookmarks

    m_removeAllBookmarksAction = new QAction(tr("&Remove all bookmarks"), this);
    connect(m_removeAllBookmarksAction, SIGNAL(triggered()), SLOT(on_removeAllBookmarks_triggered()));

    // about

    m_aboutAction = new QAction(tr("&About"), this);
    m_aboutAction->setIcon(QIcon::fromTheme("help-about"));
    m_aboutAction->setIconVisibleInMenu(true);
    connect(m_aboutAction, SIGNAL(triggered()), SLOT(on_about_triggered()));
}

void MainWindow::createToolBars()
{
    // file

    m_fileToolBar = addToolBar(tr("&File"));
    m_fileToolBar->setObjectName("fileToolBar");

    foreach(QString action, m_settings->value("mainWindow/fileToolBar", QStringList() << "openInNewTab" << "refresh").toStringList())
    {
        if(action == "open") { m_fileToolBar->addAction(m_openAction); }
        else if(action == "openInNewTab") { m_fileToolBar->addAction(m_openInNewTabAction); }
        else if(action == "refresh") { m_fileToolBar->addAction(m_refreshAction); }
        else if(action == "saveCopy") { m_fileToolBar->addAction(m_saveCopyAction); }
        else if(action == "print") { m_fileToolBar->addAction(m_printAction); }
    }

    // edit

    m_editToolBar = addToolBar(tr("&Edit"));
    m_editToolBar->setObjectName("editToolBar");

    foreach(QString action, m_settings->value("mainWindow/editToolBar", QStringList() << "currentPage" << "numberOfPages" << "previousPage" << "nextPage").toStringList())
    {
        if(action == "currentPage") { m_editToolBar->addWidget(m_currentPageLineEdit); }
        else if(action == "numberOfPages") { m_editToolBar->addWidget(m_numberOfPagesLabel); }
        else if(action == "previousPage") { m_editToolBar->addAction(m_previousPageAction); }
        else if(action == "nextPage") { m_editToolBar->addAction(m_nextPageAction); }
        else if(action == "firstPage") { m_editToolBar->addAction(m_firstPageAction); }
        else if(action == "lastPage") { m_editToolBar->addAction(m_lastPageAction); }
        else if(action == "jumpToPage") { m_editToolBar->addAction(m_jumpToPageAction); }
    }

    // view

    m_viewToolBar = addToolBar(tr("&View"));
    m_viewToolBar->setObjectName("viewToolBar");

    foreach(QString action, m_settings->value("mainWindow/viewToolBar", QStringList() << "scaleFactor" << "zoomIn" << "zoomOut").toStringList())
    {
        if(action == "continuousMode") { m_viewToolBar->addAction(m_continuousModeAction); }
        else if(action == "twoPagesMode") { m_viewToolBar->addAction(m_twoPagesModeAction); }
        else if(action == "scaleFactor") { m_viewToolBar->addWidget(m_scaleFactorComboBox); }
        else if(action == "zoomIn") { m_viewToolBar->addAction(m_zoomInAction); }
        else if(action == "zoomOut") { m_viewToolBar->addAction(m_zoomOutAction); }
        else if(action == "originalSize") { m_viewToolBar->addAction(m_originalSizeAction); }
        else if(action == "fitToPageWidth") { m_viewToolBar->addAction(m_fitToPageWidthAction); }
        else if(action == "fitToPageSize") { m_viewToolBar->addAction(m_fitToPageSizeAction); }
        else if(action == "rotateLeft") { m_viewToolBar->addAction(m_rotateLeftAction); }
        else if(action == "rotateRight") { m_viewToolBar->addAction(m_rotateRightAction); }
        else if(action == "fullscreen") { m_viewToolBar->addAction(m_fullscreenAction); }
        else if(action == "presentation") { m_viewToolBar->addAction(m_presentationAction); }
    }

    // search

    m_searchToolBar = new QToolBar(tr("&Search"), this);
    m_searchToolBar->setObjectName("searchToolBar");

    m_searchToolBar->setHidden(true);
    m_searchToolBar->setMovable(false);

    addToolBar(Qt::BottomToolBarArea, m_searchToolBar);

    m_searchToolBar->addWidget(m_searchLineEdit);
    m_searchToolBar->addWidget(m_matchCaseCheckBox);
    m_searchToolBar->addWidget(m_highlightAllCheckBox);
    m_searchToolBar->addAction(m_findPreviousAction);
    m_searchToolBar->addAction(m_findNextAction);
    m_searchToolBar->addAction(m_cancelSearchAction);
}

void MainWindow::createDocks()
{
    // outline

    m_outlineDock = new QDockWidget(tr("&Outline"), this);
    m_outlineDock->setObjectName("outlineDock");
    m_outlineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_outlineDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::LeftDockWidgetArea, m_outlineDock);

    m_outlineDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F6));
    m_outlineDock->hide();

    m_outlineView = new QTreeView(this);
    m_outlineView->setAlternatingRowColors(true);
    m_outlineView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_outlineView->header()->setVisible(false);
    m_outlineView->header()->setResizeMode(QHeaderView::Stretch);

    connect(m_outlineView, SIGNAL(clicked(QModelIndex)), SLOT(on_outline_clicked(QModelIndex)));

    m_outlineDock->setWidget(m_outlineView);

    // properties

    m_propertiesDock = new QDockWidget(tr("&Properties"), this);
    m_propertiesDock->setObjectName("propertiesDock");
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_propertiesDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::LeftDockWidgetArea, m_propertiesDock);

    m_propertiesDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F7));
    m_propertiesDock->hide();

    m_propertiesView = new QTableView(this);
    m_propertiesView->setAlternatingRowColors(true);
    m_propertiesView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_propertiesView->horizontalHeader()->setVisible(false);
    m_propertiesView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_propertiesView->verticalHeader()->setVisible(false);
    m_propertiesView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    m_propertiesDock->setWidget(m_propertiesView);

    // thumbnails

    m_thumbnailsDock = new QDockWidget(tr("&Thumbnails"), this);
    m_thumbnailsDock->setObjectName("thumbnailsDock");
    m_thumbnailsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_thumbnailsDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    addDockWidget(Qt::RightDockWidgetArea, m_thumbnailsDock);

    m_thumbnailsDock->toggleViewAction()->setShortcut(QKeySequence(Qt::Key_F8));
    m_thumbnailsDock->hide();

    m_thumbnailsView = new QGraphicsView(this);

    m_thumbnailsDock->setWidget(m_thumbnailsView);
}

void MainWindow::createMenus()
{
    // file

    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_openInNewTabAction);
    m_fileMenu->addAction(m_refreshAction);
    m_fileMenu->addAction(m_saveCopyAction);
    m_fileMenu->addAction(m_printAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    // edit

    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    m_editMenu->addAction(m_previousPageAction);
    m_editMenu->addAction(m_nextPageAction);
    m_editMenu->addAction(m_firstPageAction);
    m_editMenu->addAction(m_lastPageAction);
    m_editMenu->addAction(m_jumpToPageAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_searchAction);
    m_editMenu->addAction(m_findPreviousAction);
    m_editMenu->addAction(m_findNextAction);
    m_editMenu->addAction(m_cancelSearchAction);
    m_editMenu->addSeparator();
    m_editMenu->addAction(m_settingsAction);

    // view

    m_viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu->addAction(m_continuousModeAction);
    m_viewMenu->addAction(m_twoPagesModeAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_zoomInAction);
    m_viewMenu->addAction(m_zoomOutAction);
    m_viewMenu->addAction(m_originalSizeAction);
    m_viewMenu->addAction(m_fitToPageWidthAction);
    m_viewMenu->addAction(m_fitToPageSizeAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_rotateLeftAction);
    m_viewMenu->addAction(m_rotateRightAction);
    m_viewMenu->addSeparator();

    QMenu* toolBarsMenu = m_viewMenu->addMenu(tr("&Tool bars"));
    toolBarsMenu->addAction(m_fileToolBar->toggleViewAction());
    toolBarsMenu->addAction(m_editToolBar->toggleViewAction());
    toolBarsMenu->addAction(m_viewToolBar->toggleViewAction());

    QMenu* docksMenu = m_viewMenu->addMenu(tr("&Docks"));
    docksMenu->addAction(m_outlineDock->toggleViewAction());
    docksMenu->addAction(m_propertiesDock->toggleViewAction());
    docksMenu->addAction(m_thumbnailsDock->toggleViewAction());

    m_viewMenu->addAction(m_fontsAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_fullscreenAction);
    m_viewMenu->addAction(m_presentationAction);

    // tabs

    m_tabsMenu = menuBar()->addMenu(tr("&Tabs"));
    m_tabsMenu->addAction(m_previousTabAction);
    m_tabsMenu->addAction(m_nextTabAction);
    m_tabsMenu->addSeparator();
    m_tabsMenu->addAction(m_closeTabAction);
    m_tabsMenu->addAction(m_closeAllTabsAction);
    m_tabsMenu->addSeparator();

    // bookmarks

    m_bookmarksMenu = menuBar()->addMenu(tr("&Bookmarks"));
    m_bookmarksMenu->addAction(m_addBookmarkAction);
    m_bookmarksMenu->addAction(m_removeAllBookmarksAction);
    m_bookmarksMenu->addSeparator();

    // help

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::restoreTabs()
{
    if(m_settings->value("mainWindow/restoreTabs", false).toBool())
    {
        QFile file(QFileInfo(QDir(QFileInfo(m_settings->fileName()).path()), "tabs.xml").filePath());

        if(file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QDomDocument document;

            if(document.setContent(&file))
            {
                disconnect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));

                QDomElement rootElement = document.firstChildElement();
                QDomElement tabElement = rootElement.firstChildElement();

                while(!tabElement.isNull())
                {
                    if(openInNewTab(tabElement.attribute("filePath")))
                    {
                        currentTab()->setContinousMode(static_cast< bool >(tabElement.attribute("continuousMode").toUInt()));
                        currentTab()->setTwoPagesMode(static_cast< bool >(tabElement.attribute("twoPagesMode").toUInt()));

                        currentTab()->setScaleMode(static_cast< DocumentView::ScaleMode >(tabElement.attribute("scaleMode").toUInt()));
                        currentTab()->setScaleFactor(tabElement.attribute("scaleFactor").toFloat());

                        currentTab()->setRotation(static_cast< Poppler::Page::Rotation >(tabElement.attribute("rotation").toUInt()));

                        currentTab()->jumpToPage(tabElement.attribute("currentPage").toInt());
                    }

                    tabElement = tabElement.nextSiblingElement();
                }

                connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(on_tabWidget_currentChanged(int)));
            }

            file.close();
        }
    }
}

void MainWindow::saveTabs()
{
    QFile file(QFileInfo(QDir(QFileInfo(m_settings->fileName()).path()), "tabs.xml").filePath());

    if(m_settings->value("mainWindow/restoreTabs", false).toBool())
    {
        if(file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QDomDocument document;

            QDomElement rootElement = document.createElement("tabs");
            document.appendChild(rootElement);

            for(int index = 0; index < m_tabWidget->count(); index++)
            {
                QDomElement tabElement = document.createElement("tab");

                tabElement.setAttribute("filePath", QFileInfo(tab(index)->filePath()).absoluteFilePath());
                tabElement.setAttribute("currentPage", tab(index)->currentPage());

                tabElement.setAttribute("continuousMode", static_cast< uint >(tab(index)->continousMode()));
                tabElement.setAttribute("twoPagesMode", static_cast< uint >(tab(index)->twoPagesMode()));

                tabElement.setAttribute("scaleMode", static_cast< uint >(tab(index)->scaleMode()));
                tabElement.setAttribute("scaleFactor", tab(index)->scaleFactor());

                tabElement.setAttribute("rotation", static_cast< uint >(tab(index)->rotation()));

                rootElement.appendChild(tabElement);
            }

            QTextStream textStream(&file);
            document.save(textStream, 4);

            file.close();
        }
    }
    else
    {
        file.remove();
    }
}

void MainWindow::restoreBookmarks()
{
    if(m_settings->value("mainWindow/restoreBookmarks", false).toBool())
    {
        QFile file(QFileInfo(QDir(QFileInfo(m_settings->fileName()).path()), "bookmarks.xml").filePath());

        if(file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QDomDocument document;

            if(document.setContent(&file))
            {
                QDomElement rootElement = document.firstChildElement();
                QDomElement bookmarkElement = rootElement.firstChildElement();

                while(!bookmarkElement.isNull())
                {
                    Bookmark* bookmark = new Bookmark(bookmarkElement.attribute("filePath"));

                    QDomElement jumpToPageElement = bookmarkElement.firstChildElement();

                    while(!jumpToPageElement.isNull())
                    {
                        bookmark->addJumpToPage(jumpToPageElement.attribute("page").toInt());

                        jumpToPageElement = jumpToPageElement.nextSiblingElement();
                    }

                    connect(bookmark, SIGNAL(openTriggered(QString)), SLOT(on_bookmark_openTriggered(QString)));
                    connect(bookmark, SIGNAL(openInNewTabTriggered(QString)), SLOT(on_bookmark_openInNewTabTriggered(QString)));
                    connect(bookmark, SIGNAL(jumpToPageTriggered(QString,int)), SLOT(on_bookmark_jumpToPageTriggered(QString,int)));

                    m_bookmarksMenu->addMenu(bookmark);

                    bookmarkElement = bookmarkElement.nextSiblingElement();
                }
            }

            file.close();
        }
    }
}

void MainWindow::saveBookmarks()
{
    QFile file(QFileInfo(QDir(QFileInfo(m_settings->fileName()).path()), "bookmarks.xml").filePath());

    if(m_settings->value("mainWindow/restoreBookmarks", false).toBool())
    {
        if(file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QDomDocument document;

            QDomElement rootElement = document.createElement("bookmarks");
            document.appendChild(rootElement);

            foreach(QAction* action, m_bookmarksMenu->actions())
            {
                Bookmark* bookmark = qobject_cast< Bookmark* >(action->menu());

                if(bookmark != 0)
                {
                    QDomElement bookmarkElement = document.createElement("bookmark");
                    bookmarkElement.setAttribute("filePath", QFileInfo(bookmark->filePath()).absoluteFilePath());

                    foreach(int page, bookmark->pages())
                    {
                        QDomElement jumpToPageElement = document.createElement("jumpToPage");
                        jumpToPageElement.setAttribute("page", page);

                        bookmarkElement.appendChild(jumpToPageElement);
                    }

                    rootElement.appendChild(bookmarkElement);
                }
            }

            QTextStream textStream(&file);
            document.save(textStream, 4);

            file.close();
        }
    }
    else
    {
        file.remove();
    }
}

#ifdef WITH_DBUS

MainWindowAdaptor::MainWindowAdaptor(MainWindow* mainWindow) : QDBusAbstractAdaptor(mainWindow)
{
}

MainWindow* MainWindowAdaptor::mainWindow() const
{
    return qobject_cast< MainWindow* >(parent());
}

bool MainWindowAdaptor::open(const QString& filePath, int page)
{
    return mainWindow()->open(filePath, page);
}

bool MainWindowAdaptor::openInNewTab(const QString& filePath, int page)
{
    return mainWindow()->openInNewTab(filePath, page);
}

bool MainWindowAdaptor::refreshOrOpenInNewTab(const QString& filePath, int page)
{
    return mainWindow()->refreshOrOpenInNewTab(filePath, page);
}

# endif // WITH_DBUS