/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1-dev
 Written in Qt
 
                      WebCrawler.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
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

#include "webcrawler.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QUrl>
#include <QThread>
#include <QDeadlineTimer>

/**
 * @brief Constructor from parent Graph thread. Inits variables.
 * @param url
 * @param maxNc
 * @param maxLinksPerPage
 * @param extLinks
 * @param intLinks
 */
WebCrawler::WebCrawler(QQueue<QUrl> *urlQueue,
        const QUrl &startUrl,
        const QStringList &urlPatternsIncluded,
        const QStringList &urlPatternsExcluded,
        const QStringList &linkClasses,
        const int &maxN,
        const int &maxLinksPerPage,
        const bool &intLinks,
        const bool &childLinks,
        const bool &parentLinks,
        const bool &selfLinks,
        const bool &extLinksIncluded,
        const bool &extLinksCrawl,
        const bool &socialLinks,
        const int &delayBetween) {

    qDebug () << "WebCrawler constructed on thread:"
              << thread()
              << "Initializing variables...";

    m_urlQueue = urlQueue;

    m_initialUrl = startUrl;

    // Initialize user-defined control variables and limits

    m_urlPatternsIncluded = urlPatternsIncluded;    // list of url patterns to include
    m_urlPatternsExcluded = urlPatternsExcluded;    // list of url patterns to include
    m_linkClasses = linkClasses;                    // list of link classes to include
    m_maxUrls=maxN;                                 // max urls we'll check == max nodes in the social network
    m_maxLinksPerPage = maxLinksPerPage;            // max links per page to search for

    m_intLinks = intLinks;
    m_selfLinks = selfLinks;
    m_childLinks = childLinks;
    m_parentLinks = parentLinks;

    m_extLinksIncluded = extLinksIncluded;
    m_extLinksCrawl = extLinksCrawl;
    m_socialLinks = socialLinks;
    m_socialLinksExcluded << "facebook.com"
                          << "twitter.com"
                          << "linkedin.com"
                          << "instagram.com"
                          << "pinterest.com"
                          << "telegram.org"
                          << "telegram.me"
                          << "youtube.com"
                          << "reddit.com"
                          << "tumblr.com"
                          << "flickr.com"
                          << "plus.google.com";

    m_delayBetween = delayBetween;


    knownUrls.clear();                              // a map of all known urls to their node number

    m_discoveredNodes=1;                            // Counts discovered nodes -- Set the counter to 1, as we already know the initial url

    knownUrls[m_initialUrl]=m_discoveredNodes;      // Add the initial url to the map of known urls as node numbered 1.

    qDebug() << "initialUrl:" << m_initialUrl.toString()
             << " m_maxUrls " << m_maxUrls
             << " m_maxLinksPerPage " << m_maxLinksPerPage
             << " m_intLinks " << m_intLinks
             << " m_extLinksIncluded " << m_extLinksIncluded
             << " m_extLinksCrawl"<<m_extLinksCrawl
             << " m_socialLinks"<<m_socialLinks;

}




/**
 * @brief Called from Graph when a network reply for a new page download has finished
 * to do the actual parsing of that page's html source from the reply bytearray.
 * First, we start by reading all from http reply to a QString called page.
 * Then we parse the page string, searching for url substrings.
 * @param reply
 */
void WebCrawler::parse(QNetworkReply *reply){

    qDebug () << "Parsing new network reply, on thread:" << this->thread();

    // Find the node the response HTML belongs to
    // Get this from the reply object request method
    QUrl currentUrl = reply->request().url();
    QString currentUrlStr = currentUrl.toString();
    QString locationHeader = reply->header(QNetworkRequest::LocationHeader).toString();
    int sourceNode = knownUrls [ currentUrl ];
    QString scheme = currentUrl.scheme();
    QString host = currentUrl.host();
    QUrl baseUrl = QUrl( scheme + "://" + host);
    QString path = currentUrl.path();
    qDebug() << "Reply HTML belongs to url:" << currentUrlStr
             << " sourceNode:" << sourceNode
             << "host:" << host
             << "path:" << path;


    // Check for redirects
    if ( locationHeader != "" && locationHeader != currentUrlStr ) {
        qDebug () << "REDIRECT - Location response header:"
                  << locationHeader
                  << "differs from currentUrl:" << currentUrlStr
                  << "Calling newLink() to create node redirect, and RETURNing...";
        newLink( sourceNode, locationHeader , true );
        return;
    }

    QUrl newUrl;
    QString newUrlStr;
    int start=-1, end=-1, equal=-1 , invalidUrlsInPage =0; // index=-1;
    int validUrlsInPage = 0;

    QByteArray ba = reply->readAll();       // read all data from the reply into a bytearray
    reply->deleteLater();                   // schedule reply to be deleted
    QString page(ba);                       // construct a QString from the bytearray

    // Create a md5 hash of the page code
    QString md5(QCryptographicHash::hash(ba,QCryptographicHash::Md5).toHex());

    qDebug () << "Reply MD5 sum:" << md5.toLocal8Bit();

    // If there are no links inside the HTML source, return
    if (!page.contains ("href"))  {

        //FIXME: Frameset pages are not parsed! See docs/manual.html for example.

        qDebug() << "Empty or not useful html from:"
                 << currentUrl
                 << "page size:" << page.size()
                 << "\npage contents: " << page
                 << "\nRETURN ##";

        return;

    }

    //    qDebug() <<  " \npage contents: " << page << "\n\n";

    // We only search inside <body>...</body> tags
    qDebug() << "Finding <body></body> tags";

    start=page.indexOf ("<body");
    end=page.indexOf ("</body>");

    if ( start != -1 && end != -1 ) {
        page = page.remove(0, start);       // remove everything until <body>
        end=page.indexOf ("</body>");       // find new index pos of </body>
        page = page.left(end);              // keep everything until </body>
    }
    else if ( start == -1  ) {
        qDebug() << "ERROR IN opening <body> tag";
    }
    else if ( end == -1  ) {
        qDebug() << "ERROR IN locating closing </body> tag";
    }


    // Main Loop: While there are more links in the page, parse them
    qDebug() << "Searching for href links inside the page code...";
    while (page.contains(" href")) {

        if ( QThread::currentThread()->isInterruptionRequested() ) {
            qDebug () <<"STOP because currentThread()->isInterruptionRequested() == true.";
            emit finished("message from parse() -  interruptionRequested");
            return;
        }
        if (m_maxUrls>0) {
            if (m_discoveredNodes >= m_maxUrls ) {
                qDebug () <<"STOP because I have reached m_maxUrls.";
                emit finished("message from parse() -  discoveredNodes > maxNodes");
                return;
            }
        }

        // remove whitespace from the start and the end
        // all whitespace sequence becomes single space
        page=page.simplified();

        start=page.indexOf (" href");		//Find its pos
        // Why " href" instead of "href"?
        // Because href might be inside random strings/tokens or
        // even in share links (ie. whatsapp://sadadasd &href=url)
        // By searching for " href" we avoid some of these cases, without
        // introducing other serious problems.

        page = page.remove(0, start);		//erase everything up to href

        equal=page.indexOf ("=");			// Find next equal sign (=)

        page=page.remove(0, equal+1);		//Erase everything up to =

        if (page.startsWith("\"") ) {
            page.remove(0,1);
            end=page.indexOf ("\"");
        }
        else if (page.startsWith("\'") ){
            page.remove(0,1);
            end=page.indexOf ("\'");
        }
        else {
            //end=page.indexOf ("\'");
        }

        newUrlStr=page.left(end);			//Save new url to newUrl :)

        newUrlStr=newUrlStr.simplified();
        qDebug() << "Found new URL:"<< newUrlStr << "Examining it...";

        newUrl = QUrl(newUrlStr);

        if (newUrl.isRelative()) {
            qDebug() << "newUrl is RELATIVE. Merging baseUrl with it.";
            newUrl=baseUrl.resolved(newUrl);
        }

        if (!newUrl.isValid()) {
            invalidUrlsInPage ++;
            qDebug() << "newUrl is INVALID: "
                        << newUrl.toString()
                        << "in page " << currentUrlStr
                        << "SKIPPING IT. Will continue in this page, only if invalidUrlsInPage < 200";
            if (invalidUrlsInPage > 200) {
                qDebug() << "RETURN because invalid newUrls > 200";
                emit finished("invalidUrlsInPage > 200");
                return;
            }
            continue;
        }

        // TODO - REMOVE LAST / FROM EVERY PATH NOT ONLY ROOT PATH
        if (newUrl.path() == "/") {
            newUrl.setPath("");
        }

        newUrlStr = newUrl.toString();

        qDebug() << "newUrl is VALID:" << newUrlStr
                 << "Checking if it is resource and if is allowed...";

        // Skip css, favicon, rss, ping, etc
        if ( newUrlStr.startsWith("#", Qt::CaseInsensitive) ||
             newUrlStr.endsWith("feed/", Qt::CaseInsensitive) ||
             newUrlStr.endsWith("rss/", Qt::CaseInsensitive) ||
             newUrlStr.endsWith("atom/", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith("xmlrpc.php", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".xml", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".ico", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".gif", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".png", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".jpg", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".js", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".css", Qt::CaseInsensitive) ||
             newUrl.fileName().endsWith(".rsd", Qt::CaseInsensitive)   )    {
            qDebug()<< "newUrl is a page resource or anchor (rss, favicon, etc) "
                        << "Skipping...";
            continue;
        }

        // Check if newUrl is compatible with the url patterns the user asked for
        m_urlPatternAllowed = true;

        for (constIterator = m_urlPatternsIncluded.constBegin();
             constIterator != m_urlPatternsIncluded.constEnd();
             ++constIterator)  {

            urlPattern = (*constIterator).toLocal8Bit().constData();

            if (urlPattern.isEmpty()) {
                continue;
            }

            if ( newUrl.toString().contains( urlPattern ) ) {
                qDebug() << "newUrl in allowed url patterns:"
                          << urlPattern
                          << "OK.";
                break;
            }
            else {
                qDebug() << "newUrl not in allowed url patterns. I WILL SKIP IT. ";
                m_urlPatternAllowed = false;
            }

        }


        m_urlPatternNotAllowed = false;

        for (constIterator = m_urlPatternsExcluded.constBegin();
             constIterator != m_urlPatternsExcluded.constEnd();
             ++constIterator)  {

            urlPattern = (*constIterator).toLocal8Bit().constData();

            if (urlPattern.isEmpty())
                continue;
            if ( newUrl.toString().contains( urlPattern ) ) {
                qDebug() << "newUrl in excluded url patterns:"
                          << urlPattern
                          << "I WILL SKIP IT.";
                m_urlPatternNotAllowed = true;
                break;
            }
            else {
                qDebug() << "newUrl not in excluded url patterns. OK.";
            }

        }


        if (m_urlPatternAllowed && !m_urlPatternNotAllowed) {

            if ( newUrl.isRelative() ) {
                newUrl = currentUrl.resolved(newUrl);
                newUrlStr = newUrl.toString();

                qDebug() << "newUrl is RELATIVE."
                            << "host: " << host
                            << "resolved url:"
                            << newUrl.toString();

                if (!m_intLinks ){
                    qDebug()<< "SKIPPING node/edge creation because internal URLs are forbidden.";
                    continue;
                }

                if (currentUrl.path() == newUrl.path()) {
                    if  (m_selfLinks) {
                        qDebug()<< "Self-link found. Creating it!";

                        newLink(sourceNode, newUrl, false);
                    }
                    else {
                        qDebug()<< "Self-link found but self-links are not allowed. I will not create it.";
                    }

                }
                else {
                    qDebug()<< "Internal URLs allowed. Calling newLink() ";
                    this->newLink(sourceNode, newUrl, true);

                }
            }
            else {

                qDebug() << "newUrl is ABSOLUTE.";

                if ( newUrl.scheme() != "http"  && newUrl.scheme() != "https"  &&
                     newUrl.scheme() != "ftp" && newUrl.scheme() != "ftps") {
                    qDebug() << "INVALID newUrl SCHEME"
                                << newUrl.toString()
                                << "CONTINUE.";
                    continue;
                }

                if (  newUrl.host() != host  ) {
                    qDebug()<< "newUrl ABSOLUTE & EXTERNAL.";
                    if ( !m_extLinksIncluded ) {
                        qDebug()<< "External URLs forbidden. CONTINUE";
                        continue;
                    }
                    else {
                        m_urlIsSocial = false;
                        if ( !m_socialLinks ) {
                            for (constIterator = m_socialLinksExcluded.constBegin();
                                 constIterator != m_socialLinksExcluded.constEnd();
                                 ++constIterator)  {
                                urlPattern = (*constIterator).toLocal8Bit().constData();
                                if ( newUrl.host().contains ( urlPattern) ) {
                                    m_urlIsSocial = true;
                                    break;
                                }
                            }
                            if ( m_urlIsSocial) {
                                qDebug() << "newUrl in excluded social links:"
                                         << urlPattern
                                         << "CONTINUE ";
                                continue;
                            }
                        }
                        if ( m_extLinksCrawl ) {
                            qDebug()<< "External URLs are included and to be crawled. Calling newLink...";
                            newLink(sourceNode, newUrl, true);
                        }
                        else {
                            qDebug()<< "External URLs included but not to be crawled. Calling newLink() but the url will not be added to the queue";
                            newLink(sourceNode, newUrl, false);
                        }
                    }
                }
                else {
                    qDebug()<< "newUrl ABSOLUTE & INTERNAL.";

                    if (!m_intLinks){
                        qDebug()<< "Internal URLs forbidden."
                                  << "SKIPPING node creation";
                        continue;
                    }

                    if (  newUrl.path () == path && !m_selfLinks) {
                        qDebug()<< "Self links forbidden. CONTINUE.";
                        continue;
                    }

                    if ( newUrl.isParentOf(currentUrl) && !m_parentLinks ) {
                        qDebug()<< "Parent URLs forbidden. CONTINUE";
                        continue;
                    }
                    if ( currentUrl.isParentOf(newUrl) && !m_childLinks ) {
                        qDebug()<< "Child URLs forbidden. CONTINUE";
                        continue;
                    }

                    qDebug()<< "Internal, absolute newURL allowed. Calling newLink...";
                    newLink(sourceNode, newUrl, true);
                }
            }

        } // end if in allowed patterns

        validUrlsInPage ++;

        qDebug() << "validUrlsInPage:" << validUrlsInPage << "in page:" << currentUrlStr;

        // If the user has specified a maxLinksPerPage limit then,
        // if we have reached it, stop parsing this page
        if ( m_maxLinksPerPage  != 0 ) {
            if ( validUrlsInPage > m_maxLinksPerPage ) {
                qDebug () <<"STOP because I reached m_maxLinksPerPage "
                         <<m_maxLinksPerPage << " per page."  ;
                break;
            }
        }

    } // end while there are more links

}





/**
 * @brief ??
 * @param s
 * @param target
 * @param enqueue_to_frontier
 */
void WebCrawler::newLink(int s, QUrl target,  bool enqueue_to_frontier) {

    qDebug() << "Creating new edge, source:" <<  s << "target:" << target.toString();

    if (m_maxUrls>0 && m_discoveredNodes >= m_maxUrls ) {
            qDebug () <<"STOP because we have reached m_maxUrls!";
            emit finished("maxpages from newLink");
            return;
    }

    // check if the new url has been discovered previously
    QMap<QUrl, int>::const_iterator index = knownUrls.constFind(target);
    if (  index!= knownUrls.constEnd() ) {
        qDebug()<< "Target already discovered (in knownUrls) as node:" << index.value();
        if  (s !=index.value()) {

            if ( QThread::currentThread()->isInterruptionRequested() ) {
                qDebug () <<"STOP because currentThread()->isInterruptionRequested() == true.";
                emit finished("message from parse() -  interruptionRequested");
                return;
            }

            qDebug()<< "Emitting signalCreateEdge"
                    << s << "->"
                    << index.value()
                    << "and RETURN.";
            emit signalCreateEdge (s, index.value() );
        }
        else {
            qDebug()<< "Self links not allowed. RETURN.";
        }
        return;
    }

    m_discoveredNodes++;
    knownUrls[target]=m_discoveredNodes;

    if ( QThread::currentThread()->isInterruptionRequested() ) {
        qDebug () <<"STOP because currentThread()->isInterruptionRequested() == true.";
        emit finished("message from parse() -  interruptionRequested");
        return;
    }

    qDebug()<< "Target just discovered. Emitting signalCreateNode:" << m_discoveredNodes
            << "for url:"<< target.toString();

    emit signalCreateNode( m_discoveredNodes, target.toString(), false);

    if (enqueue_to_frontier) {

        m_urlQueue->enqueue(target);

        qDebug()<< "Enqueued new node:" << m_discoveredNodes <<  "to urlQueue."
                   << "queue size: "<<  m_urlQueue->size()
                   << " - Emitting signalStartSpider...";

        // Check if we need to add some delay between requests
        if (m_delayBetween) {
            int m_wait_msecs = rand() % m_delayBetween;
            qDebug() << "delay requested between signalStartSpider() calls. Setting a deadline for:" << m_wait_msecs << "msecs";
            QDeadlineTimer deadline(m_wait_msecs);
            do {
//                qDebug() << "deadline.remainingTime():" << deadline.remainingTime() << "msecs";
            } while (deadline.remainingTime() > 0 && this->thread()->isRunning());

            if ( QThread::currentThread()->isInterruptionRequested() ) {
                qDebug () <<"STOP because currentThread()->isInterruptionRequested() == true.";
                emit finished("message from parse() -  interruptionRequested");
                return;
            }

            emit signalStartSpider();
        }
        else {
            if ( QThread::currentThread()->isInterruptionRequested() ) {
                qDebug () <<"STOP because currentThread()->isInterruptionRequested() == true.";
                emit finished("message from parse() -  interruptionRequested");
                return;
            }
            emit signalStartSpider();
        }



    }
    else {
        qDebug()<< "NOT adding new node to queue";
    }

    if ( QThread::currentThread()->isInterruptionRequested() ) {
        qDebug () <<"STOP because currentThread()->isInterruptionRequested() == true.";
        emit finished("message from parse() -  interruptionRequested");
        return;
    }

    qDebug()<< "Emitting signalCreateEdge"
            << s << "->" << m_discoveredNodes;

    emit signalCreateEdge (s, m_discoveredNodes);


}



WebCrawler::~WebCrawler() {

    qDebug() << "Destructor running. Clearing vars...";

    knownUrls.clear();
    m_urlPatternsIncluded.clear();
    urlPattern="";
    m_urlPatternsExcluded.clear();
    m_linkClasses.clear();
    m_discoveredNodes = 0;


}

