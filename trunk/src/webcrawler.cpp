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
    if (seed.contains(" ")) return; 
    //maxnodes we'll check
    maxNodes=maxRecursion;
    //put the seed to a queue
    frontier.enqueue(seed);
    //είμαστε στον πρώτο κόμβο
    num=1;	
    //Βάλε τον αριθμό στο διάνυσμα
    //με τους δείκτες των σελίδων
    source.append(num);
    http = new QHttp(this); 
    //connect done() signal of http to load() of 2ond Reader class
    connect (http,SIGNAL( done(bool) ), &reader, SLOT( load() ) ); 
	qDebug("Start a new QThread!");

    //start thread
    if (!isRunning()) 
        start(QThread::TimeCriticalPriority);
}


void WebCrawler::run(){
    do{ 
        //πάρε το πρώτο url από την ουρά
        baseUrl = frontier.head();
        //φτιάξε έναν κόμβο
        emit createNode(baseUrl, num);
        int src;
        if (num>1) {
            //αν δεν είναι το πρώτο μας
            //φτιάξε και την ακμή
            qDebug()<< "Creating edge " << source[num-1] << " " <<  num;
            //Στο source έχουμε κρατήσει τον δείκτη
            //της σελίδας όπου βρήκαμε το url
            emit createEdge (source[num-1], num); 
        }
        qDebug ("Nodes %i", num);
        //Αν ο χρήστης έχει δώσει και http
        //αφαίρεσέ το
        if (baseUrl.contains ("http://" ) ) 
                baseUrl=baseUrl.remove ("http://");
        qDebug() << "Scanning " <<  baseUrl.toAscii();	
        int index;
        //Σπάσε το URL, αν χρειάζεται
        //σε domain και σελίδα
        if ( (index=baseUrl.indexOf ("/")) !=-1 ) {
            qDebug() << baseUrl.left(index).toAscii();
            qDebug() <<baseUrl.remove(0, index).toAscii() ;
            http->setHost(baseUrl.left(index) ); 		
            http->get(baseUrl.remove(0, index) ); 
        }
        else {
            http->setHost(baseUrl); 		
            http->get("/"); 
        }
        //κλείδωσε το mutex
        mutex.lock();
        qDebug() << "ZZzz We should wait a bit...";
        //Βάλε το thread για ύπνο 
        //και όσες μεταβλητές περιλαμβάνονται στο mutex
        newDataRead.wait(&mutex);
        //Ξεκλείδωσέ το
        mutex.unlock();
        qDebug ("OK. Continuing: frontier size= %i", frontier.size());
        //Μείωσε τα maxnodes
        maxNodes--;
        //Αύξησε τo num
        num++;
        //Αφαίρεσε το head
        frontier.dequeue();
        //Αν δεν έχουμε άλλο URL,
        //τερμάτισε την επανάληψη
        if (frontier.size() ==0 ) break;
        
        
    } while ( maxNodes>0 );
    qDebug() << "Finished!";
}






void Reader::load(){
    if (!isRunning()) 
        start(QThread::NormalPriority);
}




void Reader::run(){
    qDebug()  << "READER: read something!";	
    QString newUrl;
    int at, at1;
    ba=http->readAll(); 
    QString page(ba);
    int src=source.size();
    //if a href doesnt exist, return   
    if (!page.contains ("a href"))  {
         qDebug() << "READER: Empty or not usefull data from " << baseUrl.toAscii();
         newDataRead.wakeAll();
         return;
    }
    mutex.lock();    
    //as long there is a href in the page...
    while (page.contains("a href")) {
        page=page.simplified();
        //Find its pos
        at=page.indexOf ("a href");
	    //Erase everything 8 chars  -- FIXME
        page.remove(0, at+8); 
        if (page.startsWith("\"") ) newUrl.remove(0,1);
        //Τι γίνεται όμως αν υπάρχει ' ;
        //SOS: Βρες το τέλος
        at1=page.indexOf ("\"");
        //Κράτα το URL
        newUrl=page.left(at1);
        newUrl=newUrl.simplified();
        //Τύπωσε το
        qDebug() << "NewUrl = " << newUrl.toAscii();
        //SOS: Κάποιοι έλεγχοι χρειάζονται εδώ...
        //Αν δεν ξεκινάει με http://...
        if ( !newUrl.startsWith ("http://") ) {
            newUrl=baseUrl+"/"+page.left(at1); 
        }
        qDebug() << "NewUrl =" << newUrl.toAscii() << ".";
        //Αν δεν το έχουμε βρει ήδη...
        if (!frontier.contains (newUrl) ) {
            frontier.enqueue(newUrl);
            source.append(src);
            qDebug()<< "frontier size "<<  frontier.size() << " source= " <<  src;
        }
        else //αλλιώς μην το βάλεις στην ουρά
            qDebug() << "BaseUrl "  <<  baseUrl.toAscii() << " already scanner. Skipping";
    }
    newDataRead.wakeAll();
    mutex.unlock();
}


