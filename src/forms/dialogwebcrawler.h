/**
 * @file dialogwebcrawler.h
 * @brief Declares the DialogWebCrawler class for configuring and initiating web-based network crawling in SocNetV.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


#ifndef WEBCRAWLERDIALOG_H
#define WEBCRAWLERDIALOG_H

class QUrl;

#include <QDialog>

#include "ui_dialogwebcrawler.h"


class DialogWebCrawler: public QDialog
{
    Q_OBJECT
public:
    explicit DialogWebCrawler (QWidget *parent = Q_NULLPTR);

public slots:
    void checkErrors ();
    void getUserChoices ();
    QStringList parseTextEditInput(const QString &html);
signals:
    void userChoices( const QUrl &startUrl,
                      const QStringList &,
                      const QStringList &,
                      const QStringList &,
                      const int &maxNodes,
                      const int &maxLinks,
                      const bool &intLinks,
                      const bool &childLinks,
                      const bool &parentLinks,
                      const bool &selfLinks,
                      const bool &extLinksAllowed,
                      const bool &extLinksCrawl,
                      const bool &socialLinks,
                      const bool &delayedRequests
                      );
    void webCrawlerDialogError(QString);
private:
    Ui::DialogWebCrawler ui;
    QString seedUrlInputStr;
    QUrl seedUrl ;
    int maxLinksPerPage, maxUrlsToCrawl;
    bool extLinks, intLinks;
    bool extLinksAllowed;
    bool childLinks, parentLinks;
    bool socialLinks;
    QStringList linkClasses;
    QStringList urlPatternsIncluded;
    QStringList urlPatternsExcluded;

};



#endif
