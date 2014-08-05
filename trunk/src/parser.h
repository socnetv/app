/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 1.2
 Written in Qt
 
                         parser.h  -  description
                             -------------------
    copyright            : (C) 2005-2014 by Dimitris B. Kalamaras
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

#ifndef PARSER_H
#define PARSER_H

using namespace std;

#include <QThread>
#include <QPointF>
#include <QMutex>
#include <QObject>
#include <QtXml>	
#include <QMultiMap>

/** 	
	Main class for network file parsing and loading
	Currently, it supports Pajek, Adjacency, Graphviz, GraphML
*/
class Parser :  public QThread {
	Q_OBJECT
public:
	
	bool load(QString fn, int iNS, QString iNC, QString iNSh, QString iNNC,
		 int iNNS, QString iNLC, int iNLS , QString iEC, int w, int h, int format, int sm_mode);
	bool loadPajek();
	bool loadAdjacency();
	bool loadDot();
	bool loadGraphML();
	bool loadGML();
	bool loadGW();
	bool loadDL();
	bool loadSimpleList();
	bool loadWeighedList();
	bool loadTwoModeSociomatrix();

	void dotProperties(QString str, float &, QString &label, QString &shape, QString &color, QString &fontName, QString &fontColor );
	void readGraphML (QXmlStreamReader &);
	void readGraphMLElementGraph(QXmlStreamReader &);
	void readGraphMLElementNode (QXmlStreamReader &);
	void endGraphMLElementNode (QXmlStreamReader &);
	void readGraphMLElementEdge (QXmlStreamAttributes &);
	void endGraphMLElementEdge (QXmlStreamReader &);
	void readGraphMLElementData (QXmlStreamReader &);
	void readGraphMLElementUnknown (QXmlStreamReader &);
	void readGraphMLElementKey (QXmlStreamAttributes &);
	bool xmlStreamHasAttribute( QXmlStreamAttributes &, QString ) const ;
	void readGraphMLElementDefaultValue(QXmlStreamReader &);
	void readGraphMLElementNodeGraphics (QXmlStreamReader &);
	void readGraphMLElementEdgeGraphics (QXmlStreamReader &);
	
	bool isComment(QString str);  
signals:
	void createNode( 
			int num, int size, QString color,
			QString numColor, int numSize,
			QString label, QString lColor, int lSize,
			QPointF p,
			QString shape, bool signalMW);

	void createEdge (int, int, float, QString, int, bool, bool);
	void fileType(int, QString, int, int, bool);
	void removeDummyNode (int);
	
protected:
	void run();
private: 
	QMutex mutex;
	QHash<QString, int> nodeNumber;
	QHash<QString, QString> keyFor, keyName, keyType, keyDefaultValue ;
	QMultiMap<int, int> firstModeMultiMap, secondModeMultiMap;
	QXmlStreamReader *xml;
	QString fileName, networkName, initNodeColor, initEdgeColor, initNodeShape, initNodeNumberColor, initNodeLabelColor;
	QString nodeColor, edgeColor, edgeType, nodeShape, nodeLabel, edgeLabel, nodeNumberColor, nodeLabelColor;
	int gwWidth, gwHeight;
	int totalLinks, aNodes, fileFormat, two_sm_mode, undirected;
	int initNodeSize,  initNodeNumberSize, nodeNumberSize, initNodeLabelSize, nodeLabelSize, source, target, nodeSize;
	float initEdgeWeight, edgeWeight, arrowSize;
	float bez_p1_x,bez_p1_y, bez_p2_x, bez_p2_y;
	  
	bool arrows, bezier, conv_OK;
	bool bool_key, bool_node, bool_edge;
	
	QString key_id, key_value, key_name, key_what, key_type;
	QString node_id, edge_id, edge_source, edge_target; 
	double randX, randY;
};


#endif
