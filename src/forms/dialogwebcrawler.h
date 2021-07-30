/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.2
 Written in Qt
 
                         dialogwebcrawler.h  -  description
                             -------------------
    copyright            : (C) 2005-2021 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : https://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

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
