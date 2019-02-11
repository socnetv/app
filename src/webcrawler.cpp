/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.5
 Written in Qt
 
                         webcrawler.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
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
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QDebug>
#include <QQueue>
#include <QThread>


/*
 * frontier is our Url buffer (global)
 */
QQueue<QUrl> frontier;




/**
 * @brief spider's constructor - does nothing
 */
WebCrawler_Spider::WebCrawler_Spider() {
    qDebug() << "   wc_spider::WebCrawler_Spider() - thread() - " << thread();
}


/**
 * @brief  Called from Graph to init variables.
  * @param url
 * @param maxN
 * @param maxLinksPerPage
 * @param extLinks
 * @param intLinks
 */
void WebCrawler_Spider::load(QNetworkAccessManager *NetworkManager,
                             WebCrawler_Parser *wc_parser,
                             const QString &url,
                             const int &maxN,
                             const bool &delayedRequests) {
    qDebug() << "   wc_spider::load() - thread() - " << thread()
             << "Initializing vars ...";

    //manager = NetworkManager;

    QNetworkAccessManager *manager= new QNetworkAccessManager(this);

    connect (this, &WebCrawler_Spider::getUrl,
             manager, &QNetworkAccessManager::get);

    connect ( manager, &QNetworkAccessManager::finished,
               wc_parser, &WebCrawler_Parser::parse );


    m_seed=url;                             // the initial url/domain we will crawl
    m_maxNodes=maxN;                        // maximum urls we'll check == max nodes in the resulted network
    m_delayedRequests = delayedRequests;    // controls if we will wait between requests
    m_visitedNodes = 0;

}




/**
 * @brief Spider main functionality
 * Takes urls from frontier queue and downloads their HTML source code.
 */
void WebCrawler_Spider::visitUrls(){
    qDebug() << "   wc_spider::get() - ";

    //repeat forever....
    do {
        //  or until we crawl all urls in frontier.
        if (frontier.size() ==0 ) {
            qDebug () <<"##   wc_spider::get() - Frontier is empty. "
                        "No more urls to crawl. Breaking loop. ##"  ;
            break;
        }

        // or until we have reached m_maxNodes
        if (m_maxNodes>0) {
            if (m_visitedNodes == m_maxNodes ) {
                qDebug () <<"##   wc_spider::get() - Reached m_maxNodes!"
                         << "STOP ##" ;
                emit finished("message from spider: visitedNodes > maxnodes. ");
                break;
            }
        }

        qDebug() << "   wc_spider::get() - frontier size " << frontier.size()
                << " - Take the first url from frontier  ";

        currentUrl = frontier.dequeue();

        qDebug() << "   wc_spider::get() - url: "
                 <<  currentUrl << endl
                 << "url not visited. "
                 << "Increasing visitedNodes to" << m_visitedNodes + 1
                  << "Downloading html...";

        //request = new QNetworkRequest;
        QNetworkRequest request;
        request.setUrl(currentUrl);
        request.setRawHeader(
                    "User-Agent",
                    "SocNetV harmless spider - see https://socnetv.org");

        if (m_delayedRequests) {
            m_wait_msecs = rand() % 1000;
            qDebug() << "   wc_spider::get() - Sleeping for" << m_wait_msecs << "msecs";
            QThread::msleep(m_wait_msecs);
        }

        qDebug() << "   wc_spider::get() - emitting getUrl() to NetworkManager for url"
                 <<  currentUrl;

        emit getUrl(request);

        Q_UNUSED(reply);
        m_visitedNodes++;

        if ( QThread::currentThread()->isInterruptionRequested() ) {
            qDebug() << "==   wc_spider::get() - Thread isInterruptionRequested() - RETURN!!!";
             return;
         }
    } while ( 1 );

    qDebug() << "==   wc_spider::get() - Finished infinite loop!";
}





WebCrawler_Spider::~WebCrawler_Spider() {
    qDebug() << "   wc_spider::~WebCrawler_Spider() - deleting http object";
    m_visitedNodes = 0;
//    delete manager;
//    manager=0;

    qDebug() << "   wc_spider::~WebCrawler_Spider() - deleting reply object";

}




/**
 * @brief constructor called from parent thread - does nothing
 */
WebCrawler_Parser::WebCrawler_Parser() {
    qDebug () << "   wc_parser::WebCrawler_Parser() - thread:" << thread();

}



/**
 * @brief Called from parent Graph thread. Inits variables.
 * @param url
 * @param maxN
 * @param maxLinksPerPage
 * @param extLinks
 * @param intLinks
 */
void WebCrawler_Parser::load(const QString &url,
                             const QStringList &urlPatternsIncluded,
                             const QStringList &urlPatternsExcluded,
                             const QStringList &linkClasses,
                             const int &maxN,
                             const int &maxLinksPerPage,
                             const bool &extLinks,
                             const bool &intLinks,
                             const bool &childLinks,
                             const bool &parentLinks,
                             const bool &selfLinks) {

    qDebug () << "   wc_parser::load() - thread():"
              << thread()
              << "Initializing variables ";

    // Initialize user-defined control variables and limits

    m_seed=QUrl(url);                              // the initial url/domain we will crawl
    if (m_seed.path() == "/") {
        m_seed.setPath("");
    }
    m_urlPatternsIncluded = urlPatternsIncluded;    // list of url patterns to include
    m_urlPatternsExcluded = urlPatternsExcluded;    // list of url patterns to include
    m_linkClasses = linkClasses;                    // list of link classes to include
    m_maxNodes=maxN;                                // max urls we'll check == max nodes in the social network
    m_maxLinksPerPage = maxLinksPerPage;            // max links per page to search for
    m_extLinks = extLinks;
    m_intLinks = intLinks;
    m_selfLinks = selfLinks;
    m_childLinks = childLinks;
    m_parentLinks = parentLinks;

    //clear global variables
    frontier.clear();                               // the Url buffer (global)
    ba.clear();                                     // the byte array which stores the network reply
    knownUrls.clear();                              // a map of all known urls
    m_discoveredNodes=0;                            // a counter of discovered nodes

    m_discoveredNodes++;
    frontier.enqueue(m_seed);
    knownUrls[m_seed]=m_discoveredNodes;

    qDebug() << "   wc_parser::load() - seed:" << m_seed
             << " seed_host: " << m_seed.host()
             << " Added seed to frontier queue and knownUrls map. "
             << " Node " << m_discoveredNodes << " should be already created. "
             << " m_maxNodes " << m_maxNodes
             << " m_maxLinksPerPage " << m_maxLinksPerPage
             << " m_extLinks " << m_extLinks
             << " m_intLinks " << m_intLinks;

}




/**
 * @brief Called when NetworkManager has finished.
 * This method does the actual parsing of each page's html source from the reply bytearray.
 * First, we start by reading all from http reply to a QString called page.
 * Then we parse the page string, searching for url substrings.
 * @param reply
 */
void WebCrawler_Parser::parse(QNetworkReply *reply){
    qDebug () << "   wc_parser::parse() - thread:" << this->thread();

    // Find to which node the response HTML belongs to
    // Get this from the reply object request method
    QUrl currentUrl = reply->request().url();
    QString currentUrlStr = currentUrl.toString();
    QString locationHeader = reply->header(QNetworkRequest::LocationHeader).toString();
    int sourceNode = knownUrls [ currentUrl ];
    QString scheme = currentUrl.scheme();
    QString host = currentUrl.host();
    QUrl baseUrl = QUrl( scheme + "://" + host);
    QString path = currentUrl.path();
    qDebug() << "   wc_parser::parse() - HTML of url "
             << currentUrlStr << " sourceNode " << sourceNode;
    qDebug() << "   wc_parser::parse() - host " << host
              << " path " << path;


//    qDebug () << "   wc_parser::parse() -  original locationHeader"
//              << reply->header(QNetworkRequest::LocationHeader) ;
//    qDebug () << "   wc_parser::parse() -  decoded locationHeader" << locationHeader ;

//    qDebug () << "   wc_parser::parse() -  encoded currentUrl  " << currentUrl;
//    qDebug () << "   wc_parser::parse() -  decoded currentUrl " << currentUrlStr;


    // Check for redirects
    if ( locationHeader != "" && locationHeader != currentUrlStr ) {
        qDebug () << "&&   wc_parser::parse() Location response header "
                  << locationHeader
                  << " differs from currentUrl " << currentUrlStr
                  << " Creating node redirect - Creating edge - RETURN ";
        newLink( sourceNode, locationHeader , true );
        return;
    }


    QUrl newUrl;
    QString newUrlStr;
    int start=-1, end=-1, equal=-1 , invalidUrlsInPage =0; // index=-1;
    int validUrlsInPage = 0;

    ba=reply->readAll();            // read all data from the reply into a bytearray
    QString page(ba);               // construct a QString from the bytearray

    QString md5(QCryptographicHash::hash(ba,QCryptographicHash::Md5).toHex());
    qDebug () << "   wc_parser::parse() - MD5 sum:" << md5.toLocal8Bit();


    // If there are no links inside the HTML source, return
    if (!page.contains ("href"))  {

        //FIXME: Frameset pages are not parsed! See docs/manual.html for example.

        qDebug() << "##   wc_parser::parse() - Empty or not useful html from "
                 << currentUrl
                 << " page size " << page.size()
                 << " \npage contents: " << page
                 << " RETURN ##";

        return;

    }

    //    qDebug() <<  " \npage contents: " << page << endl << endl;

    // We only search inside <body>...</body> tags
    qDebug() << "   wc_parser::parse() - Finding <body></body> tags";
    start=page.indexOf ("<body");
    end=page.indexOf ("</body>");
    if ( start != -1 && end != -1 ) {
        page = page.remove(0, start);       // remove everything until <body>
        end=page.indexOf ("</body>");       // find new index pos of </body>
        page = page.left(end);              // keep everything until </body>
    }
    else if ( start == -1  ) {
        qDebug() << "   wc_parser::parse() - ERROR IN opening <body> tag";
    }
    else if ( end == -1  ) {
        qDebug() << "   wc_parser::parse() - ERROR IN locating closing </body> tag";
    }


    // Main Loop: While there are more links in the page, parse them
    while (page.contains("href")) {

        if (m_maxNodes>0) {
            if (m_discoveredNodes >= m_maxNodes ) {
                qDebug () <<"!!   wc_parser::parse() - Reached m_maxNodes! STOP ";
                emit finished("message from parse() -  discoveredNodes > maxNodes");
                return;
            }
        }

        // remove whitespace from the start and the end
        // all whitespace sequence becomes single space
        page=page.simplified();

        start=page.indexOf ("href");		//Find its pos

        page = page.remove(0, start);		//erase everything up to href

        equal=page.indexOf ("=");			// Find next equal sign (=)

        page = page.remove(0, equal+1);		//Erase everything up to =

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
        qDebug() << "   wc_parser::parse() - found newUrlStr "<< newUrlStr;

        newUrl = QUrl(newUrlStr);

        if (newUrl.isRelative()) {
            qDebug() << "   wc_parser::parse() - newUrl is RELATIVE. Merging baseUrl with this";
            newUrl=baseUrl.resolved(newUrl);
        }

        if (!newUrl.isValid()) {
            invalidUrlsInPage ++;
            qDebug() << "   wc_parser::parse() - found INVALID newUrl "
                        << newUrl.toString()
                        << " in page " << currentUrlStr
                        << " Will CONTINUE only if invalidUrlsInPage < 200";
            if (invalidUrlsInPage > 200) {
                qDebug() << "   wc_parser::parse() -  INVALID newUrls > 200";
                emit finished("invalidUrlsInPage > 200");
                return;
            }
            continue;
        }

        // TODO - REMOVE LAST / FROM EVERY PATH NOT ONLY ROOT PATH
        if (newUrl.path() == "/") {
            newUrl.setPath("");
        }

        qDebug() << "@@   wc_parser::parse() - found VALID newUrl: "
                 << newUrl.toString();


        newUrlStr = newUrl.toString();

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
                qDebug()<< "!!   wc_parser::parse() -  # newUrl "
                        << " seems a page resource or anchor (rss, favicon, etc) "
                        << "Skipping...";
                continue;
         }


         // Check if newUrl is compatible with the url patterns the user asked for
         m_urlPatternAllowed = true;
         for (constIterator = m_urlPatternsIncluded.constBegin();
              constIterator != m_urlPatternsIncluded.constEnd();
              ++constIterator)  {
             //qDebug() << (*constIterator).toLocal8Bit().constData() << endl;
             urlPattern = (*constIterator).toLocal8Bit().constData();
             if (urlPattern.isEmpty())
                 continue;
             if ( newUrl.toString().contains( urlPattern ) ) {
                 qDebug() << "--   wc_parser::parse() -  newUrl in allowed url patterns:"
                          << urlPattern
                          <<"Parsing";
                 break;
             }
             else {
                 qDebug() << "!!   wc_parser::parse() -  newUrl not in allowed url patterns. CONTINUE ";
                 m_urlPatternAllowed = false;
             }

         }


         m_urlPatternNotAllowed = false;
         for (constIterator = m_urlPatternsExcluded.constBegin();
              constIterator != m_urlPatternsExcluded.constEnd();
              ++constIterator)  {
             //qDebug() << (*constIterator).toLocal8Bit().constData() << endl;
             urlPattern = (*constIterator).toLocal8Bit().constData();
             if (urlPattern.isEmpty())
                 continue;
             if ( newUrl.toString().contains( urlPattern ) ) {
                 qDebug() << "!!   wc_parser::parse() -  newUrl in excluded url patterns:"
                          << urlPattern
                          << "CONTINUE ";
                m_urlPatternNotAllowed = true;
                break;
             }
             else {
                 qDebug() << "--   wc_parser::parse() -  newUrl not in excluded url patterns. Parsing";
             }

         }


        if (m_urlPatternAllowed && !m_urlPatternNotAllowed) {

            if ( newUrl.isRelative() ) {
                newUrl = currentUrl.resolved(newUrl);
                newUrlStr = newUrl.toString();

                qDebug() << "   wc_parser::parse() - newUrl is RELATIVE."
                            << " host: " << host
                            << " resolved url "
                            << newUrl.toString();

                if (!m_intLinks ){
                    qDebug()<< "   wc_parser::parse() - Internal URLs forbidden."
                            << " SKIPPING node creation";
                            continue;
                }

                if (currentUrl.path() == newUrl.path()) {
                    if  (m_selfLinks) {
                        qDebug()<< "   wc_parser::parse() - "
                                <<  " Creating self link";

                        newLink(sourceNode, newUrl, false);
                    }
                    else {
                        qDebug()<< "   wc_parser::parse() - "
                                << "currentUrl.path() = newUrl.path()"
                                <<  "Self links not allowed. CONTINUE.";
                    }

                }
                else {
                    qDebug()<< "   wc_parser::parse() - Internal URLs allowed."
                            <<  " Creating new node and ADDING it to frontier...";
                    this->newLink(sourceNode, newUrl, true);

                }
            }
            else {
                qDebug() << "   wc_parser::parse() - newUrl is ABSOLUTE.";

                if ( newUrl.scheme() != "http"  && newUrl.scheme() != "https"  &&
                          newUrl.scheme() != "ftp" && newUrl.scheme() != "ftps") {
                    qDebug() << "   wc_parser::parse() - INVALID newUrl SCHEME"
                                << newUrl.toString()
                                << "CONTINUE.";
                    continue;
                }

                if (  newUrl.host() != host  ) {
                    qDebug()<< "   wc_parser::parse() - newUrl ABSOLUTE & EXTERNAL.";
                    if ( !m_extLinks ) {
                        qDebug()<< "   wc_parser::parse() - External URLs forbidden."
                                <<" Creating new node but NOT ADDING it to frontier...";
                        newLink(sourceNode, newUrl, false);
                    }
                    else {
                        qDebug()<< "   wc_parser::parse() - External URLs allowed."
                                <<" Creating new node and ADDING it to frontier...";
                        newLink(sourceNode, newUrl, true);
                    }
                }
                else {
                    qDebug()<< "   wc_parser::parse() - newUrl ABSOLUTE & INTERNAL.";

                    if (!m_intLinks){
                        qDebug()<< "   wc_parser::parse() - Internal URLs forbidden."
                                  << " SKIPPING node creation";
                        continue;
                    }

                    if (  newUrl.path () == path && !m_selfLinks) {
                        qDebug()<< "   wc_parser::parse() - "
                                <<  "Self links forbidden. CONTINUE.";
                        continue;
                    }

                    if ( newUrl.isParentOf(currentUrl) && !m_parentLinks ) {
                        qDebug()<< "   wc_parser::parse() - "
                                << "Parent URLs forbidden. CONTINUE";
                        continue;
                    }
                    if ( currentUrl.isParentOf(newUrl) && !m_childLinks ) {
                        qDebug()<< "   wc_parser::parse() - "
                                << "Child URLs forbidden. CONTINUE";
                        continue;
                    }

                    qDebug()<< "   wc_parser::parse() -  Internal, absolute newURL allowed."
                            <<" Creating new node and ADDING it to frontier...";
                    newLink(sourceNode, newUrl, true);
                }
            }

        }

        validUrlsInPage ++;

        qDebug() << "   wc_parser::parse() - validUrlsInPage " << validUrlsInPage;

        // If the user has specified a maxLinksPerPage limit then,
        // if we have reached it, stop parsing this page
        if ( m_maxLinksPerPage  != 0 ) {
            if ( validUrlsInPage > m_maxLinksPerPage ) {
                qDebug () <<"!!   wc_spider::parse() Reached m_maxLinksPerPage "
                         <<m_maxLinksPerPage << " - STOP parsing this page."  ;
                break;
            }
        }
    } // end while there are more links

}





/**
 * @brief signals node creation  - Called from wc_parser::load()
 * @param s
 * @param target
 * @param enqueue_to_frontier
 */
void WebCrawler_Parser::newLink(int s, QUrl target,  bool enqueue_to_frontier) {
    qDebug() << "   wc_parser::newLink() - source" <<  s << "target" << target.toString();

    if (m_maxNodes>0) {
        if (m_discoveredNodes >= m_maxNodes ) {
            qDebug () <<"##   wc_parser::newLink() -  We have reached m_maxNodes!"
                     << " - STOP! ###" ;
            emit finished("maxpages from newLink");
            return;
        }
    }


    // check if the new url has been discovered previously
    QMap<QUrl, int>::const_iterator index = knownUrls.find(target);
    if (   index!= knownUrls.end() ) {
        qDebug()<< "--   wc_parser::newLink() - target already discovered "
                << " in knownUrls as node:" << index.value();
        if  (s !=index.value()) {
            qDebug()<< "--   wc_parser::newLink() - creating link"
                    << s << "->"
                    << index.value()
                    << "then RETURN.";
            emit signalCreateEdge (s, index.value() );
        }
        else {
            qDebug()<< "--   wc_parser::newLink() - Self links not allowed. RETURN.";
        }
        return;
    }

    m_discoveredNodes++;
    knownUrls[target]=m_discoveredNodes;
    emit signalCreateNode( m_discoveredNodes, target.toString(), false);
    qDebug()<< "**   wc_parser::newLink() - Creating node " << m_discoveredNodes
            << " url "<< target.toString();

    if (enqueue_to_frontier) {

        frontier.enqueue(target);
        qDebug()<< "**   wc_parser::newLink() - Enqueuing new node to frontier "
                   << " frontier size: "<<  frontier.size();

        emit startSpider();
    }
    else {
        qDebug()<< "##   wc_parser::newLink() - NOT enqueuing to frontier";
    }

    qDebug()<< "--   wc_parser::newLink() - Creating edge from "
            << s << " to " << m_discoveredNodes;

    emit signalCreateEdge (s, m_discoveredNodes);


}



WebCrawler_Parser::~WebCrawler_Parser() {
        qDebug() << "   wc_parser::~WebCrawler_Parser() - clearing vars";
        ba.clear();
        frontier.clear();
        knownUrls.clear();
        m_urlPatternsIncluded.clear();
        urlPattern="";
        m_urlPatternsExcluded.clear();
        m_linkClasses.clear();
        m_discoveredNodes = 0;


}

