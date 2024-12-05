/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 3.2
 Written in Qt
 
                         parser.h  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

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
 * Used while parsing edge lists
 */
struct Actor {
    QString key;
    int value;
};


/**
 * @brief The CompareActors class
 * Implements a min-priority queue
 * Used while parsing weighted edge lists
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
 * @brief Defines a class for network file loading and parsing
 *
 * Supports GraphML, Pajek, Adjacency, Graphviz, UCINET, EdgeLists etc
 *
 */
class Parser :  public QObject {
	Q_OBJECT
public:
	
    Parser();
    ~Parser();
    void load(const QString &fileName, const QString &codecName, const int &defNodeSize,
              const QString &defNodeColor, const QString &defNodeShape, const QString &defNodeNumberColor,
              const int &defNodeNumberSize, const QString &defNodeLabelColor, const int &defNodeLabelSize ,
              const QString &defEdgeColor, const int &canvasWidth, const int &canvasHeight, const int &format,
              const QString &delim=QString(), const int &sm_mode=1, const bool &sm_has_labels=false);

    bool parseAsPajek(const QByteArray &rawData);
    bool parseAsAdjacency(const QByteArray &rawData, const QString &delimiter=",", const bool &sm_has_labels=false);
    bool parseAsDot(const QByteArray &rawData);
    bool parseAsGraphML(const QByteArray &rawData);
    bool parseAsGML(const QByteArray &rawData);
    bool parseAsDL(const QByteArray &rawData);
    bool parseAsEdgeListSimple(const QByteArray &rawData, const QString &delimiter);
    bool parseAsEdgeListWeighted(const QByteArray &rawData, const QString &delimiter);
    bool parseAsTwoModeSociomatrix(const QByteArray &rawData);

    bool readDLKeywords(QStringList &strList, int &N, int &NM, int &NR, int &NC, bool &fullmatrixFormat, bool &edgelist1Format);

    void readDotProperties(QString str, qreal &, QString &label,
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
	void readGraphMLElementDefaultValue(QXmlStreamReader &);
	void readGraphMLElementNodeGraphics (QXmlStreamReader &);
	void readGraphMLElementEdgeGraphics (QXmlStreamReader &);
    void createMissingNodeEdges();

	bool isComment(QString str);  
    void createRandomNodes(const int &fixedNum=1,const QString &label=QString(),
                           const int &newNodes=1);


signals:

    void signalAddNewRelation( const QString & relName, const bool &changeRelation=false);
    void signalSetRelation(int, const bool &updateUI=true);
    void signalCreateNode( const int &num,
                     const int &size,
                     const QString &color,
                     const QString &numColor,
                     const int &numSize,
                     const QString &label,
                     const QString &lColor,
                     const int &lSize,
                     const QPointF &p,
                     const QString &shape,
                     const QString &iconPath=QString(),
                     const bool &signalMW=false);
    void signalCreateNodeAtPosRandom(const bool &signalMW=false);
    void signalCreateNodeAtPosRandomWithLabel (const int &num,
                                         const QString &label,
                                         const bool &signalMW=false
                                         );

    void signalCreateEdge (const int &source, const int &target, const qreal &weight,
                     const QString &color, const int &edgeDirType,
                     const bool &arrows, const bool &bezier,
                     const QString &edgeLabel=QString(),
                     const bool &signalMW=false);
    void signalFileLoaded(const int &fileType,
                          const QString &fileName,
                          const QString &netName,
                          const int &totalNodes,
                          const int &totalLinks,
                          const int &edgeDirType,
                          const qint64 &elapsedTime,
                          const QString &message=QString());


    void removeDummyNode (int);
    void finished(QString);
	
protected:

private:

    bool validateAndInitialize(const QByteArray &rawData, const QString &delimiter, const bool &sm_has_labels, QStringList &nodeLabels);
    void resetCounters();
    bool doParseAdjacency(QTextStream &ts, const QString &delimiter, const QStringList &nodeLabels);
    void createNodeWithDefaults(int nodeIndex, const QString &label);
    bool createEdgesForRow(const QStringList &currentRow, int rowIndex);
    bool containsReservedKeywords(const QString &str) const;

    QHash<QString, int> nodeHash;
	QHash<QString, QString> keyFor, keyName, keyType, keyDefaultValue ;
    QHash<QString, QString> edgesMissingNodesHash;
    QStringList edgeMissingNodesList,edgeMissingNodesListData,relationsList;
	QMultiMap<int, int> firstModeMultiMap, secondModeMultiMap;
	QXmlStreamReader *xml;
    QString fileDirPath;
    QString m_textCodecName;
    QString networkName;
    QString initNodeColor, initNodeShape, initNodeCustomIcon;
    QString initNodeNumberColor, initNodeLabelColor;
    QString initEdgeColor, initEdgeLabel, delimiter;
    QString errorMessage;
    QString nodeColor, edgeColor, edgeType, nodeShape, nodeLabel, edgeLabel;
    QString nodeIconPath;
    QString nodeNumberColor, nodeLabelColor;
    QString key_id, key_value, key_name, key_what, key_type;
    QString node_id, edge_id, edge_source, edge_target, edge_weight, edge_directed;
	int gwWidth, gwHeight;
    int totalLinks, totalNodes, fileFormat, two_sm_mode, edgeDirType;
    int initNodeSize,  initNodeNumberSize, nodeNumberSize, initNodeLabelSize;
    int nodeLabelSize, source, target, nodeSize;
    qreal initEdgeWeight, edgeWeight, arrowSize;
    qreal bez_p1_x,bez_p1_y, bez_p2_x, bez_p2_y;
    bool fileLoaded, missingNode;
	bool arrows, bezier, conv_OK;
    bool bool_key, bool_node, bool_edge, fileContainsNodeColors;
    bool fileContainsNodeCoords, fileContainsLinkColors;
    bool fileContainsLinkLabels;
	double randX, randY;
};


#endif
