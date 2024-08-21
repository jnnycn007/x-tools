﻿/***************************************************************************************************
 * Copyright 2024 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded in "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "StyleSheetManager.h"

#include <QApplication>
#include <QDir>
#include <QFileInfoList>
#include <QMessageBox>
#include <QPainter>

#include "./Settings.h"
#include "./xExec.h"

namespace xTools {

StyleSheetManager::StyleSheetManager(QObject* parent)
    : acss::QtAdvancedStylesheet(parent)
{
    m_nameFriendlyNameMap.insert("dark_amber", tr("Dark Amber"));
    m_nameFriendlyNameMap.insert("dark_blue", tr("Dark Blue"));
    m_nameFriendlyNameMap.insert("dark_cyan", tr("Dark Cyan"));
    m_nameFriendlyNameMap.insert("dark_lightgreen", tr("Dark Light Green"));
    m_nameFriendlyNameMap.insert("dark_pink", tr("Dark Pink"));
    m_nameFriendlyNameMap.insert("dark_purple", tr("Dark Purple"));
    m_nameFriendlyNameMap.insert("dark_red", tr("Dark Red"));
    m_nameFriendlyNameMap.insert("dark_teal", tr("Dark Teal"));
    m_nameFriendlyNameMap.insert("dark_yellow", tr("Dark Yellow"));

    m_nameFriendlyNameMap.insert("light_amber", tr("Light Amber"));
    m_nameFriendlyNameMap.insert("light_blue", tr("Light Blue"));
    m_nameFriendlyNameMap.insert("light_cyan", tr("Light Cyan"));
    m_nameFriendlyNameMap.insert("light_cyan_500", tr("Light Cyan(500)"));
    m_nameFriendlyNameMap.insert("light_lightgreen", tr("Light Light Green"));
    m_nameFriendlyNameMap.insert("light_pink", tr("Light Pink"));
    m_nameFriendlyNameMap.insert("light_purple", tr("Light Purple"));
    m_nameFriendlyNameMap.insert("light_red", tr("Light Red"));
    m_nameFriendlyNameMap.insert("light_teal", tr("Light Teal"));
    m_nameFriendlyNameMap.insert("light_yellow", tr("Light Yellow"));

    m_primaryColorMap.insert("dark_amber", "#ffd740");
    m_primaryColorMap.insert("dark_blue", "#448aff");
    m_primaryColorMap.insert("dark_cyan", "#4dd0e1");
    m_primaryColorMap.insert("dark_lightgreen", "#8bc34a");
    m_primaryColorMap.insert("dark_pink", "#ff4081");
    m_primaryColorMap.insert("dark_purple", "#ab47bc");
    m_primaryColorMap.insert("dark_red", "#ff1744");
    m_primaryColorMap.insert("dark_teal", "#1de9b6");
    m_primaryColorMap.insert("dark_yellow", "#ffff00");

    m_primaryColorMap.insert("light_amber", "#ffc400");
    m_primaryColorMap.insert("light_blue", "#2979ff");
    m_primaryColorMap.insert("light_cyan", "#00e5ff");
    m_primaryColorMap.insert("light_cyan_500", "#00bcd4");
    m_primaryColorMap.insert("light_lightgreen", "#64dd17");
    m_primaryColorMap.insert("light_pink", "#ff4081");
    m_primaryColorMap.insert("light_purple", "#e040fb");
    m_primaryColorMap.insert("light_red", "#ff1744");
    m_primaryColorMap.insert("light_teal", "#1de9b6");
    m_primaryColorMap.insert("light_yellow", "#ffea00");

    QDir dir(xTools::Settings::instance()->settingsPath());
    if (!dir.exists("output")) {
        dir.mkdir("output");
    }

    QString appDir = QApplication::applicationDirPath();
    setStylesDirPath(appDir + "/3rd_styles");
    setOutputDirPath(xTools::Settings::instance()->settingsPath() + "/output");
    setCurrentStyle("qt_material");
    setCurrentTheme(themeName());

    qInfo() << "The styles directory is:" << stylesDirPath();
    qInfo() << "The output directory is:" << outputDirPath();
    qInfo() << "The current style is:" << currentStyle();
    qInfo() << "The current theme is:" << currentTheme();

    bool checked = enableStylesheet();
    QAction* action = new QAction(tr("Enable Stylesheet"));
    action->setCheckable(true);
    action->setChecked(checked);
    if (checked) {
        updateApplicationStylesheet();
    }

    connect(action, &QAction::triggered, this, &StyleSheetManager::setApplicationStylesheetEnabled);

    m_themeActionGroup = new QActionGroup(this);
    m_themeMenu = new QMenu(tr("Application Stylesheet"));
    m_themeMenu->addAction(action);
    m_themeMenu->addSeparator();

    loadThemes();
}

StyleSheetManager::~StyleSheetManager() {}

StyleSheetManager& StyleSheetManager::singleton()
{
    static StyleSheetManager instance;
    return instance;
}

QMenu* StyleSheetManager::themeMenu() const
{
    return m_themeMenu;
}

QString StyleSheetManager::themeName()
{
    QString ret = xTools::Settings::instance()->value("themeName").toString();
    if (ret.isEmpty()) {
        ret = QString("dark_blue");
    }

    return ret;
}

void StyleSheetManager::setThemeName(const QString& themeName)
{
    xTools::Settings::instance()->setValue("themeName", themeName);
    setCurrentTheme(themeName);
    if (enableStylesheet()) {
        updateStylesheet();
    }
}

void StyleSheetManager::updateApplicationStylesheet()
{
    if (qApp) {
        updateStylesheet();
        qApp->setStyleSheet(styleSheet());
    }
}

void StyleSheetManager::loadThemes()
{
    QStringList themeList = themes();
    QStringList m_darkThemes;
    QStringList m_lightThemes;
    for (QString& theme : themeList) {
        if (theme.contains("dark_")) {
            m_darkThemes.append(theme);
        } else if (theme.contains("light_")) {
            m_lightThemes.append(theme);
        }
    }

    setupActions(m_darkThemes, m_themeMenu, m_themeActionGroup);
    setupActions(m_lightThemes, m_themeMenu, m_themeActionGroup);
}

void StyleSheetManager::updateActions()
{
    for (QAction* action : m_themeActionGroup->actions()) {
        QString theme = action->data().toString();
        QString color = m_primaryColorMap.value(theme);
        updateActionIcon(action, color);
    }
}

void StyleSheetManager::updateActionIcon(QAction* action, const QString& color)
{
    QPixmap pixmap(64, 64);
    pixmap.fill(color);
    if (action->isChecked()) {
        QPainter painter(&pixmap);
        QRectF tmp = pixmap.rect() - QMargins(16, 16, 16, 16);
        painter.setPen(Qt::black);
        painter.setBrush(Qt::white);
        painter.drawRect(tmp);
        painter.end();
    }
    action->setIcon(QIcon(pixmap));
}

bool StyleSheetManager::enableStylesheet()
{
    auto settings = xTools::Settings::instance();
    return settings->value("enableStylesheet", false).toBool();
}

void StyleSheetManager::setEnableStylesheet(bool enable)
{
    auto settings = xTools::Settings::instance();
    settings->setValue("enableStylesheet", enable);
}

void StyleSheetManager::setupActions(const QStringList& themes, QMenu* menu, QActionGroup* group)
{
    for (const QString& theme : themes) {
        bool isValidName = m_nameFriendlyNameMap.contains(theme);
        QString txt = isValidName ? this->m_nameFriendlyNameMap.value(theme) : theme;
        QString color = this->m_primaryColorMap.value(theme);
        QAction* action = new QAction(txt, menu);
        action->setData(theme);
        action->setCheckable(true);
        group->addAction(action);
        menu->addAction(action);
        if (theme == themeName()) {
            action->setChecked(true);
        }

        updateActionIcon(action, color);

        connect(action, &QAction::triggered, this, [this, theme] {
            updateActions();
            setThemeName(theme);
            if (enableStylesheet()) {
                updateApplicationStylesheet();
            } else {
                QMessageBox::warning(nullptr, tr("Warning"), tr("Please enable stylesheet first!"));
            }
        });
    }
}

void StyleSheetManager::setApplicationStylesheetEnabled(bool enable)
{
    setEnableStylesheet(enable);
    for (auto action : m_themeActionGroup->actions()) {
        if (action->isChecked() && enable) {
            emit action->triggered();
            break;
        }
    }

    if (!enable) {
        xToolsTryToRebootApp();
    }
}

} // namespace xTools