/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.6
 Written in Qt 4.4
 
                         parser.cpp  -  description
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

#include "parser.h"

#include <QFile>
#include <QTextStream>
#include <QString>
#include <QRegExp>
#include <QStringList>
#include <QtDebug>		//used for qDebug messages
#include <QPointF>
#include <QMessageBox>
#include <list>
#include "graph.h"	//needed for setParent

void Parser::load(QString fn, int iNS, QString iNC, QString iLC, QString iNSh, bool iSL, int width, int height){
	qDebug("Parser: load()");
	initNodeSize=iNS;
	initNodeColor=iNC;
	initLinkColor=iLC;
	initNodeShape=iNSh;
	initShowLabels=iSL;;
	undirected=FALSE; arrows=FALSE; bezier=FALSE;
	networkName="";
	fileName=fn;
	gwWidth=width;
	gwHeight=height;
	randX=0;
	randY=0;
	qDebug("Parser: calling start() to start a new QThread!");
	if (!isRunning()) 
		start(QThread::NormalPriority);
}



/**
	Tries to load a file as DL-formatted network. If not it returns -1
*/
int Parser::loadDL(){
	qDebug ("Parser: loadDL");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );

	QString str, label;
	
	int i=0, j=0, lineCounter=0, mark=0, nodeNum=0;
	float weight=0;
	bool labels_flag=false, data_flag=false, intOK=false, floatOK=false;
	QStringList lineElement;
	networkName="";
	totalLinks=0;

	while ( !ts.atEnd() )   {
		str= ts.readLine();
		lineCounter++;
		if ( str.startsWith("%") || str.startsWith("#")  || str.isEmpty() ) continue;  //neglect comments
	
		if ( (lineCounter == 1) &&  (!str.startsWith("DL",Qt::CaseInsensitive)  ) ) {  
			qDebug("*** Parser-loadDL(): not a DL file. Aborting!");
			file.close();
			return -1;
		}

		if (str.startsWith( "N=", Qt::CaseInsensitive) ||  str.startsWith( "N =", Qt::CaseInsensitive) )  {   
			mark=str.indexOf("=");
			str=str.right(str.size()-mark-1);
			qDebug()<< "N = : " << str.toAscii() ;
			aNodes=str.toInt(&intOK,10);   
			if (!intOK) { qDebug()<< "Parser: loadDL(): conversion error..." ; return -1;}
		}

		if (str.startsWith( "FORMAT =", Qt::CaseInsensitive) || str.startsWith( "FORMAT=", Qt::CaseInsensitive))  {   
			mark=str.indexOf("=");
			str=str.right(str.size()-mark-1);
			qDebug()<<  "FORMAT = : " <<  str.toAscii() ;
		}

		if (str.startsWith( "labels", Qt::CaseInsensitive) ) {
		 	labels_flag=true; data_flag=false;
			continue;
		}
		else if ( str.startsWith( "data:", Qt::CaseInsensitive) || str.startsWith( "data :", Qt::CaseInsensitive) ) {
		 	data_flag=true; labels_flag=false;
			continue;
		}

		if (labels_flag) {  //read a label and create a node in a random position

			label=str;
			randX=rand()%gwWidth;
			randY=rand()%gwHeight;
			nodeNum++;
			emit createNode(nodeNum, initNodeSize, initNodeColor, label, initNodeColor, QPointF(randX, randY), initNodeShape, initShowLabels);
		
		}
		if ( data_flag){		//read edges
			//SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS) 
			lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);
			j=0;
			for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
				qDebug()<< (*it1).toAscii() ;
				if ( (*it1)!="0"){ //here is an non-zero edge weight...
					qDebug()<<  "Parser-loadDL(): there is an edge here";
					weight=(*it1).toFloat(&floatOK); 
					undirected=false;
					arrows=true;
					bezier=false;
					emit createEdge(i+1, j+1, weight, initLinkColor, undirected, arrows, bezier);
					totalLinks++;
					qDebug()<<  "Link from Node i= " <<  i+1 << " to j= "<< j+1;
					qDebug() << "TotalLinks= " << totalLinks;

				}
				j++;
			}
			i++;
		

		}
	}
	//sanity check
	if (nodeNum != aNodes) { 
		qDebug()<< "Error: aborting";
		return -1;
	}
	emit fileType(5, networkName, aNodes, totalLinks);
	qDebug() << "Parser-loadDL()";
	return 1;

}

/**
	Tries to load the file as Pajek-formatted network. If not it returns -1
*/
int Parser::loadPajek(){
	qDebug ("Parser: loadPajek");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	QString str, label, nodeColor, linkColor, nodeShape, temp;
	
	QStringList lineElement;
	bool ok=FALSE, intOk=FALSE, check1=FALSE, check2=FALSE;
	bool nodes_flag=FALSE, edges_flag=FALSE, arcs_flag=FALSE, arcslist_flag=FALSE, matrix_flag=FALSE;
	bool fileContainsNodeColors=FALSE, fileContainsNodesCoords=FALSE;
	bool fileContainsLinksColors=FALSE;
	bool zero_flag=FALSE;
	int  lineCounter=0, i=0, j=0, miss=0, source= -1, target=-1, nodeNum, colorIndex=-1, coordIndex=-1;
	float weight=1;
	list<int> listDummiesPajek;
	networkName="noname";
	totalLinks=0;
	aNodes=0;
	j=0;  //counts how many real nodes exist in the file 
	miss=0; //counts missing nodeNumbers. 
	//if j + miss < nodeNum, it creates (nodeNum-miss) dummy nodes which are deleted in the end.
	QList <int> toBeDeleted;
	while ( !ts.atEnd() )   {
		str= ts.readLine();
		lineCounter++;
		if ( str.startsWith("%") || str.isEmpty() ) continue;

		if (!edges_flag && !arcs_flag && !nodes_flag && !arcslist_flag && !matrix_flag) {
			qDebug("Parser-loadPajek(): reading headlines");
			if ( (lineCounter == 1) &&  (!str.contains("network",Qt::CaseInsensitive) && !str.contains("vertices",Qt::CaseInsensitive) ) ) {  
				//this is not a pajek file. Abort
				qDebug("*** Parser-loadPajek(): Not a Pajek file. Aborting!");
				file.close();
				return -1;
			}
   			else if (str.contains( "network",Qt::CaseInsensitive) )  { //NETWORK NAME
				if (str.contains(" ")) {
					lineElement=str.split(QRegExp("\\s+"));	//split at one or more spaces
					qDebug()<<"Parser-loadPajek(): possible net name: "<<lineElement[1];
					if (lineElement[1]!="") 
						networkName=lineElement[1];
				}
				else 
					networkName = "Unknown";
				qDebug()<<"Parser-loadPajek(): network name: "<<networkName;
				continue;
			}
			if (str.contains( "vertices", Qt::CaseInsensitive) )  {   
				lineElement=str.split(QRegExp("\\s+"));
				if (!lineElement[1].isEmpty()) 	aNodes=lineElement[1].toInt(&intOk,10);   
				qDebug ("Parser-loadPajek(): Vertices %i.",aNodes);
				continue;
			}
			qDebug("Parser-loadPajek(): headlines end here");
		}
		/**SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS) IN SEVERAL ELEMENTS*/
		lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);

		if ( str.contains( "*edges", Qt::CaseInsensitive) ) {
		 	edges_flag=true; arcs_flag=false; arcslist_flag=false; matrix_flag=false;
			continue;
		}
		else if ( str.contains( "*arcs", Qt::CaseInsensitive) ) { 
			arcs_flag=true; edges_flag=false; arcslist_flag=false; matrix_flag=false;
			continue;
		}
		else if ( str.contains( "*arcslist", Qt::CaseInsensitive) ) { 
			arcs_flag=false; edges_flag=false; arcslist_flag=true; matrix_flag=false;
			continue;
		}
		else if ( str.contains( "*matrix", Qt::CaseInsensitive) ) { 
			arcs_flag=false; edges_flag=false; arcslist_flag=false; matrix_flag=true;
			continue;
		}

		/** READING NODES */
		if (!edges_flag && !arcs_flag && !arcslist_flag && !matrix_flag) {
			qDebug("=== Reading nodes ===");
			nodes_flag=TRUE;
			nodeNum=lineElement[0].toInt(&intOk, 10);
			qDebug()<<"nodeNum "<<nodeNum;
			if (nodeNum==0) {
				qDebug ("Node is zero numbered! Raising zero-start-flag - increasing nodenum");
				zero_flag=TRUE;
			}
			if (zero_flag){
				nodeNum++;
			}
			if (lineElement.size() < 2 ){
				label=lineElement[0];
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				nodeColor=initNodeColor;
				nodeShape=initNodeShape;
			}
			else {	/** NODELABEL */
				label=lineElement[1];
				qDebug()<< "node label: " << lineElement[1].toAscii();
				str.remove (0, str.lastIndexOf(label)+label.size() );	
				qDebug()<<"cropped str: "<< str.toAscii();
				if (label.contains('"', Qt::CaseInsensitive) )
					label=label.remove('\"');
				qDebug()<<"node label now: " << label.toAscii();
								
				/** NODESHAPE: There are four possible . */
				if (str.contains("Ellipse", Qt::CaseInsensitive) ) nodeShape="ellipse";
				else if (str.contains("circle", Qt::CaseInsensitive) ) nodeShape="circle";
				else if (str.contains("box", Qt::CaseInsensitive) ) nodeShape="box";
				else if (str.contains("triangle", Qt::CaseInsensitive) ) nodeShape="triangle";
				else nodeShape="diamond";
				/** NODECOLORS */
				//if there is an "ic" tag, a specific NodeColor for this node follows...
				if (str.contains("ic",Qt::CaseInsensitive)) { 
					for (register int c=0; c< lineElement.count(); c++) {
						if (lineElement[c] == "ic") { 
							//the colourname is at c+1 position.
							nodeColor=lineElement[c+1];
							
							fileContainsNodeColors=TRUE;
							break;
						}
					}
					qDebug()<<"nodeColor:" << nodeColor;
					if (nodeColor.contains (".") )  nodeColor=initNodeColor;
				}
				else { //there is no nodeColor. Use the default
					qDebug("No nodeColor");
					fileContainsNodeColors=FALSE;
					nodeColor=initNodeColor;
					
				}
				/**READ NODE COORDINATES */
				if ( str.contains(".",Qt::CaseInsensitive) ) { 
					for (register int c=0; c< lineElement.count(); c++)   {
						temp=lineElement.at(c);
						qDebug()<< temp.toAscii();
						if ((coordIndex=temp.indexOf(".", Qt::CaseInsensitive)) != -1 ){ 	
							if (lineElement.at(c-1) == "ic" ) continue;  //pajek declares colors with numbers!
							if ( !temp[coordIndex-1].isDigit()) continue;  //needs 0.XX
							if (c+1 == lineElement.count() ) {//first coord zero, i.e: 0  0.455
								qDebug ()<<"coords: " <<lineElement.at(c-1).toAscii() << " " <<temp.toAscii() ;
								randX=lineElement.at(c-1).toDouble(&check1);
								randY=temp.toDouble(&check2);
							}
							else {
								qDebug ()<<"coords: " << temp.toAscii() << " " <<lineElement[c+1].toAscii();
								randX=temp.toDouble(&check1);
								randY=lineElement[c+1].toDouble(&check2);
							}

							if (check1 && check2)    {
								randX=randX * gwWidth;
								randY=randY * gwHeight;
								fileContainsNodesCoords=TRUE;
							}
							if (randX <= 0.0 || randY <= 0.0 ) {
								randX=rand()%gwWidth;
								randY=rand()%gwHeight;
							}
							break;
						}
					}
					qDebug()<<"Coords: "<<randX << randY<< gwHeight;
				}
				else { 
					fileContainsNodesCoords=FALSE;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug()<<"No coords. Using random "<<randX << randY;
				}
			}
			/**START NODE CREATION */
			qDebug ()<<"Creating node numbered "<< nodeNum << " Real nodes count (j)= "<< j+1;
			j++;  //Controls the real number of nodes.
			//If the file misses some nodenumbers then we create dummies and delete them afterwards!
			if ( j + miss < nodeNum)  {
				qDebug ()<<"MW There are "<< j << " nodes but this node has number "<< nodeNum;
				for (int num=j; num< nodeNum; num++) {
					qDebug()<< "Parser-loadPajek(): Creating dummy node number num = "<< num;
					emit createNode(num,initNodeSize, nodeColor, label, lineElement[3], QPointF(randX, randY), nodeShape, initShowLabels);
					listDummiesPajek.push_back(num);  
					miss++;
				}
			}
			else if ( j > nodeNum ) {
				qDebug ("Error: This Pajek net declares this node with nodeNumber smaller than previous nodes. Aborting");
				return -1;	
			}
			emit createNode(nodeNum,initNodeSize, nodeColor, label, nodeColor, QPointF(randX, randY), nodeShape, initShowLabels);
			initNodeColor=nodeColor; 
		} 	
		/**NODES CREATED. CREATE EDGES/ARCS NOW. */		
		else {
			if (j && j!=aNodes)  {  //if there were more or less nodes than the file declared
				qDebug()<<"*** WARNING ***: The Pajek file declares " << aNodes <<"  nodes, but I found " <<  j << " nodes...." ;
				aNodes=j;
			}
			else if (j==0) {  //if there were no nodes at all, we need to create them now.
				qDebug()<< "The Pajek file declares "<< aNodes<< " but I didnt found any nodes. I will create them....";
				for (int num=j+1; num<= aNodes; num++) {
					qDebug() << "Parser-loadPajek(): Creating node number i = "<< num;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					emit createNode(num,initNodeSize, initNodeColor, QString::number(i), "black", QPointF(randX, randY), initNodeShape, initShowLabels);
				}
				j=aNodes;
			}
			if (edges_flag && !arcs_flag)   {  /**EDGES */
				qDebug("Parser-loadPajek(): ==== Reading edges ====");
				qDebug()<<lineElement;
				source =  lineElement[0].toInt(&ok, 10);
				target = lineElement[1].toInt(&ok,10);
				if (source == 0 || target == 0 ) return -1;  //  i --> (i-1)   internally
				else if (source < 0 && target >0  ) {  //weights come first...
					weight = lineElement[0].toFloat(&ok);
					source=  lineElement[1].toInt(&ok, 10);
					if (lineElement.count()>2) {
						target = lineElement[2].toInt(&ok,10);
					}
					else {
						target = lineElement[1].toInt(&ok,10);  //self link
					}
				}
				else if (lineElement.count()>2)
					weight =lineElement[2].toFloat(&ok);
				else 
					weight=1.0;

				qDebug()<<"Parser-loadPajek(): weight "<< weight;

				if (lineElement.contains("c", Qt::CaseSensitive ) ) {
					qDebug("Parser-loadPajek(): file with link colours");
					fileContainsLinksColors=TRUE;
					colorIndex=lineElement.indexOf( QRegExp("[c]"), 0 )  +1;
					if (colorIndex >= lineElement.count()) linkColor=initLinkColor;
					else 	linkColor=lineElement [ colorIndex ];
					if (linkColor.contains (".") )  linkColor=initLinkColor;
					qDebug()<< " current color "<< linkColor;
 				}
				else  {
					qDebug("Parser-loadPajek(): file with no link colours");
					linkColor=initLinkColor;
				}
				undirected=true;
				arrows=true;
				bezier=false;
				qDebug()<< "Parser-loadPajek(): Create edge between " << source << " - "<< target;
				emit createEdge(source, target, weight, linkColor, undirected, arrows, bezier);
				totalLinks=totalLinks+2;

			} //end if EDGES 
			else if (!edges_flag && arcs_flag)   {  /** ARCS */
				qDebug("Parser-loadPajek(): === Reading arcs ===");
				source=  lineElement[0].toInt(&ok, 10);
				target = lineElement[1].toInt(&ok,10);
				if (source == 0 || target == 0 ) return -1;   //  i --> (i-1)   internally
				else if (source < 0 && target >0 ) {  //weights come first...
					weight = lineElement[0].toFloat(&ok);
					source=  lineElement[1].toInt(&ok, 10);
					if (lineElement.count()>2) {
						target = lineElement[2].toInt(&ok,10);
					}
					else {
						target = lineElement[1].toInt(&ok,10);  //self link
					}
				}
				else if (lineElement.count()>2)
					weight =lineElement[2].toFloat(&ok);
				else 
					weight=1.0;

				if (lineElement.contains("c", Qt::CaseSensitive ) ) {
					qDebug("Parser-loadPajek(): file with link colours");
					linkColor=lineElement.at ( lineElement.indexOf( QRegExp("[c]"), 0 ) + 1 );
					fileContainsLinksColors=TRUE;
				}
				else  {
					qDebug("Parser-loadPajek(): file with no link colours");
					linkColor=initLinkColor;
				}
				undirected=false;
				arrows=true;
				bezier=false;
				qDebug()<<"Parser-loadPajek(): Creating arc from node "<< source << " to node "<< target << " with weight "<< weight;
				emit createEdge(source, target, weight, linkColor, undirected, arrows, bezier);
				totalLinks++;
			} //else if ARCS
			else if (arcslist_flag)   {  /** ARCSlist */
				qDebug("Parser-loadPajek(): === Reading arcs list===");
				if (lineElement[0].startsWith("-") ) lineElement[0].remove(0,1);
				source= lineElement[0].toInt(&ok, 10);
				fileContainsLinksColors=FALSE;
				linkColor=initLinkColor;
				undirected=false;
				arrows=true;
				bezier=false;
				for (register int index = 1; index < lineElement.size(); index++) {
					target = lineElement.at(index).toInt(&ok,10);					
					qDebug()<<"Parser-loadPajek(): Creating arc source "<< source << " target "<< target << " with weight "<< weight;
					emit createEdge(source, target, weight, linkColor, undirected, arrows, bezier);
					totalLinks++;
				}
			} //else if ARCSLIST
			else if (matrix_flag)   {  /** matrix */
				qDebug("Parser-loadPajek(): === Reading matrix of edges===");
				i++;
				source= i;
				fileContainsLinksColors=FALSE;
				linkColor=initLinkColor;
				undirected=false;
				arrows=true;
				bezier=false;
				for (target = 0; target < lineElement.size(); target ++) {
					if ( lineElement.at(target) != "0" ) {
						weight = lineElement.at(target).toFloat(&ok);					
						qDebug()<<"Parser-loadPajek(): Creating arc source "<< source << " target "<< target +1<< " with weight "<< weight;
						emit createEdge(source, target+1, weight, linkColor, undirected, arrows, bezier);
						totalLinks++;
					}
				}
			} //else if matrix
		} //end if BOTH ARCS AND EDGES
	} //end WHILE
	file.close();
	if (j==0) return -1;
	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(1, networkName, aNodes, totalLinks);
	qDebug("Parser-loadPajek(): Removing all dummy aNodes, if any");
	if (listDummiesPajek.size() > 0 ) {
		qDebug("Trying to delete the dummies now");
		for ( list<int>::iterator it=listDummiesPajek.begin(); it!=listDummiesPajek.end(); it++ ) {
			emit removeDummyNode(*it);
		}
	}
	qDebug("Parser-loadPajek(): Clearing DumiesList from Pajek");
	listDummiesPajek.clear();
	exit(0);
	return 1;

}






/**
	Tries to load the file as adjacency sociomatrix-formatted. If not it returns -1
*/
int Parser::loadAdjacency(){
	qDebug("Parser: loadAdjacency()");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	networkName="";
	QString str;
	QStringList lineElement;
	int i=0, j=0,  aNodes=0;
	float weight=1;
	bool intOK=FALSE;


	while ( !ts.atEnd() )   {
		str= ts.readLine() ;
		str=str.simplified();  // transforms "/t", "  ", etc to plain " ".
		if (str.isEmpty() ) continue;	
		if ( str.contains("vertices",Qt::CaseInsensitive) || (str.contains("network",Qt::CaseInsensitive) || str.contains("graph",Qt::CaseInsensitive)  || str.contains("digraph",Qt::CaseInsensitive) ||  str.contains("DL",Qt::CaseInsensitive) || str.contains("list",Qt::CaseInsensitive)) || str.contains("graphml",Qt::CaseInsensitive) || str.contains("xml",Qt::CaseInsensitive)  ) {
			qDebug()<< "*** Parser:loadAdjacency(): Not an Adjacency-formatted file. Aborting!!";
			file.close();		
 		 	return -1;    
		}

		lineElement=str.split(" ");
		if (i == 0 ) {
			aNodes=lineElement.count();
			qDebug("Parser-loadAdjacency(): There are %i nodes in this file", aNodes);		
			for (j=0; j<aNodes; j++) {
				qDebug("Parser-loadAdjacency(): Calling createNode() for node %i", j+1);
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				qDebug()<<"Parser-loadAdjacency(): no coords. Using random "<<randX << randY;

	// 			nodeNum,initNodeSize,nodeColor, label, lColor, QPointF(X, Y), nodeShape
				emit createNode(j+1,initNodeSize, initNodeColor, QString::number(j+1), "black", QPointF(randX, randY), initNodeShape, false);
			}
		}
		qDebug("Parser-loadAdjacency(): Finished creating new nodes");
		if ( aNodes != (int) lineElement.count() ) return -1;	
		j=0;
		qDebug("Parser-loadAdjacency(): Starting creating links");		
		for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
			if ( (*it1)!="0"){
				qDebug("Parser-loadAdjacency(): there is a link here");
				weight=(*it1).toFloat(&intOK);
				undirected=false;
				arrows=true;
				bezier=false;
				emit createEdge(i+1, j+1, weight, initLinkColor, undirected, arrows, bezier);
				totalLinks++;

				qDebug("Link from Node i=%i to j=%i", i+1, j+1);
				qDebug("TotalLinks= %i", totalLinks);
/*				if (i>j &&  activeGraph.hasEdge (j+1, i+1)==0 ) { 
					qDebug("Node j=%i not linked with i=%i, but the opposite. Non-Symmetric Sociomatrix.", j+1, i+1);
					symmetricAdjacency=FALSE;
				}*/
			}
/*			else if (i>j && activeGraph.hasEdge (j+1, i+1)!=0 ) { 
					qDebug("Node i=%i not linked with j=%i, but the opposite. Non-Symmetric Sociomatrix.", i+1, j+1);
					symmetricAdjacency=FALSE;
				}*/
			j++;
		}
		i++;
	}
	file.close();

	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(2, networkName, aNodes, totalLinks);
	return 1;
}






/**
	Tries to load a file as GraphML (not GML) formatted network. 
	If not it returns -1
*/
int Parser::loadGraphML(){
	qDebug("Parser: loadGraphML()");
	aNodes=1;
	nodeNumber.clear();
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;

	QXmlStreamReader *xml = new QXmlStreamReader();
	xml->setDevice(&file);
	
	while (!xml->atEnd()) {
		xml->readNext();
		qDebug()<< " loadGraphML(): xml->token"<< xml->tokenString();
		if (xml->isStartElement()) {
			qDebug()<< " loadGraphML(): element name "<< xml->name()<<" version " << xml->attributes().value("version")  ;
			if (xml->name() == "graphml") {
				qDebug()<< " loadGraphML(): OK. NamespaceUri is "<< xml->namespaceUri();
				readGraphML(*xml);				
			}
			else {
				xml->raiseError(QObject::tr(" loadGraphML(): The file is not an GraphML version 1.0 file."));
				qDebug()<< "*** loadGraphML(): Error in startElement  ";
 				return -1;
			}
		}
	}
	emit fileType(4, networkName, aNodes, totalLinks);
	return 1;
}


//Called from loadGraphML
void Parser::readGraphML(QXmlStreamReader &xml){
	qDebug()<< " Parser: readGraphML()";
	Q_ASSERT(xml.isStartElement() && xml.name() == "graph");
	//start reading the GraphML document until QXmlStreamReader reaches the end().
	while (!xml.atEnd()) {
		xml.readNext();	//read next token
		
		if (xml.isStartElement()) {		//new token (graph, node, or edge) starts here
			qDebug()<< "  readGraphML(): element starts here: "<< xml.name() ;
			if (xml.name() == "graph")	//graph definition token
				readGraphMLGraphElement(xml);
			else if (xml.name() == "key")	//key definition token
				readGraphMLKeys(xml);
			else if (xml.name() == "node")	//graph definition token
				readGraphMLNodeElement(xml);
			else if (xml.name() == "data")	//data definition token
				readGraphMLDataElement(xml);

			else if (xml.name() == "edge")	//edge definition token
				readGraphMLEdgeElement(xml);
			else
				readGraphMLUnknownElement(xml);
		}
		if (xml.isEndElement()) {		//token ends here
			qDebug()<< "  readGraphML():  element ends here: "<< xml.name() ;
				if (xml.name() == "node")	//node definition end 
					endGraphMLNodeElement(xml);
		}
	}
	
}


void Parser::readGraphMLKeys(QXmlStreamReader &xml){
	qDebug()<< "   Parser: readGraphMLKeys()";
	QString id = xml.attributes().value("id").toString();
 	qDebug()<< "    key id "<< id;
	QString what = xml.attributes().value("for").toString();
	keyFor [id] = what;
	qDebug()<< "    key for "<< what;
	QString name =xml.attributes().value("attr.name").toString();
	keyName [id] = name;
	qDebug()<< "    key attr.name "<< name;
	QString type=xml.attributes().value("attr.type").toString();
	keyType [id] = type;
	qDebug()<< "    key attr.type "<< type;

	xml.readNext();
	if (xml.name() == "default") {
		QString value=xml.readElementText();
		keyDefaultValue [id] = value;
	}
}


void Parser::readGraphMLGraphElement(QXmlStreamReader &xml){
	qDebug()<< "   Parser: readGraphMLGraphElement()";
	qDebug()<< "    edgedefault "<< xml.attributes().value("edgedefault");
	qDebug()<< "    graph id  "  << xml.attributes().value("id");	
}



void Parser::readGraphMLNodeElement(QXmlStreamReader &xml){
	qDebug()<<"   Parser: readGraphMLNodeElement()";
	QString  id = (xml.attributes().value("id")).toString();
	qDebug()<<"    node id "<<  id << " index " << aNodes;

	nodeNumber[id]=aNodes;
	QString color = initNodeColor;
	QString shape = initNodeShape;
	randX=rand()%gwWidth;
	randY=rand()%gwHeight;

	emit createNode(aNodes, initNodeSize, color, id, color, QPointF(randX,randY), shape, initShowLabels);
	
	aNodes++;
}

void Parser::endGraphMLNodeElement(QXmlStreamReader &xml){
	qDebug()<<"   Parser: endGraphMLNodeElement()";
}


void Parser::readGraphMLDataElement (QXmlStreamReader &xml){
	qDebug()<< "  Parser: readGraphMLDataElement()";
	QString id = xml.attributes().value("key").toString();
	QString value=xml.readElementText();
	keyDefaultValue [id] = value;
	
}
	
void Parser::readGraphMLEdgeElement(QXmlStreamReader &xml){
	qDebug()<< "  Parser: readGraphMLEdgeElement()";
	QString s = xml.attributes().value("source").toString();
	QString t = xml.attributes().value("target").toString();
	if ( (xml.attributes().value("directed")).toString() == "false") 
		undirected = "true";
	source = nodeNumber [s];
	target = nodeNumber [t];
	qDebug()<< "   edge source "<< s << " num "<< source;
	qDebug()<< "   edge target "<< t << " num "<< target;

	emit createEdge(source, target, 1, initLinkColor, undirected, arrows, bezier);	
}




void Parser::readGraphMLUnknownElement(QXmlStreamReader &xml) {
	qDebug()<< "Parser: readGraphMLUnknownElement()";
    Q_ASSERT(xml.isStartElement());

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isEndElement())
            break;

        if (xml.isStartElement())
            readGraphMLUnknownElement(xml);
    }
}

/**
	Tries to load a file as GML formatted network. If not it returns -1
*/
int Parser::loadGML(){
	qDebug("Parser: loadGML()");
	QFile file ( fileName );
	QString str, temp;
	int fileLine=0, start=0, end=0;
	Q_UNUSED(start);
	Q_UNUSED(end);

	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	while (!ts.atEnd() )   {
		str= ts.readLine() ;
		fileLine++;
		qDebug ()<<"Reading fileLine "<< fileLine;
		if ( fileLine == 1 ) {
			qDebug ()<<"Reading fileLine = "<< fileLine;
			if ( !str.startsWith("graph", Qt::CaseInsensitive) ) {
				qDebug() << "*** Parser:loadGML(): Not an GML-formatted file. Aborting";
				file.close();
				return -1;  
			}

		}
		if ( str.startsWith("directed",Qt::CaseInsensitive) ) { 	 //key declarations
		}
		else if ( str.startsWith("id",Qt::CaseInsensitive) ) { 	 
		}
		else if ( str.startsWith("label",Qt::CaseInsensitive) ) { 	 
		}
		else if ( str.startsWith("node",Qt::CaseInsensitive) ) { 	 //node declarations
		}
		else if ( str.startsWith("edge",Qt::CaseInsensitive) ) { 	 //edge declarations
		}


	}
	emit fileType(5, networkName, aNodes, totalLinks);
	return 1;
}

/**
	Tries to load the file as Dot (Graphviz) formatted network. If not it returns -1
*/
int Parser::loadDot(){
	qDebug("Parser: loadDotNetwork");
	int fileLine=0, j=0, aNum=-1;
	int start=0, end=0, nodeValue=1, edgeValue=1;
	QString str, temp, label, node, nodeShape,nodeLabel, fontName, fontColor, edgeShape, edgeColor, edgeLabel;
	QStringList lineElement;
	QString nodeColor="red", linkColor="black";
	QStringList labels;
	QList<QString> nodeSequence;   //holds edges
	QList<QString> nodesDiscovered; //holds nodes;
	initShowLabels=TRUE;	
	undirected=FALSE; arrows=TRUE; bezier=FALSE;
	source=0, target=0;
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	aNodes=0;
	while (!ts.atEnd() )   {
		str= ts.readLine() ;
		fileLine++;
		qDebug ()<<"Reading fileLine "<< fileLine;
		if (str.isEmpty() ) continue;
		str=str.simplified();
		str=str.trimmed();
		if ( fileLine == 1 ) {
			qDebug ()<<"Reading fileLine = " <<fileLine;
			if ( str.contains("vertices",Qt::CaseInsensitive) || (str.contains("network",Qt::CaseInsensitive) || str.contains("DL",Qt::CaseInsensitive) || str.contains("list",Qt::CaseInsensitive)) || str.startsWith("<graphml",Qt::CaseInsensitive) || str.startsWith("<?xml",Qt::CaseInsensitive)) {
				qDebug() << "*** Parser:loadDot(): Not an GraphViz -formatted file. Aborting";
				file.close();				
				return -1;
			}
			if ( str.contains("digraph", Qt::CaseInsensitive) ) {
				qDebug("This is a digraph");
				//symmetricAdjacency=FALSE; 
				lineElement=str.split(" ");
				if (lineElement[1]=="{" ) networkName="Noname";
				else networkName=lineElement[1];
				continue; 
			}
			else if ( str.contains("graph", Qt::CaseInsensitive) ) {
				qDebug("This is a graph");
				//symmetricAdjacency=TRUE; 
				lineElement=str.split(" ");
				if (lineElement[1]=="{" ) networkName="Noname";
				else networkName=lineElement[1];
				continue;
			}
			else {
				qDebug()<<" *** Parser:loadDot(): Not a GraphViz file. Abort: dot format can only start with \" (di)graph netname {\"";
				return -1;  				
			}
			
		}
		if ( str.startsWith("node",Qt::CaseInsensitive) ) { 	 //Default node properties
			qDebug("Node properties found!");
			start=str.indexOf('[');
			end=str.indexOf(']');
			temp=str.right(str.size()-end-1);
			str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug()<<"Properties start at "<< start<< " and end at "<< end;
			qDebug()<<str.toAscii();
			str=str.simplified();
			qDebug()<<str.toAscii();
			start=0;
			end=str.count();

			dotProperties(str, nodeValue, nodeLabel, nodeShape, nodeColor, fontName, fontColor );

			qDebug ("Ooola! Finished node properties - let's see if there are any nodes after that!");
			temp.remove(';');
			qDebug()<<temp.toAscii();
			temp=temp.simplified();
			qDebug()<<temp.toAscii();
			if ( temp.contains(',') )
				labels=temp.split(' ');	
			else if (temp.contains(' ') )
				labels=temp.split(' ');
			for (j=0; j<(int)labels.count(); j++) {
				qDebug()<<"node label: "<<labels[j].toAscii()<<"." ;
				if (nodesDiscovered.contains(labels[j])) {qDebug("discovered"); continue;}
				aNodes++;
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				qDebug()<<"Creating node at "<< randX<<","<< randY<<" label " << labels[j].toAscii(); 
				emit createNode(aNodes, initNodeSize, nodeColor, labels[j], nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
				aNum=aNodes;
				nodesDiscovered.push_back( labels[j]);
				qDebug()<<" Total aNodes: "<<  aNodes<< " nodesDiscovered = "<< nodesDiscovered.size();
			}
		}
		else if ( str.startsWith("edge",Qt::CaseInsensitive) ) { //Default edge properties
			qDebug("Edge properties found...");
			start=str.indexOf('[');
			end=str.indexOf(']');
			str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug()<<"Properties start at "<< start <<" and end at "<< end;
			qDebug()<<str.toAscii();
			str=str.simplified();
			qDebug()<<str.toAscii();
			start=0;
			end=str.count();
//			dotProperties(str, edgeValue, nodeLabel, nodeShape, edgeColor, fontName, fontColor );

			qDebug ("Finished the properties!");
		}
		//ti ginetai an grafei p.x. "node-1" -> "node-2"
		//ti ginetai an exeis mesa sxolia ? p.x. sto telos tis grammis //
		else if (str.contains('-',Qt::CaseInsensitive)) {  
			qDebug("Edge found...");
			end=str.indexOf('[');
			if (end!=-1) {
				temp=str.right(str.size()-end-1); //keep the properties
				temp=temp.remove(']');
				qDebug()<<"edge properties "<<temp.toAscii();
				dotProperties(temp, edgeValue, edgeLabel, edgeShape, edgeColor, fontName, fontColor );
			}
			else end=str.indexOf(';');
			//FIXME It cannot parse nodes with names containing the '-' character!!!!
			str=str.mid(0, end).remove('\"');  //keep only edges
			qDebug()<<"edges "<<str.toAscii();
			
			if (!str.contains("->",Qt::CaseInsensitive)){  //non directed = symmetric links
				nodeSequence=str.split("-");
			}
			else { 		//directed
				nodeSequence=str.split("->");
			}
			for ( QList<QString>::iterator it=nodeSequence.begin(); it!=nodeSequence.end(); it++ )  {
				node=(*it).simplified();
				if ( (aNum=nodesDiscovered.indexOf( node ) ) == -1) {
					aNodes++;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug()<<"Creating node at "<< randX <<","<< randY<<" label "<<node.toAscii(); 
					emit createNode(aNodes, initNodeSize, nodeColor, node , nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
					nodesDiscovered.push_back( node  );
					qDebug()<<" Total aNodes " << aNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
					target=aNodes;
					if (it!=nodeSequence.begin()) {
						qDebug()<<"Drawing Link between node "<< source<< " and node " <<target;
						emit createEdge(source,target, edgeValue, edgeColor, undirected, arrows, bezier);
					}
				}
				else {
					target=aNum+1;
					qDebug("Node already exists. Vector num: %i ",target);
					if (it!=nodeSequence.begin()) {
						qDebug()<<"Drawing Link between node "<<source<<" and node " << target;
						emit createEdge(source,target, edgeValue, edgeColor, undirected, arrows, bezier);
					}
				}
				source=target;
			}
			nodeSequence.clear();
			qDebug("Finished reading fileLine %i ",fileLine);
		}
		else if ( str.contains ("[",Qt::CaseInsensitive) ) { 	 //Default node properties
			qDebug("Node properties found but with no Node keyword in the beggining!");
			start=str.indexOf('[');
			end=str.indexOf(']');
			temp=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug()<<"Properties start at "<< start<< " and end at " << end;
			temp=temp.simplified();
			qDebug()<<temp.toAscii();
			dotProperties(temp, nodeValue, label, nodeShape, nodeColor, fontName, fontColor );
			qDebug ("Finished the properties!");

			if (start > 2 ) {//there is a node definition here
				node=str.left(start).remove('\"').simplified();
				qDebug()<<"node label: "<<node.toAscii()<<"." ;
				if (!nodesDiscovered.contains(node)) {
					qDebug("not discovered node"); 
					aNodes++;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug()<<"Creating node at "<<  randX << " "<< randY<< " label "<<node.toAscii(); 
					emit createNode(aNodes, initNodeSize, nodeColor, label, nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
					aNum=aNodes;
					nodesDiscovered.push_back( node);
					qDebug()<<" Total aNodes: "<<  aNodes<< " nodesDiscovered = "<< nodesDiscovered.size();
				}
				else {
					qDebug("discovered node - skipping it!");
				}
			}
		}
	}
  	file.close();
	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(3, networkName, aNodes, totalLinks);
	return 1;
}





void Parser::dotProperties(QString str, int &nValue, QString &label, QString &shape, QString &color, QString &fontName, QString &fontColor ){
	int next=0;
	QString prop, value;	
	bool ok=FALSE;
			do  {		//stops when it passes the index of ']'
				next=str.indexOf('=', 1);
				qDebug("Found next = at %i. Start is at %i", next, 1);
				prop=str.mid(0, next).simplified();	
				qDebug()<<"Prop: "<<prop.toAscii() ;
				str=str.right(str.count()-next-1).simplified();
				qDebug()<<"whatsleft: "<<str.toAscii() ;
				if ( str.indexOf('\"') == 0) {
					qDebug("found text, parsing...");
					next=str.indexOf('\"', 1);
					value=str.left(next).simplified().remove('\"');
					
					if (prop=="label") {
						qDebug()<<"Found label "<<value.toAscii();
						label=value.trimmed();
						qDebug()<<"Assigned label "<<label.toAscii();
					}
					else if (prop=="fontname"){
						qDebug()<<"Found fontname"<<value.toAscii();
						fontName=value.trimmed();
					}
					str=str.right(str.count()-next-1).simplified();
					qDebug()<<"whatsleft: "<<str.toAscii() <<".";
				}
				else {
					if (str.isEmpty()) break;
					if ( str.contains(',') )
						next=str.indexOf(',');
					else if ( str.contains(' ') )
						next=str.indexOf(' ');
					value=str.mid(0, next).simplified();
					
					qDebug()<<"Prop Value: "<<value.toAscii() ;
					if (prop=="value") {
						qDebug()<<"Found value "<<value.toAscii();
						nValue=value.trimmed().toInt(&ok, 10);
						qDebug()<<"Assigned value %i"<<nValue; 
					}
					else if (prop=="color") {
						qDebug()<<"Found color "<<value.toAscii();
						color=value.trimmed();
						qDebug()<<"Assigned node color "<<color.toAscii()<<".";
					}
					else if (prop=="fontcolor") {
						qDebug()<<"Found fontcolor "<<value.toAscii();
						fontColor=value.trimmed();
					}
					else if (prop=="shape") {
						shape=value.trimmed();
						qDebug()<<"Found node shape "<<shape.toAscii();
					}
					qDebug()<<"count"<< str.count()<<  " next "<< next;
					str=str.right(str.count()-next).simplified();
					qDebug()<<"whatsleft: "<<str.toAscii()<<".";
					if ( (next=str.indexOf('=', 1))==-1) break;
				}
			} while (!str.isEmpty());

}


/** starts the new thread calling the load* methods
*/

void Parser::run()  {
	qDebug("**** QThread/Parser: This is a thread, running!");
	if (networkName=="") networkName="Unnamed!";
	if ( loadPajek() ==1 ) {
		qDebug("Parser: this is a Pajek network");
	}
	else if (loadAdjacency()==1 ) {
		qDebug("Parser: this is an adjacency-matrix network");
	}
	else if (loadDot()==1 ) {
		qDebug("Parser: this is an GraphViz (dot) network");
	}    
	else if (loadGraphML()==1){
		qDebug("Parser: this is an GraphML network");
	}
	else if (loadGML()==1){
		qDebug("Parser: this is an GML (gml) network");
	}
	else if (loadDL()==1){
		qDebug("Parser: this is an DL formatted (.dl) network");
	}
	else{
	qDebug("**** QThread/Parser: end of routine!");
	}
}
