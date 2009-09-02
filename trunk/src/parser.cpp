/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.80
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

void Parser::load(QString fn, int iNS, QString iNC, QString iNSh, QString iNNC, int iNNS, QString iNLC, int iNLS , QString iEC, int width, int height)
{
	qDebug("Parser: load()");
	initNodeSize=iNS;
	initNodeColor=iNC;
	initNodeShape=iNSh;
	initNodeNumberColor=iNNC;
	initNodeNumberSize=iNNS;
	initNodeLabelColor=iNLC;
	initNodeLabelSize=iNLS;
	
	initEdgeColor=iEC;
	
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
	qDebug ("\n\nParser: loadDL");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );

	QString str, label;
	
	int i=0, j=0, lineCounter=0, mark=0, nodeNum=0;
	edgeWeight=0;
	bool labels_flag=false, data_flag=false, intOK=false, floatOK=false;
	QStringList lineElement;
	networkName="";
	totalLinks=0;

	while ( !ts.atEnd() )   {
		str= ts.readLine();
		lineCounter++;


		if ( isComment(str) ) 
			continue;
				
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
			qDebug()<<"Creating node at "<< randX<<","<< randY;
			emit createNode(
						nodeNum, initNodeSize,initNodeColor, 
						initNodeNumberColor, initNodeNumberSize,
						label, initNodeLabelColor, initNodeLabelSize, 
						QPointF(randX, randY), 
						initNodeShape
						);
		
		}
		if ( data_flag){		//read edges
			//SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS) 
			lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);
			j=0;
			for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
				qDebug()<< (*it1).toAscii() ;
				if ( (*it1)!="0"){ //here is an non-zero edge weight...
					qDebug()<<  "Parser-loadDL(): there is an edge here";
					edgeWeight=(*it1).toFloat(&floatOK); 
					undirected=false;
					arrows=true;
					bezier=false;
					emit createEdge(i+1, j+1, edgeWeight, initEdgeColor, undirected, arrows, bezier);
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
	emit fileType(5, networkName, aNodes, totalLinks, undirected);
	qDebug() << "Parser-loadDL()";
	return 1;

}

/**
	Tries to load the file as Pajek-formatted network. If not it returns -1
*/
int Parser::loadPajek(){
	qDebug ("\n\nParser: loadPajek");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	QString str, label, temp;
	nodeColor="";
	edgeColor="";
	nodeShape="";
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
		
		if ( isComment(str)  ) 
			continue;

		lineCounter++;

		if (lineCounter==1) {
			if ( str.contains("graph",Qt::CaseInsensitive)  
				|| str.contains("digraph",Qt::CaseInsensitive) 
				|| str.contains("DL",Qt::CaseInsensitive) 
				|| str.contains("list",Qt::CaseInsensitive)
				|| str.contains("graphml",Qt::CaseInsensitive) 
				|| str.contains("xml",Qt::CaseInsensitive)  
				) {
				qDebug()<< "*** Parser:loadPajeck(): Not an Pajek-formatted file. Aborting!!";
				file.close();		
	 		 	return -1;    
			}
		}

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
					qDebug()<<"Creating node at "<< randX<<","<< randY;
					
					emit createNode(
									num, initNodeSize, nodeColor,
									initNodeNumberColor, initNodeNumberSize,
									label, lineElement[3], initNodeLabelSize,
									QPointF(randX, randY), 
									nodeShape
									);
					listDummiesPajek.push_back(num);  
					miss++;
				}
			}
			else if ( j > nodeNum ) {
				qDebug ("Error: This Pajek net declares this node with nodeNumber smaller than previous nodes. Aborting");
				return -1;	
			}
			qDebug()<<"Creating node at "<< randX<<","<< randY;

			emit createNode(
							nodeNum,initNodeSize, nodeColor, 
							initNodeNumberColor, initNodeNumberSize,
							label, initNodeLabelColor, initNodeLabelSize, 
							QPointF(randX, randY), 
							nodeShape
							);
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
					emit createNode(
									num,initNodeSize, initNodeColor,
									initNodeNumberColor, initNodeNumberSize, 
									QString::number(i), initNodeLabelColor,initNodeLabelSize, 
									 QPointF(randX, randY), 
									 initNodeShape
									 );
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
					edgeWeight  = lineElement[0].toFloat(&ok);
					source=  lineElement[1].toInt(&ok, 10);
					if (lineElement.count()>2) {
						target = lineElement[2].toInt(&ok,10);
					}
					else {
						target = lineElement[1].toInt(&ok,10);  //self link
					}
				}
				else if (lineElement.count()>2)
					edgeWeight =lineElement[2].toFloat(&ok);
				else 
					edgeWeight =1.0;

				qDebug()<<"Parser-loadPajek(): weight "<< weight;

				if (lineElement.contains("c", Qt::CaseSensitive ) ) {
					qDebug("Parser-loadPajek(): file with link colours");
					fileContainsLinksColors=TRUE;
					colorIndex=lineElement.indexOf( QRegExp("[c]"), 0 )  +1;
					if (colorIndex >= lineElement.count()) edgeColor=initEdgeColor;
					else 	edgeColor=lineElement [ colorIndex ];
					if (edgeColor.contains (".") )  edgeColor=initEdgeColor;
					qDebug()<< " current color "<< edgeColor;
 				}
				else  {
					qDebug("Parser-loadPajek(): file with no link colours");
					edgeColor=initEdgeColor;
				}
				undirected=true;
				arrows=true;
				bezier=false;
				qDebug()<< "Parser-loadPajek(): Create edge between " << source << " - "<< target;
				emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
				totalLinks=totalLinks+2;

			} //end if EDGES 
			else if (!edges_flag && arcs_flag)   {  /** ARCS */
				qDebug("Parser-loadPajek(): === Reading arcs ===");
				source=  lineElement[0].toInt(&ok, 10);
				target = lineElement[1].toInt(&ok,10);
				if (source == 0 || target == 0 ) return -1;   //  i --> (i-1)   internally
				else if (source < 0 && target >0 ) {  //weights come first...
					edgeWeight  = lineElement[0].toFloat(&ok);
					source=  lineElement[1].toInt(&ok, 10);
					if (lineElement.count()>2) {
						target = lineElement[2].toInt(&ok,10);
					}
					else {
						target = lineElement[1].toInt(&ok,10);  //self link
					}
				}
				else if (lineElement.count()>2)
					edgeWeight  =lineElement[2].toFloat(&ok);
				else 
					edgeWeight =1.0;

				if (lineElement.contains("c", Qt::CaseSensitive ) ) {
					qDebug("Parser-loadPajek(): file with link colours");
					edgeColor=lineElement.at ( lineElement.indexOf( QRegExp("[c]"), 0 ) + 1 );
					fileContainsLinksColors=TRUE;
				}
				else  {
					qDebug("Parser-loadPajek(): file with no link colours");
					edgeColor=initEdgeColor;
				}
				undirected=false;
				arrows=true;
				bezier=false;
				qDebug()<<"Parser-loadPajek(): Creating arc from node "<< source << " to node "<< target << " with weight "<< weight;
				emit createEdge(source, target, edgeWeight , edgeColor, undirected, arrows, bezier);
				totalLinks++;
			} //else if ARCS
			else if (arcslist_flag)   {  /** ARCSlist */
				qDebug("Parser-loadPajek(): === Reading arcs list===");
				if (lineElement[0].startsWith("-") ) lineElement[0].remove(0,1);
				source= lineElement[0].toInt(&ok, 10);
				fileContainsLinksColors=FALSE;
				edgeColor=initEdgeColor;
				undirected=false;
				arrows=true;
				bezier=false;
				for (register int index = 1; index < lineElement.size(); index++) {
					target = lineElement.at(index).toInt(&ok,10);					
					qDebug()<<"Parser-loadPajek(): Creating arc source "<< source << " target "<< target << " with weight "<< weight;
					emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
					totalLinks++;
				}
			} //else if ARCSLIST
			else if (matrix_flag)   {  /** matrix */
				qDebug("Parser-loadPajek(): === Reading matrix of edges===");
				i++;
				source= i;
				fileContainsLinksColors=FALSE;
				edgeColor=initEdgeColor;
				undirected=false;
				arrows=true;
				bezier=false;
				for (target = 0; target < lineElement.size(); target ++) {
					if ( lineElement.at(target) != "0" ) {
						edgeWeight  = lineElement.at(target).toFloat(&ok);					
						qDebug()<<"Parser-loadPajek(): Creating arc source "<< source << " target "<< target +1<< " with weight "<< weight;
						emit createEdge(source, target+1, edgeWeight, edgeColor, undirected, arrows, bezier);
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
	emit fileType(1, networkName, aNodes, totalLinks, undirected);
	
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
	qDebug("\n\nParser: loadAdjacency()");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	networkName="";
	QString str;
	QStringList lineElement;
	int i=0, j=0,  aNodes=0, newCount=0, lastCount=0;
	edgeWeight=1.0;
	bool intOK=FALSE;

	i=1;
	while ( i < 11 &&  !ts.atEnd() )   {
		str= ts.readLine() ;
		str=str.simplified();  			

		if ( isComment(str) ) 
			continue;

		if ( str.contains("vertices",Qt::CaseInsensitive) 
			|| str.contains("network",Qt::CaseInsensitive) 
			|| str.contains("graph",Qt::CaseInsensitive)  
			|| str.contains("digraph",Qt::CaseInsensitive) 
			|| str.contains("DL",Qt::CaseInsensitive) 
			|| str.contains("list",Qt::CaseInsensitive)
			|| str.contains("graphml",Qt::CaseInsensitive) 
			|| str.contains("xml",Qt::CaseInsensitive)  
			) {
			qDebug()<< "*** Parser:loadAdjacency(): Not an Adjacency-formatted file. Aborting!!";
			file.close();		
 		 	return -1;    
		}

		newCount = (str.split(" ")).count();
		qDebug() << str;
		qDebug() << "newCount "<<newCount << " i " << i;		
		if  ( (newCount != lastCount && i>1 ) || (newCount < i) ) {
			// line element count differ, therefore this can't be an adjacency matrix
			qDebug()<< "*** Parser:loadAdjacency(): Not an Adjacency-formatted file. Aborting!!";
			file.close();		
 		 	return -1;    
		}

		lastCount=newCount;
			
		i++;
	}
		
	ts.reset();
	ts.seek(0);
	i=0;


	while ( !ts.atEnd() )   {
		str= ts.readLine() ;
		str=str.simplified();  // transforms "/t", "  ", etc to plain " ".
			
		if ( isComment(str) ) 
			continue; 

		lineElement=str.split(" ");
		if (i == 0 ) {
			aNodes=lineElement.count();
			qDebug("Parser-loadAdjacency(): There are %i nodes in this file", aNodes);		
			for (j=0; j<aNodes; j++) {
				qDebug("Parser-loadAdjacency(): Calling createNode() for node %i", j+1);
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				qDebug()<<"Parser-loadAdjacency(): no coords. Using random "<<randX << randY;

				emit createNode( j+1,initNodeSize,  initNodeColor, 
								initNodeNumberColor, initNodeNumberSize, 				
								QString::number(j+1), initNodeLabelColor, initNodeLabelSize, 
								QPointF(randX, randY), 
								initNodeShape
								);
			}
		}
		qDebug("Parser-loadAdjacency(): Finished creating new nodes");
		if ( aNodes != (int) lineElement.count() ) return -1;	
		j=0;
		qDebug("Parser-loadAdjacency(): Starting creating links");		
		for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
			if ( (*it1)!="0"){
				qDebug("Parser-loadAdjacency(): there is a link here");
				edgeWeight =(*it1).toFloat(&intOK);
				undirected=false;
				arrows=true;
				bezier=false;
				emit createEdge(i+1, j+1, edgeWeight, initEdgeColor, undirected, arrows, bezier);
				totalLinks++;

				qDebug("Link from Node i=%i to j=%i", i+1, j+1);
				qDebug("TotalLinks= %i", totalLinks);
			}

			j++;
		}
		i++;
	}
	file.close();

	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(2, networkName, aNodes, totalLinks, undirected);
	return 1;
}






/**
	Tries to load a file as GraphML (not GML) formatted network. 
	If not it returns -1
*/
int Parser::loadGraphML(){
	qDebug("\n\nParser: loadGraphML()");
	aNodes=0;
	totalLinks=0;
	nodeNumber.clear();
	bool_key=false; bool_node=false; bool_edge=false;
	key_id = "";
	key_name = "";
	key_type = "";
	key_value = "";
	initEdgeWeight = 1;
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;

	QXmlStreamReader *xml = new QXmlStreamReader();
	
	xml->setDevice(&file);
	
	while (!xml->atEnd()) {
		xml->readNext();
		qDebug()<< " loadGraphML(): xml->token "<< xml->tokenString();
		if (xml->isStartElement()) {
			qDebug()<< " loadGraphML(): element name "<< xml->name().toString()<<" version " << xml->attributes().value("version").toString()  ;
			if (xml->name() == "graphml") {	//this is a GraphML document, call method.
				qDebug()<< " loadGraphML(): OK. NamespaceUri is "<< xml->namespaceUri().toString();
				readGraphML(*xml);				
			}
			else {	//not a GraphML doc, return -1.
				xml->raiseError(QObject::tr(" loadGraphML(): The file is not an GraphML version 1.0 file."));
				qDebug()<< "*** loadGraphML(): Error in startElement - The file is not an GraphML version 1.0 file ";
				file.close();
 				return -1;
			}
		}
		else if  ( xml->tokenString() == "Invalid" ){
			xml->raiseError(QObject::tr(" loadGraphML(): The file is not an GraphML version 1.0 file."));
			qDebug()<< "*** loadGraphML(): Error in startElement - The file is not an GraphML version 1.0 file ";
			file.close();
			return -1;
		}
	}


	emit fileType(4, networkName, aNodes, totalLinks, undirected);
	//clear our mess - remove every hash element...
	keyFor.clear();
	keyName.clear();
	keyType.clear(); 
	keyDefaultValue.clear();
	nodeNumber.clear();
	return 1;
}


/*
 * Called from loadGraphML
 * This method checks the xml token name and calls the appropriate function.
 */
void Parser::readGraphML(QXmlStreamReader &xml){
	qDebug()<< " Parser: readGraphML()";
	bool_node=false;
	bool_edge=false;
	bool_key=false;
	Q_ASSERT(xml.isStartElement() && xml.name() == "graph");
	
	while (!xml.atEnd()) { //start reading until QXmlStreamReader end().
		xml.readNext();	//read next token
	
		if (xml.isStartElement()) {	//new token (graph, node, or edge) here
			qDebug()<< "\n  readGraphML(): start of element: "<< xml.name().toString() ;
			if (xml.name() == "graph")	//graph definition token
				readGraphMLElementGraph(xml);
				
			else if (xml.name() == "key")	{//key definition token
				QXmlStreamAttributes xmlStreamAttr = xml.attributes();
				readGraphMLElementKey(  xmlStreamAttr );
			}
			else if (xml.name() == "default") //default key value token 
				readGraphMLElementDefaultValue(xml);

			else if (xml.name() == "node")	//graph definition token
				readGraphMLElementNode(xml);
				
			else if (xml.name() == "data")	//data definition token
				readGraphMLElementData(xml);
			
			else if ( xml.name() == "ShapeNode") {
				bool_node =  true;
			}			
			else if (	 ( 
							xml.name() == "Geometry" 
							|| xml.name() == "Fill"
							|| xml.name() == "BorderStyle"
							|| xml.name() == "NodeLabel"
							|| xml.name() == "Shape" 
						)
						&& 	bool_node 
					) {
				readGraphMLElementNodeGraphics(xml);
			}
			
			else if (xml.name() == "edge")	{//edge definition token
				QXmlStreamAttributes xmlStreamAttr = xml.attributes();
				readGraphMLElementEdge( xmlStreamAttr  );
			}
				
			else if ( xml.name() == "BezierEdge") {
				bool_edge =  true;
			}			

			else if (	 ( 
							xml.name() == "Path"
							|| xml.name() == "LineStyle"
							|| xml.name() == "Arrows"
							|| xml.name() == "EdgeLabel" 
						)
						&& 	bool_edge 
					) {
				readGraphMLElementEdgeGraphics(xml);
			}
			
			else
				readGraphMLElementUnknown(xml);
		}
		
		if (xml.isEndElement()) {		//token ends here
			qDebug()<< "  readGraphML():  element ends here: "<< xml.name().toString() ;
				if (xml.name() == "node")	//node definition end 
					endGraphMLElementNode(xml);
				else if (xml.name() == "edge")	//edge definition end 
					endGraphMLElementEdge(xml);
		}
	}
	
}


// this method reads a graph definition 
// called at Graph element
void Parser::readGraphMLElementGraph(QXmlStreamReader &xml){
	qDebug()<< "   Parser: readGraphMLElementGraph()";
	QXmlStreamAttributes xmlStreamAttr = xml.attributes();
	QString defaultDirection = xmlStreamAttr.value("edgedefault").toString();
	qDebug()<< "    edgedefault "<< defaultDirection;
	if (defaultDirection=="undirected"){
		undirected = true;
	}
	else {
		undirected = false;
	}
	networkName = xmlStreamAttr.value("id").toString();
	qDebug()<< "    graph id  "  << networkName; //store graph id to return it afterwards 
}



// this method is needed because the QXmlStreamReader::hasAttribute
// has been implemented in Qt 4.5. Therefore we need this ugly hack to 
// be able to compile SocNetV in all previous Qt4 version. :(
//FIXME: This will be obsolete soon
bool Parser::xmlStreamHasAttribute( QXmlStreamAttributes &xmlStreamAttr, QString str) const
{
	int size = xmlStreamAttr.size();
	for (register int  i = 0 ; i < size ; i++) {
		qDebug() << "		xmlStreamHasAttribute(): " << xmlStreamAttr.at(i).name().toString() << endl;
		if ( xmlStreamAttr.at(i).name() == str) 
			return true;
	}
	return false;	
}



// this method reads a key definition 
// called at key element
void Parser::readGraphMLElementKey ( QXmlStreamAttributes &xmlStreamAttr )
{
	qDebug()<< "   Parser: readGraphMLElementKey()";
	key_id = xmlStreamAttr.value("id").toString();
 	qDebug()<< "    key id "<< key_id;
	key_what = xmlStreamAttr.value("for").toString();
	keyFor [key_id] = key_what;
	qDebug()<< "    key for "<< key_what;

	// if (xmlStreamAttr.hasAttribute("attr.name") ) {  // to be enabled in later versions..	
	if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("attr.name") ) ) {
		key_name =xmlStreamAttr.value("attr.name").toString();
		keyName [key_id] = key_name;
		qDebug()<< "    key attr.name "<< key_name;		
	}
	//if (xmlStreamAttr.hasAttribute("attr.type") ) {
	if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("attr.type") ) ) {		
		key_type=xmlStreamAttr.value("attr.type").toString();
		keyType [key_id] = key_type;
		qDebug()<< "    key attr.type "<< key_type;
	}
	//else if (xmlStreamAttr.hasAttribute("yfiles.type") ) {
	else if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("yfiles.type") ) ) {
		key_type=xmlStreamAttr.value("yfiles.type").toString();
		keyType [key_id] = key_type;
		qDebug()<< "    key yfiles.type "<< key_type;
	}

}


// this method reads default key values 
// called at a default element (usually nested inside key element)
void Parser::readGraphMLElementDefaultValue(QXmlStreamReader &xml) {
	qDebug()<< "   Parser: readGraphMLElementDefaultValue()";

	key_value=xml.readElementText();
	keyDefaultValue [key_id] = key_value;	//key_id is already stored 
	qDebug()<< "    key default value is "<< key_value;
	if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "    this key default value "<< key_value << " is for node size";
			conv_OK=false;
			initNodeSize= key_value.toInt(&conv_OK);
			if (!conv_OK) initNodeSize = 8;  
	}	
	if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "    this key default value "<< key_value << " is for nodes shape";
			initNodeShape= key_value; 
	}
	if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "    this key default value "<< key_value << " is for nodes color";
			initNodeColor= key_value; 
	}
	if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "    this key default value "<< key_value << " is for node labels color";
			initNodeLabelColor= key_value; 
	}
	if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "    this key default value "<< key_value << " is for node labels size";
			conv_OK=false;
			initNodeLabelSize= key_value.toInt(&conv_OK);
			if (!conv_OK) initNodeLabelSize = 8;  
	}	
	if (keyName.value(key_id) == "weight" && keyFor.value(key_id) == "edge" ) {
			qDebug()<< "    this key default value "<< key_value << " is for edges weight";
			conv_OK=false;
			initEdgeWeight= key_value.toFloat(&conv_OK);
			if (!conv_OK) initEdgeWeight = 1;  
	}
	if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
			qDebug()<< "    this key default value "<< key_value << " is for edges color";
			initEdgeColor= key_value; 
	}

}



// this method reads basic node attributes and sets the nodeNumber.
// called at the start of a node element
void Parser::readGraphMLElementNode(QXmlStreamReader &xml){
	QXmlStreamAttributes xmlStreamAttr = xml.attributes();
	node_id = (xmlStreamAttr.value("id")).toString();
	aNodes++;
	qDebug()<<"   Parser: readGraphMLElementNode() node id "<<  node_id << " index " << aNodes << " added to nodeNumber map";

	nodeNumber[node_id]=aNodes;

	//copy default node attribute values. 
	//Some might change when reading element data, some will stay the same...   	
	nodeColor = initNodeColor;
	nodeShape = initNodeShape;
	nodeSize = initNodeSize;
	nodeNumberSize=initNodeNumberSize;
	nodeNumberColor=initNodeNumberColor;
	nodeLabel = node_id;
	nodeLabelSize=initNodeLabelSize;
	nodeLabelColor=initNodeLabelColor;
	bool_node = true;
	randX=rand()%gwWidth;
	randY=rand()%gwHeight;

}


// this method emits the node creation signal.
// called at the end of a node element   
void Parser::endGraphMLElementNode(QXmlStreamReader &xml){
	Q_UNUSED(xml);
	
	qDebug()<<"   Parser: endGraphMLElementNode() *** signal to create node with id "
		<< node_id << " nodenumber "<< aNodes << " coords " << randX << ", " << randY;
	emit createNode(
					aNodes, nodeSize, nodeColor,
					nodeNumberColor, nodeNumberSize,  
					nodeLabel, nodeLabelColor, nodeLabelSize, 
					QPointF(randX,randY), 
					nodeShape
					);
	bool_node = false;
	
}


// this method reads basic edge creation properties.
// called at the start of an edge element
void Parser::readGraphMLElementEdge(QXmlStreamAttributes &xmlStreamAttr){
	qDebug()<< "   Parser: readGraphMLElementEdge() id: " <<	xmlStreamAttr.value("id").toString();
	QString s = xmlStreamAttr.value("source").toString();
	QString t = xmlStreamAttr.value("target").toString();
	if ( (xmlStreamAttr.value("directed")).toString() == "false") 
		undirected = "true";
	source = nodeNumber [s];
	target = nodeNumber [t];
	edgeWeight=initEdgeWeight;
	bool_edge= true;
	qDebug()<< "    edge source "<< s << " num "<< source;
	qDebug()<< "    edge target "<< t << " num "<< target;

	
}


// this method emits the edge creation signal.
// called at the end of edge element   
void Parser::endGraphMLElementEdge(QXmlStreamReader &xml){
	Q_UNUSED(xml);
	qDebug()<<"   Parser: endGraphMLElementEdge() *** emitting signal to create edge from "<< source << " to " << target;
	//FIXME need to return edge label as well!
	emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
	totalLinks++;
	bool_edge= false;
}


/*
 * this method reads data for edges and nodes 
 * called at a data element (usually nested inside a node an edge element) 
 */ 
void Parser::readGraphMLElementData (QXmlStreamReader &xml){
	QXmlStreamAttributes xmlStreamAttr = xml.attributes();
	key_id = xmlStreamAttr.value("key").toString();
	key_value=xml.text().toString();
	qDebug()<< "   Parser: readGraphMLElementData(): key_id: " <<  key_id <<  " key_value "<< key_value;
	if (key_value.trimmed() == "") 
	{
		qDebug()<< "   Parser: readGraphMLElementData(): text: " << key_value;
		xml.readNext();
		key_value=xml.text().toString();
		qDebug()<< "   Parser: readGraphMLElementData(): text: " << key_value; 
		if (  key_value.trimmed() != "" ) { //if there's simple text after the StartElement,
				qDebug()<< "   Parser: readGraphMLElementData(): key_id " << key_id 
						<< " value is simple text " <<key_value ;
		}
		else {  //no text, probably more tags. Return...
			qDebug()<< "   Parser: readGraphMLElementData(): key_id " << key_id 
							<< " for " <<keyFor.value(key_id) << ". More elements nested here, continuing";
			return;  
		}
		
	}
	
	if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node color: "<< key_value << " for this node";
			nodeColor= key_value; 
	}
	else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "node" ){
		 	qDebug()<< "     Data found. Node label: "<< key_value << " for this node";
		 	nodeLabel = key_value;
	}
	else if (keyName.value(key_id) == "x_coordinate" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node x: "<< key_value << " for this node";
			conv_OK=false;
			randX= key_value.toFloat( &conv_OK ) ;
			if (!conv_OK)
				randX = 0; 
			else 
				randX=randX * gwWidth;
			qDebug()<< "     Using: "<< randX;
	}
	else if (keyName.value(key_id) == "y_coordinate" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node y: "<< key_value << " for this node"; 
			conv_OK=false;
			randY= key_value.toFloat( &conv_OK );
			if (!conv_OK)
				randY = 0;  
			else 
				randY=randY * gwHeight;	
			qDebug()<< "     Using: "<< randY;
	}
	else if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node size: "<< key_value << " for this node"; 
			conv_OK=false;
			nodeSize= key_value.toInt ( &conv_OK );
			if (!conv_OK)
				nodeSize = initNodeSize;  
			qDebug()<< "     Using: "<< nodeSize;
	}
	else if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node label size: "<< key_value << " for this node"; 
			conv_OK=false;
			nodeLabelSize= key_value.toInt ( &conv_OK );
			if (!conv_OK)
				nodeLabelSize = initNodeLabelSize;  
			qDebug()<< "     Using: "<< nodeSize;
	}
	else if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node" ){
		 	qDebug()<< "     Data found. Node label Color: "<< key_value << " for this node";
		 	nodeLabelColor = key_value;
	}
	else if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node shape: "<< key_value << " for this node";
			nodeShape= key_value; 
	}	
	else if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
			qDebug()<< "     Data found. Edge color: "<< key_value << " for this edge";
			edgeColor= key_value; 
	}
	else if ( ( keyName.value(key_id) == "value" ||  keyName.value(key_id) == "weight" ) && keyFor.value(key_id) == "edge" ) {
			conv_OK=false;
			edgeWeight= key_value.toFloat( &conv_OK );
			if (!conv_OK) 
				edgeWeight = 1.0;  	
 			qDebug()<< "     Data found. Edge value: "<< key_value << " Using "<< edgeWeight << " for this edge";       
	}
	else if ( keyName.value(key_id) == "size of arrow"  && keyFor.value(key_id) == "edge" ) {
			conv_OK=false;
			float temp = key_value.toFloat( &conv_OK );
			if (!conv_OK) arrowSize = 1;
			else  arrowSize = temp;
			qDebug()<< "     Data found. Edge arrow size: "<< key_value << " Using  "<< arrowSize << " for this edge";
	}


	
}



/**
 * 	Reads node graphics data and properties: label, color, shape, size, coordinates, etc.
 */
void Parser::readGraphMLElementNodeGraphics(QXmlStreamReader &xml) {
	qDebug()<< "       Parser: readGraphMLElementNodeGraphics(): element name "<< xml.name().toString();
	float tempX =-1, tempY=-1, temp=-1;
	QString color;
	QXmlStreamAttributes xmlStreamAttr = xml.attributes();
	
	if ( xml.name() == "Geometry" ) {

			if ( xmlStreamHasAttribute ( xmlStreamAttr, "x") ) {
				conv_OK=false;
				tempX = xml.attributes().value("x").toString().toFloat (&conv_OK) ;
				if (conv_OK) 
					randX = tempX;	
			}
			if ( xmlStreamHasAttribute ( xmlStreamAttr, "y") ) {
				conv_OK=false;
				tempY = xml.attributes().value("y").toString().toFloat (&conv_OK) ;
				if (conv_OK)
					randY = tempY;
			}
			qDebug()<< "        Node Coordinates: " << tempX << " " << tempY << " Using coordinates" << randX<< " "<<randY;
			if (xmlStreamHasAttribute ( xmlStreamAttr, "width") ) {
				conv_OK=false;
				temp = xmlStreamAttr.value("width").toString().toFloat (&conv_OK) ;
				if (conv_OK)
					nodeSize = temp;
				qDebug()<< "        Node Size: " << temp<< " Using nodesize" << nodeSize;
			}
			if (xmlStreamHasAttribute ( xmlStreamAttr, "shape") ) {
				nodeShape = xmlStreamAttr.value("shape").toString();
				qDebug()<< "        Node Shape: " << nodeShape;
			}

	}
	else if (xml.name() == "Fill" ){
		if ( xmlStreamHasAttribute ( xmlStreamAttr, "color") ) {
			nodeColor= xmlStreamAttr.value("color").toString();	
			qDebug()<< "        Node color: " << nodeColor;
		}
		
	}
	else if ( xml.name() == "BorderStyle" ) {
		
		
	}
	else if (xml.name() == "NodeLabel" ) {
		key_value=xml.readElementText();  //see if there's simple text after the StartElement
		if (!xml.hasError()) {
			qDebug()<< "         Node Label "<< key_value;		
			nodeLabel = key_value;
		}
		else {
			qDebug()<< "         Can't read Node Label. There must be more elements nested here, continuing";  
		}
	}
	else if (xml.name() == "Shape" ) {
		if ( xmlStreamHasAttribute ( xmlStreamAttr, "type") ) {
			nodeShape= xmlStreamAttr.value("type").toString();	
			qDebug()<< "        Node shape: " << nodeShape;
		}
	
	}
		 

}

void Parser::readGraphMLElementEdgeGraphics(QXmlStreamReader &xml) {
	qDebug()<< "       Parser: readGraphMLElementEdgeGraphics() element name "<< xml.name().toString();

	float tempX =-1, tempY=-1, temp=-1;
	QString color, tempString;
	QXmlStreamAttributes xmlStreamAttr = xml.attributes();
	
	if ( xml.name() == "Path" ) {
			//if ( xmlStreamAttr.hasAttribute("sx") ) {
			if ( xmlStreamHasAttribute ( xmlStreamAttr, "sx") ) {
				conv_OK=false;
				tempX = xmlStreamAttr.value("sx").toString().toFloat (&conv_OK) ;
				if (conv_OK) 
					bez_p1_x = tempX;
				else bez_p1_x = 0 ;
			}
			//if ( xmlStreamAttr.hasAttribute("sy") ) {
			if ( xmlStreamHasAttribute ( xmlStreamAttr, "sy") ) {
				conv_OK=false;
				tempY = xmlStreamAttr.value("sy").toString().toFloat (&conv_OK) ;
				if (conv_OK)
					bez_p1_y = tempY;
				else bez_p1_y = 0 ;
			}
			//if ( xmlStreamAttr.hasAttribute("tx") ) {
			if ( xmlStreamHasAttribute ( xmlStreamAttr, "tx") ) {
				conv_OK=false;
				tempX = xmlStreamAttr.value("tx").toString().toFloat (&conv_OK) ;
				if (conv_OK) 
					bez_p2_x = tempX;
				else bez_p2_x = 0 ;
			}
			//if ( xmlStreamAttr.hasAttribute("ty") ) {
			if ( xmlStreamHasAttribute ( xmlStreamAttr, "ty") ) {
				conv_OK=false;
				tempY = xmlStreamAttr.value("ty").toString().toFloat (&conv_OK) ;
				if (conv_OK)
					bez_p2_y = tempY;
				else bez_p2_y = 0 ;
			}
			qDebug()<< "        Edge Path control points: " << bez_p1_x << " " << bez_p1_y << " " << bez_p2_x << " " << bez_p2_y;
	}
	else if (xml.name() == "LineStyle" ){
		if ( xmlStreamHasAttribute ( xmlStreamAttr, "color") ) {
			edgeColor= xmlStreamAttr.value("color").toString();	
			qDebug()<< "        Edge color: " << edgeColor;
		}
		if ( xmlStreamHasAttribute ( xmlStreamAttr, "type") ) {
			edgeType= xmlStreamAttr.value("type").toString();	
			qDebug()<< "        Edge type: " << edgeType;
		}
		if ( xmlStreamHasAttribute ( xmlStreamAttr, "width") ) {
			temp = xmlStreamAttr.value("width").toString().toFloat (&conv_OK) ;
			if (conv_OK)
				edgeWeight = temp;
			else 
				edgeWeight=1.0;
			qDebug()<< "        Edge width: " << edgeWeight;
		}
		
	}
	else if ( xml.name() == "Arrows" ) {
		if ( xmlStreamHasAttribute ( xmlStreamAttr, "source") ) {
			tempString = xmlStreamAttr.value("source").toString();	
			qDebug()<< "        Edge source arrow type: " << tempString;
		}
		if ( xmlStreamHasAttribute ( xmlStreamAttr, "target") ) {
			tempString = xmlStreamAttr.value("target").toString();	
			qDebug()<< "        Edge target arrow type: " << tempString;
		}

	
		
	}
	else if (xml.name() == "EdgeLabel" ) {
		key_value=xml.readElementText();  //see if there's simple text after the StartElement
		if (!xml.hasError()) {
			qDebug()<< "         Edge Label "<< key_value;		
			//probably there's more than simple text after StartElement
			edgeLabel = key_value;
		}
		else {
			qDebug()<< "         Can't read Edge Label. More elements nested ? Continuing with blank edge label....";
			edgeLabel = "" ;  
		}
	}
		 


}


void Parser::readGraphMLElementUnknown(QXmlStreamReader &xml) {
	qDebug()<< "Parser: readGraphMLElementUnknown()";
    Q_ASSERT(xml.isStartElement());
	qDebug()<< "   "<< xml.name().toString() ;
}





/**
	Tries to load a file as GML formatted network. If not it returns -1
*/
int Parser::loadGML(){
	qDebug("\n\nParser: loadGML()");
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
	emit fileType(6, networkName, aNodes, totalLinks, undirected);
	qDebug() << "Parser-loadGML()";
	return 1;
}


/**
	Tries to load the file as Dot (Graphviz) formatted network. If not it returns -1
*/
int Parser::loadDot(){
	qDebug("\n\nParser: loadDotNetwork");
	int fileLine=0, aNum=-1;
	int start=0, end=0, next=0;
	QString label, node, nodeLabel, fontName, fontColor, edgeShape, edgeColor, edgeLabel, networkLabel;
	QString str, temp, prop, value ;
	QStringList lineElement;
	nodeColor="red"; 
	edgeColor="black";
	nodeShape="";
	edgeWeight=1.0;
	float nodeValue=1.0;
	QStringList labels;
	QList<QString> nodeSequence;   //holds edges
	QList<QString> nodesDiscovered; //holds nodes;
		
	undirected=false; arrows=TRUE; bezier=FALSE;
	source=0, target=0;
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	aNodes=0;
	while (!ts.atEnd() )   {
		str= ts.readLine() ;
		str=str.simplified();
		str=str.trimmed();

		if ( isComment (str) ) 
			continue;

		fileLine++;
		qDebug ()<<"Reading fileLine "<< fileLine;

		if ( fileLine == 1 ) {
			if ( str.contains("vertices",Qt::CaseInsensitive) 	//Pajek
				|| str.contains("network",Qt::CaseInsensitive)	//Pajek?
				|| str.contains("[",Qt::CaseInsensitive)    	// GML
				|| str.contains("DL",Qt::CaseInsensitive) 		//DL format
				|| str.contains("list",Qt::CaseInsensitive)		//list 
				|| str.startsWith("<graphml",Qt::CaseInsensitive)  // GraphML
				|| str.startsWith("<?xml",Qt::CaseInsensitive)
				) {
				qDebug() << "*** Parser:loadDot(): Not an GraphViz -formatted file. Aborting";
				file.close();				
				return -1;
			}

			if ( str.contains("digraph", Qt::CaseInsensitive) ) {
				lineElement=str.split(" ");
				if (lineElement[1]=="{" ) networkName="Noname";
				else networkName=lineElement[1];
				qDebug() << "This is a DOT DIGRAPH named " << networkName;
				continue; 
			}
			else if ( str.contains("graph", Qt::CaseInsensitive) ) {
				lineElement=str.split(" ");
				if (lineElement[1]=="{" ) networkName="Noname";
				else networkName=lineElement[1];
				qDebug() << "This is a DOT GRAPH named " << networkName;
				continue;
			}
			else {
				qDebug()<<" *** Parser:loadDot(): Not a GraphViz file. Abort: dot format can only start with \" (di)graph netname {\"";
				return -1;  				
			}
		}

		if ( str.startsWith("label",Qt::CaseInsensitive)
			|| str.startsWith("mincross",Qt::CaseInsensitive)
			|| str.startsWith("ratio",Qt::CaseInsensitive) 
			) { 	 //Default network properties
			next=str.indexOf('=', 1);
			qDebug("Found next = at %i. Start is at %i", next, 1);
			prop=str.mid(0, next).simplified();	
			qDebug()<<"Prop: "<<prop.toAscii() ;
			value=str.right(str.count()-next-1).simplified();
			qDebug() << "Value "<< value;
			if ( prop == "label" ){
				networkLabel= value;
			}
			else if ( prop == "ratio" ){
				
			}
			else if ( prop == "mincross" ){
				
			}

		}
		else if ( str.startsWith("node",Qt::CaseInsensitive) ) { 	 //Default node properties
			qDebug() << "* Node properties found...";
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
			dotProperties(str, nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor );
			qDebug ("* Finished NODE PROPERTIES");
		}
		else if ( str.startsWith("edge",Qt::CaseInsensitive) ) { //Default edge properties
			qDebug("* Edge properties found...");
			start=str.indexOf('[');
			end=str.indexOf(']');
			str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug()<<"Properties start at "<< start <<" and end at "<< end;
			qDebug()<<str.toAscii();
			str=str.simplified();
			qDebug()<<str.toAscii();
			start=0;
			end=str.count();
			qDebug ("* Finished EDGE PROPERTIES!");
		}
		//ti ginetai an exeis mesa sxolia ? p.x. sto telos tis grammis //
		else if ( !str.startsWith('[', Qt::CaseInsensitive) 
					&& !str.contains("--",Qt::CaseInsensitive)
					&& !str.contains("->",Qt::CaseInsensitive) 
					) {
				qDebug()<< "* A node definition must be here ..." << str;
				end=str.indexOf('['); 
				if (end!=-1) {
					temp=str.right(str.size()-end-1); //keep the properties
					temp=temp.remove(']');
					temp=temp.remove(';');
					node=str.left(end-1);
					node=node.remove('\"');
					qDebug()<<"node named "<<node.toAscii();
					qDebug()<<"node properties "<<temp.toAscii();
					nodeLabel=node;  //Will change only if label exists in dotProperties
					dotProperties(temp, nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor );
					if (nodeLabel=="") nodeLabel=node;
					aNodes++;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug()<<" *** Creating node "<< aNodes 
							<< " at "<< randX <<","<< randY
							<<" label "<<node.toAscii() 
							<< " colored "<< initNodeColor
							<< "initNodeSize " << initNodeSize
							<< "initNodeNumberColor " <<initNodeNumberColor
							<< "initNodeNumberSize " << initNodeNumberSize
							<< "initNodeLabelColor " << initNodeLabelColor
							<< "nodeShape" <<  initNodeShape; 
					emit createNode(
									aNodes, initNodeSize, initNodeColor,
									initNodeNumberColor, initNodeNumberSize,  
									nodeLabel , initNodeLabelColor, initNodeLabelSize,
									QPointF(randX,randY), 
									initNodeShape
									);
					nodesDiscovered.push_back( node  );			// Note that we push the numbered nodelabel whereas we create the node with its file specified node label.  
					qDebug()<<" * Total aNodes " << aNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
					target=aNodes;
					
				}
				else 
					end=str.indexOf(';');
				qDebug ("* Finished node!");
		}
		else if (str.contains('-',Qt::CaseInsensitive)) {  
			qDebug("* Edge definition found ...");
			end=str.indexOf('[');
			if (end!=-1) {
				temp=str.right(str.size()-end-1); //keep the properties
				temp=temp.remove(']');
				temp=temp.remove(';');
				qDebug()<<"edge properties "<<temp.toAscii();
				dotProperties(temp, edgeWeight, edgeLabel, edgeShape, edgeColor, fontName, fontColor );
			}
			else 
				end=str.indexOf(';');
				
			//FIXME Cannot parse nodes named with '-' character
			str=str.mid(0, end).remove('\"');  //keep only edges

			qDebug()<<"edges "<<str.toAscii();
			
			if (!str.contains("->",Qt::CaseInsensitive) ){  //non directed = symmetric links
				if ( str.contains("--",Qt::CaseInsensitive) ) 
						nodeSequence=str.split("--");
				else 
						nodeSequence=str.split("-");
			}
			else { 											//is directed
				nodeSequence=str.split("->");
			}
			//Create all nodes defined in nodeSequence
			for ( QList<QString>::iterator it=nodeSequence.begin(); it!=nodeSequence.end(); it++ )  {
				node=(*it).simplified();
				qDebug () << " nodeSequence node "<< node;
				if ( (aNum=nodesDiscovered.indexOf( node ) ) == -1) {
					aNodes++;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug()<<" *** Creating node "<< aNodes 
							<< " at "<< randX <<","<< randY
							<<" label "<<node.toAscii() 
							<< " colored "<< nodeColor
							<< "initNodeSize " << initNodeSize
							<< "initNodeNumberColor " <<initNodeNumberColor
							<< "initNodeNumberSize " << initNodeNumberSize
							<< "initNodeLabelColor " << initNodeLabelColor
							<< "nodeShape" <<  initNodeShape; 
					emit createNode(
									aNodes, initNodeSize, nodeColor,
									initNodeNumberColor, initNodeNumberSize,  
									node , initNodeLabelColor, initNodeLabelSize,
									QPointF(randX,randY), 
									initNodeShape
									);
					nodesDiscovered.push_back( node  );
					qDebug()<<" * Total aNodes " << aNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
					target=aNodes;
					if (it!=nodeSequence.begin()) {
						qDebug()<<"-- Drawing Link between node "<< source<< " and node " <<target;
						emit createEdge(source,target, edgeWeight, edgeColor, undirected, arrows, bezier);
					}
				}
				else {
					target=aNum+1;
					qDebug("# Node already exists. Vector num: %i ",target);
					if (it!=nodeSequence.begin()) {
						qDebug()<<"-- Drawing Link between node "<<source<<" and node " << target;
						emit createEdge(source,target, edgeWeight , edgeColor, undirected, arrows, bezier);
					}
				}
				source=target;
			}
			nodeSequence.clear();
			qDebug("Finished reading fileLine %i ",fileLine);
		}
		else if ( str.contains ("[",Qt::CaseInsensitive) ) { 	 //Default node properties - no node keyword
			qDebug("* Node properties found but with no Node keyword in the beginning!");
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
					qDebug()<<"***  Creating node at "<<  randX << " "<< randY<< " label "<<node.toAscii() << " colored "<< nodeColor; 
					emit createNode(
									aNodes, initNodeSize, nodeColor,
									initNodeNumberColor, initNodeNumberSize,  
									label, initNodeLabelColor, initNodeLabelSize, 
									QPointF(randX,randY),
									nodeShape
									);
					aNum=aNodes;
					nodesDiscovered.push_back( node);
					qDebug()<<" Total aNodes: "<<  aNodes<< " nodesDiscovered = "<< nodesDiscovered.size();
				}
				else {
					qDebug("discovered node - skipping it!");
				}
			}
		}
		else {
			qDebug() <<  "  Redudant data: "<< str.toAscii();
		}
	}
  	file.close();
	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(3, networkName, aNodes, totalLinks, undirected);
	return 1;
}





void Parser::dotProperties(QString str, float &nValue, QString &label, QString &shape, QString &color, QString &fontName, QString &fontColor ){
	int next=0;
	QString prop, value;
	
	//FIXME Implement a qstringlist here splitted from str in ,	
	bool ok=FALSE;
			do  {		//stops when it passes the index of ']'
				next=str.indexOf('=', 1);
				qDebug("Found next = at %i. Start is at %i", next, 1);
				prop=str.mid(0, next).simplified();	
				qDebug()<<"Prop: "<<prop.toAscii() ;
				str=str.right(str.count()-next-1).simplified();
				if (str.startsWith(",")) 
					str=str.right(str.count()-1).simplified();

				qDebug()<<"whatsleft: "<<str.toAscii() ;

					
				if ( str.indexOf('\"') == 0) {
					qDebug("found text, parsing...");
					next=str.indexOf('\"', 1);
					value=str.left(next).simplified().remove('\"');
					
					if (prop=="label") {
						label=value.trimmed();
						qDebug()<<"Found label "<<value.toAscii() <<". Assigned label "<<label.toAscii();
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
					if ( str.contains(',') ){
						next=str.indexOf(',');
						value=str.mid(0, next).simplified();
					}

					else if ( str.contains(' ') ){
						next=str.indexOf(' ');
						value=str.mid(0, next).simplified();
					}
					else		{
						value=str;
					}
					
					qDebug()<<"Prop Value: "<<value.toAscii() ;
					if (prop=="value") {
						qDebug()<<"Found value "<<value.toAscii();
						nValue=value.trimmed().toFloat(&ok);
						if ( ok ) 
							qDebug()<<"Assigned value "<<nValue;
						else 
							 qDebug()<<"Error in value conversion ";
					}
					else if (prop=="color") {
						color=value.trimmed();
						qDebug()<<"Found color "<<value.toAscii()  <<". Assigned color "<<color.toAscii()<<".";
					}
					else if (prop=="fillcolor") {
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
					str=str.right(str.count()-next-1).simplified();
					qDebug()<<"whatsleft: "<<str.toAscii()<<".";
					if ( (next=str.indexOf('=', 1))==-1) break;
				}
			} while (!str.isEmpty());

}






int Parser::loadList(){
	qDebug() << "Parser: loadList()";
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) 
		return -1;
	QTextStream ts( &file );
	networkName="";
	QString str;
	QStringList lineElement;
	int i=0, j=0, num=0, source=0, target=0, newCount=0, lastCount=0, maxNodeCreated=0;
	
	bool lineHasEqualAmountElements=true, intOK=false;
	
	edgeWeight=1.0;

	while ( i < 10 &&  !ts.atEnd() )   {
		str= ts.readLine() ;
		str=str.simplified();  			

		if ( isComment(str) ) 
			continue;

		newCount = (str.split(" ")).count();
		
		if (newCount != lastCount && i>0 ) {
			lineHasEqualAmountElements=false;	
			//element count differ, i.e each line has a different number edges
			// 1 2 6
			// 2 3 4 5
			// 3 1 2 4 6
		}
		
		lastCount=newCount;
			
		i++;
	}
	
	if (lastCount == 3 && lineHasEqualAmountElements ){
		emit askWhatIsTheThirdElement();
	}

	if ( lineHasEqualAmountElements ) {
		qDebug() << "Parser: loadList()" << " line Has Equal Amount of Elements";		
	}
	else {
		qDebug() << "Parser: loadList()" << " line With Different Amount of Elements";
	}
		
	ts.reset();
	ts.seek(0);
	i=0;
	maxNodeCreated = 0;
	
	while ( !ts.atEnd() )   {
		str= ts.readLine() ;
		qDebug()<< " str " << str;
		str=str.simplified();  
			
		if ( isComment(str) ) 
			continue; 
			
		if ( str.contains("vertices",Qt::CaseInsensitive) 
			|| str.contains("network",Qt::CaseInsensitive) 
			|| str.contains("graph",Qt::CaseInsensitive)  
			|| str.contains("digraph",Qt::CaseInsensitive) 
			|| str.contains("DL",Qt::CaseInsensitive) 
			|| str.contains("list",Qt::CaseInsensitive)
			|| str.contains("graphml",Qt::CaseInsensitive) 
			|| str.contains("xml",Qt::CaseInsensitive)  
			) {
			qDebug()<< "*** Parser:loadList(): Not an Adjacency-formatted file. Aborting!!";
			file.close();		
 		 	return -1;    
		}

		lineElement=str.split(" ");

		if ( !lineHasEqualAmountElements ) {
			qDebug () << " Parser::loadList() - file lines have different amount of elements...";
			i=0;
			for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
				num = (*it1).toInt(&intOK); 
				if ( !intOK ||  num == 0 ) {
					qDebug()<< "Error! Stumbled upon a zero or an error occured during a string conversion to integer...";
					file.close();
					return -1;
				}

				if (i==0){
					source = num;
					qDebug() << "	source node: " << source;
				}
				else {
					target = num;
					qDebug() << "	target node: " << target;
				}


				if (maxNodeCreated < num ) {
					for ( j = maxNodeCreated ; j != num ; j++ ) {
						randX=rand()%gwWidth;
						randY=rand()%gwHeight;
						qDebug()<<"		random coords "<<randX << " "<< randY;
						emit createNode( j+1, initNodeSize,  initNodeColor, 
								initNodeNumberColor, initNodeNumberSize, 				
								QString::number(j+1), initNodeLabelColor, initNodeLabelSize, 
								QPointF(randX, randY), 
								initNodeShape
								);
					}
					maxNodeCreated = num ;
				}

				if ( i != 0) {
					qDebug("	there is a link here");
					undirected=false;
					arrows=true;
					bezier=false;
					emit createEdge(source, target, initEdgeWeight, initEdgeColor, undirected, arrows, bezier);
					totalLinks++;
		
					qDebug("	link from Node i=%i to j=%i . TotalLinks= %i ", source, target, totalLinks);
				}
				i++;
			}
		}
		else if ( lineHasEqualAmountElements ) {
			qDebug () << "Parser::loadList() - file lines have equal amount of elements...";			
			source =  (lineElement[0]).toInt(&intOK);
			target =  (lineElement[1]).toInt(&intOK);
			qDebug() << "	source node " << source;
			qDebug() << "	target node " << target;

			if  ( lastCount == 3 ){
				edgeWeight=(lineElement[2]).toDouble(&intOK);
				if (!intOK)	
					edgeWeight=1.0;
				qDebug () << "	list file declares edge weight: " << edgeWeight;
			}
			else if ( lastCount == 2 ) {
				edgeWeight=1.0;
				qDebug () << "	list file NOT declaring edge weight. Setting default: " << edgeWeight;
			}
			else {
				qDebug () << "Error. This is not a source-target-weight file. Aborting. ";
				file.close();
				return -1;
			}

			if (maxNodeCreated < source ) {
					for ( j = maxNodeCreated ; j != source ; j++ ) {
						qDebug()<< "	source node " << source << "is smaller than maxNodeCreated - we need to create node "<< j+1;
						randX=rand()%gwWidth;
						randY=rand()%gwHeight;
						qDebug()<<"	using random coords "<<randX << " "<< randY;
						emit createNode( j+1, initNodeSize,  initNodeColor, 
								initNodeNumberColor, initNodeNumberSize, 				
								QString::number(j+1), initNodeLabelColor, initNodeLabelSize, 
								QPointF(randX, randY), 
								initNodeShape
								);
					}
					maxNodeCreated = source ;
			}

			if (maxNodeCreated < target ) {
					for ( j = maxNodeCreated ; j != target; j++ ) {
						qDebug()<< "	target node " << target << "	is smaller than maxNodeCreated - creating node "<< j+1;
						randX=rand()%gwWidth;
						randY=rand()%gwHeight;
						qDebug()<<"	using random coords "<<randX << " "<< randY;
						emit createNode( j+1, initNodeSize,  initNodeColor, 
								initNodeNumberColor, initNodeNumberSize, 				
								QString::number(j+1), initNodeLabelColor, initNodeLabelSize, 
								QPointF(randX, randY), 
								initNodeShape
								);
					}
					maxNodeCreated = target ;
			}

			qDebug("Parser-loadList(): Creating link now... ");
			undirected=false;
			arrows=true;
			bezier=false;
			emit createEdge(source, target, edgeWeight, initEdgeColor, undirected, arrows, bezier);
			totalLinks++;
			qDebug("	link from node i=%i to j=%i . TotalLinks= %i ", source, target, totalLinks);
		}

	} //end ts.stream while here
	file.close();

	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(7, networkName, aNodes, totalLinks, undirected);
	qDebug() << "Parser-loadList() ending and returning...";
	return 1;
	
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
		qDebug("Parser: this is a GraphViz (dot) network");
	}    
	else if (loadGraphML()==1){
		qDebug("Parser: this is a GraphML network");
	}
	else if (loadGML()==1){
		qDebug("Parser: this is a GML (gml) network");
	}
	else if (loadDL()==1){
		qDebug("Parser: this is a DL formatted (.dl) network");
	}
	else if (loadList()==1){
		qDebug("Parser: this is an list formatted (.list) network");
	}
	else{
	qDebug("**** QThread/Parser: end of routine!");
	}
}



//Returns TRUE if QString str is a comment inside the network file. 
bool Parser::isComment(QString str){
	if ( str.startsWith("#", Qt::CaseInsensitive) 
			|| str.startsWith("/*", Qt::CaseInsensitive)
			|| str.startsWith("%", Qt::CaseInsensitive)
			|| str.startsWith("/*", Qt::CaseInsensitive)
			|| str.startsWith("//", Qt::CaseInsensitive)
			|| str.isEmpty()
			) {
			qDebug () << " Parser: a comment or an empty line was found. Skipping..."; 
			return true;
		}
		return false;

}