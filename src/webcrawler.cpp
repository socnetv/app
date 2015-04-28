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
#include <QMutex>
#include <QWaitCondition>

QNetworkAccessManager *http;

QNetworkReply *reply;

const int bufferSize = 30; //size of circular buffer

QQueue<QString> frontier;  //our circular buffer

QMap <int, int> sourceMap;
QMap <QString, bool> visitedUrls;
QMap <QString, int> knownUrls;
QByteArray ba;

QWaitCondition frontierNotEmpty;
QWaitCondition frontierNotFull;
QMutex mutex;

QString currentUrl="", seed="", domain="", seed_domain = "", previous_domain="";


int maxPages, discoveredNodes=0, currentNode=0, maxRecursion;
bool goOut=false, seedStartsWithWWW=false;

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
    currentUrl="", domain="", previous_domain="";
    currentNode = 0;

    //seed url can't have spaces...
    seed=seed.simplified();
    if (seed.contains(" "))	{
        seed.remove(" ");
    }

    if ( seed.startsWith("//" ) )
        seed.remove ("//");
    else if ( seed.startsWith("/" ) )
        seed.replace (0,1,"");

    // extract the seed domain
     seed_domain = seed;

    // add http for seed url
   if ( !seed.startsWith("http://" ) || !seed.startsWith("https://" ) )
       seed = "http://" + seed;

   // extract plain domain name
    if ( seed_domain.startsWith("//" ) )
        seed_domain.remove ("//");
    else if ( seed_domain.startsWith("/" ) )
        seed_domain.replace (0,1,"");
    else if ( seed_domain.startsWith( "http://", Qt::CaseInsensitive   ) )
        seed_domain.remove ("http://");
    else if ( seed_domain.startsWith( "https://",  Qt::CaseInsensitive ) )
        seed_domain.remove ("https://");
    if ( seed_domain.startsWith  ("www.",  Qt::CaseInsensitive ) ) {
        seedStartsWithWWW = true;
        seed_domain=seed_domain.remove ("www.");
    }
    if ( seed_domain.indexOf ("/") !=-1 )
        seed_domain = seed_domain.left(seed_domain.indexOf ("/"));

    qDebug() << "WebCrawler::load():  seed: " << seed
             << " seed_domain: " << seed_domain
             << " Adding seed to frontier queue and knownUrls map. "
             << " Creating node 1.";

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

//    connect (http, &QNetworkAccessManager::finished,
//             wc_spider, &WebCrawler_Spider::httpFinished);

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
void WebCrawler::terminateReaderQuit (){
    //tell the parser thread that we must quit!
    if (parserThread.isRunning() )
        parserThread.quit();
    //tell the spider thread that we must quit!
    if (spiderThread.isRunning() )
        spiderThread.quit();
}



/*
 * WebCrawer_Spider main functionality
 * Parses urls from frontier and downloads their data.
*  When http signals finished() the response data are passed to
*  wc_parser thread to parse them
*/
void WebCrawler_Spider::get(){
    int pos;
    do { 	//repeat forever....

        if (frontier.size() ==0 ) {     //  until we crawl all urls in frontier.
            qDebug () <<"   wc_spider::get() #### Frontier is empty: "
                     <<frontier.size() << " - we will stop now "  ;
            break;
        }

        if ( maxRecursion  == 0 ) {     //  or until we reach maxRecursion
            qDebug () <<"   wc_spider::get() #### Reached maxRecursion: "
                     <<maxRecursion << " - we will stop now "  ;
            break;
        }

        if (maxPages>0) {               // or until we have reached maxPages
            if (currentNode == maxPages ) {
                qDebug () <<"   wc_spider::get(): #### Seems we have reached maxPages!"
                         << " - we will stop now" ;
                break;
            }
        }

        currentNode++;

        qDebug() << "   wc_spider::get():"
                 <<" frontier size " << frontier.size()
                 << " Increasing currentNode to: " << currentNode
                 << " - Take the first url from frontier  ";

        currentUrl = frontier.dequeue();

        qDebug() << "   wc_spider::get():  currentUrl: "
                 <<  currentUrl.toLatin1();

        if ( ! visitedUrls[currentUrl] ) {

            qDebug() << "   wc_spider::get(): currentUrl not visited."
                        << " Adding it to visitedUrls - Checking it.";

            visitedUrls[currentUrl]=true;

            if ( currentUrl.contains ("//" ) ) {
                qDebug() << "   wc_spider::get(): currentUrl absolute, "
                         << "probably external. Needs check";

                if ( !currentUrl.contains( seed_domain, Qt::CaseInsensitive) ) {

                    domain = currentUrl;
                    qDebug() << "   wc_spider::get(): currentUrl is EXTERNAL. ";

                    if  ( !goOut  ) {
                         qDebug() << "   wc_spider::get(): goOut is false. "
                                  << " CONTINUE...";
                         continue;
                    }
                    else {
                        //find new domain
                        if ( domain.contains ("http://" ) )
                            domain.remove ("http://");
                        if ( domain.contains ("https://" ) )
                            domain.remove ("https://");
                        if ( domain.contains ("//" ) )
                            domain.remove ("//");
                        if ( (pos=domain.indexOf ("/")) !=-1 ) {
                            domain = domain.left(pos);
                        }
                        qDebug() << "   wc_spider::get(): new domain is: "
                                 << domain.toLatin1();
                    }
                }
                else {
                    if ( currentNode == 1 ){
                        qDebug() << "   wc_spider::get(): "
                                 << "this is the seed url - creating node";
                    }
                    domain = seed_domain;
                }
            }
            else {
                qDebug() << "   wc_spider::get(): relative currentUrl INTERNAL."
                         << " Using current domain: "
                         << domain.toLatin1();

                if (currentUrl.startsWith('.', Qt::CaseInsensitive) ) {
                    qDebug () << "   wc_spider::get(): "
                              << " currentUrl startsWith a dot."
                              << " Removing dot from url and adding /";
                    currentUrl=currentUrl.remove(0, 1);
                    currentUrl = "/"+currentUrl;
                }
                else if (!currentUrl.startsWith('/',  Qt::CaseInsensitive) ) {
                    qDebug () << "   wc_spider::get(): "
                              << " currentUrl is relative to some other page"
                              << " adding / to url";
                    currentUrl = "/"+currentUrl;
                }
                if (seedStartsWithWWW)
                    currentUrl = "http://www." + domain + currentUrl;
                else
                    currentUrl = "http://" + domain + currentUrl;

            }


           qDebug() << "   wc_spider::get(): currentUrl: "
                      <<  currentUrl.toLatin1()
                       << " downloading html ";

           http = new QNetworkAccessManager(this);

           connect ( http, &QNetworkAccessManager::finished,
                         this, &WebCrawler_Spider::httpFinished );

            QNetworkRequest *request = new QNetworkRequest;
            request->setUrl(QUrl(currentUrl));
            request->setRawHeader(
                        "User-Agent",
                        "SocNetV innocent spider 1 - see http://socnetv.sf.net");

            QNetworkReply *reply =  http->get(*request) ;

        }
        else {
            qDebug() << "   wc_spider::get(): currentUrl "
                     << " already visited. Skipping.";
            continue;
        }

        if ( domain != previous_domain && (currentNode!=1) ) {
            qDebug () << "   wc_spider::get(): **** NEW DOMAIN " ;
        }
        else {
            qDebug () << "   wc_spider::get(): **** SAME DOMAIN ";
        }

        qDebug () <<"   wc_spider::get(): setting previous domain to domain";
        previous_domain = domain;		//set previous domain

    } while ( 1 );

    qDebug() << "   wc_spider::get() Finished!";
}


void WebCrawler_Spider::httpFinished(QNetworkReply *reply){
    qDebug() << "   wc_spider::httpFinished()";
//    mutex.lock();
//    frontierNotEmpty.wakeAll();
//    mutex.unlock();
//    qDebug() << "   wc_spider::httpFinished(): MUTEX UNLOCKED for frontierNotEmpty";
    emit parse (reply);

//    if (currentNode >= 1 && frontier.size() <= bufferSize  ) {
//        qDebug() << "   wc_spider::get(): MUTEX LOCKED - waiting frontierNotFull...";
//        mutex.lock();
//        frontierNotFull.wait(&mutex);
//        mutex.unlock();
//        qDebug () <<"   wc_spider::get(): Waking up: frontier size = "
//                 << frontier.size();
//    }


}


/*
 * This method is called when http has finished all pending requests.
 * First, we start by reading all from http to the QString page.
 * Then we parse the page string, searching for url substrings.
 */
void WebCrawler_Parser::parse(QNetworkReply *reply){
    qDebug() << "   wc_parser::parse()";


    qDebug()  << "   wc_parser::parse(): read something!";
    QString newUrl;

    int start=-1, end=-1, equal=-1 ;// index=-1;

    ba=reply->readAll();
    QString page(ba);


    if (!page.contains ("a href"))  { //if a href doesnt exist, return
        //FIXME: Frameset pages are not parsed! See docs/manual.html for example.
        qDebug() << "   wc_parser::parse(): ### Empty or not useful data from "
                 << currentUrl.toLatin1() << " RETURN";
//        mutex.lock();
//        frontierNotFull.wakeAll();
//        mutex.unlock();
        return;
    }

    qDebug() << "   wc_parser::parse(): erasing <head></head>";
    start=page.indexOf ("<head>");		//Find head pos
    end=page.indexOf ("</head>");		//Find head pos
    page = page.remove(start, end);		//erase head

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
                 << " detected newUrl: "
                 << newUrl.toLatin1();

        // check if the new url is known
        QMap<QString, int>::const_iterator index = knownUrls.find(newUrl);
        if (   index!= knownUrls.end() ) {
            qDebug()<< "   wc_parser::parse(): newUrl already checked "
                    << " in knownUrls - Just creating an edge from "
                    << currentNode << " to " << index.value();
            emit signalCreateEdge (currentNode, index.value() );
            continue;
        }

        // if the new url is unknown
        // check if it is absolute or relative url

        if ( newUrl.contains("http://", Qt::CaseInsensitive) ||
             newUrl.contains("https://", Qt::CaseInsensitive) ) {

              qDebug()<< "   wc_parser::parse(): absolute newUrl ";

              //	...skip css, favicon, rss, ping, etc
              if ( newUrl.endsWith(".css", Qt::CaseInsensitive) ||
                   newUrl.endsWith("feed/", Qt::CaseInsensitive) ||
                   newUrl.endsWith("rss/", Qt::CaseInsensitive) ||
                   newUrl.endsWith("atom/", Qt::CaseInsensitive) ||
                   newUrl.endsWith("xmlrpc.php", Qt::CaseInsensitive) ||
                   newUrl.endsWith("?rsd", Qt::CaseInsensitive) ||
                   newUrl.endsWith(".xml", Qt::CaseInsensitive) ||
                   newUrl.endsWith("favicon.ico", Qt::CaseInsensitive) ||
                   newUrl.endsWith("favicon.gif", Qt::CaseInsensitive) ||
                   newUrl.endsWith("favicon.jpg", Qt::CaseInsensitive) ||
                   newUrl.endsWith("css?H", Qt::CaseInsensitive)  ) {
                    qDebug()<< "   wc_parser::parse(): absolute newUrl "
                            << " seems a non page element (rss, favicon, etc) "
                            << "Skipping...";
                    //	emit createNode(currentUrl, currentNode);
                    //  perhaps we paint that node with a different colour
                    // or check a variable?
                    continue;
               }

               //     Check if the absolute newUrl is EXTERNAL
                if (  !newUrl.contains(seed_domain, Qt::CaseInsensitive ) ||
                      !newUrl.startsWith( "http://"+seed_domain, Qt::CaseInsensitive)  ) {
                    qDebug()<< "   wc_parser::parse(): absolute newUrl "
                             <<  " is unknown and EXTERNAL ";
                    if ( !goOut ) {
                        qDebug()<< "   wc_parser::parse(): goOut = false . "
                                <<" Creating new node but NOT ADDING it to frontier...";
                        this->createNode(newUrl, false);
                        emit signalCreateEdge (currentNode, discoveredNodes);
                    }
                    else {
                        qDebug()<< "   wc_parser::parse():  goOut = true  "
                                <<" Creating new node and ADDING it to frontier...";
                        this->createNode(newUrl, true);
                        emit signalCreateEdge (currentNode, discoveredNodes);
                    }
                }
                else {
                    qDebug()<< "   wc_parser::parse(): absolute newUrl"
                            << " is unknown and INTERNAL "
                            <<" Creating new node and ADDING it to frontier...";
                    this->createNode(newUrl, true);
                    emit signalCreateEdge (currentNode, discoveredNodes);
                }

        }
        else {   //	if this is relative url ....

            //	...skip css, favicon, rss, ping, etc
             if ( newUrl.startsWith("#", Qt::CaseInsensitive) ||
                     newUrl.endsWith(".css", Qt::CaseInsensitive) ||
                  newUrl.endsWith("feed/", Qt::CaseInsensitive) ||
                  newUrl.endsWith("rss/", Qt::CaseInsensitive) ||
                  newUrl.endsWith("atom/", Qt::CaseInsensitive) ||
                  newUrl.endsWith("xmlrpc.php", Qt::CaseInsensitive) ||
                  newUrl.endsWith("?rsd", Qt::CaseInsensitive) ||
                  newUrl.endsWith(".xml", Qt::CaseInsensitive) ||
                  newUrl.endsWith("favicon.ico", Qt::CaseInsensitive) ||
                  newUrl.endsWith("favicon.gif", Qt::CaseInsensitive) ||
                  newUrl.endsWith("favicon.jpg", Qt::CaseInsensitive) ||
                  newUrl.endsWith("css?H", Qt::CaseInsensitive)   )    {
                    qDebug()<< "   wc_parser::parse(): # relative newUrl "
                            << " seems a page resource or anchor (rss, favicon, etc) "
                            << "Skipping...";
                    //	emit createNode(currentUrl, currentNode);
                    //  perhaps we paint that node with a different colour or
                    // check a variable?
                    continue;
                }

            qDebug()<< "   wc_parser::parse(): relative newUrl "
                    <<  " is unknown and INTERNAL "
                    <<  " Creating new node and ADDING it to frontier...";
            this->createNode(newUrl, true);
            emit signalCreateEdge (currentNode, discoveredNodes);

        }
    }

}




//signals node creation  Called from wc_parser::load()

void WebCrawler_Parser::createNode(QString newUrl, bool enqueue_to_frontier) {
    qDebug() << "   wc_parser::createNode() ";
    discoveredNodes++;
    sourceMap[ discoveredNodes ] = currentNode;
    knownUrls[newUrl]=discoveredNodes;
    emit signalCreateNode(newUrl, discoveredNodes);

    if (enqueue_to_frontier) {
        frontier.enqueue(newUrl);
        qDebug()<< "**   wc_parser::createNode(): Creating node " << discoveredNodes
                << " newUrl "<< newUrl << " Frontier size: "<<  frontier.size()
                << " = discoveredNodes: " << discoveredNodes
                <<  " - source: " <<  sourceMap[ discoveredNodes ];
        emit startSpider();
    }
    else {
        qDebug()<< "##   wc_parser::createNode(): Creating node " << discoveredNodes
                << " newUrl "<< newUrl << " NOT enqueuing to frontier"
                << "  discoveredNodes: " <<discoveredNodes
                <<  " - source: " <<  sourceMap[ discoveredNodes ];
    }

}


