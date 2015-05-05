/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.6
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
#include <QVector>


QQueue<QUrl> frontier;  //our url buffer

QMap <QUrl, bool> visitedUrls;
QMap <QUrl, int> knownUrls;
QByteArray ba;

QString  seed="", domain="", seed_domain = "";

int maxPages, discoveredNodes=0, visitedNodes=0, maxRecursion=1;
bool m_extLinks, m_intLinks;

void WebCrawler::load(
        QString url,
        int maxN,
        int maxRec,
        bool extLinks,
        bool intLinks){

    seed=url;       //the initial url/domain we will crawl
    maxPages=maxN;  //maxPages we'll check
    maxRecursion = maxRec;
    m_extLinks = extLinks;
    m_intLinks = intLinks;
    //clear global variables
    frontier.clear();
    visitedUrls.clear();
    knownUrls.clear();
    domain="";
    visitedNodes = 0;
    discoveredNodes=0;

    //seed url can't have spaces nor capital letters...
    seed=seed.simplified().toLower();

    if (seed.contains(" "))	{
        seed.remove(" ");
    }

    if ( seed.startsWith("//" ) ) {
           seed.remove ("//");
    }
    else if ( seed.startsWith("/" ) ) {
           seed.replace (0,1,"");
    }

    // extract the seed domain
     seed_domain = seed;

     // add http for seed url
    if ( !seed.startsWith("http:" ) )
       seed = "http://" + seed;



   // extract plain domain name
    if ( seed_domain.startsWith( "http://", Qt::CaseInsensitive   ) )
        seed_domain.remove ("http://");
    else if ( seed_domain.startsWith( "https://",  Qt::CaseInsensitive ) )
        seed_domain.remove ("https://");
    if ( seed_domain.startsWith  ("www.",  Qt::CaseInsensitive ) ) {
        seed_domain=seed_domain.remove ("www.");
    }
    if ( seed_domain.indexOf ("/") !=-1 )
        seed_domain = seed_domain.left(seed_domain.indexOf ("/"));

    qDebug() << "WebCrawler::WebCrawler()  seed: " << seed
             << " seed_domain: " << seed_domain
             << " Adding seed to frontier queue and knownUrls map. "
             << " Creating node " << discoveredNodes+1;

    discoveredNodes++;
    frontier.enqueue(QUrl(seed));
    knownUrls[QUrl(seed)]=discoveredNodes;
    createNode(seed, discoveredNodes);

    qDebug() << "WebCrawler::WebCrawler()create spider and parser threads";
    WebCrawler_Parser *wc_parser = new WebCrawler_Parser;
    WebCrawler_Spider *wc_spider = new WebCrawler_Spider;

    qDebug() << "WebCrawler::WebCrawler() I will move parser/spider to new QThreads!";

    wc_parser->moveToThread(&parserThread);
    wc_spider->moveToThread(&spiderThread);

    connect(&parserThread, &QThread::finished,
            wc_parser, &QObject::deleteLater);

    connect(&spiderThread, &QThread::finished,
            wc_spider, &QObject::deleteLater);

    connect(this, &WebCrawler::operateSpider,
            wc_spider, &WebCrawler_Spider::get);

    connect(wc_parser, &WebCrawler_Parser::signalCreateNode,
            this, &WebCrawler::slotCreateNode);
    connect(wc_parser, &WebCrawler_Parser::signalCreateEdge,
            this, &WebCrawler::slotCreateEdge);

    connect (wc_spider, &WebCrawler_Spider::finished,
             this, &WebCrawler::terminateThreads);

    connect (wc_parser, &WebCrawler_Parser::finished,
             this, &WebCrawler::terminateThreads);

    connect (wc_spider, &WebCrawler_Spider::parse,
                 wc_parser, &WebCrawler_Parser::parse );

    connect (wc_parser, &WebCrawler_Parser::startSpider,
             wc_spider, &WebCrawler_Spider::get );


    parserThread.start();
    spiderThread.start();

    // start the spider thread
    emit operateSpider();

}

WebCrawler::~WebCrawler() {
    qDebug() << "WebCrawler::~WebCrawler() DESTRUCTOR " ;
    terminateThreads();
    frontier.clear();
    ba.clear();
    visitedUrls.clear();
    knownUrls.clear();
    domain="";
    visitedNodes = 0;
    discoveredNodes=0;
}

//void WebCrawler::load(){
//    qDebug () << "WebCrawler::load()  start threads";
//    parserThread.start();
//    spiderThread.start();

//    qDebug () << "WebCrawler::load()  operate spider thread";
//    emit operateSpider();
//}

//called from wc_parser createNode signal
void WebCrawler::slotCreateNode(QString url, int number) {
    emit createNode(url, number);
}

//called from wc_parser createEdge signal
void WebCrawler::slotCreateEdge (int source, int target){
    emit createEdge (source, target);
}


//called from Graph, when closing network, to terminate all processes
//also called indirectly when wc_spider finishes
void WebCrawler::terminateThreads (QString reason){
    qDebug() << "WebCrawler::terminateThreads() - reason " << reason;
    //tell the parser thread that we must quit!
    if (parserThread.isRunning() )
        parserThread.quit();
    //tell the spider thread that we must quit!
    if (spiderThread.isRunning() ) {
        spiderThread.quit();
        emit signalLayoutNodeSizesByOutDegree(true);
     }

}


/*
 * Constructor connects http manager signal
 * to httpfinished() slot
 * which in turn emits http reply to WebCrawler_Parser
 */
WebCrawler_Spider::WebCrawler_Spider(){

    http = new QNetworkAccessManager(this);

    connect ( http, &QNetworkAccessManager::finished,
              this, &WebCrawler_Spider::httpFinished );

}



/*
 * WebCrawer_Spider main functionality
 * Parses urls from frontier and downloads their data.
*  When http signals finished() the response data are passed to
*  wc_parser thread to parse them
*/
void WebCrawler_Spider::get(){

    do { 	//repeat forever....

        //  or until we crawl all urls in frontier.
        if (frontier.size() ==0 ) {
            qDebug () <<"   wc_spider::get() #### Frontier is empty: "
                     <<frontier.size() << " - we will stop now "  ;
            break;
        }

        //  or until we reach maxRecursion
        if ( maxRecursion  == 0 ) {
            qDebug () <<"   wc_spider::get() #### Reached maxRecursion: "
                     <<maxRecursion << " - we will stop now "  ;
            break;
        }

        // or until we have reached maxPages
        if (maxPages>0) {
            if (visitedNodes == maxPages ) {
                qDebug () <<"   wc_spider::get(): #### Seems we have reached maxPages!"
                         << " - we will stop now" ;
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

        if ( ! visitedUrls[currentUrl] ) {

            qDebug() << "   wc_spider::get(): currentUrl not visited."
                     << " Increasing visitedNodes to: " << visitedNodes + 1
                     << " Adding it to visitedUrls map - let us visit it.";

            visitedUrls[currentUrl]=true;

            qDebug() << "   wc_spider::get(): currentUrl: "
                     <<  currentUrl
                      << " downloading html ";

            request = new QNetworkRequest;
            request->setUrl(currentUrl);
            request->setRawHeader(
                        "User-Agent",
                        "SocNetV innocent spider 1 - see http://socnetv.sf.net");

            QNetworkReply *reply =  http->get(*request) ;

            visitedNodes++;

        }
        else {
            qDebug() << "   wc_spider::get(): currentUrl "
                     << " already visited. Skipping.";
            continue;
        }

    } while ( 1 );

    qDebug() << "   wc_spider::get() Finished!";
}


void WebCrawler_Spider::httpFinished(QNetworkReply *reply){
    qDebug() << "   wc_spider::httpFinished()";
    emit parse (reply);
}




/*
 * This method is called when http has finished all pending requests.
 * First, we start by reading all from http to the QString page.
 * Then we parse the page string, searching for url substrings.
 */
void WebCrawler_Parser::parse(QNetworkReply *reply){
    // find hte node the response html belongs to
    // we get this from the reply object request method
    QUrl requestUrl = reply->request().url();
    QString requestUrlStr = requestUrl.toString();
    QString locationHeader = reply->header(QNetworkRequest::LocationHeader).toString();
    int sourceNode = knownUrls [ requestUrl ];
    QString host = requestUrl.host();
    QString path = requestUrl.path();
    qDebug() << "   wc_parser::parse() response html of url "
             << requestUrlStr << " sourceNode " << sourceNode
              << " host " << host
              << " path " << path;

    qDebug () << "decoded locationHeader" << locationHeader ;

    qDebug () << "encoded requestUrl  " << requestUrl;
    qDebug () << "decoded requestUrl " << requestUrlStr;
    qDebug () << "requestUrl host " << host;

    if ( locationHeader != "" && locationHeader != requestUrlStr ) {
        qDebug () << "   wc_parser::parse() Location response header "
                  << locationHeader
                  << " differs from requestUrl " << requestUrlStr
                  << " Creating node redirect - Creating edge - RETURN ";
        newLink( sourceNode, locationHeader , true );
        return;
    }



    QUrl newUrl;
    QString newUrlStr;
    int start=-1, end=-1, equal=-1 , invalidUrlsCount =0; // index=-1;

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

        if (maxPages>0) {
            if (discoveredNodes >= maxPages ) {
                qDebug () <<"   wc_parser::newLink(): #### Seems we have reached maxPages!"
                         << " - STOP!" ;
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
            invalidUrlsCount ++;
            qDebug() << "   wc_parser::parse(): found INVALID newUrl "
                        << newUrl.toString()
                        << " in page " << requestUrlStr
                        << " Will CONTINUE only if invalidUrlsCount < 200";
            if (invalidUrlsCount > 200) {
                qDebug() << "   wc_parser::parse(): INVALID newUrls > 200";
                emit finished("invalidUrlsCount > 200");
                return;
            }
            continue;
        }

        qDebug() << "   wc_parser::parse(): "
                 << " found VALID newUrl : "
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
                qDebug()<< "   wc_parser::parse(): # newUrl "
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
                        << " requestUrl.path() = requestUrl.path()"
                        <<  " Creating new node, NOT ADDING it to frontier...";
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

    }

}




//signals node creation  Called from wc_parser::load()

void WebCrawler_Parser::newLink(int s, QUrl target,  bool enqueue_to_frontier) {
    qDebug() << "   wc_parser::newLink() : from s " <<  s
                << " to target " << target.toString();

    if (maxPages>0) {
        if (discoveredNodes >= maxPages ) {
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

    discoveredNodes++;
    knownUrls[target]=discoveredNodes;
    emit signalCreateNode( target.toString(), discoveredNodes);
    qDebug()<< "**   wc_parser::newLink(): Creating node " << discoveredNodes
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
            << s << " to " << discoveredNodes;

    emit signalCreateEdge (s, discoveredNodes);


}


