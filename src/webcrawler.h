/**
 * @file WebCrawler.h
 * @brief Declares the WebCrawler class for extracting and processing network data from web pages and online resources.
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


#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <QNetworkReply>
#include <QQueue>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

using namespace std;

/**
 * @brief The WebCrawler class
 * Parses HTML code it receives, locates urls inside it and puts them into a url queue (passed from the parent)
 * while emitting signals to the parent to create new nodes and edges between them.
 */
class WebCrawler : public QObject  {
    Q_OBJECT
    //    QThread wc_spiderThread;
public:

    WebCrawler (
            QQueue<QUrl> *urlQueue,
            const QUrl &startUrl,
            const QStringList &urlPatternsIncluded,
            const QStringList &urlPatternsExcluded,
            const QStringList &linkClasses,
            const int &maxNodes,
            const int &maxLinksPerPage,
            const bool &intLinks = true,
            const bool &childLinks = true,
            const bool &parentLinks = false,
            const bool &selfLinks = false,
            const bool &extLinksIncluded = false,
            const bool &extLinksCrawl = false,
            const bool &socialLinks = false,
            const int &delayBetween  = 0
            );

    ~WebCrawler();

public slots:
    void parse(QNetworkReply *reply);
    void newLink(int s, QUrl target, bool enqueue_to_frontier);

signals:
    void signalCreateNode(const int &no,
                          const QString &url,
                          const bool &signalMW=false);
    void signalCreateEdge (const int &source, const int &target);
    void signalStartSpider();
    void finished (QString);

private:
    QQueue<QUrl> *m_urlQueue;
    QMap <QUrl, int> knownUrls;
    QUrl m_initialUrl;
    int m_maxUrls;
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

    int m_delayBetween;

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



#endif
