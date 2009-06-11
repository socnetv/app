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
QString baseUrl;
QQueue<QString> frontier;
QVector<int> source;
QByteArray ba;
QWaitCondition newDataRead;
QMutex mutex;



void WebCrawler::load (QString seed, int maxRecursion, int maxTime, bool goOut){ 

	if (seed.contains(" "))		//urls can't have spaces... 
		return; 
	
	maxNodes=maxRecursion;		//maxnodes we'll check
	frontier.enqueue(seed);		//put the seed to a queue
	num=1;						//start at node 1	
	source.append(num);			//append num to the source vector, holding page index
	http = new QHttp(this); 	
	
	//connect done() signal of http to load() of 2ond Reader class
	connect (http,SIGNAL( done(bool) ), &reader, SLOT( load() ) ); 
	
	qDebug() << "WebCrawler:: I will start a new QThread!";
	
	if (!isRunning()) 
		start(QThread::TimeCriticalPriority);
}




void WebCrawler::run(){
	QString domain, path; 
	do{ 	//until we reach maxRecursion=maxNodes level.
		
		baseUrl = frontier.head();	//take the first url from the frontier : this is our baseUrl
		qDebug()<< "			Creating node " << num << " label " << baseUrl;
		emit createNode(baseUrl, num);
		if (num>1) {
			//if ain't the 1st node, then create an edge from the previous node.  
			qDebug()<< "			Creating edge from " << source[num-1] << " -> " <<  num;
			//source vector holds the index of the page where this url was found.
			emit createEdge (source[num-1], num); 
		}
		qDebug() << "			Nodes now: " << num;
		//If baseUrl includes http, erase it.
		if (baseUrl.contains ("http://" ) ) 
				baseUrl=baseUrl.remove ("http://");
		qDebug() << "			Now, I will start scanning " <<  baseUrl.toAscii();	
		int index;
		//break baseUrl, if needed, to domain and page.
		if ( (index=baseUrl.indexOf ("/")) !=-1 ) {
			domain = baseUrl.left(index);
			qDebug() << "			Host domain: "<<  domain.toAscii();
			path = baseUrl.remove(0, index);
			qDebug() << "			Webpage to get: "<<  path.toAscii() ;
			http->setHost( domain ); 		
			http->get( path ); 
		}
		else { 
			qDebug() << " 			clean domain detected " << baseUrl.toAscii();
			http->setHost(baseUrl); 		
			http->get("/"); 
		}
		//lock mutex
		mutex.lock();
		qDebug() << "			ZZzz We should wait a bit...";
		//Thread goes to sleep to protect all variables (locked by mutex). 
		newDataRead.wait(&mutex);
		//Unlock it
		mutex.unlock();
		qDebug () <<"				OK. Waking up to continue: frontier size = " << frontier.size();
		qDebug () <<"				Decrease maxnodes";
		maxNodes--;
		qDebug () <<"				Increase num";
		num++;
		frontier.dequeue();			//Dequeue head
		qDebug () <<"				if frontier is empty, break recursion " <<frontier.size()  ; 
		if (frontier.size() ==0 ) break;

	} while ( maxNodes>0 );
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
    qDebug()  << "		READER: read something!";	
    QString newUrl;
    int start=-1, end=-1, equal=-1;
    ba=http->readAll(); 
    QString page(ba);
    qDebug()  << "		"<< page.toAscii();
    int src=source.size();
    //if a href doesnt exist, return   
    if (!page.contains ("a href"))  {
		 qDebug() << "		READER: Empty or not usefull data from " << baseUrl.toAscii();
		 newDataRead.wakeAll();
		 return;
    }
    mutex.lock();    
    //as long there is a href in the page...
    while (page.contains("a href")) {
		page=page.simplified();
		//Find its pos
		start=page.indexOf ("a href");
		page = page.remove(0, start);
		qDebug() << "		READER: New Url??? : " << page.left(20);
		equal=page.indexOf ("=");
		//Erase everything up to =
		page = page.remove(0, equal+1); 
		qDebug() << "		READER: New Url??? : " << page.left(20);
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
		
		//Save new url to newUrl :)
		newUrl=page.left(end);
		newUrl=newUrl.simplified();
		//Print it

		qDebug() << "		READER: NewUrl =" << newUrl.toAscii() << ".";
		//Αν δεν το έχουμε βρει ήδη...
		if (!frontier.contains (newUrl) ) {
			frontier.enqueue(newUrl);
			source.append(src);
			qDebug()<< "		READER: Adding that to frontier. Frontier Size "<<  frontier.size() << " Current source= " <<  src;
		}
		else //αλλιώς μην το βάλεις στην ουρά
			qDebug() << "		READER: BaseUrl "  <<  baseUrl.toAscii() << " already scanned. Skipping";
	}
	newDataRead.wakeAll();
	mutex.unlock();
}


