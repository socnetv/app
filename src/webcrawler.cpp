/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt
 
                         webcrawler.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QDebug>
#include <QQueue>


/*
 * frontier is our Url buffer (global)
 */
QQueue<QUrl> frontier;


/*
 * constructor called from parent thread
 * - Inits variables
 * - Connects http NetworkManager signal to httpfinished() slot
 * which in turn emits http reply to WebCrawler_Parser
 */
WebCrawler_Spider::WebCrawler_Spider(QString url,
        int maxN,
        int maxLinksPerPage,
        bool extLinks,
        bool intLinks) {
    qDebug() << "   wc_spider::WebCrawler_Spider() "
                << " this->thread() " << this->thread()
                 << " Initializing vars ...";

    m_seed=url;       //the initial url/domain we will crawl
    m_maxPages=maxN;  //maxPages we'll check
    m_maxLinksPerPage = maxLinksPerPage;
    m_extLinks = extLinks;
    m_intLinks = intLinks;
    m_visitedNodes = 0;

    qDebug() << "   wc_spider::WebCrawler_Spider() Creating http";
    http = new QNetworkAccessManager(this);

    qDebug() << "   wc_spider::WebCrawler_Spider()  Connecting http signals";
    connect ( http, &QNetworkAccessManager::finished,
              this, &WebCrawler_Spider::httpFinished );

    qDebug () << "  wc_spider::WebCrawler_Spider() http->thread() "
              << http->thread() ;

}



WebCrawler_Spider::~WebCrawler_Spider() {
    m_visitedNodes = 0;
    delete http;
}

/*
 * WebCrawer_Spider main functionality
 * Parses urls from frontier and downloads their data.
*  When http signals finished() the response data are passed to
*  wc_parser thread to parse them
*/
void WebCrawler_Spider::get(){
    qDebug() << "   wc_spider::get():";
    do { 	//repeat forever....

        //  or until we crawl all urls in frontier.
        if (frontier.size() ==0 ) {
            qDebug () <<"   wc_spider::get() #### Frontier is empty: "
                     <<frontier.size() << " - we will stop now "  ;
            break;
        }

        // or until we have reached maxPages
        if (m_maxPages>0) {
            if (m_visitedNodes == m_maxPages ) {
                qDebug () <<"   wc_spider::get(): #### Reached m_maxPages!"
                         << " - STOP" ;
                emit finished("maxpages from spider");
                break;
            }
        }

        qDebug() << "   wc_spider::get():"
                 <<" frontier size " << frontier.size()
                << " - Take the first url from frontier  ";

        currentUrl = frontier.dequeue();

        qDebug() << "   wc_spider::get():  currentUrl: "
                 <<  currentUrl;


        qDebug() << "   wc_spider::get(): currentUrl not visited."
                 << " Increasing visitedNodes to: " << m_visitedNodes + 1
                 << " Let us visit it.";

        qDebug() << "   wc_spider::get(): currentUrl: "
                 <<  currentUrl
                  << " downloading html ";

        request = new QNetworkRequest;
        request->setUrl(currentUrl);
        request->setRawHeader(
                    "User-Agent",
                    "SocNetV innocent spider 1 - see http://socnetv.sf.net");

        qDebug() << "   wc_spider::get(): http->get() ";

        qDebug() << "   wc_spider::get() http->thread() " << http->thread() ;

        QNetworkReply *reply =  http->get(*request) ;
        Q_UNUSED(reply);
        m_visitedNodes++;


    } while ( 1 );

    qDebug() << "   wc_spider::get() Finished!";
}


void WebCrawler_Spider::httpFinished(QNetworkReply *reply){
    qDebug() << "   wc_spider::httpFinished()";
    emit parse (reply);
}




//constructor called from parent thread - does nothing
WebCrawler_Parser::WebCrawler_Parser(QString url,
                                     int maxN,
                                     int maxLinksPerPage,
                                     bool extLinks,
                                     bool intLinks) {
    qDebug () << "   wc_parser::WebCrawler_Parser() this->thread() "
              << this->thread()
                 << " Initializing variables ";

    m_seed=QUrl (url);   //the initial url/domain we will crawl
    m_maxPages=maxN;  //maxPages we'll check
    m_maxLinksPerPage = maxLinksPerPage;
    m_extLinks = extLinks;
    m_intLinks = intLinks;

    //clear global variables
    frontier.clear();
    ba.clear();
    knownUrls.clear();
    m_discoveredNodes=0;

    m_discoveredNodes++;
    frontier.enqueue(m_seed);
    knownUrls[m_seed]=m_discoveredNodes;

    qDebug() << "   wc_parser::WebCrawler_Parser()  seed: " << m_seed
             << " seed_domain: " << m_seed.host()
             << " Added seed to frontier queue and knownUrls map. "
             << " Node " << m_discoveredNodes << " should be already created. "
             << " m_maxPages " << m_maxPages
             << " m_maxLinksPerPage " << m_maxLinksPerPage
             << " m_extLinks " << m_extLinks
             << " m_intLinks " << m_intLinks;

}


WebCrawler_Parser::~WebCrawler_Parser() {
        ba.clear();
        frontier.clear();
        knownUrls.clear();
        m_discoveredNodes = 0;


}
/*
 * This method is called when http has finished all pending requests.
 * First, we start by reading all from http to the QString page.
 * Then we parse the page string, searching for url substrings.
 */
void WebCrawler_Parser::parse(QNetworkReply *reply){
    qDebug () << "   wc_parser::parse() thread " << this->thread();
    // find to which node the response HTML belongs to
    // Get this from the reply object request method
    QUrl requestUrl = reply->request().url();
    QString requestUrlStr = requestUrl.toString();
    QString locationHeader = reply->header(QNetworkRequest::LocationHeader).toString();
    int sourceNode = knownUrls [ requestUrl ];
    QString host = requestUrl.host();
    QString path = requestUrl.path();
    qDebug() << "   wc_parser::parse() response HTML of url "
             << requestUrlStr << " sourceNode " << sourceNode
              << " host " << host
              << " path " << path;

    qDebug () << "   wc_parser::parse(): original locationHeader"
              << reply->header(QNetworkRequest::LocationHeader) ;
    qDebug () << "   wc_parser::parse(): decoded locationHeader" << locationHeader ;

    qDebug () << "   wc_parser::parse(): encoded requestUrl  " << requestUrl;
    qDebug () << "   wc_parser::parse(): decoded requestUrl " << requestUrlStr;

    if ( locationHeader != "" && locationHeader != requestUrlStr ) {
        qDebug () << "&&   wc_parser::parse() Location response header "
                  << locationHeader
                  << " differs from requestUrl " << requestUrlStr
                  << " Creating node redirect - Creating edge - RETURN ";
        newLink( sourceNode, locationHeader , true );
        return;
    }


    QUrl newUrl;
    QString newUrlStr;
    int start=-1, end=-1, equal=-1 , invalidUrlsInPage =0; // index=-1;
    int validUrlsInPage = 0;
    ba=reply->readAll();
    QString page(ba);

    if (!page.contains ("href"))  { //if a href doesnt exist, return
        //FIXME: Frameset pages are not parsed! See docs/manual.html for example.
        qDebug() << "   wc_parser::parse(): ### Empty or not useful html from "
                 << requestUrl
                 << " page size " << page.size()
                 << " \npage contents: " << page
                 << " RETURN";
        return;
    }

    qDebug() << "   wc_parser::parse(): erasing <head></head>";
    start=page.indexOf ("<head");		//Find head pos -- deliberately open tag
    end=page.indexOf ("</head>");		//Find head pos
    if ( start != -1 && end != -1 ) {
        page = page.remove(start, end);		//erase head
    }
    else if ( start == -1  ) {
        qDebug() << "   wc_parser::parse(): ERROR IN locating tag <head> start";
    }
    else if ( end == -1  ) {
        qDebug() << "   wc_parser::parse(): ERROR IN locating tag </head> end";
    }

    while (page.contains("<script")) {
        qDebug () << "   wc_parser::parse(): deleting scripts";
        start=page.indexOf ("<script");		//Find pos -- deliberately open tag
        end=page.indexOf ("</script>");		//Find pos
        if ( start != -1 && end != -1 ) {
            page = page.remove(start, end);		//erase head
        }
    }

    while (page.contains("href")) {	//as long there is a href in the page...

        if (m_maxPages>0) {
            if (m_discoveredNodes >= m_maxPages ) {
                qDebug () <<"!!   wc_parser::parse(): Reached maxPages! STOP ";
                emit finished("maxpages from parse()");
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

        newUrl = QUrl(newUrlStr);

        if (!newUrl.isValid()) {
            invalidUrlsInPage ++;
            qDebug() << "   wc_parser::parse(): found INVALID newUrl "
                        << newUrl.toString()
                        << " in page " << requestUrlStr
                        << " Will CONTINUE only if invalidUrlsInPage < 200";
            if (invalidUrlsInPage > 200) {
                qDebug() << "   wc_parser::parse(): INVALID newUrls > 200";
                emit finished("invalidUrlsInPage > 200");
                return;
            }
            continue;
        }

        qDebug() << "@@   wc_parser::parse(): found VALID newUrl: "
                 << newUrlStr
                 << " in page " << requestUrlStr
                 << " decoded newUrl " << newUrl.toString();


        newUrlStr = newUrl.toString();

        //	...skip css, favicon, rss, ping, etc
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
                qDebug()<< "!!   wc_parser::parse(): # newUrl "
                        << " seems a page resource or anchor (rss, favicon, etc) "
                        << "Skipping...";
                continue;
         }


        if ( newUrl.isRelative() ) {
            newUrl = requestUrl.resolved(newUrl);
            newUrlStr = newUrl.toString();

            qDebug() << "   wc_parser::parse(): isRelative TRUE"
                        << " host: " << host
                        << " resolved url "
                        << newUrl.toString();

            if (!m_intLinks ){
                qDebug()<< "   wc_parser::parse(): m_IntLinks = FALSE"
                        << " SKIPPING node creation";
                        continue;
            }

            if (requestUrl.path() == newUrl.path()) {
                qDebug()<< "   wc_parser::parse(): m_IntLinks = TRUE"
                        << " requestUrl.path() = newUrl.path()"
                        <<  " Creating self link only";
                this->newLink(sourceNode, newUrl, false);

            }
            else {
                qDebug()<< "   wc_parser::parse(): m_IntLinks = TRUE"
                        <<  " Creating new node and ADDING it to frontier...";
                this->newLink(sourceNode, newUrl, true);

            }
        }
        else {
            qDebug() << "   wc_parser::parse(): isRelative FALSE";

            if ( newUrl.scheme() != "http"  && newUrl.scheme() != "https"  &&
                      newUrl.scheme() != "ftp" && newUrl.scheme() != "ftps") {
                qDebug() << "   wc_parser::parse(): found INVALID newUrl SCHEME"
                            << newUrl.toString();
                continue;
            }

            if (  newUrl.host() != host  ) {
                qDebug()<< "   wc_parser::parse(): absolute newUrl "
                         <<  " is EXTERNAL ";
                if ( !m_extLinks ) {
                    qDebug()<< "   wc_parser::parse(): m_extLinks = false . "
                            <<" Creating new node but NOT ADDING it to frontier...";
                    this->newLink(sourceNode, newUrl, false);
                }
                else {
                    qDebug()<< "   wc_parser::parse():  m_extLinks = true  "
                            <<" Creating new node and ADDING it to frontier...";
                    this->newLink(sourceNode, newUrl, true);
                }
            }
            else {
                qDebug()<< "   wc_parser::parse(): absolute newUrl"
                        << " is INTERNAL ";

                if (!m_intLinks){
                    qDebug()<< "   wc_parser::parse(): m_IntLinks = FALSE"
                              << " SKIPPING node creation";
                    continue;
                }
                qDebug()<< "   wc_parser::parse(): m_IntLinks = TRUE"
                        <<" Creating new node and ADDING it to frontier...";
                this->newLink(sourceNode, newUrl, true);
            }
        }

        validUrlsInPage ++;
        qDebug() << "   wc_parser::parse(): validUrlsInPage " << validUrlsInPage;
        //  or until we reach maxRecursion
        if ( m_maxLinksPerPage  != 0 ) {
            if ( validUrlsInPage > m_maxLinksPerPage ) {
                qDebug () <<"!!   wc_spider::parse() Reached m_maxLinksPerPage "
                         <<m_maxLinksPerPage << " - we will stop now "  ;
                break;
            }
        }
    }

}




//signals node creation  Called from wc_parser::load()

void WebCrawler_Parser::newLink(int s, QUrl target,  bool enqueue_to_frontier) {
    qDebug() << "   wc_parser::newLink() : from s " <<  s
                << " to target " << target.toString();

    if (m_maxPages>0) {
        if (m_discoveredNodes >= m_maxPages ) {
            qDebug () <<"   wc_parser::newLink(): #### Seems we have reached maxPages!"
                     << " - STOP!" ;
            emit finished("maxpages from newLink");
            return;
        }
    }

    // check if the new url has been discovered previously
    QMap<QUrl, int>::const_iterator index = knownUrls.find(target);
    if (   index!= knownUrls.end() ) {
        qDebug()<< "--   wc_parser::newLink(): target already discovered "
                << " in knownUrls. Creating edge from "
                << s << " to " << index.value();
        emit signalCreateEdge (s, index.value() );
        return;
    }

    m_discoveredNodes++;
    knownUrls[target]=m_discoveredNodes;
    emit signalCreateNode( target.toString(), m_discoveredNodes);
    qDebug()<< "**   wc_parser::newLink(): Creating node " << m_discoveredNodes
            << " url "<< target.toString();

    if (enqueue_to_frontier) {

        frontier.enqueue(target);
        qDebug()<< "**   wc_parser::newLink(): Enqueuing new node to frontier "
                   << " frontier size: "<<  frontier.size();

        emit startSpider();
    }
    else {
        qDebug()<< "##   wc_parser::newLink(): NOT enqueuing to frontier";
    }

    qDebug()<< "--   wc_parser::newLink(): Creating edge from "
            << s << " to " << m_discoveredNodes;

    emit signalCreateEdge (s, m_discoveredNodes);


}


