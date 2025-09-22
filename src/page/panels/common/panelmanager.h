﻿/***************************************************************************************************
 * Copyright 2025-2025 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of eTools project.
 *
 * eTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include <QButtonGroup>
#include <QList>
#include <QMenu>
#include <QStackedLayout>
#include <QToolButton>
#include <QVariantMap>
#include <QWidget>
#include <QWidgetAction>

class Panel;
class PanelManager : public QWidget
{
    Q_OBJECT
public:
    explicit PanelManager(QWidget *parent = nullptr);
    ~PanelManager() override;

    QVariantMap save() const;
    void load(const QVariantMap &parameters);
    QList<QToolButton *> buttons() const;
    void inputBytes(const QByteArray &bytes, const QString &flag);

signals:
    void visibleChanged(bool visible);

public:
    template<typename T>
    T *addPanel(const QString &name, const QString &icon)
    {
        return addPanel<T>(name, QIcon(icon));
    }

    template<typename T>
    T *addPanel(const QString &name, const QIcon &icon)
    {
        static_assert(std::is_base_of<Panel, T>::value, "T must inherit from Panel");
        T *panel = new T();
        m_layout->addWidget(panel);

        QToolButton *button = new QToolButton();
        button->setText(name);
        button->setIcon(icon);
        button->setCheckable(true);
        button->setToolTip(name);
        // Show icon only
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_buttonGroup.addButton(button);
        connect(button, &QToolButton::clicked, this, [this, panel]() {
            this->m_layout->setCurrentWidget(panel);
            this->setVisible(true);
            this->m_panelButton->setChecked(true);
            emit visibleChanged(true);
        });

        QMenu *menu = nullptr;
        QWidget *menuWidget = panel->menuWidget();
        if (menuWidget) {
            menu = new QMenu(button);
            QWidgetAction *menuAction = new QWidgetAction(menu);
            menuAction->setDefaultWidget(menuWidget);
            menu->addAction(menuAction);
            button->setMenu(menu);
            button->setPopupMode(QToolButton::MenuButtonPopup);
        } else {
            menu = panel->buttonMenu();
            if (menu) {
                button->setMenu(menu);
                button->setPopupMode(QToolButton::MenuButtonPopup);
            }
        }

        if (menu) {
            connect(menu, &QMenu::aboutToShow, this, [this, button, panel]() {
                button->setChecked(true);
                this->m_layout->setCurrentWidget(panel);
                this->setVisible(true);
                this->m_panelButton->setChecked(true);
                emit visibleChanged(true);
            });
        }

        if (m_layout->count() == 1) {
            button->setChecked(true);
        }

        m_panels.append(panel);
        return panel;
    }

    template<typename T>
    T *getPanel() const
    {
        static_assert(std::is_base_of<Panel, T>::value, "T must inherit from Panel");
        for (Panel *panel : m_panels) {
            if (qobject_cast<T *>(panel)) {
                return static_cast<T *>(panel);
            }
        }

        return nullptr;
    }

private:
    QList<Panel *> m_panels;
    QButtonGroup m_buttonGroup;
    QStackedLayout *m_layout;
    QToolButton *m_panelButton;
};
