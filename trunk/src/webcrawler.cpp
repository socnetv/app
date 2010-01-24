/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.81
 Written in Qt 4.4
 
                         webcrawler.h  -  description
                             -------------------
    copyright            : (C) 2005-2010 by Dimitris B. Kalamaras
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
#include <QHttp>
#include <QDebug>
#include <QQueue>
#include <QVector>
#include <QMutex>
#include <QWaitCondition>

QHttp *http;
QQueue<QString> frontier;
QMap <int, int> sourceMap;
QMap <QString, bool> checkedMap;
QMap <QString, int> discoveredMap;
QByteArray ba;
QWaitCondition newDataRead;
QMutex mutex;
QString baseUrl="", seed="", domain="", seed_domain = "", previous_domain="", path="", urlPrefix="";
int maxNodes, discoveredNodes=0, currentNode, maxRecursion;
bool goOut=false, hasUrlPrefix=false;

void WebCrawler::load (QString url, int maxN, int maxRec, bool gOut){ 

	if (seed.contains(" "))		//urls can't have spaces... 
		return; 

	seed=url;
	maxNodes=maxN;		//maxnodes we'll check
	maxRecursion = maxRec;
	goOut = gOut; 

	//clear global variables	
	frontier.clear();
	sourceMap.clear();
	checkedMap.clear();
	discoveredMap.clear();
	baseUrl="", domain="", previous_domain="", path="", urlPrefix="";
	
	frontier.enqueue(seed);		//queue the seed to frontier
        currentNode=1;			//start at node 1
	discoveredNodes = 1;
	
	http = new QHttp(this); 	
	
	//connect done() signal of http to load() of 2ond Reader class
	connect (http,SIGNAL( done(bool) ), &reader, SLOT( load() ) ); 
	
	qDebug() << "WebCrawler:: I will start a new QThread!";
	
	connect (	
                &reader, SIGNAL( signalCreateNode (QString, int) ),
                this, SLOT(slotCreateNode(QString, int ) )
                ) ;

	connect (
		&reader, SIGNAL(signalCreateEdge (int, int)), 
		this, SLOT(slotCreateEdge (int, int) ) 
		);


	if (!isRunning()) 
		start(QThread::TimeCriticalPriority);
}



//called from reader createNode signal
void WebCrawler::slotCreateNode(QString url, int number) {
	emit createNode(url, number);
}

//called from reader createEdge signal
void WebCrawler::slotCreateEdge (int source, int target){
	emit createEdge (source, target);	
}



/*	 This thread sets host and url to http object. 
* 	When http signals done() then reader thread is loaded
*  to parse the data
*/
void WebCrawler::run(){
	int pos, second_pos;
	do { 	//repeat forever....

		if (currentNode>1 ) {
			maxRecursion --; 
		}

		if (frontier.size() ==0 ) { 	// .... or until we crawl all urls in frontier.		
			qDebug () <<"		WebCrawler #### Frontier is empty: " <<frontier.size() << " - we will stop now "  ;			
			break;
		}
			
		if ( maxRecursion  == 0 ) { 	// .... or until we reach maxRecursion
			qDebug () <<"		WebCrawler #### Reached maxRecursion: " <<maxRecursion << " - we will stop now "  ;	
			break;
		}

		if (maxNodes>0) {  				// .... or until we have reached maxNodes		
			if (currentNode == maxNodes ) {
				qDebug () <<"		WebCrawler: #### Seems we have reached maxnodes!" << " - we will stop now" ;
				break;
			}
		}


		baseUrl = frontier.head();	//take the first url from the frontier - name it baseUrl
	
		if ( ! checkedMap[baseUrl] ){
			
			checkedMap[baseUrl]=true;
			
			if (baseUrl.contains ("http://" ) || baseUrl.contains ("https://" )) {	// If baseUrl (except seed) contains http, it is external
					if  ( currentNode != 1 && !goOut  ) {	// if the user don't want to crawl external links, just continue.
						if (  !baseUrl.contains( seed_domain, Qt::CaseInsensitive)   ) {
							qDebug() << "		WebCrawler: external baseUrl detected: "<<baseUrl  
								<< " But we will not crawl it. Continuing...";
							continue;
						}
					}
					else if ( currentNode == 1){
						qDebug() << "		WebCrawler: creating node 1 with label " << baseUrl.toAscii();
						discoveredMap[baseUrl]=currentNode;
						emit createNode(baseUrl, 1);
					}
					//else erase http and parse the rest.
					qDebug() << "		WebCrawler: external baseUrl detected: "<<baseUrl 
							<< " Removing http:// and setting this as get token"; 
					baseUrl=baseUrl.remove ("http://");
					if ( (pos=baseUrl.indexOf ("/")) !=-1 ) { //break baseUrl, if needed, to domain and page.
						domain = baseUrl.left(pos);
						qDebug() << "		WebCrawler: Host domain: "<<  domain.toAscii();
						path = baseUrl.remove(0, pos);
						qDebug() << "		WebCrawler: Webpage to get: "<<  path.toAscii() ;
						http->setHost( domain );
						seed_domain = domain; 		
						http->get( path );
						 
						if ( (second_pos=path.lastIndexOf ("/")) !=-1 ){
							urlPrefix = path.left(second_pos);
							hasUrlPrefix=true;
							qDebug() << "		WebCrawler: urlPrefix: "<<  urlPrefix.toAscii() ;
						}
					}
					else {
						qDebug() << "		WebCrawler: Host domain is the url: " << baseUrl.toAscii() << " I' ll just get /...";
						domain = baseUrl;
						seed_domain = domain;
						http->setHost(domain); 		
						http->get("/"); 
					}
	
			}
			else { //no http:
					if (currentNode==1) {		//only if this is the seed node
						if ( (pos=baseUrl.indexOf ("/")) !=-1 ) {
							domain = baseUrl.left(pos);
							qDebug() << "		WebCrawler: Initial Host domain: "<<  domain.toAscii();
							path = baseUrl.remove(0, pos);
							qDebug() << "		WebCrawler: Initial Webpage to get: "<<  path.toAscii() ;
							seed_domain = domain;
							http->setHost( domain ); 		
							http->get( path ); 
						}
						else {
							qDebug() << "		WebCrawler: Initial url: " << baseUrl.toAscii() << " I' ll just get /...";
							domain = baseUrl;
							http->setHost(domain); 		
							http->get("/"); 					
						}
						emit createNode(baseUrl, 1);
					}
					else {
						qDebug() << "		WebCrawler: internal url detected " << baseUrl.toAscii() << " I will use previous domain "<< domain.toAscii();
						if (baseUrl.startsWith('.', Qt::CaseInsensitive) ) 
							baseUrl=baseUrl.remove(0, 1);
						else if (!baseUrl.startsWith('/',  Qt::CaseInsensitive) ) {
							baseUrl = "/"+baseUrl;
							qDebug() << "			adding / to baseUrl " << baseUrl;
						}
								
						http->setHost(previous_domain); 		
						http->get(baseUrl); 
					}
			}
			
		}
		else { //else don't do nothing!
			qDebug() << "		WebCrawler: baseUrl "  <<  baseUrl.toAscii() << " already scanned. Skipping.";
			frontier.dequeue();			//Dequeue head
			continue;
		}
		
		if (	domain != previous_domain && (currentNode!=1) ) {
			qDebug () << "		WebCrawler: **** NEW DOMAIN " ;
		}
		else {
			qDebug () << "		WebCrawler: **** SAME DOMAIN ";
		}
		
   
		//lock mutex
		mutex.lock();
		qDebug() << "		WebCrawler: ZZzz We should wait a bit..."
					<<"frontier size " << frontier.size() << " currentNode " << currentNode ;
		//Thread goes to sleep to protect all variables (locked by mutex). 
		newDataRead.wait(&mutex);
		//Unlock it
		mutex.unlock();
		qDebug () <<"		WebCrawler: OK. Waking up to continue: frontier size = " << frontier.size();




		qDebug () <<"		WebCrawler: Increasing currentNode, dequeuing frontier and setting previous domain to domain";
		currentNode++;
		frontier.dequeue();			//Dequeue head
		previous_domain = domain;		//set previous domain
		

	} while ( 1 ); 

	if (reader.isRunning() )		//tell the other thread that we must quit! 
		reader.quit();
	qDebug() << "			Finished!";
}



//called from Graph, when closing network, to terminate all processes
void WebCrawler::terminateReaderQuit (){
	if (reader.isRunning() )		//tell the other thread that we must quit! 
		reader.quit();
}


 
/* 
*  This method starts the Reader thread
* 	It is called when the http object has emitted the done() signal
* 	(that is, when last pending request has finished).
*/ 
void Reader::load(){
	if (!isRunning()) 
		start(QThread::NormalPriority);
}



/*
 * This method is all that the Reader thread does.
 * Essentially, it's called when http has finished all pending requests.
 * First, we start by reading all from http to the QString page.
 * Then we parse the page string, searching for url substrings. 
 */ 
void Reader::run(){
	qDebug()  << "			READER: read something!";	
	QString newUrl;
	bool createNodeFlag = false, createEdgeFlag=false ;
	int start=-1, end=-1, equal=-1 ;// index=-1;
	ba=http->readAll(); 
	QString page(ba);

	if (!page.contains ("a href"))  { //if a href doesnt exist, return   
		//FIXME: Frameset pages are not parsed! See docs/manual.html for example.
		 qDebug() << "			READER: ### Empty or not usefull data from " << baseUrl.toAscii() << " RETURN";
		 newDataRead.wakeAll();
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
		
		qDebug()<< "			READER: page still contains links - Parsing " << newUrl.toAscii();
		// if this is not the 1st node, and it has been already checked ...
		QMap<QString, int>::const_iterator index = discoveredMap.find(newUrl);
		if (   index!= discoveredMap.end() ) {
			qDebug()<< "			READER: #---> newUrl "  <<  newUrl.toAscii() 
					<< " already CHECKED - Just creating an edge from " << currentNode << " to " << index.value();
			//this->createEdge (sourceMap [ index.value() ], index.value());	// ... then create an edge from the previous node ...
			this->createEdge (currentNode, index.value() );
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
							qDebug()<< "			READER: # absolute newUrl " << newUrl 
									<< " must be a web 2.0 element (rss, favicon, etc) or file. Skipping...";
								//	emit createNode(baseUrl, currentNode);
								//  perhaps we paint that node with a different colour or check a variable?
								//continue;
						}
				if ( !goOut ) {		// ... and we need to limit ourselves within the seed domain...  
					if (  !newUrl.startsWith(seed_domain, Qt::CaseInsensitive ) ||
						 !newUrl.startsWith( "http://"+seed_domain, Qt::CaseInsensitive)  ) {	//...then check if the newUrl is out of the seed domain
					  	qDebug()<< "			READER: # absolute newUrl "  <<  newUrl.toAscii() 
					  			<< " is OUT OF the seed (original) domain. I will create a node but NOT add it to frontier...";
						this->createNode(newUrl, false);
						this->createEdge(currentNode, discoveredNodes); 
					 }
					else {
						qDebug()<< "			READER: absolute newUrl" << newUrl.toAscii()
								<< " appears INSIDE the seed domain "  
								<< seed_domain << " - I will create a node here..." ;
						this->createNode(newUrl, true);
						this->createEdge(currentNode, discoveredNodes);
					}
					

				}
				else {				// ... else if we can go out the seed domain, then just add it.
					qDebug()<< "			READER: absolute newUrl" << newUrl.toAscii()
						<< " is outside the seed domain "  
						<< seed_domain << " - and we are allowed to go there, so I will create a node here..." ;
					this->createNode(newUrl, true);
					this->createEdge(currentNode, discoveredNodes);
				}
	
		}
		else {	//	if this is an internal or relative url ....
				//  ...and an index, then skip it.
				if (newUrl == "index.html" || newUrl == "index.htm" || newUrl == "index.php"){
					qDebug()<< "			READER: # non-absolute newUrl "  <<  newUrl.toAscii() 
					  		<< " must be an index file. Creating edge from 1 to " << discoveredNodes;
					 this->createEdge ( 1 , discoveredNodes);	
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
							qDebug()<< "			READER: # non-absolute newUrl " << newUrl 
									<< " must be a web 2.0 element (rss, favicon, etc) or file. Skipping...";
								//	emit createNode(baseUrl, currentNode);
								//  perhaps we paint that node with a different colour or check a variable?
								continue;
						}
				}
				// .. else create node and add it to frontier.
				qDebug()<< "			READER: non-absolute newUrl "  <<  newUrl.toAscii() 
						<<  " first time visited. I will create a node for it and add it to frontier"; 
				this->createNode(newUrl, true);
				this->createEdge(currentNode, discoveredNodes);

		}
	}
	newDataRead.wakeAll();
	mutex.unlock();
}




//signals node creation  Called from Reader::load()

void Reader::createNode(QString newUrl, bool enqueue_to_frontier) {
	discoveredNodes++;
	sourceMap[ discoveredNodes ] = currentNode;
	discoveredMap[newUrl]=discoveredNodes;
	if (enqueue_to_frontier) {
		frontier.enqueue(newUrl);
		qDebug()<< "\n\n		READER: * Creating node " << discoveredNodes 
				<< " newUrl "<< newUrl << " Frontier size: "<<  frontier.size() 
				<< " = discoveredNodes: " <<discoveredNodes<<  " - source: " <<  sourceMap[ discoveredNodes ] 
				<< "\n\n";

	}
	else {
		qDebug()<< "\n\n		READER: * Creating node " << discoveredNodes 
				<< " newUrl "<< newUrl << " NOT enqueuing to frontier"
				<< "  discoveredNodes: " <<discoveredNodes<<  " - source: " <<  sourceMap[ discoveredNodes ]
				<< "\n\n";	
	}

	emit signalCreateNode(newUrl, discoveredNodes);
}



//signals edge creation  Called from Reader::load
void Reader::createEdge (int source, int target){
	qDebug()<< "\n\n		READER: --> Creating edge from " << source 
							<< " to "<< target << "\n\n";
	emit signalCreateEdge (source, target);	
}

