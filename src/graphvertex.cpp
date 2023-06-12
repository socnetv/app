/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 3.1-dev
 Written in Qt
 
                         graphvertex.cpp  -  description
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

#include "graphvertex.h"

#include <QtDebug>		//used for qDebug messages

#include "graph.h"
#include "graphicsnode.h"


GraphVertex::GraphVertex(Graph* parentGraph,
                         const int &name,
                         const int &val,
                         const int &relation,
                         const int &size,
                         const QString &color,
                         const QString &numColor,
                         const int &numSize,
                         const QString &label,
                         const QString &labelColor,
                         const int &labelSize,
                         const QPointF &p,
                         const QString &shape,
                         const QString &iconPath,
                         const int &edgesEstimate): m_graph (parentGraph)
{ 
    qDebug() << "vertex:"<< name << "initializing...edgesEstimate:" << edgesEstimate;

    m_number=name;
	m_value=val;
	m_size=size;
	m_color=color;
	m_numberColor=numColor;
	m_numberSize=numSize;
	m_label=label;
	m_labelColor=labelColor;
	m_labelSize=labelSize;
	m_shape=shape;
    m_iconPath=iconPath;
	m_x=p.x();
	m_y=p.y();
    // Use the given edges estimate to allocate memory
    // to prevent reallocations and memory fragmentation.
    if (edgesEstimate > 0) {
        //FIXME m_outLinkColors list need update when we remove vertices/edges
        //m_outLinkColors.reserve(edgesEstimate);
        m_outEdgeLabels.reserve(edgesEstimate);
        m_outEdges.reserve(edgesEstimate);
        m_inEdges.reserve(edgesEstimate);
        m_neighborhoodList.reserve(edgesEstimate);
    }

    m_outEdgesCounter = 0;
    m_inEdgesCounter = 0;
    m_outDegree = 0;
    m_inDegree = 0;
    m_localDegree = 0;
    m_Eccentricity = 0;
    m_distanceSum = 0;
    m_DC=0; m_SDC=0; m_DP=0; m_SDP=0; m_CC=0; m_SCC=0; m_BC=0; m_SBC=0;
    m_SC=0; m_SSC=0; m_IRCC=0; m_SIRCC=0;
    m_CLC=0; m_hasCLC=false;
    m_curRelation=relation;
    m_enabled = true;

    // Connect signal for edge visibility directly to the parent signal!
    connect( this, &GraphVertex::signalSetEdgeVisibility,
             m_graph, &Graph::signalSetEdgeVisibility);

}


/**
 * @brief constructor with default values
 * @param name
 */
GraphVertex::GraphVertex(const int &name) {
    qDebug() << "name"<<  name << "initializing with default values";
    m_number=name;
	m_value=1;
	m_size=9;
	m_color="black";
	m_label="";
	m_labelColor="black";
	m_shape="circle";
    m_outEdgesCounter=0;
    m_inEdgesCounter=0;
    m_Eccentricity=0;
    m_DC=0; m_SDC=0; m_DP=0; m_SDP=0; m_CC=0; m_SCC=0; m_BC=0; m_SBC=0;
    m_IRCC=0; m_SIRCC=0; m_SC=0; m_SSC=0;
    m_curRelation=0;
}


/**
 * @brief Returns the vertex number
 * @return
 */
int GraphVertex::number() const {
    return m_number;
}

/**
 * @brief Sets the vertex number
 * @param number
 */
void GraphVertex::setNumber(const int &number) {
    m_number=number;
}

/**
 * @brief Toggles the status of the vertex
 * @param flag
 */
void GraphVertex::setEnabled(const bool &status) {
    m_enabled=status;
}

/**
 * @brief Returns true if the vertex is enabled
 * @return bool
 */
bool GraphVertex::isEnabled() const {
    return m_enabled;
}

/**
 * @brief Sets the size of the vertex
 * @param size
 */
void GraphVertex::setSize(const int &size ) {
    m_size=size;
}

/**
 * @brief Returns the size of the vertex
 * @return
 */
int GraphVertex::size() const {
    return m_size;
}

/**
 * @brief Sets the shape of the vertex
 * @param shape
 * @param iconPath
 */
void GraphVertex::setShape(const QString &shape, const QString &iconPath) {
    m_shape=shape;
    m_iconPath=iconPath;
}

/**
 * @brief Returns the shape of the vertex
 * @return
 */
QString GraphVertex::shape() const {
    return m_shape;
}

/**
 * @brief Returns the path of the vertex shape
 * @return
 */
QString GraphVertex::shapeIconPath() {
    return m_iconPath;
}

/**
 * @brief Sets the vertex color
 * @param color
 */
void GraphVertex::setColor(const QString &color) {
    m_color=color;
}

/**
 * @brief Returns the vertex color
 * @return QString
 */
QString GraphVertex::color() const {
    return m_color;
}

/**
 * @brief Returns the vertex color in pajek format
 */
QString GraphVertex::colorToPajek(){
    if (m_color.startsWith("#")) {
        return  ("RGB"+m_color.right( m_color.size()-1 )).toUpper()  ;
    }
    return m_color;
}

/**
 * @brief Sets the color of the vertex number
 * @param color
 */
void GraphVertex::setNumberColor (const QString &color) {
    m_numberColor = color;
}

/**
 * @brief Returns the color of the vertex number
 * @return
 */
QString GraphVertex::numberColor() const {
    return m_numberColor;
}

/**
 * @brief Sets the size of the vertex number
 * @param size
 */
void GraphVertex::setNumberSize (const int &size) {
    m_numberSize=size;
}

/**
 * @brief Returns the size of the vertex number
 * @return
 */
int GraphVertex::numberSize() const {
    return m_numberSize;
}

/**
 * @brief Sets the distance (in pixels) of the vertex number from the vertex
 * @param distance
 */
void GraphVertex::setNumberDistance (const int &distance) {
    m_numberDistance=distance;
}

/**
 * @brief Returns the distance (in pixels) of the vertex number from the vertex
 * @return
 */
int GraphVertex::numberDistance() const {
    return m_numberDistance;
}

/**
 * @brief Sets the label of the vertex
 * @param label
 */
void GraphVertex::setLabel (const QString &label) {
    m_label=label;
}

/**
 * @brief Returns the vertex label
 * @return
 */
QString GraphVertex::label() const {
    return m_label;
}

/**
 * @brief Sets the color of the vertex label
 * @param labelColor
 */
void GraphVertex::setLabelColor (const QString &labelColor) {
    m_labelColor=labelColor;
}

/**
 * @brief Returns the color of the vertex label
 * @return
 */
QString GraphVertex::labelColor() const {
    return m_labelColor;
}

/**
 * @brief Sets the size of the vertex label
 * @param size
 */
void GraphVertex::setLabelSize(const int &size) {
    m_labelSize=size;
}

/**
 * @brief Returns the size of the vertex label
 * @return
 */
int GraphVertex::labelSize() const {
    return m_labelSize;
}

/**
 * @brief Sets the distance (in pixels) of the label from the vertex
 * @param distance
 */
void GraphVertex::setLabelDistance (const int &distance) {
    m_labelDistance=distance;
}

/**
 * @brief Returns the distance (in pixels) of the label from the vertex
 * @return
 */
int GraphVertex::labelDistance() const {
    return m_labelDistance;
}

/**
 * @brief Sets the horizontal position (in pixels) of the vertex
 * @param x
 */
void GraphVertex::setX(const qreal &x) {
    m_x=x;
}

/**
 * @brief Returns the horizontal position (in pixels) of the vertex
 * @return
 */
qreal GraphVertex::x() const {
    return m_x;
}

/**
 * @brief Sets the vertical position (in pixels) of the vertex
 * @param y
 */
void GraphVertex::setY(const qreal &y) {
    m_y=y;
}

/**
 * @brief Returns the vertical position (in pixels) of the vertex
 * @return
 */
qreal GraphVertex::y() const {
    return m_y;
}

/**
 * @brief Sets the point where the vertex is positioned
 * @param p
 */
void GraphVertex::setPos (QPointF &p) {
    m_x=p.x();
    m_y=p.y();
}


/**
 * @brief Returns the point where the vertex is positioned
 * @return
 */
QPointF GraphVertex::pos () const {
    return QPointF ( x(), y() );
}


/**
 * @brief Sets the x coordinate of the displacement vector
 * @param x
 */
void GraphVertex::set_dispX (qreal x) { m_disp.rx() = x ; }

/**
 * @brief Sets the y coordinate of the displacement vector
 * @param y
 */
void GraphVertex::set_dispY (qreal y) { m_disp.ry() = y ; }



/**
 * @brief Returns displacement vector
 * @return
 */
QPointF& GraphVertex::disp() { return m_disp; }



/**
* @brief Changes the current relation of this vertex
*
* @param newRel
*/
void GraphVertex::setRelation(int newRel) {
//    qDebug() << "vertex" << number() << "current rel:" << m_curRelation << "new rel:" << newRel;
    // first make false all edges of current relation
    setEnabledEdgesByRelation(m_curRelation, false);
    // then make true all edges of new relation
    setEnabledEdgesByRelation(newRel, true);
    // update current relation
    m_curRelation=newRel;
}



/**
 * @brief Adds an outbound edge to vertex v2 with weight w
 *
 * @param target
 * @param weight
 */
void GraphVertex::addOutEdge (const int &v2, const qreal &weight, const QString &color, const QString &label) {
//    qDebug() << "vertex" << number() << "adding new outbound edge"<< "->"<< v2
//             << "weight"<< weight<< "relation" << m_curRelation;
    // do not use [] operator - silently creates an item if key do not exist
    m_outEdges.insert(
                v2, pair_i_fb(m_curRelation, pair_f_b(weight, true) ) );
    setOutLinkColor(v2, color);
    setOutEdgeLabel(v2, label);
}



/**
 * @brief Checks if the vertex has an enabled outbound edge to the given vertex. Returns the edge weight or 0.
 *
 * If allRelations is true, then all relations are checked
 *
 * @param v2
 * @param allRelations
 * @return qreal
 */
qreal GraphVertex::hasEdgeTo(const int &v2, const bool &allRelations){
    qreal m_weight=0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.constFind(v2);
    while (it1 != m_outEdges.constEnd() && it1.key() == v2 ) {
        if (!allRelations) {
            if ( it1.value().first == m_curRelation  ) {
                edgeStatus=it1.value().second.second;
                if ( edgeStatus == true) {
                    m_weight=it1.value().second.first;
                }
                return m_weight;
            }
        }
        else {
                m_weight=it1.value().second.first;
                return m_weight;
        }
        ++it1;
    }

    return m_weight;
}



/**
 * @brief Removes the outbound edge to vertex v2
 *
 * @param v2
 */
void GraphVertex::removeOutEdge (const int v2) {

//    qDebug() << "vertex" << number() << "removing outEdge to" << v2;

    if (outEdgesCount() == 0) {
        return;
    }

    H_edges::const_iterator it1=m_outEdges.constFind(v2);
    while (it1 != m_outEdges.constEnd() && it1.key() == v2 ) {
        if ( it1.value().first == m_curRelation ) {
//            qDebug() << " *** vertex " << m_number << " connected to "
//                     << it1.key() << " relation " << it1.value().first
//                     << " weight " << it1.value().second.first
//                     << " enabled ? " << it1.value().second.second
//                     << " Erasing outEdge from m_outEdges ";
            m_outEdges.erase(it1);
            break;
        }
        ++it1;

    }

}


/**
 * @brief Sets the status of an outbound edge to the given target vertex
 *
 * @param target
 * @param status
 */
void GraphVertex::setOutEdgeEnabled (const int &target, bool status){
//    qDebug() << "vertex" << number() << "setting outEdge to" << target << "new status" << status;
    int linkTarget=0;
    qreal weight =0;
    int relation = 0;
    H_edges::iterator it1;
    for ( it1 = m_outEdges.begin(); it1 != m_outEdges.end(); ++ it1) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            linkTarget=it1.key();
            if ( linkTarget == target ) {
                weight = it1.value().second.first;
//                qDebug() << " *** vertex " << m_number << " connected to "
//                         << linkTarget << " relation " << relation
//                         << " weight " << weight
//                         << " status " << it1.value().second.second;
                it1.value() = pair_i_fb(m_curRelation, pair_f_b(weight, status) );
                emit signalSetEdgeVisibility (m_curRelation, m_number, target, status );
                break;
            }
        }
    }
}


/**
 * @brief Sets the weight of the outbound edge to the given vertex
 *
 * @param target
 * @param weight
 */
void GraphVertex::setOutEdgeWeight(const int &target, const qreal &weight){
//    qDebug() << "vertex" << number() << "changing weight of outEdge to" << target << "new weight" << weight;
    H_edges::const_iterator it1=m_outEdges.constFind(target);
    // Find the current edge, remove it and add an updated one.
    while (it1 != m_outEdges.constEnd() ) {
        if ( it1.key() == target && it1.value().first == m_curRelation ) {
            m_outEdges.erase(it1);
            break;
        }
        ++it1;
    }
    // Insert the updated edge
    m_outEdges.insert(
                target, pair_i_fb(m_curRelation, pair_f_b(weight, true) ) );
}


/**
 * @brief Sets the color of the outbound edge to the given vertex
 * @param v2
 * @param color
 */
void GraphVertex::setOutLinkColor(const int &v2, const QString &color) {
    m_outLinkColors[v2]=color;
}


/**
 * @brief Returns the color of the outbound edge to the given vertex
 * @param v2
 * @return
 */
QString GraphVertex::outLinkColor(const int &v2) {
    return ( m_outLinkColors.contains(v2) ) ? m_outLinkColors.value(v2) : "black";
}

/**
 * @brief Sets the label of the outbound edge to the given vertex
 * @param v2
 * @param label
 */
void GraphVertex::setOutEdgeLabel(const int &v2, const QString &label) {
    m_outEdgeLabels[v2]=label;
}

/**
 * @brief Returns the label of the outbound edge to the given vertex
 * @param v2
 * @return
 */
QString GraphVertex::outEdgeLabel(const int &v2) const {
    return ( m_outEdgeLabels.contains(v2) ) ? m_outEdgeLabels.value(v2) : QString();
}


/**
 * @brief Adds an inbound edge from vertex v1
 *
 * @param source
 * @param weight
 */
void GraphVertex::addInEdge (const int &v1, const qreal &weight) {
//    qDebug() << "vertex" << number() << "adding new inbound edge"<< "<-"<< v1
//             << "weight"<< weight<< "relation" << m_curRelation;
    m_inEdges.insert(
                v1, pair_i_fb (m_curRelation, pair_f_b(weight, true) ) );
}



/**
 * @brief Checks if the vertex has an enabled inbound edge from v2 and returns the edge weight or 0.
 *
 * If allRelations is true, then all relations are checked
 *
 * @param v2
 * @return
 */
qreal GraphVertex::hasEdgeFrom(const int &v2, const bool &allRelations){
    qreal m_weight=0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_inEdges.constFind(v2);
    while (it1 != m_inEdges.constEnd() && it1.key() == v2) {
        if (!allRelations) {
            if ( it1.value().first == m_curRelation  ) {
                edgeStatus=it1.value().second.second;
                if ( edgeStatus == true) {
                    m_weight=it1.value().second.first;
                    return m_weight;
                }
                return m_weight;
            }
        }
        else {
                m_weight=it1.value().second.first;
                return m_weight;
        }
        ++it1;
    }

    return m_weight;
}



/**
 * @brief Removes the inbound edge from vertex v2
 *
 * @param v2
 */
void GraphVertex::removeInEdge(const int v2){
//    qDebug() << "vertex" << number() << "removing inEdge from" << v2;

    if (inEdgesCount()==0) {
        return;
    }

    H_edges::const_iterator it=m_inEdges.constFind(v2);
    while (it != m_inEdges.constEnd() ) {
        if ( it.key() == v2 && it.value().first == m_curRelation ) {
//            qDebug() << " *** vertex " << m_number << " connected from  "
//                     << it.key() << " relation " << it.value().first
//                     << " weight " << it.value().second.first
//                     << " enabled ? " << it.value().second.second
//                     << " Erasing inEdge from m_inEdges ";
            m_inEdges.erase(it);
            break;
        }
        ++it;
    }

}






/**
 * @brief Sets the status of an inbound edge from the given source vertex
 *
 * @param source
 * @param status
 */
void GraphVertex::setInEdgeEnabled (const int &source, bool status){
//    qDebug() << "vertex" << number() << ", toggling status of inEdge:" << number() << "<-" << source << "new status:" << status;
    int linkTarget=0;
    qreal weight =0;
    int relation = 0;
    H_edges::iterator it1;
    for ( it1 = m_inEdges.begin(); it1 != m_inEdges.end(); ++ it1) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            linkTarget=it1.key();
            if ( linkTarget == source ) {
                weight = it1.value().second.first;
//                qDebug() << " *** vertex " << m_number << " connected to "
//                         << linkTarget << " relation " << relation
//                         << " weight " << weight
//                         << " status " << it1.value().second.second;
                it1.value() = pair_i_fb(m_curRelation, pair_f_b(weight, status) );
                emit signalSetEdgeVisibility (m_curRelation, source, m_number, status );
                break;
            }
        }
    }
}


/**
 * @brief Sets the weight of the inbound edge from the given vertex
 *
 * @param source
 * @param weight
 */
void GraphVertex::setInEdgeWeight(const int &source, const qreal &weight){
//    qDebug() << "vertex" << number() << "changing weight of inEdge from" << source << "new weight" << weight;
    H_edges::const_iterator it1=m_inEdges.constFind(source);
    // Find the current edge, remove it and add an updated one.
    while (it1 != m_inEdges.constEnd() ) {
        if ( it1.key() == source && it1.value().first == m_curRelation ) {
            m_inEdges.erase(it1);
            break;
        }
        ++it1;
    }
    // Insert the updated edge
    m_inEdges.insert(
                source, pair_i_fb(m_curRelation, pair_f_b(weight, true) ) );
}




/**
 * @brief Computes and returns the number of active outbound arcs (outEdges) for the current relation
 *
 * @return int
 */
int GraphVertex::outEdgesCount() {
    m_outEdgesCounter = 0;
    int relation=0;
    bool edgeStatus = false;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_outEdgesCounter++;
            }
        }
        ++it1;
    }
    return m_outEdgesCounter;
}



/**
 * @brief Returns the number of active outbound arcs. Avoid using it alone.
 *
 * WARNING: You need to compute m_outEdgesCounter before calling this method
 *
 * @return int
 */
int GraphVertex::outEdgesCountConst() const {
    return m_outEdgesCounter;
}



/**
 * @brief Returns the number of active inbound arcs to this vertex for the current relation
 *
 * @return int
 */
int GraphVertex::inEdgesCount() {
    m_inEdgesCounter = 0;
    int relation=0;
    bool edgeStatus = false;
    H_edges::const_iterator it1=m_inEdges.constBegin();
    while (it1 != m_inEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_inEdgesCounter++;
            }
        }
        ++it1;
    }
    return m_inEdgesCounter;
}


/**
 * @brief Returns the number of active inbound arcs
 *
 * WARNING: Needs to have inEdges called before the call to this method
 *
 * @return int
 */
int GraphVertex::inEdgesCountConst() const {
    return m_inEdgesCounter;
}


/**
 * @brief Returns true if the vertex has at least one outEdge
 * @return bool
 */
bool GraphVertex::isOutLinked() {
    return (outEdgesCount() > 0) ? true:false;
}


/**
 * @brief Returns true if there is an outEdge from this vertex
 * @return
 */
bool GraphVertex::isInLinked() {
    return  (inEdgesCount() > 0) ? true:false;
}


/**
 * @brief Toggles this vertex as isolated or not.
 * @param isolated
 */
void GraphVertex::setIsolated(bool isolated) {
    m_isolated = isolated;
}


/**
 * @brief Returns true if the vertex is isolated (no inbound our outbound edges)
 * @return bool
 */
bool GraphVertex::isIsolated() {
    return !(isOutLinked() || isInLinked()) ;
}



/**
 * @brief Returns a qhash of all enabled outEdges, in the active relation or all relations if allRelations is true.
 *
 * @param allRelations
 *
 * @return QHash<int,qreal>*
 */
QHash<int,qreal> GraphVertex::outEdgesEnabledHash(const bool &allRelations){
//    qDebug() << "vertex " << number();
    QHash<int,qreal> enabledOutEdges;
    qreal m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if (!allRelations) {
            if ( relation == m_curRelation ) {
                edgeStatus=it1.value().second.second;
                if ( edgeStatus == true) {
                    m_weight=it1.value().second.first;
                    enabledOutEdges.insert(it1.key(), m_weight);
                }
            }
        }
        else {
            if ( !enabledOutEdges.contains(it1.key() )) {
                edgeStatus=it1.value().second.second;
                if ( edgeStatus == true) {
                    m_weight=it1.value().second.first;
                    enabledOutEdges.insert(it1.key(), m_weight);
                }
            }
        }
        ++it1;
    }
    return enabledOutEdges;
}


/**
 * @brief Returns a qhash of all edges to neighbors in all relations
 *
 * @return
 */
QHash<int, qreal>* GraphVertex::outEdgesAllRelationsUniqueHash() {
    QHash<int,qreal> *outEdgesAll = new QHash<int,qreal>;
    qreal m_weight=0;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        if ( !outEdgesAll->contains(it1.key() )) {
                m_weight=it1.value().second.first;
                outEdgesAll->insert(it1.key(), m_weight);
        }
        ++it1;
    }
//    qDebug() << "vertex" << number() << "outEdges count:"<< outEdgesAll->size();
    return outEdgesAll;

}

/**
 * @brief Returns a qhash of all reciprocal edges to neighbors in the active relation
 *
 * @return  QHash<int,qreal>*
 */
QHash<int, qreal> GraphVertex::reciprocalEdgesHash(){
    m_reciprocalEdges.clear();
    qreal m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                if (this->hasEdgeFrom (it1.key()) == m_weight ) {
                    m_reciprocalEdges.insert(it1.key(), m_weight);
                }
            }
        }
        ++it1;
    }

//    qDebug() << "vertex" << number() << "reciprocalEdges count:" << m_reciprocalEdges.size();

    return m_reciprocalEdges;
}



/**
 * @brief Returns a list of all neighbors mutually connected to this vertex in the active relation.
 *
 * The returned list does not include the vertex itself, even if it self-connected.
 * Same as calling GraphVertex::reciprocalEdgesHash().keys() which returns a QList of int keys,
 * where each key is a vertex reciprocally connected to this one.
 *
 * @return  QList<int>
 */
QList<int> GraphVertex::neighborhoodList(){

    m_neighborhoodList.clear();
    qreal m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                if ( this->number() != it1.key() && this->hasEdgeFrom (it1.key()) == m_weight ) {
                    m_neighborhoodList << it1.key();
                }
            }
        }
        ++it1;
    }
    return m_neighborhoodList;
}




/**
 * @brief Returns a qhash of all enabled inEdges in the active relation
 *
 * @return  QHash<int,qreal>*
 */
QHash<int,qreal>* GraphVertex::inEdgesEnabledHash() {
    QHash<int,qreal> *enabledInEdges = new QHash<int,qreal>;
    qreal m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_inEdges.constBegin();
    while (it1 != m_inEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                enabledInEdges->insert(it1.key(), m_weight);
            }
        }
        ++it1;
    }
//    qDebug() << "vertex" << number() << "enabled inEdges count:"<< enabledInEdges->size();
    return enabledInEdges;
}





/**
 * @brief Returns the outDegree (the sum of all enabled outEdges weights) of this vertex
 *
 * @return int
 */
int GraphVertex::degreeOut() {
//    qDebug() << "vertex" << number();
    m_outDegree=0;
    qreal m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                m_outDegree += m_weight;
            }
        }
        ++it1;
    }
    return m_outDegree;
}

/**
 * @brief Returns the outDegree. Avoid using it alone.
 *
 * @return int
 */
int GraphVertex::outDegreeConst() {
    return m_outDegree;
}


/**
 * @brief Returns the indegree (the sum of all enabled inEdges weights) of this vertex
 *
 * @return int
 */
int GraphVertex::degreeIn() {
//    qDebug() << "vertex" << number();
    m_inDegree=0;
    qreal m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_inEdges.constBegin();
    while (it1 != m_inEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                m_inDegree += m_weight;
            }
        }
        ++it1;
    }

    return m_inDegree;
}


/**
 * @brief Returns the indegree. Avoid using it alone.
 *
 * @return int
 */
int GraphVertex::inDegreeConst() {
    return m_inDegree;
}



/**
 * @brief Returns the localDegree of the vertex.
 *
 * The localDegree is the degreeOut + degreeIn minus the edges counted twice.
 *
 * @return
 */
int GraphVertex::localDegree(){  int v2=0;
    int relation = 0;
    bool edgeStatus=false;
    m_localDegree = (degreeOut() + degreeIn() );

    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                v2=it1.key();
                if (this->hasEdgeFrom (v2) ) m_localDegree--;
            }
        }
        ++it1;
    }

//    qDebug() << "vertex" << number()  << "localDegree:" << m_localDegree;
	return m_localDegree;
}








/**
 * @brief Changes the status of all unilateral (non-reciprocal) outbound edges, in current relation
 *
 * @param status
 */
void GraphVertex::setEnabledUnilateralEdges(const bool &status){
//    qDebug() << "vertex:" << number() << "setting unilateral edges of relation" << relation << "to" << status;
    int target=0;
    qreal weight=0;
    H_edges::iterator it;
    for ( it = m_outEdges.begin(); it != m_outEdges.end(); ++it) {
        if ( it.value().first == m_curRelation ) {
            target=it.key();
            weight = it.value().second.first;
            if (hasEdgeFrom(target)==0) {
//                qDebug() << "vertex:" << number() << "Changing the status of unilateral outbound edge to" << target << "new status" << status << "and emitting signal to Graph....";
                it.value() = pair_i_fb(m_curRelation, pair_f_b(weight, status) );
                emit signalSetEdgeVisibility (m_curRelation, m_number, target, status );
            }
        }
    }
}



/**
 * @brief Changes the status of all edges in the given relation
 *
 * @param relation
 * @param status
 */
void GraphVertex::setEnabledEdgesByRelation(const int relation, const bool status ){
//    qDebug() << "vertex:" << number() << "setting edges of relation" << relation << "to" << status;
    int target=0;
    qreal weight =0;
    int edgeRelation=0;
    H_edges::iterator it1;
    for ( it1 = m_outEdges.begin(); it1 != m_outEdges.end(); ++ it1) {
        edgeRelation = it1.value().first;
        if ( edgeRelation == relation ) {
            target=it1.key();
            weight = it1.value().second.first;
            it1.value() = pair_i_fb(relation, pair_f_b(weight, status) );
            emit signalSetEdgeVisibility ( relation, m_number, target, status );
        }
    }
}




/**
 * @brief Stores the geodesic distance to vertex v1
 *
 * @param v1
 * @param dist
 */
void GraphVertex::setDistance (const int &v1, const qreal &d) {
    m_distance.insert( v1, pair_i_f(m_curRelation, d ) );
}


/**
 * @brief Reserves N items for the distance hash. See QHash Algorithmic Complexity
 * Not to be used on large nets, atm.
 * @param N
 */
void GraphVertex::reserveDistance (const int &N) {
    m_distance.reserve(N);
}



/**
 * @brief Returns the geodesic distance to vertex v1.
 *
 * If d to v1 has not been set previously, returns RAND_MAX
 *
 * @param v1
 */
qreal GraphVertex::distance (const int &v1) {
    qreal d=RAND_MAX;
    int relation=0;
    H_distance::const_iterator it1=m_distance.constFind(v1);
    while (it1 != m_distance.constEnd() && it1.key() == v1 ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            d = it1.value().second;
            break;
        }
        ++it1;
    }
//    qDebug() << "vertex" << number()  << "distance to" << v1 << "is" << d;
    return d;
}


/**
 * @brief Removes all items from m_distance hash dictionary
 */
void GraphVertex::clearDistance() {
    m_distance.clear();
}




/**
 * @brief Stores the number of shortest paths from this vertex to vertex v1
 *
 * @param v1
 * @param sp
 */
void GraphVertex::setShortestPaths (const int &v1, const int &sp) {
//    qDebug() << "vertex" << number()  << "setting shortest paths count to" << v1 << "equal to" << sp;
    m_shortestPaths.insert( v1, pair_i_i( m_curRelation, sp ) );
}


/**
 * @brief Returns the stored number of shortest paths to vertex v1
 *
 * If it has not been set previously, then returns 0
 *
 * @param v1
 */
int GraphVertex::shortestPaths (const int &v1) {
    int sp=0;
    int relation=0;
    H_shortestPaths::const_iterator it1=m_shortestPaths.constFind(v1);
    while (it1 != m_shortestPaths.constEnd() && it1.key() == v1 ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            sp = it1.value().second;
            break;
        }
        ++it1;
    }
//    qDebug() << "vertex" << number()  << "shortest paths to" << v1 << "count" << sp;
    return sp;
}


/**
 * @brief Reserves N items for the ShortestPaths hash.
 *
 * See QHash Algorithmic Complexit. Not to be used on large nets, atm.
 *
 * @param N
 */
void GraphVertex::reserveShortestPaths (const int &N) {
    m_shortestPaths.reserve(N);
}


/**
 * @brief Removes all items from m_shortestPaths hash dictionary
 */
void GraphVertex::clearShortestPaths() {
    m_shortestPaths.clear();
}


/**
 * @brief Stores the eccentricity of the vertex
 * @param c
 */
void GraphVertex::setEccentricity(const qreal &c){
    m_Eccentricity=c;
}

/**
 * @brief Returns the stored eccentricity of the vertex
 * @return
 */
qreal GraphVertex::eccentricity() {
    return m_Eccentricity;
}

/**
 * @brief Stores the pair dependency of the vertex
 * @param c
 */
void GraphVertex::setDelta (const qreal &c){
    m_delta=c;
}

/**
 * @brief Returns the stored pair dependency of the vertex
 * @return
 */
qreal GraphVertex::delta() {
    return m_delta;
}



/**
 * @brief Clears the list of predecessors of this vertex
 */
void GraphVertex::clearPs()	{  
	myPs.clear();
}
	
/**
 * @brief Appends a vertex to the list of predecessors of this vertex
 * @param vertex
 */
void GraphVertex::appendToPs(const int &vertex ) {
//    qDebug()<<"vertex"<< number()<< "appending vertex" <<  vertex << "to myPs";
	myPs.append(vertex); 
}


/**
 * @brief Returns the list of predecessors of this vertex
 * @return
 */
L_int GraphVertex::Ps(void) {
	 return myPs;
}



/**
 * @brief  Returns the number of cliques sized size this vertex belongs to
 *
 * @param size
 * @return
 */
int GraphVertex::cliques (const int &ofSize)
{
    return m_cliques.values( ofSize ).size();
}


/**
 * @brief Adds clique to my cliques
 *
 * @param clique
 */
void GraphVertex::cliqueAdd (const QList<int> &clique) {
//    qDebug()<<"vertex"<< number()<< "adding clique with:" << clique;
    m_cliques.insert(clique.size(), clique);
}


GraphVertex::~GraphVertex() {
//    qDebug()<<"vertex"<< number()<< "destroying...";
    m_outEdges.clear();
    m_outEdges.squeeze();
    m_inEdges.clear();
    m_inEdges.squeeze();
    m_reciprocalEdges.clear();
    m_reciprocalEdges.squeeze();

    m_outLinkColors.clear();
    m_outLinkColors.squeeze();

    m_outEdgeLabels.clear();
    m_outEdgeLabels.squeeze();

    clearPs();

    m_shortestPaths.clear();
    m_shortestPaths.squeeze();

    m_distance.clear();
    m_distance.squeeze();

    m_neighborhoodList.clear();

    m_cliques.clear();
    m_cliques.squeeze();

}


