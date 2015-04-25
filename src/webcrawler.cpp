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
QNetworkRequest *request;
QNetworkReply *reply;

const int bufferSize = 200; //size of circular buffer

QQueue<QString> frontier;  //our circular buffer

QMap <int, int> sourceMap;
QMap <QString, bool> visitedUrls;
QMap <QString, int> knownUrls;
QByteArray ba;

QWaitCondition frontierNotEmpty;
QWaitCondition frontierNotFull;
QMutex mutex;

QString currentUrl="", seed="", domain="", seed_domain = "", previous_domain="";
QString path="", urlPrefix="";

int maxPages, discoveredNodes=0, currentNode, maxRecursion;
bool goOut=false, hasUrlPrefix=false;

void WebCrawler::load (QString url, int maxN, int maxRec, bool gOut){ 

    seed=url;       //the initial url/domain we will crawl
    maxPages=maxN;		//maxPages we'll check
    maxRecursion = maxRec;
    goOut = gOut;

    //clear global variables
    frontier.clear();
    sourceMap.clear();
    visitedUrls.clear();
    knownUrls.clear();
    currentUrl="", domain="", previous_domain="", path="", urlPrefix="";


    //seed url can't have spaces...
    seed=seed.simplified();
    if (seed.contains(" "))	{
        seed.remove(" ");
    }

    if ( seed.startsWith ("http://", Qt::CaseInsensitive ) )
        seed.remove ("http://");
    else if ( seed.startsWith  ("https://",  Qt::CaseInsensitive ) )
        seed.remove ("https://");

    if ( seed.startsWith("//" ) )
        seed.remove ("//");
    else if ( seed.startsWith("/" ) )
        seed.replace (0,1,"");

    // extract the seed domain
     seed_domain = seed;

    if ( seed_domain.startsWith  ("www.",  Qt::CaseInsensitive ) )
        seed_domain=seed_domain.remove ("www.");

    if ( seed_domain.indexOf ("/") !=-1 )
        seed_domain = seed_domain.left(seed_domain.indexOf ("/"));

    qDebug() << "WebCrawler:: seed: " << seed
             << " seed_domain: " << seed_domain ;

    frontier.enqueue(seed); // queue the seed url to frontier
    currentNode=1;          // current node counter
    discoveredNodes = 1;    // counts how many nodes have been discovered

    qDebug() << "	WebCrawler: creating node 1 with label "
             << currentUrl.toLatin1();

    knownUrls[seed]=currentNode;

    emit createNode(seed, 1);

    WebCrawler_Parser *wc_parser = new WebCrawler_Parser;
    WebCrawler_Spider *wc_spider = new WebCrawler_Spider;

    qDebug() << "WebCrawler:: I will start new QThreads!";

    wc_parser->moveToThread(&parserThread);
    wc_spider->moveToThread(&spiderThread);

    connect(&parserThread, &QThread::finished, wc_parser, &QObject::deleteLater);
    connect(&spiderThread, &QThread::finished, wc_spider, &QObject::deleteLater);

  //  connect(this, &WebCrawler::operateParser, wc_parser, &WebCrawler_Parser::parse);
    connect(this, &WebCrawler::operateSpider, wc_spider, &WebCrawler_Spider::get);

    connect(wc_parser, &WebCrawler_Parser::signalCreateNode, this, &WebCrawler::slotCreateNode);
    connect(wc_parser, &WebCrawler_Parser::signalCreateEdge, this, &WebCrawler::slotCreateEdge);

    connect(wc_spider, &WebCrawler_Spider::resultReady, wc_parser, &WebCrawler_Parser::parse);


    parserThread.start();
    spiderThread.start();

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
    if (parserThread.isRunning() )		//tell the parser thread that we must quit!
        parserThread.quit();
    if (spiderThread.isRunning() )		//tell the spider thread that we must quit!
        spiderThread.quit();

}



/*
 * Parses urls from frontier and downloads their data.
*  When http signals finished() then wc_parser thread is loaded8
*  to parse the data
*/
void WebCrawler_Spider::get(){
    int pos;

    do { 	//repeat forever....


        qDebug() << "		WebCrawler_Spider: "
                 <<" frontier size " << frontier.size()
                 << " currentNode " << currentNode;

        if (currentNode != 1) {
//            qDebug() << " MUTEX LOCK - Waiting for frontierNotFull...";
//            mutex.lock();
//            frontierNotFull.wait(&mutex);
//            mutex.unlock();
//            qDebug () <<"		WebCrawler_Spider: Waking up to continue: frontier size = "
//                     << frontier.size();
        }


        if (frontier.size() ==0 ) {     //  until we crawl all urls in frontier.
            qDebug () <<"	WebCrawler #### Frontier is empty: "
                     <<frontier.size() << " - we will stop now "  ;
            break;
        }

        if ( maxRecursion  == 0 ) {     //  or until we reach maxRecursion
            qDebug () <<"	WebCrawler #### Reached maxRecursion: "
                     <<maxRecursion << " - we will stop now "  ;
            break;
        }

        if (maxPages>0) {               // or until we have reached maxPages
            if (currentNode == maxPages ) {
                qDebug () <<"	WebCrawler: #### Seems we have reached maxPages!"
                         << " - we will stop now" ;
                break;
            }
        }

        // take the first url from frontier - name it currentUrl
        // initially this is the seed url entered by the user
        currentUrl = frontier.head();

        if ( ! visitedUrls[currentUrl] ) {

            qDebug() << "	WebCrawler: currentUrl "  <<  currentUrl.toLatin1()
                     << " not visited. Adding it to visitedUrls - Checking it.";

            visitedUrls[currentUrl]=true;

            if ( currentUrl.contains ("//" ) ) {
                qDebug() << "	WebCrawler: currentUrl probably external: "
                         << currentUrl;

                if ( !currentUrl.contains( seed_domain, Qt::CaseInsensitive) ) {

                    qDebug() << "	WebCrawler: yes, external currentUrl it is: "
                             <<currentUrl;

                    if  ( currentNode != 1 && !goOut  ) {
                        // if the user don't want to crawl external links...
                         qDebug() << " We will not crawl it. Continuing...";
                         continue;

                    }
                    else if ( currentNode == 1 ){

                    }
                    if ( currentUrl.contains ("http://" ) )
                        domain=currentUrl.remove ("http://");

                    if ( currentUrl.contains ("https://" ) )
                        domain=currentUrl.remove ("https://");

                    if ( currentUrl.contains ("//" ) )
                        domain=currentUrl.remove ("//");

                    if ( (pos=domain.indexOf ("/")) !=-1 ) { //find new domain
                        domain = domain.left(pos);
                        qDebug() << "		WebCrawler: Host domain is the url: "
                                 << domain.toLatin1();
                    }
                    else {
                        qDebug() << "		WebCrawler: Host domain is the url: "
                                 << domain.toLatin1() ;
                        seed_domain = domain;
                    }


                }
            }
            else {
                qDebug() << "	no // in currentUrl - internal or seed url";
                if (currentNode==1) {
//                    qDebug() << "	currentNode " <<  currentNode
//                                << " - this is the seed url - creating node";
//                    domain = seed_domain;
                    //emit createNode(currentUrl, 1);
                }
                else {
                    qDebug() << "	currentNode " <<  currentNode
                                << " - this is internal url "
                                   << " using current domain"
                                   << domain.toLatin1();

                    if (currentUrl.startsWith('.', Qt::CaseInsensitive) )
                        currentUrl=currentUrl.remove(0, 1);
                    else if (!currentUrl.startsWith('/',  Qt::CaseInsensitive) ) {
                        currentUrl = "/"+currentUrl;
                        qDebug() << "			adding / to currentUrl " << currentUrl;
                    }
                }
            }


            // download currentUrl
            http = new QNetworkAccessManager(this);
            request = new QNetworkRequest;
            request->setUrl(QUrl(currentUrl));


            //TODO connect finished() signal to load() of 2ond wc_parser class
            connect (http,SIGNAL( finished(QNetworkReply*) ),
                     this, SLOT( result(QNetworkReply*) ) );

            //http->get(*request) ;
            http->get(QNetworkRequest(QUrl("http://qt-project.org")));

        }
        else {
            // currentUrl has been crawled already - don't do nothing!
            qDebug() << "	WebCrawler: currentUrl "  <<  currentUrl.toLatin1()
                     << " already visited. Skipping.";
            frontier.dequeue();			//Dequeue head
            continue;
        }

        if (	domain != previous_domain && (currentNode!=1) ) {
            qDebug () << "		WebCrawler: **** NEW DOMAIN " ;
        }
        else {
            qDebug () << "		WebCrawler: **** SAME DOMAIN ";
        }




        qDebug () <<"		WebCrawler: Increasing currentNode, dequeuing frontier and setting previous domain to domain";
        currentNode++;
        frontier.dequeue();			//Dequeue head
        previous_domain = domain;		//set previous domain


    } while ( 1 );


    qDebug() << "		Spider Finished!";
}


void WebCrawler_Spider::result(QNetworkReply *reply){
    qDebug() << "WebCrawler_Spider - emitting result";
    emit resultReady(reply);
}



/*
 * This method is called when http has finished all pending requests.
 * First, we start by reading all from http to the QString page.
 * Then we parse the page string, searching for url substrings.
 */
void WebCrawler_Parser::parse(QNetworkReply *reply){
    qDebug()  << "			wc_parser: read something!";
    QString newUrl;
     ba=reply->readAll();
    bool createNodeFlag = false, createEdgeFlag=false ;
//    Q_UNUSED(createNodeFlag);
//    Q_UNUSED(createEdgeFlag);
    int start=-1, end=-1, equal=-1 ;// index=-1;
    QString page(ba);


    if (!page.contains ("a href"))  { //if a href doesnt exist, return
        //FIXME: Frameset pages are not parsed! See docs/manual.html for example.
        qDebug() << "			wc_parser: ### Empty or not useful data from " << currentUrl.toLatin1() << " RETURN";
        frontierNotEmpty.wakeAll();
        return;
    }
    mutex.lock();

    while (page.contains("href")) {	//as long there is a href in the page...
        createNodeFlag = false;
        createEdgeFlag = false;

        page=page.simplified();		// remove whitespace from the start and the end - all whitespace sequence becomes single space

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

        qDebug()<< "			wc_parser: page still contains links - Parsing " << newUrl.toLatin1();
        // if this is not the 1st node, and it has been already checked ...
        QMap<QString, int>::const_iterator index = knownUrls.find(newUrl);
        if (   index!= knownUrls.end() ) {
            qDebug()<< "			wc_parser: #---> newUrl "  <<  newUrl.toLatin1()
                    << " already CHECKED - Just creating an edge from " << currentNode << " to " << index.value();

            emit signalCreateEdge (currentNode, index.value() );
            continue;											// .... and continue skipping it!
        }

        // if this is the first node or it is visited for the first time ...
        if ( newUrl.contains("http://", Qt::CaseInsensitive) || newUrl.contains("https://", Qt::CaseInsensitive) ) {	//if this is an absolute url
            if (true) //flag to display css/rss icons
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
                     newUrl.endsWith("css?H", Qt::CaseInsensitive)  )

                {
                    qDebug()<< "			wc_parser: # absolute newUrl " << newUrl
                            << " must be a web 2.0 element (rss, favicon, etc) or file. Skipping...";
                    //	emit createNode(currentUrl, currentNode);
                    //  perhaps we paint that node with a different colour or check a variable?
                    //continue;
                }
            if ( !goOut ) {		// ... and we need to limit ourselves within the seed domain...
                if (  !newUrl.startsWith(seed_domain, Qt::CaseInsensitive ) ||
                      !newUrl.startsWith( "http://"+seed_domain, Qt::CaseInsensitive)  ) {	//...then check if the newUrl is out of the seed domain
                    qDebug()<< "			wc_parser: # absolute newUrl "  <<  newUrl.toLatin1()
                            << " is OUT OF the seed (original) domain. I will create a node but NOT add it to frontier...";
                    this->createNode(newUrl, false);

                    emit signalCreateEdge (currentNode, discoveredNodes);
                }
                else {
                    qDebug()<< "			wc_parser: absolute newUrl" << newUrl.toLatin1()
                            << " appears INSIDE the seed domain "
                            << seed_domain << " - I will create a node here..." ;
                    this->createNode(newUrl, true);
                    emit signalCreateEdge (currentNode, discoveredNodes);
                }


            }
            else {				// ... else if we can go out the seed domain, then just add it.
                qDebug()<< "			wc_parser: absolute newUrl" << newUrl.toLatin1()
                        << " is outside the seed domain "
                        << seed_domain << " - and we are allowed to go there, so I will create a node here..." ;
                this->createNode(newUrl, true);
                emit signalCreateEdge (currentNode, discoveredNodes);
            }

        }
        else {	//	if this is an internal or relative url ....
            //  ...and an index, then skip it.
            if (newUrl == "index.html" || newUrl == "index.htm" || newUrl == "index.php"){
                qDebug()<< "			wc_parser: # non-absolute newUrl "  <<  newUrl.toLatin1()
                        << " must be an index file. Creating edge from 1 to " << discoveredNodes;
                emit signalCreateEdge ( 1 , discoveredNodes);
                continue;
            }

            //	...different treatment for css, favicon, rss, ping,
            else if (true) {//flag to display css/rss icons
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
                     newUrl.endsWith("css?H", Qt::CaseInsensitive)   )
                {
                    qDebug()<< "			wc_parser: # non-absolute newUrl " << newUrl
                            << " must be a web 2.0 element (rss, favicon, etc) or file. Skipping...";
                    //	emit createNode(currentUrl, currentNode);
                    //  perhaps we paint that node with a different colour or check a variable?
                    continue;
                }
            }
            // .. else create node and add it to frontier.
            qDebug()<< "			wc_parser: non-absolute newUrl "  <<  newUrl.toLatin1()
                    <<  " first time visited. I will create a node for it and add it to frontier";
            this->createNode(newUrl, true);
            emit signalCreateEdge (currentNode, discoveredNodes);


        }
    }
    frontierNotEmpty.wakeAll();
    mutex.unlock();
}




//signals node creation  Called from wc_parser::load()

void WebCrawler_Parser::createNode(QString newUrl, bool enqueue_to_frontier) {
    discoveredNodes++;
    sourceMap[ discoveredNodes ] = currentNode;
    knownUrls[newUrl]=discoveredNodes;
    if (enqueue_to_frontier) {
        frontier.enqueue(newUrl);
        qDebug()<< "\n\n		wc_parser: * Creating node " << discoveredNodes
                << " newUrl "<< newUrl << " Frontier size: "<<  frontier.size()
                << " = discoveredNodes: " <<discoveredNodes<<  " - source: " <<  sourceMap[ discoveredNodes ]
                   << "\n\n";

    }
    else {
        qDebug()<< "\n\n		wc_parser: * Creating node " << discoveredNodes
                << " newUrl "<< newUrl << " NOT enqueuing to frontier"
                << "  discoveredNodes: " <<discoveredNodes<<  " - source: " <<  sourceMap[ discoveredNodes ]
                   << "\n\n";
    }

    emit signalCreateNode(newUrl, discoveredNodes);
}


