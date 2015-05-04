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


QQueue<QString> frontier;  //our circular buffer

QMap <int, int> sourceMap;
QMap <QString, bool> visitedUrls;
QMap <QString, int> knownUrls;
QByteArray ba;

QString currentUrl="", seed="", domain="", seed_domain = "";

int maxPages, discoveredNodes=0, visitedNodes=0, maxRecursion=1;

bool goOut=false, onlyExternalUrls=true;

void WebCrawler::load (QString url, int maxN, int maxRec, bool gOut){ 

    seed=url;       //the initial url/domain we will crawl
    maxPages=maxN;  //maxPages we'll check
    maxRecursion = maxRec;
    goOut = gOut;

    //clear global variables
    frontier.clear();
    sourceMap.clear();
    visitedUrls.clear();
    knownUrls.clear();
    currentUrl="", domain="";
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

    qDebug() << "WebCrawler::load():  seed: " << seed
             << " seed_domain: " << seed_domain
             << " Adding seed to frontier queue and knownUrls map. "
             << " Creating node " << discoveredNodes+1;

    discoveredNodes++;
    frontier.enqueue(seed);
    knownUrls[seed]=discoveredNodes;
    createNode(seed, discoveredNodes);

    // create spider and parser threads
    WebCrawler_Parser *wc_parser = new WebCrawler_Parser;
    WebCrawler_Spider *wc_spider = new WebCrawler_Spider;

    qDebug() << "WebCrawler::load(): I will start new QThreads!";

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
                emit finished("maxpages");
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
                     <<  currentUrl.toLatin1()
                      << " downloading html ";

            request = new QNetworkRequest;
            request->setUrl(QUrl(currentUrl));
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



QString WebCrawler_Parser::urlDomain(QString url) {
    qDebug() << "   wc_parser::urlDomain() find which domain from " << url;
    //find new domain
    int pos;
    if ( url.contains ("http://" ) )
        url.remove ("http://");
    if ( url.contains ("https://" ) )
        url.remove ("https://");
    if ( url.contains ("//" ) )
        url.remove ("//");
    if ( (pos=url.indexOf ("/")) !=-1 ) {
        url = url.left(pos);
    }
    qDebug() << "   wc_parser::domain(): new domain is: "
             << url.toLatin1();
    return url;
}


/*
 * This method is called when http has finished all pending requests.
 * First, we start by reading all from http to the QString page.
 * Then we parse the page string, searching for url substrings.
 */
void WebCrawler_Parser::parse(QNetworkReply *reply){
    // find hte node the response html belongs to
    // we get this from the reply object request method
    QString requestUrl = reply->request().url().toString();
    QString locationHeader = reply->header(QNetworkRequest::LocationHeader).toString();
    int sourceNode = knownUrls [ requestUrl ];
    qDebug () << "locationHeader" << reply->header(QNetworkRequest::LocationHeader) ;
    qDebug () << "requestUrl " << requestUrl ;
    if ( locationHeader != "" && locationHeader != requestUrl ) {
        qDebug () << "   wc_parser::parse() Location response header "
                  << locationHeader
                  << " differs from requestUrl " << requestUrl;
        discoveredNodes++;
        createNode( locationHeader , true );
        emit signalCreateEdge(sourceNode , discoveredNodes);
        qDebug () << "   wc_parser::parse() increasing discoveredNodes, "
                     << " creating redirect node and link to it. RETURN";
        return;
    }


    QString sourceBaseUrl = urlDomain(requestUrl);
    qDebug() << "   wc_parser::parse() response html of url "
             << requestUrl << " which is source node " << sourceNode
              << " sourceBaseUrl " << sourceBaseUrl;

    QString newUrl;

    int start=-1, end=-1, equal=-1 ;// index=-1;

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



    while (page.contains("href")) {	//as long there is a href in the page...

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

        newUrl=page.left(end);			//Save new url to newUrl :)

        newUrl=newUrl.simplified();

        qDebug() << "   wc_parser::parse(): "
                 << " found newUrl: "
                 << newUrl;

         qDebug() << "utf8 " << newUrl.toUtf8();

        if ( newUrl == "/") {
            newUrl = "http://" + sourceBaseUrl;
            qDebug() << "   wc_parser::parse(): newUrl is /"
                        << " same as source: " << newUrl;
        }
        else if ( newUrl == (requestUrl + "/") ) {
            newUrl = "http://" + sourceBaseUrl;
            qDebug() << "   wc_parser::parse(): newUrl is "
                        << " same as source: " << newUrl;
        }

        if (maxPages>0) {
            if (discoveredNodes >= maxPages ) {
                qDebug () <<"   wc_parser::parse(): #### Seems we have reached maxPages!"
                         << " - we will stop now" ;
                emit finished("maxpages");
                break;
            }
        }

        // check if the new url has been discovered previously
        QMap<QString, int>::const_iterator index = knownUrls.find(newUrl);
        if (   index!= knownUrls.end() ) {
            qDebug()<< "   wc_parser::parse(): newUrl already discovered "
                    << " in knownUrls - Just creating an edge from "
                    << sourceNode << " to " << index.value();
            emit signalCreateEdge (sourceNode, index.value() );
            continue;
        }

        //	...skip css, favicon, rss, ping, etc
         if ( newUrl.startsWith("#", Qt::CaseInsensitive) ||
              newUrl.endsWith(".css", Qt::CaseInsensitive) ||
              newUrl.endsWith("feed/", Qt::CaseInsensitive) ||
              newUrl.endsWith("rss/", Qt::CaseInsensitive) ||
              newUrl.endsWith("atom/", Qt::CaseInsensitive) ||
              newUrl.endsWith("xmlrpc.php", Qt::CaseInsensitive) ||
              newUrl.endsWith("?rsd", Qt::CaseInsensitive) ||
              newUrl.endsWith(".xml", Qt::CaseInsensitive) ||
              newUrl.endsWith(".ico", Qt::CaseInsensitive) ||
              newUrl.endsWith(".gif", Qt::CaseInsensitive) ||
              newUrl.endsWith(".jpg", Qt::CaseInsensitive) ||
              newUrl.endsWith(".png", Qt::CaseInsensitive) ||
              newUrl.endsWith(".js", Qt::CaseInsensitive) ||
              newUrl.endsWith("css?H", Qt::CaseInsensitive)   )    {
                qDebug()<< "   wc_parser::parse(): # newUrl "
                        << " seems a page resource or anchor (rss, favicon, etc) "
                        << "Skipping...";
                continue;
         }

        discoveredNodes++;
        qDebug() << "   wc_parser::parse(): newUrl is unknown. "
                 << " discoveredNodes: " << discoveredNodes
                 << "  Checking if it is absolute or relative url";

        if ( newUrl.contains("http://", Qt::CaseInsensitive) ||
             newUrl.contains("ftp://", Qt::CaseInsensitive)  ||
             newUrl.contains("https://", Qt::CaseInsensitive) ||
             newUrl.contains("//", Qt::CaseInsensitive)  ) {

              qDebug()<< "   wc_parser::parse(): absolute newUrl ";

               //     Check if the absolute newUrl is EXTERNAL
                if (  !newUrl.contains(sourceBaseUrl, Qt::CaseInsensitive ) ||
                      !newUrl.startsWith( "http://"+sourceBaseUrl, Qt::CaseInsensitive) )   //fixme should check for source domain
                {
                    qDebug()<< "   wc_parser::parse(): absolute newUrl "
                             <<  " is unknown and EXTERNAL ";
                    if ( !goOut ) {
                        qDebug()<< "   wc_parser::parse(): goOut = false . "
                                <<" Creating new node but NOT ADDING it to frontier...";
                        this->createNode(newUrl, false);
                        qDebug()<< "--   wc_parser::parse(): Creating link from "
                                << sourceNode << " to " << discoveredNodes;
                        emit signalCreateEdge (sourceNode, discoveredNodes);
                    }
                    else {
                        qDebug()<< "   wc_parser::parse():  goOut = true  "
                                <<" Creating new node and ADDING it to frontier...";
                        this->createNode(newUrl, true);
                        qDebug()<< "--   wc_parser::parse(): Creating link from "
                                << sourceNode << " to " << discoveredNodes;
                        emit signalCreateEdge (sourceNode, discoveredNodes);
                    }
                }
                else {
                    qDebug()<< "   wc_parser::parse(): absolute newUrl"
                            << " is unknown and INTERNAL ";

                    if (onlyExternalUrls){
                        qDebug()<< "   wc_parser::parse(): onlyExternalUrls = TRUE"
                                  << " SKIPPING node creation";
                        continue;
                    }
                    qDebug()<< "   wc_parser::parse(): onlyExternalUrls = FALSE"
                            <<" Creating new node and ADDING it to frontier...";
                    this->createNode(newUrl, true);
                    qDebug()<< "--   wc_parser::parse(): Creating link from "
                            << sourceNode << " to " << discoveredNodes;
                    emit signalCreateEdge (sourceNode, discoveredNodes);
                }

        }
        else {   //	if this is relative url ....

            qDebug()<< "   wc_parser::parse(): relative newUrl "
                    <<  " is unknown and INTERNAL ";
            if (onlyExternalUrls ){
                qDebug()<< "   wc_parser::parse(): onlyExternalUrls = TRUE"
                        << " SKIPPING node creation";
                        continue;
            }
            qDebug()<< "   wc_parser::parse(): onlyExternalUrls = FALSE"
                    <<  " Creating new node and ADDING it to frontier...";
            if (newUrl.startsWith('/', Qt::CaseInsensitive ) ){
                qDebug () << "   wc_parser::parse(): relative newUrl starts with /."
                          << " Creating node with sourceBaseUrl";
                newUrl= "http://" + sourceBaseUrl + newUrl;
            }
            else if (newUrl.startsWith('.', Qt::CaseInsensitive) ) {
                qDebug () << "   wc_parser::parse(): relative newUrl starts with dot."
                          << " Removing dot from url and adding /"
                          << " Creating node with sourceBaseUrl";
                newUrl=newUrl.remove(0, 1);
                newUrl= "http://" + sourceBaseUrl + "/" + newUrl;
            }
            else {
                qDebug () << "   wc_parser::parse(): relative newUrl not starting "
                          << " either with dot or /. Something WRONG HERE???"
                           << " Creating node with sourceBaseUrl";
                newUrl="http://" + sourceBaseUrl + "/" + newUrl;
            }
            this->createNode(newUrl, true);

            qDebug()<< "--   wc_parser::parse(): Creating link from "
                    << sourceNode << " to " << discoveredNodes;

            emit signalCreateEdge (sourceNode, discoveredNodes);

        }
    }

}




//signals node creation  Called from wc_parser::load()

void WebCrawler_Parser::createNode(QString newUrl,  bool enqueue_to_frontier) {
    qDebug() << "   wc_parser::createNode() : newUrl " <<  newUrl;

    knownUrls[newUrl]=discoveredNodes;
    emit signalCreateNode(newUrl, discoveredNodes);

    if (enqueue_to_frontier) {
        frontier.enqueue(newUrl);
        qDebug()<< "**   wc_parser::createNode(): Creating node " << discoveredNodes
                << " url "<< newUrl << " Frontier size: "<<  frontier.size()
                << " = discoveredNodes: " << discoveredNodes;
        emit startSpider();
    }
    else {
        qDebug()<< "##   wc_parser::createNode(): Creating node " << discoveredNodes
                << " url "<< newUrl << " NOT enqueuing to frontier"
                << "  discoveredNodes: " <<discoveredNodes;
    }

}


