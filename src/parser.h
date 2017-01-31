/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 2.3
 Written in Qt
 
                         parser.h  -  description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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

#include <QThread>
#include <QHash>
#include <QStringList>
#include <QPointF>
#include <QObject>
#include <QMultiMap>
#include <QDebug>
class QXmlStreamReader;
class QXmlStreamAttributes;




/**
 * @brief The Actor struct
 * Used in loadEdgeListWeighed and loadEdgeListSimple
 */
struct Actor {
    QString key;
    int value;
};


/**
 * @brief The CompareActors class
 * Implements a min-priority queue
 * Used in loadEdgeListWeighed
 */
class CompareActors {
    public:
    bool operator()(Actor& t1, Actor& t2)
    {
       if (t1.value== t2.value)
            return t1.key  > t2.key ;
//       qDebug () << t1.value << " > " << t2.value << "?"
//                 << ( t1.value > t2.value ) ;
       return t1.value > t2.value;  //minimum priority
       // Returns true if t2.value smaller than t1.value
    }
};


/**
 * @brief The Parser class
 * Main class for network file parsing and loading
 * Supports GraphML, Pajek, Adjacency, Graphviz, UCINET, EdgeLists etc
 */
class Parser :  public QObject {
	Q_OBJECT
public:
	
    Parser(const QString fn, const QString codec, const int iNS,
              const QString iNC, const QString iNSh, const QString iNNC,
              const int iNNS, const QString iNLC, const int iNLS ,
              const QString iEC, const int w, const int h, const int format,
              const int sm_mode,
           const QString delim=QString::null);
    ~Parser();
    void run();
	bool loadPajek();
	bool loadAdjacency();
	bool loadDot();
	bool loadGraphML();
	bool loadGML();
	bool loadGW();
	bool loadDL();
    bool loadEdgeListSimple(const QString &delimiter);
    bool loadEdgeListWeighed(const QString &delimiter);
	bool loadTwoModeSociomatrix();

    void readDotProperties(QString str, float &, QString &label,
                       QString &shape, QString &color, QString &fontName,
                       QString &fontColor );
    bool readGraphML(QXmlStreamReader &);
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
    void createMissingNodeEdges();

	bool isComment(QString str);  
    void createRandomNodes(const int &fixedNum=1,const QString &label=QString::null,
                           const int &newNodes=1);

    void loadFileError(const QString &errorMessage);

signals:
    void addRelation( const QString & relName, const bool &changeRelation=false);
    void relationSet( int );
	void createNode( 
            const int &num, const int &size, const QString &color,
            const QString &numColor, const int &numSize,
            const QString &label, const QString &lColor, const int &lSize,
            const QPointF &p,
            const QString &shape, const bool &signalMW=false);
    void createNodeAtPosRandom(const bool &signalMW=false);
    void createNodeAtPosRandomWithLabel (const int &num,
                                         const QString &label,
                                         const bool &signalMW=false
                                         );

    void edgeCreate (const int &source, const int &target, const float &weight,
                     const QString &color, const int &edgeDirType,
                     const bool &arrows, const bool &bezier,
                     const QString &edgeLabel=QString::null,
                     const bool &signalMW=false);
    void networkFileLoaded(int fileType,
                           QString fileName,
                           QString netName,
                           int totalNodes,
                           int totalLinks,
                           bool edgeDirType,
                           const QString &message=QString::null);


	void removeDummyNode (int);
    void finished(QString);
	
protected:

private: 
    QHash<QString, int> nodeHash;
	QHash<QString, QString> keyFor, keyName, keyType, keyDefaultValue ;
    QHash<QString, QString> edgesMissingNodesHash;
    QStringList edgeMissingNodesList,edgeMissingNodesListData, relationsList;
	QMultiMap<int, int> firstModeMultiMap, secondModeMultiMap;
	QXmlStreamReader *xml;
    QString fileName, userSelectedCodecName, networkName, initNodeColor;
    QString initEdgeColor, initNodeShape, initNodeNumberColor, initNodeLabelColor;
    QString initEdgeLabel, delimiter, errorMessage;
    QString nodeColor, edgeColor, edgeType, nodeShape, nodeLabel, edgeLabel;
    QString nodeNumberColor, nodeLabelColor;
    QString key_id, key_value, key_name, key_what, key_type;
    QString node_id, edge_id, edge_source, edge_target, edge_weight, edge_directed;
	int gwWidth, gwHeight;
    int totalLinks, totalNodes, fileFormat, two_sm_mode, edgeDirType;
    int initNodeSize,  initNodeNumberSize, nodeNumberSize, initNodeLabelSize;
    int nodeLabelSize, source, target, nodeSize;
	float initEdgeWeight, edgeWeight, arrowSize;
	float bez_p1_x,bez_p1_y, bez_p2_x, bez_p2_y;
    bool missingNode;
	bool arrows, bezier, conv_OK;
    bool bool_key, bool_node, bool_edge, fileContainsNodeColors;
    bool fileContainsNodeCoords, fileContainsLinkColors;
    bool fileContainsLinkLabels;
	double randX, randY;
};


#endif
