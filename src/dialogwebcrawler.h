/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt
 
                         dialogwebcrawler.h  -  description
                             -------------------
    copyright            : (C) 2005-2018 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : http://socnetv.org

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


#include <QDialog>

#include "ui_dialogwebcrawler.h"
 

class DialogWebCrawler: public QDialog
{
	Q_OBJECT
public:
    explicit DialogWebCrawler (QWidget *parent = 0);

public slots:
    void checkErrors ();
	void gatherData ();
    QStringList parseTextEditInput(const QString &html);
signals:
    void userChoices( const QString &seedUrl,
                      const QStringList &,
                      const QStringList &,
                      const QStringList &,
                      const int &maxNodes,
                      const int &maxLinks,
                      const bool &extLinks,
                      const bool &intLinks,
                      const bool &selfLinks,
                      const bool &delayedRequests
                      );
    void webCrawlerDialogError(QString);
private:
    Ui::DialogWebCrawler ui;
    QString seedUrl ;
    int maxLinksPerPage, maxUrlsToCrawl;
    bool extLinks, intLinks;
    QStringList linkClasses;
    QStringList urlPatternsIncluded;
    QStringList urlPatternsExcluded;

};



#endif
