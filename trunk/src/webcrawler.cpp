/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 0.70
 Written in Qt 4.4
 
                         webcrawler.h  -  description
                             -------------------
    copyright            : (C) 2005-2009 by Dimitris B. Kalamaras
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
	baseUrl="", domain="", previous_domain="", path="", urlPrefix="";
	
	frontier.enqueue(seed);		//queue the seed to frontier
	currentNode=1;						//start at node 1	
	discoveredNodes = 1;
	
	http = new QHttp(this); 	
	
	//connect done() signal of http to load() of 2ond Reader class
	connect (http,SIGNAL( done(bool) ), &reader, SLOT( load() ) ); 
	
	qDebug() << "WebCrawler:: I will start a new QThread!";
	
	if (!isRunning()) 
		start(QThread::TimeCriticalPriority);
}




void WebCrawler::run(){
	int pos, second_pos;
	do{ 	//repeat until we reach maxNodes.

		if (frontier.size() ==0 ) { 	//or until we reach frontier's end.		
			qDebug () <<"		WebCrawler #### Frontier is empty: " <<frontier.size() << " - we will stop now "  ;			
			break;
		}
			
		if ( maxRecursion  == 0 ) { 	//or until we reach maxRecursion
			qDebug () <<"		WebCrawler #### Reached maxRecursion: " <<maxRecursion << " - we will stop now "  ;	
			break;
		}

		if (maxNodes>0) {  //or until we have reached maxNodes!			
			if (currentNode == maxNodes ) {
				qDebug () <<"		WebCrawler: #### Seems we have reached maxnodes!" << " - we will stop now" ;
				break;
			}
		}


		baseUrl = frontier.head();	//take the first url from the frontier : this is our baseUrl
		
		//TODO: 
		if ( baseUrl.endsWith(".css", Qt::CaseInsensitive) ) { //  label contains css, paint that node with a different colour.
			qDebug()<< "		WebCrawler: Creating node " << currentNode 
					<< " with label this baselUrl " << baseUrl 
					<< " - This shouldn't be a regular node...";			
			emit createNode(baseUrl, currentNode);
		} 
		else {
			qDebug()<< "		WebCrawler: Creating node " << currentNode 
					<< " with label this baselUrl " << baseUrl;
			emit createNode(baseUrl, currentNode);			
		}


		if (currentNode>1) {	//if ain't the 1st node, then create an edge from the previous node.
  			qDebug()<< "		WebCrawler: Creating edge from " << sourceMap [ currentNode ] 
  					<< " -> " <<  currentNode;
			//source vector holds the index of the page where this url was found.
			emit createEdge (sourceMap [ currentNode ], currentNode); 
		}

		
		if ( ! checkedMap[baseUrl] ){
			checkedMap[baseUrl]=true;
			if (baseUrl.contains ("http://" ) ) {	//If baseUrl contains http, it is external, erase http and parse the rest.
					qDebug() << "		WebCrawler: external baseUrl detected: "<<baseUrl 
							<< " Removing http:// and setting this as get token"; 
					baseUrl=baseUrl.remove ("http://");
					if ( (pos=baseUrl.indexOf ("/")) !=-1 ) { //break baseUrl, if needed, to domain and page.
						domain = baseUrl.left(pos);
						qDebug() << "		WebCrawler: Host domain: "<<  domain.toAscii();
						path = baseUrl.remove(0, pos);
						qDebug() << "		WebCrawler: Webpage to get: "<<  path.toAscii() ;
						http->setHost( domain ); 		
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
						http->setHost(domain); 		
						http->get("/"); 
					}
	
			}
			else {
					if (currentNode==1) {
						if ( (pos=baseUrl.indexOf ("/")) !=-1 ) {
							domain = baseUrl.left(pos);
							qDebug() << "		WebCrawler: Initial Host domain: "<<  domain.toAscii();
							path = baseUrl.remove(0, pos);
							qDebug() << "		WebCrawler: Initial Webpage to get: "<<  path.toAscii() ;
							http->setHost( domain ); 		
							http->get( path ); 
						}
						else {
							qDebug() << "		WebCrawler: Initial url: " << baseUrl.toAscii() << " I' ll just get /...";
							domain = baseUrl;
							http->setHost(domain); 		
							http->get("/"); 					
						}
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
		
		if (currentNode>1  && domain != previous_domain) {
			maxRecursion --; 
			qDebug () << "		WebCrawler: **** NEW DOMAIN - DECREASING RECURSION DEPTH INDEX TO " << maxRecursion   ;

		}
		else if (currentNode==1) {
			seed_domain = domain;
		}
		else {
			qDebug () << "		WebCrawler: **** SAME DOMAIN - RECURSION DEPTH INDEX UNCHANGED: " << maxRecursion ;
			
		}
   
		//lock mutex
		mutex.lock();
		qDebug() << "		WebCrawler: ZZzz We should wait a bit...";
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
	int start=-1, end=-1, equal=-1;
	ba=http->readAll(); 
	QString page(ba);
	//qDebug()  << "		"<< page.toAscii();
//	int src=source.size();

	if (!page.contains ("a href"))  { //if a href doesnt exist, return   
		//FIXME: Frameset pages are not parsed! See docs/manual.html for example.
		 qDebug() << "			READER: ### Empty or not usefull data from " << baseUrl.toAscii() << " RETURN";
		 newDataRead.wakeAll();
		 return;
	}
	mutex.lock(); 

	while (page.contains("href")) {	//as long there is a href in the page...
		page=page.simplified();		// remove whitespace from the start and the end - all whitespace sequence becomes single space

		start=page.indexOf ("href");		//Find its pos

		page = page.remove(0, start);		//erase everything up to href

		equal=page.indexOf ("=");			// Find next equal sign (=)

		page = page.remove(0, equal+1);		//Erase everything up to = 
		//qDebug() << "			READER: New Url??? : " << page.left(20);

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
		
		//if ( frontier.indexOf(newUrl) ==-1  ||  frontier.indexOf(newUrl)   ) {	//If this is the first time visiting this url, add it to frontier...

			if ( newUrl.contains("http://", Qt::CaseInsensitive) ) {	//if this is an absolute url
				if ( !goOut &&  !newUrl.contains( seed_domain, Qt::CaseInsensitive)  && 
						!newUrl.contains( urlPrefix , Qt::CaseInsensitive)   ) {
					  	qDebug()<< "			READER: absolute newUrl "  <<  newUrl.toAscii() 
					  		<< " is OUT OF the original domain. Skipping...";
				}
				else {
					frontier.enqueue(newUrl);
					discoveredNodes++;
					sourceMap[ discoveredNodes	 ] = currentNode;
					qDebug()<< "			READER: absolute newUrl "  <<  newUrl.toAscii() 
								<<  " first time visited. Frontier size: "<<  frontier.size() << " = discoveredNodes: " <<discoveredNodes<<  " - source: " <<  sourceMap[ discoveredNodes ];				
				}
	
			}
			else {
				frontier.enqueue(newUrl);
				discoveredNodes++;
				sourceMap[ discoveredNodes ] = currentNode;
				qDebug()<< "			READER: non-absolute newUrl "  <<  newUrl.toAscii() 
							<<  " first time visited. Frontier size: "<<  frontier.size() << " = discoveredNodes: " <<discoveredNodes<<  " - source: " <<  sourceMap[ discoveredNodes ];
			}
		//}
		//else //else don't do nothing!
		//	qDebug() << "			READER: newUrl "  <<  newUrl.toAscii() << " already scanned. Skipping.";

	}
	newDataRead.wakeAll();
	mutex.unlock();
}


