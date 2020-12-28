/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.6
 Written in Qt
 
                         webcrawler.h  -  description
                             -------------------
    copyright         : (C) 2005-2020 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

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

#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <QNetworkReply>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkRequest;

using namespace std;


class WebCrawler_Parser : public QObject  {
    Q_OBJECT
public:
    WebCrawler_Parser();
    ~WebCrawler_Parser();
    void load (const QString &seed,
               const QStringList &urlPatternsIncluded,
               const QStringList &urlPatternsExcluded,
               const QStringList &linkClasses,
               const int &maxNodes,
               const int &maxLinksPerPage,
               const bool &intLinks,
               const bool &childLinks,
               const bool &parentLinks,
               const bool &selfLinks,
               const bool &extLinksIncluded,
               const bool &extLinksCrawl,
               const bool &socialLinks);

public slots:
    void parse(QNetworkReply *reply);
    void newLink(int s, QUrl target, bool enqueue_to_frontier);
signals:
    void signalCreateNode(const int &no,
                          const QString &url,
                          const bool &signalMW=false);
    void signalCreateEdge (const int &source, const int &target);
    void startSpider();
    void finished (QString);
private:
    QByteArray ba;
    QMap <QUrl, int> knownUrls;
    QUrl  m_seed;
    int m_maxNodes;
    int m_discoveredNodes;
    int m_maxLinksPerPage;

    bool m_intLinks;
    bool m_childLinks;
    bool m_parentLinks;
    bool m_selfLinks ;
    bool m_extLinksIncluded;
    bool m_extLinksCrawl;
    bool m_socialLinks;
    bool m_urlIsSocial;

    QStringList m_urlPatternsIncluded;
    QString urlPattern;
    QStringList m_urlPatternsExcluded;
    QStringList m_linkClasses;
    QStringList m_socialLinksExcluded;
    QStringList::const_iterator constIterator;
    bool m_urlPatternAllowed;
    bool m_urlPatternNotAllowed;
    bool m_linkClassAllowed;
};


class  WebCrawler_Spider : public QObject  {
    Q_OBJECT
public:
    WebCrawler_Spider();
    ~WebCrawler_Spider();
    void load (
              //QNetworkAccessManager *NetworkManager,
            WebCrawler_Parser *wc_parser,
               const QString &seed,
               const int &maxNodes,
               const bool &delayedRequests);

public slots:
    void visitUrls();

signals:
    void getUrl(const QNetworkRequest &request);
    void parse(QNetworkReply *reply);
    void finished (QString);
private:
    //QNetworkAccessManager *manager;

    QNetworkReply *reply;
    QUrl currentUrl ;
    QString  m_seed;
    int m_maxNodes;
    int m_visitedNodes;
    int m_wait_msecs;
    bool m_delayedRequests;

};

#endif
