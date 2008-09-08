/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.46
 Written in Qt 4.4 with KDevelop   
 
                         parser.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2008 by Dimitris B. Kalamaras
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
	Tries to load the file as Pajek-formatted network. If not it returns -1
*/
int Parser::loadPajek(){
	qDebug ("Parser: loadPajek");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	QString str, label, nodeColor,linkColor, nodeShape ;
	
	QStringList lineElement;
	bool ok=FALSE, intOk=FALSE, check1=FALSE, check2=FALSE;
	bool nodes_flag=FALSE, edges_flag=FALSE, arcs_flag=FALSE;
	bool fileContainsNodeColors=FALSE, fileContainsNodesCoords=FALSE;
	bool fileContainsLinksColors=FALSE;
	bool zero_flag=FALSE;
	int  j,miss, source = -1, target=-1, weight=1, nodeNum, colorIndex=-1;
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
		if (str.isEmpty() ) continue;
		if (!edges_flag && !arcs_flag && !nodes_flag) {
			qDebug("Parser-loadPajek(): reading headlines");
			if (str.contains ("digraph", Qt::CaseInsensitive) || str.startsWith ("graph", Qt::CaseInsensitive) || str.startsWith("<?xml", Qt::CaseInsensitive) || str.startsWith("graphml", Qt::CaseInsensitive)) {  
				//this is not a pajek file. Abort
				qDebug("Parser-loadPajek(): not a pajek file. Aborting!");
				file.close();
				return -1;
			}
   			else if (str.contains( "network",Qt::CaseInsensitive) )  { //NETWORK NAME
				lineElement=str.split(QRegExp("\\s+"));	//split at one or more spaces
				qDebug()<<"Parser-loadPajek(): possible net name: "<<lineElement[1];
				if (lineElement[1]!="") //FIXME size
					networkName=lineElement[1];
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

		if ( str.contains( "edges", Qt::CaseInsensitive) ) {
		 	edges_flag=true; arcs_flag=false; 
			continue;
		}
		else if ( str.contains( "arcs", Qt::CaseInsensitive) ) { 
			arcs_flag=true; edges_flag=false;
			continue;
		}
		/** READING NODES */
		if (!edges_flag && !arcs_flag) {
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
			/** NODELABEL */
			if (lineElement.size() < 2 ){
				label=lineElement[0];
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				nodeColor=initNodeColor;
				nodeShape=initNodeShape;
			}
			else {
				label=lineElement[1];
				qDebug("node label: " + lineElement[1].toAscii());
				str.remove (1, str.lastIndexOf(label)+label.size() );	
				qDebug("cropped str: "+ str.toAscii());
				if (label.contains('"', Qt::CaseInsensitive) )
					label=label.remove('\"');
				qDebug("node label now: " + label[1].toAscii());
								
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
				}
				else { //there is no nodeColor. Use the default
					qDebug("No nodeColor");
					fileContainsNodeColors=FALSE;
					nodeColor=initNodeColor;
					
				}
				/**NODE COORDINATES */
				if (str.contains(".",Qt::CaseInsensitive)) { 
					for (register int c=0; c< lineElement.count(); c++)   {
						if ( lineElement[c].contains(".", Qt::CaseInsensitive) )  {
							randX=lineElement[c].toDouble(&check1);
							randY=lineElement[c+1].toDouble(&check2);
							if (check1 && check2)    {
								randX=randX * gwWidth;
								randY=randY * gwHeight;
								fileContainsNodesCoords=TRUE;
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
			qDebug ("Creating node numbered %i", nodeNum);
			j++;  //Controls the real number of nodes.
			//If the file misses some nodenumbers then we create dummies and delete them afterwards!
			if ( j + miss < nodeNum)  {
				qDebug ("MW There are %i nodes but this node has number %i", j, nodeNum);
				for (int i=j; i< nodeNum; i++) {
					qDebug( "Parser-loadPajek(): Creating dummy node number i = %i ", i);
					emit createNode(i,initNodeSize, nodeColor, label, lineElement[3], QPointF(randX, randY), nodeShape, initShowLabels);
					listDummiesPajek.push_back(i);  //FIXME Pajek import
					miss++;
				}
			}
			else if ( j > nodeNum ) {
				qDebug ("Error: This Pajek net declares this node with nodeNumber smaller than previous nodes. Aborting");
				return -1;	
			}
// 			parameters....: nodeNum, nodeSize, nodeColor, nodeLabel, labelColor, Point of origin, nodeShape, showLabels
			emit createNode(nodeNum,initNodeSize, nodeColor, label, nodeColor, QPointF(randX, randY), nodeShape, initShowLabels);
			initNodeColor=nodeColor; 
		} 	
		/**NODES CREATED. CREATE EDGES/ARCS NOW. */		
		else {
			if (j && j!=aNodes)  {  //if there were more or less nodes than the file declared
				qDebug("The Pajek file declares %i nodes, but I found %i nodes....", aNodes, j);
				aNodes=j;
			}
			else if (j==0) {  //if there were no nodes at all, we need to create them now.
				qDebug("The Pajek file declares %i but I didnt found any nodes. I will create them....", aNodes);
				for (register int i=j+1; i<= aNodes; i++) {
					qDebug( "Parser-loadPajek(): Creating node number i = %i ", i);
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					emit createNode(i,initNodeSize, initNodeColor, QString::number(i), "black", QPointF(randX, randY), initNodeShape, initShowLabels);
				}
				j=aNodes;
			}
			if (edges_flag && !arcs_flag)   {  /**EDGES */
				qDebug("Parser-loadPajek(): ==== Reading edges ====");
				qDebug()<<lineElement;
				source =  lineElement[0].toInt(&ok, 10);
				target = lineElement[1].toInt(&ok,10);
				if (lineElement.count()>2)
					weight =  lineElement[2].toInt(&ok,10);
				else weight=1;
				qDebug("Parser-loadPajek(): weight %i", weight);
				if (source == 0 || target == 0 ) return -1;  //  i --> (i-1)   internally
				if (lineElement.contains("c", Qt::CaseSensitive ) ) {
					qDebug("Parser-loadPajek(): file with link colours");
					fileContainsLinksColors=TRUE;
					colorIndex=lineElement.indexOf( QRegExp("[c]"), 0 )  +1;
					if (colorIndex >= lineElement.count()) linkColor=initLinkColor;
					else 	linkColor=lineElement [ colorIndex ];
					qDebug()<< " current color "<< linkColor;
 				}
				else  {
					qDebug("Parser-loadPajek(): file with no link colours");
					linkColor=initLinkColor;
				}
				undirected=true;
				arrows=true;
				bezier=false;
				qDebug("Parser-loadPajek(): Create edge between %i - %i", source, target);
				emit createEdge(source, target, weight, linkColor, undirected, arrows, bezier);
				totalLinks=totalLinks+2;

			} //end if EDGES 
			else if (!edges_flag && arcs_flag)   {  /** ARCS */
				qDebug("Parser-loadPajek(): === Reading arcs ===");
				source=  lineElement[0].toInt(&ok, 10);
				target = lineElement[1].toInt(&ok,10);
				if (lineElement.count()>2)
					weight =lineElement[2].toInt(&ok,10);
				else 
					weight=1;
				if (source == 0 || target == 0 ) return -1;   //  i --> (i-1)   internally
				if (lineElement.contains("c", Qt::CaseSensitive ) ) {
					qDebug("Parser-loadPajek(): file with link colours");
					linkColor=lineElement [ lineElement.indexOf( QRegExp("[c]"), 0 ) + 1 ];
					fileContainsLinksColors=TRUE;
				}
				else  {
					qDebug("Parser-loadPajek(): file with no link colours");
					linkColor=initLinkColor;
				}
				undirected=false;
				arrows=true;
				bezier=false;
				qDebug("Parser-loadPajek(): Creating arc source %i target %i with weight %i", source, target, weight);
				emit createEdge(source, target, weight, linkColor, undirected, arrows, bezier);
				totalLinks++;
			} //else if ARCS
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
	int i=0, j=0, weight=1, aNodes=0;
	bool intOK=FALSE;


	while ( !ts.atEnd() )   {
		str= ts.readLine() ;
		str=str.simplified();  // transforms "/t", "  ", etc to plain " ".
		if (str.isEmpty() ) continue;	
		if ( str.contains("vertices",Qt::CaseInsensitive) || (str.contains("network",Qt::CaseInsensitive) || str.contains("graph",Qt::CaseInsensitive)  || str.contains("digraph",Qt::CaseInsensitive) ||  str.contains("DL",Qt::CaseInsensitive) || str.contains("list",Qt::CaseInsensitive)) || str.contains("graphml",Qt::CaseInsensitive) || str.contains("xml",Qt::CaseInsensitive)  )
 			 return -1;    //  this is not a adjacency matrix file

		lineElement=str.split(" ");
		if (i == 0 ) {
			aNodes=lineElement.count();
			qDebug("Parser-loadPajek(): There are %i nodes in this file", aNodes);		
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
				weight=(*it1).toInt(&intOK, 10);
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
	Tries to load a file as GraphML (not GML) formatted network. If not it returns -1
*/
int Parser::loadGraphML(){
	qDebug("Parser: loadGraphML()");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );

	QString str, temp, id, tgt, name, type, nodecolor, nodelabel, nodeshape, label_visible, posx, posy ;
	int fileLine=0, start=0, end=0;
	QMap<QString, QString> key_for, key_name, key_type;
	QMap<QString, int> nodeNumber;
	bool node_flag=FALSE, edge_flag=FALSE, key_flag=FALSE, data_flag=FALSE, shapenode_flag=FALSE, check1;

	aNodes=0;

	while (!ts.atEnd() )   {
		str= ts.readLine() ;
		fileLine++;
		qDebug ("Reading fileLine %i: ", fileLine);
		qDebug(str.toAscii());
		if (str.isEmpty() ) continue;	

		if ( str.contains("<!",Qt::CaseInsensitive) ) { 	 //comments
			continue;
		}
		if ( fileLine == 1  && ! str.contains("?xml", Qt::CaseInsensitive) ) {
			qDebug ("Not GraphML. Aborting.");
			return -1;  // Abort
		}
		else if  ( fileLine == 2 && !str.contains("graphml", Qt::CaseInsensitive) ) {
			qDebug ("Not GraphML. Aborting.");
			return -1;  // Abort
		}
		
		
		if ( str.contains("<key",Qt::CaseInsensitive) ) { 	 //key declarations
			key_flag=true;
			//find id
// 			qDebug ("find ID");
			start=str.indexOf("id");
// 			qDebug("ID is at = %i ",start);
			start=str.indexOf("\"", start+1);
// 			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
			end=str.indexOf("\"", start+1);
// 			qDebug("end \"= %i ",end);
			id=str.mid(start+1, end-start-1);  //keep whatever is inside 
			qDebug("key ID: " + id.toAscii() );
			str=str.right(str.size()-end-1);
//			qDebug("remains: " + str.toAscii() );

			//find for
// 			qDebug ("find FOR");
			start=str.indexOf("for");
//			qDebug("FOR is at = %i ",start);
			start=str.indexOf("\"", start+1);
// 			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
			end=str.indexOf("\"", start+1);
// 			qDebug("end \"= %i ",end);
			tgt=str.mid(start+1, end-start-1);  //keep whatever is inside 
			key_for[ id ] = tgt;
			qDebug("key FOR: " + key_for[ id ].toAscii() );
			str=str.right(str.size()-end-1);
//			qDebug("remains: " + str.toAscii() );


			//find attr.name
			if ( !str.contains("attr.name",Qt::CaseInsensitive) ) continue; 	 //comments
// 			qDebug ("find ATTR.NAME");
			start=str.indexOf("attr.name");
//			qDebug("ATTR is at = %i ",start);
			start=str.indexOf("\"", start+1);
//			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
			end=str.indexOf("\"", start+1);
//			qDebug("end \"= %i ",end);
			name=str.mid(start+1, end-start-1);  //keep whatever is inside 
			key_name[ id ] = name;
			qDebug("key ATTR.name: " + key_name[ id ].toAscii() );
			str=str.right(str.size()-end-1);
//			qDebug("remains: " + str.toAscii() );

			//find attr.type
			if ( !str.contains("attr.type",Qt::CaseInsensitive) ) continue; 	 //comments
//			qDebug ("find ATTR.TYPE");
			start=str.indexOf("attr.type");
//			qDebug("ATTR is at = %i ",start);
			start=str.indexOf("\"", start+1);
//			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
			end=str.indexOf("\"", start+1);
//			qDebug("end \"= %i ",end);
			name=str.mid(start+1, end-start-1);  //keep whatever is inside 
			key_type[ id ] = name;
			qDebug("key ATTR.type: " + key_type[ id ].toAscii() );
			str=str.right(str.size()-end-1);
// 			qDebug("remains: " + str.toAscii() );
		}
		if ( str.contains("</key",Qt::CaseInsensitive) ) { 	 //key declarations
			key_flag=false;
		}
		if ( str.contains("<graph",Qt::CaseInsensitive) ) { 	 //key declarations
		}

		if ( str.contains("<node",Qt::CaseInsensitive) ) { 	 //start node declarations
			node_flag=true;
			qDebug("a node is here");
			aNodes++;
			if ( str.contains("id",Qt::CaseInsensitive) ) { 	 //start id declarations
				//find id
				start=str.indexOf("id");
	// 			qDebug("ID is at = %i ",start);
				start=str.indexOf("\"", start+1);
	// 			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
				end=str.indexOf("\"", start+1);
	// 			qDebug("end \"= %i ",end);
				id=str.mid(start+1, end-start-1);  //keep whatever is inside 
				qDebug("node ID: " + id.toAscii() );
				str=str.right(str.size()-end-1);
	//			qDebug("remains: " + str.toAscii() );
				nodeNumber[id]=aNodes;
			}
			if ( str.contains("/>",Qt::CaseInsensitive)  )
			node_flag = false;
		}

		if ( node_flag && str.contains("<y:ShapeNode",Qt::CaseInsensitive) ) { 	 //start shapenode declarations
			shapenode_flag=true;	
			qDebug ("node shape starts here");
			if ( str.contains("/>",Qt::CaseInsensitive)  )
				shapenode_flag= false;
		}
		if ( node_flag && str.contains("</y:ShapeNode",Qt::CaseInsensitive) ) { 	 //start shapenode declarations
			shapenode_flag=false;	 
			qDebug ("node shape ends here");
		}
		if ( shapenode_flag && str.contains("y:Geometry",Qt::CaseInsensitive) ) { 	 //start shapenode declarations
			qDebug ("node geometry ");
			if ( str.contains("x",Qt::CaseInsensitive) && str.contains("y",Qt::CaseInsensitive) )  {
				start=str.indexOf("x");
				start=str.indexOf("\"", start+1);
				end=str.indexOf("\"", start+1);
				name=str.mid(start+1, end-start-1);  //keep whatever is inside 
				posx = name;
				qDebug("x: " + posx.toAscii() );
				str=str.right(str.size()-end-1);

				start=str.indexOf("y");
				start=str.indexOf("\"", start+1);
				end=str.indexOf("\"", start+1);
				name=str.mid(start+1, end-start-1);  //keep whatever is inside 
				posy= name;
				qDebug("y: " + posy.toAscii() );
				randX=posx.toDouble(&check1);
				randY=posy.toDouble(&check1);
				qDebug("x: %f,  y: %f ", randX, randY );
				str=str.right(str.size()-end-1);

			}

		}
		if ( shapenode_flag && str.contains("y:Fill",Qt::CaseInsensitive) ) { 	 //start shapenode declarations
			qDebug ("node fill ");
			if ( str.contains("color",Qt::CaseInsensitive) )  {
				start=str.indexOf("color");
				start=str.indexOf("\"", start+1);
				end=str.indexOf("\"", start+1);
				name=str.mid(start+1, end-start-1);  //keep whatever is inside 
				nodecolor = name;
				qDebug("color: " + nodecolor.toAscii() );
				str=str.right(str.size()-end-1);
			}

		}
		if ( shapenode_flag && str.contains("y:BorderStyle",Qt::CaseInsensitive) ) { 	 //start shapenode declarations
				qDebug ("node borderstyle ");
		}
		if ( shapenode_flag && str.contains("<y:NodeLabel",Qt::CaseInsensitive) ) { 	 //start shapenode declarations
			qDebug ("nodelabel");
			//find label
			if ( str.contains("visible",Qt::CaseInsensitive) )  {
				start=str.indexOf("visible");
				start=str.indexOf("\"", start+1);
				end=str.indexOf("\"", start+1);
				name=str.mid(start+1, end-start-1);  //keep whatever is inside 
				label_visible = name;
				qDebug("visible: " + label_visible.toAscii() );
				str=str.right(str.size()-end-1);
			}
			start=str.indexOf(">", start+1);
			end=str.indexOf("<", start+1);
			name=str.mid(start+1, end-start-1);  //keep whatever is inside 
			nodelabel=name;
			qDebug("nodelabel: " + nodelabel.toAscii() );
		
	
		}
		if ( shapenode_flag && str.contains("<y:Shape",Qt::CaseInsensitive) ) { 	 //start shapenode declarations
			//find shape
			qDebug ("nodeshape");
			if ( !str.contains("type",Qt::CaseInsensitive) ) continue; 	 //comments
			start=str.indexOf("type");
			start=str.indexOf("\"", start+1);
			end=str.indexOf("\"", start+1);
			name=str.mid(start+1, end-start-1);  //keep whatever is inside 
			if (name == "rectangle") 
				nodeshape = "box";
			else
				nodeshape = name;

			qDebug("type: " + nodeshape.toAscii() );
		}
		
		if ( str.contains("</node",Qt::CaseInsensitive) ) { 	 //end node declarations
			node_flag=false;
			emit createNode(aNodes, initNodeSize, nodecolor, nodelabel, nodecolor, QPointF(randX,randY), nodeshape, initShowLabels);
			
		}

		if ( str.contains("<edge",Qt::CaseInsensitive) ) { 	 //key declarations
			edge_flag=true;
			qDebug("an edge is here");

			if ( str.contains("id",Qt::CaseInsensitive) ) { 	 //start id declarations
				//find id
				start=str.indexOf("id");
	// 			qDebug("ID is at = %i ",start);
				start=str.indexOf("\"", start+1);
	// 			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
				end=str.indexOf("\"", start+1);
	// 			qDebug("end \"= %i ",end);
				id=str.mid(start+1, end-start-1);  //keep whatever is inside 
				qDebug("edge ID: " + id.toAscii() );
				str=str.right(str.size()-end-1);
	//			qDebug("remains: " + str.toAscii() );
			}
			if ( !str.contains("source",Qt::CaseInsensitive) || !str.contains("target",Qt::CaseInsensitive) )
				continue;
			start=str.indexOf("source");
// 			qDebug("ID is at = %i ",start);
			start=str.indexOf("\"", start+1);
// 			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
			end=str.indexOf("\"", start+1);
// 			qDebug("end \"= %i ",end);
			name=str.mid(start+1, end-start-1);  //keep whatever is inside 
			source=nodeNumber[name];
			qDebug("edge source %i", source);
			str=str.right(str.size()-end-1);

			start=str.indexOf("target");
// 			qDebug("ID is at = %i ",start);
			start=str.indexOf("\"", start+1);
// 			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
			end=str.indexOf("\"", start+1);
// 			qDebug("end \"= %i ",end);
			name=str.mid(start+1, end-start-1);  //keep whatever is inside 
			target=nodeNumber[name];
			qDebug("edge target %i", target);
			str=str.right(str.size()-end-1);
			totalLinks++;

			if ( str.contains("/>",Qt::CaseInsensitive)  )
				edge_flag = false;
		}
		//if (edge_flag &&

		if ( str.contains("</edge",Qt::CaseInsensitive) ) { 	 //key declarations
			edge_flag=false;
			emit createEdge(source, target, 1, initLinkColor, undirected, arrows, bezier);

		}
		if ( str.contains("<data",Qt::CaseInsensitive) ) { 	 //start node declarations
			data_flag=true;
			if ( str.contains("key",Qt::CaseInsensitive) ) { 	 //start id declarations
				//find id
				start=str.indexOf("key");
	// 			qDebug("ID is at = %i ",start);
				start=str.indexOf("\"", start+1);
	// 			qDebug("after \" at %i remains: " + str.right(str.size()-start).toAscii(), start );
				end=str.indexOf("\"", start+1);
	// 			qDebug("end \"= %i ",end);
				id=str.mid(start+1, end-start-1);  //keep whatever is inside 
				qDebug("key: " + id.toAscii() );
				str=str.right(str.size()-end-1);
	//			qDebug("remains: " + str.toAscii() );
				start=str.indexOf(">", start+1);
				end=str.indexOf("<", start+1);
				name=str.mid(start+1, end-start-1);  //keep whatever is inside 
				if ( node_flag) {
					if (key_name[id] == "color") {
						nodecolor=name;	
						qDebug("nodecolor " + nodecolor.toAscii() );	
					}
				}
				else if (edge_flag) {

				}


			}

		}
		if ( str.contains("</data",Qt::CaseInsensitive) ) { 	 //end node declarations
			data_flag=false;
		}


	}
	emit fileType(4, networkName, aNodes, totalLinks);
	return 1;
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
		qDebug ("Reading fileLine %i. ", fileLine);
		if ( fileLine == 1 ) {
			qDebug ("Reading fileLine= %i ", fileLine);
			if ( !str.startsWith("graph", Qt::CaseInsensitive) ) 
				return -1;  // Abort
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
		qDebug ("Reading fileLine %i. ", fileLine);
		if (str.isEmpty() ) continue;
		str=str.simplified();
		str=str.trimmed();
		if ( fileLine == 1 ) {
			qDebug ("Reading fileLine= %i ", fileLine);
			if ( str.contains("vertices",Qt::CaseInsensitive) || (str.contains("network",Qt::CaseInsensitive) || str.contains("DL",Qt::CaseInsensitive) || str.contains("list",Qt::CaseInsensitive)) || str.startsWith("<graphml",Qt::CaseInsensitive) || str.startsWith("<?xml",Qt::CaseInsensitive)) 
			 return -1;    

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
			else return -1;  // Abort: dot format can only start with " (di)graph netname {"
			
		}
		if ( str.startsWith("node",Qt::CaseInsensitive) ) { 	 //Default node properties
			qDebug("Node properties found!");
			start=str.indexOf('[');
			end=str.indexOf(']');
			temp=str.right(str.size()-end-1);
			str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug("Properties start at %i and end at %i", start, end);
			qDebug(str.toAscii());
			str=str.simplified();
			qDebug(str.toAscii());
			start=0;
			end=str.count();

			dotProperties(str, nodeValue, nodeLabel, nodeShape, nodeColor, fontName, fontColor );

			qDebug ("Ooola! Finished node properties - let's see if there are any nodes after that!");
			temp.remove(';');
			qDebug(temp.toAscii());
			temp=temp.simplified();
			qDebug(temp.toAscii());
			if ( temp.contains(',') )
				labels=temp.split(' ');	
			else if (temp.contains(' ') )
				labels=temp.split(' ');
			for (j=0; j<(int)labels.count(); j++) {
				qDebug("node label: "+labels[j].toAscii()+"." );
				if (nodesDiscovered.contains(labels[j])) {qDebug("discovered"); continue;}
				aNodes++;
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				qDebug("Creating node at %f, %f, label "+labels[j].toAscii(), randX, randY); 
				emit createNode(aNodes, initNodeSize, nodeColor, labels[j], nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
				aNum=aNodes;
				nodesDiscovered.push_back( labels[j]);
				qDebug(" Total aNodes: %i, nodesDiscovered = %i",  aNodes, nodesDiscovered.size());
			}
		}
		else if ( str.startsWith("edge",Qt::CaseInsensitive) ) { //Default edge properties
			qDebug("Edge properties found...");
			start=str.indexOf('[');
			end=str.indexOf(']');
			str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug("Properties start at %i and end at %i", start, end);
			qDebug(str.toAscii());
			str=str.simplified();
			qDebug(str.toAscii());
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
				qDebug("edge properties "+temp.toAscii());
				dotProperties(temp, edgeValue, edgeLabel, edgeShape, edgeColor, fontName, fontColor );
			}
			else end=str.indexOf(';');
			//FIXME It cannot parse nodes with names containing the '-' character!!!!
			str=str.mid(0, end).remove('\"');  //keep only edges
			qDebug("edges "+str.toAscii());
			
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
					qDebug("Creating node at %f, %f, label "+node.toAscii()+".", randX, randY); 
					emit createNode(aNodes, initNodeSize, nodeColor, node , nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
					nodesDiscovered.push_back( node  );
					qDebug(" Total aNodes: %i, nodesDiscovered = %i",  aNodes, nodesDiscovered.size());
					target=aNodes;
					if (it!=nodeSequence.begin()) {
						qDebug("Drawing Link between node %i and node %i.",source,target);
						emit createEdge(source,target, edgeValue, edgeColor, undirected, arrows, bezier);
					}
				}
				else {
					target=aNum+1;
					qDebug("Node already exists. Vector num: %i ",target);
					if (it!=nodeSequence.begin()) {
						qDebug("Drawing Link between node %i and node %i.",source,target);
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
			qDebug("Properties start at %i and end at %i", start, end);
			temp=temp.simplified();
			qDebug(temp.toAscii());
			dotProperties(temp, nodeValue, label, nodeShape, nodeColor, fontName, fontColor );
			qDebug ("Finished the properties!");

			if (start > 2 ) {//there is a node definition here
				node=str.left(start).remove('\"').simplified();
				qDebug("node label: "+node.toAscii()+"." );
				if (!nodesDiscovered.contains(node)) {
					qDebug("not discovered node"); 
					aNodes++;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug("Creating node at %f, %f, label "+node.toAscii(), randX, randY); 
					emit createNode(aNodes, initNodeSize, nodeColor, label, nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
					aNum=aNodes;
					nodesDiscovered.push_back( node);
					qDebug(" Total aNodes: %i, nodesDiscovered = %i",  aNodes, nodesDiscovered.size());
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
				qDebug("Prop: "+prop.toAscii() );
				str=str.right(str.count()-next-1).simplified();
				qDebug("whatsleft: "+str.toAscii() );
				if ( str.indexOf('\"') == 0) {
					qDebug("found text, parsing...");
					next=str.indexOf('\"', 1);
					value=str.left(next).simplified().remove('\"');
					
					if (prop=="label") {
						qDebug("Found label "+value.toAscii());
						label=value.trimmed();
						qDebug("Assigned label "+label.toAscii());
					}
					else if (prop=="fontname"){
						qDebug("Found fontname"+value.toAscii());
						fontName=value.trimmed();
					}
					str=str.right(str.count()-next-1).simplified();
					qDebug("whatsleft: "+str.toAscii() +".");
				}
				else {
					if (str.isEmpty()) break;
					if ( str.contains(',') )
						next=str.indexOf(',');
					else if ( str.contains(' ') )
						next=str.indexOf(' ');
					value=str.mid(0, next).simplified();
					
					qDebug("Prop Value: "+value.toAscii() );
					if (prop=="value") {
						qDebug("Found value "+value.toAscii());
						nValue=value.trimmed().toInt(&ok, 10);
						qDebug("Assigned value %i",nValue); 
					}
					else if (prop=="color") {
						qDebug("Found color "+value.toAscii());
						color=value.trimmed();
						qDebug("Assigned node color "+color.toAscii()+".");
					}
					else if (prop=="fontcolor") {
						qDebug("Found fontcolor "+value.toAscii());
						fontColor=value.trimmed();
					}
					else if (prop=="shape") {
						shape=value.trimmed();
						qDebug("Found node shape "+shape.toAscii());
					}
					qDebug("count %i, next %i", str.count(), next);
					str=str.right(str.count()-next).simplified();
					qDebug("whatsleft: "+str.toAscii() +".");
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
	else{
	qDebug("**** QThread/Parser: end of routine!");
	}
}
