/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 3.1.0-dev
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
                         const QString &iconPath ): m_graph (parentGraph)
{ 
    qDebug() << "vertex:"<< name << "initializing...";

    m_name=name;
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
    //FIXME m_outLinkColors list need update when we remove vertices/edges
    //m_outLinkColors.reserve(2000);
    m_outEdgeLabels.reserve(2000);
    m_outEdges.reserve(2000);
    m_inEdges.reserve(2000);
    m_neighborhoodList.reserve(1000);

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

//    connect (this, SIGNAL (setEdgeVisibility ( int, int, int, bool) ),
//             m_graph, SLOT (edgeVisibilitySet (int, int, int, bool)) );

    connect( this, &GraphVertex::setEdgeVisibility,
             m_graph, &Graph::setEdgeVisibility);

}


/**
 * @brief constructor with default values
 * @param name
 */
GraphVertex::GraphVertex(const int &name) {
    qDebug() << "name"<<  name << "initializing with default values";
    m_name=name;
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
* @brief Changes the current relation of this vertex to newRel
*
* @param newRel
*/
void GraphVertex::relationSet(int newRel) {
    qDebug() << "vertex" << name() << "current rel:" << m_curRelation << "new rel:" << newRel;
    // first make false all edges of current relation
    edgeFilterByRelation(m_curRelation, false);
    // then make true all edges of new relation
    edgeFilterByRelation(newRel, true);
    // update current relation
    m_curRelation=newRel;
}


/**
 * @brief Returns the vertex color to pajek format
 */
QString GraphVertex::colorToPajek(){
    if (m_color.startsWith("#")) {
        return  ("RGB"+m_color.right( m_color.size()-1 )).toUpper()  ;
    }
    return m_color;
}



/**
 * @brief Adds an outbound edge to vertex v2 with weight w
 *
 * @param target
 * @param weight
 */
void GraphVertex::edgeAddTo (const int &v2, const qreal &weight, const QString &color, const QString &label) {
    qDebug() << "vertex" << name() << "adding new outbound edge"<< "->"<< v2
             << "weight"<< weight<< "relation" << m_curRelation;
    // do not use [] operator - silently creates an item if key do not exist
    m_outEdges.insert(
                v2, pair_i_fb(m_curRelation, pair_f_b(weight, true) ) );
    setOutLinkColor(v2, color);
    setOutEdgeLabel(v2, label);
}


/**
 * @brief Sets the status of the edge to the given target vertex
 *
 * @param target
 * @param status
 */
void GraphVertex::setOutEdgeEnabled (const int target, bool status){
    qDebug() << "vertex" << name() << "setting outEdge to" << target << "new status" << status;
    int linkTarget=0;
    qreal weight =0;
    int relation = 0;
    QMultiHash<int, pair_i_fb >::iterator it1;
    for ( it1 = m_outEdges.begin(); it1 != m_outEdges.end(); ++ it1) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            linkTarget=it1.key();
            if ( linkTarget == target ) {
                weight = it1.value().second.first;
                qDebug() << " *** vertex " << m_name << " connected to "
                         << linkTarget << " relation " << relation
                         << " weight " << weight
                         << " status " << it1.value().second.second;
                it1.value() = pair_i_fb(m_curRelation, pair_f_b(weight, status) );
                emit setEdgeVisibility (m_curRelation, m_name, target, status );
                break;
            }
        }
    }
}


/**
 * @brief Adds an inbound edge from vertex v1
 *
 * @param source
 * @param weight
 */
void GraphVertex::edgeAddFrom (const int &v1, const qreal &weight) {
    qDebug() << "vertex" << name() << "adding new inbound edge"<< "<-"<< v1
             << "weight"<< weight<< "relation" << m_curRelation;
    m_inEdges.insert(
                v1, pair_i_fb (m_curRelation, pair_f_b(weight, true) ) );
}


/**
 * @brief Changes the weight of the outbound edge to the given vertex
 *
 * @param target
 * @param weight
 */
void GraphVertex::changeOutEdgeWeight(const int &target, const qreal &weight){
    qDebug() << "vertex" << name() << "changing weight of outEdge to" << target << "new weight" << weight;
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
 * @brief Removes the outbound edge to vertex v2
 *
 * @param v2
 */
void GraphVertex::edgeRemoveTo (const int v2) {

    qDebug() << "vertex" << name() << "removing outEdge to" << v2;

    if (outEdgesCount() == 0) {
        return;
    }

    H_edges::const_iterator it1=m_outEdges.constFind(v2);
    while (it1 != m_outEdges.constEnd() && it1.key() == v2 ) {
        if ( it1.value().first == m_curRelation ) {
//            qDebug() << " *** vertex " << m_name << " connected to "
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
 * @brief Removes the inbound edge from vertex v2
 *
 * @param v2
 */
void GraphVertex::edgeRemoveFrom(const int v2){
    qDebug() << "vertex" << name() << "removing inEdge from" << v2;

    if (inEdgesCount()==0) {
        return;
    }

    H_edges::const_iterator it=m_inEdges.constFind(v2);
    while (it != m_inEdges.constEnd() ) {
        if ( it.key() == v2 && it.value().first == m_curRelation ) {
//            qDebug() << " *** vertex " << m_name << " connected from  "
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
 * @brief Filters outbound edges over or under a specified threshold weight
 *
 * @param qreal m_threshold
 * @param bool overThreshold
 */
void GraphVertex::edgeFilterByWeight(const qreal m_threshold, const bool overThreshold){
    qDebug() << "vertex" << name() << "filtering edges with threshold" << m_threshold;
	int target=0;
    qreal weight=0;
    QMultiHash<int, pair_i_fb >::iterator it;
    for ( it = m_outEdges.begin(); it != m_outEdges.end(); ++ it) {
        if ( it.value().first == m_curRelation ) {
            target=it.key();
            weight = it.value().second.first;
            if (overThreshold) {
                // We will filter out all edges with weights ABOVE the m_threshold
                if ( weight >= m_threshold ) {
                    qDebug() << "edge to:" << target << "weight:" << weight << "will be disabled. Emitting signal...";
                    it.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false) );
                    emit setEdgeVisibility (m_curRelation, m_name, target, false );
                }
                else {
                    qDebug() << "edge to:" << target << "weight:" << weight << "will be enabled. Emitting signal...";
                    it.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true) );
                    emit setEdgeVisibility (m_curRelation, m_name, target, true );
                }
            }
            else {
                // We will filter out all edges BELOW the m_threshold
                 if ( weight <= m_threshold ) {
                    qDebug() << "edge to:" << target << "weight:" << weight << "will be disabled. Emitting signal...";
                    it.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false) );
                    emit setEdgeVisibility (m_curRelation, m_name, target, false );
                }
                else {
                    qDebug() << "edge to:" << target << "weight:" << weight << "will be enabled. Emitting signal...";
                    it.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true) );
                    emit setEdgeVisibility (m_curRelation, m_name, target, true );
                }
            }
        }
    }
}







/**
 * @brief Disables all unilateral (non-reciprocal) edges in current relation
 *
 * If allRelations is true, then all relations are checked
 *
 * @param toggle
 */
void GraphVertex::edgeFilterUnilateral(const bool &toggle){
    qDebug() << "vertex:" << name();
    int target=0;
    qreal weight=0;
    QMultiHash<int, pair_i_fb >::iterator it;
    for ( it = m_outEdges.begin(); it != m_outEdges.end(); ++it) {
        if ( it.value().first == m_curRelation ) {
            target=it.key();
            weight = it.value().second.first;
            if (hasEdgeFrom(target)==0) {   // \todo != weight would be more precise?
                    if ( !toggle ) {
                        qDebug() << "unilateral edge to" << target<< "will be disabled. Emitting signal to Graph....";
                        it.value() = pair_i_fb(m_curRelation, pair_f_b(weight, false) );
                        emit setEdgeVisibility (m_curRelation, m_name, target, false );
                    }
                    else {
                        qDebug() << "unilateral edge to" << target<< "will be enabled. Emitting signal to Graph....";
                        it.value() = pair_i_fb(m_curRelation, pair_f_b(weight, true) );
                        emit setEdgeVisibility (m_curRelation, m_name, target, true );
                    }
            }
        }
    }
}



/**
 * @brief Toggles all edges of the given relation to the new status
 *
 * @param relation
 * @param status
 */
void GraphVertex::edgeFilterByRelation(const int relation, const bool status ){
//    qDebug() << "vertex:" << name() << "setting edges of relation" << relation << "to" << status;
    int target=0;
    qreal weight =0;
    int edgeRelation=0;
    QMultiHash<int, pair_i_fb >::iterator it1;
    for ( it1 = m_outEdges.begin(); it1 != m_outEdges.end(); ++ it1) {
        edgeRelation = it1.value().first;
        if ( edgeRelation == relation ) {
            target=it1.key();
            weight = it1.value().second.first;
            it1.value() = pair_i_fb(relation, pair_f_b(weight, status) );
            emit setEdgeVisibility ( relation, m_name, target, status );
        }
        else {

        }

    }
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
 * @brief Returns a qhash of all enabled outEdges in the active relation
 *
 * @return  QHash<int,qreal>*
 */
QHash<int,qreal> GraphVertex::outEdgesEnabledHash(const bool &allRelations){
    //qDebug() << "vertex " << name();
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
 * @brief  Returns a qhash of all edges to neighbors in all relations
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
    qDebug() << "vertex" << name() << "outEdges count:"<< outEdgesAll->count();
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

    qDebug() << "vertex" << name() << "reciprocalEdges count:" << m_reciprocalEdges.count();

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
                if ( this->name() != it1.key() && this->hasEdgeFrom (it1.key()) == m_weight ) {
                    m_neighborhoodList << it1.key();
                }
            }
        }
        ++it1;
    }
    return m_neighborhoodList;
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
    qDebug() << "vertex" << name() << "enabled inEdges count:"<< enabledInEdges->count();
    return enabledInEdges;
}



/**
 * @brief Returns the number of active inbound arcs
 *
 * Needs to have inEdges called before the call to this method
 *
 * @return int
 */
int GraphVertex::inEdgesCountConst() const {
    return m_inEdgesCounter;
}



/**
 * @brief Returns the outDegree (the sum of all enabled outEdges weights) of this vertex
 *
 * @return int
 */
int GraphVertex::degreeOut() {
    qDebug() << "vertex" << name();
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
    qDebug() << "vertex" << name();
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

    qDebug() << "vertex" << name()  << "localDegree:" << m_localDegree;
	return m_localDegree;
}


/**
 * @brief Checks if this vertex has an enabled outbound edge to the given vertex and returns the weight of the edge
 *
 * @param v2
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
 * @brief Checks if this vertex has an inbound edge from v2 and returns the weight of the link
 * only if the inLink is enabled.
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
 * @brief Sets the geodesic distance to vertex v1
 *
 * @param v1
 * @param dist
 */
void GraphVertex::setDistance (const int &v1, const qreal &d) {
    m_distance.insert( v1, pair_i_f(m_curRelation, d ) );
}


/**
 * @brief Reserves N items for the distance hash. See QHash Algorithmic Complexity
 * * Not to be used on large nets, atm.
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
    qDebug() << "vertex" << name()  << "distance to" << v1 << "is" << d;
    return d;
}


/**
 * @brief Removes all items from m_distance hash dictionary
 */
void GraphVertex::clearDistance() {
    m_distance.clear();
}




/**
 * @brief Sets the nymber of shortest paths to vertex v1
 *
 * @param v1
 * @param sp
 */
void GraphVertex::setShortestPaths (const int &v1, const int &sp) {
    qDebug() << "vertex" << name()  << "setting shortest paths count to" << v1 << "equal to" << sp;
    m_shortestPaths.insert( v1, pair_i_i( m_curRelation, sp ) );
}


/**
 * @brief Returns the number of shortest paths to vertex v1
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
    qDebug() << "vertex" << name()  << "shortest paths to" << v1 << "count" << sp;
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
    qDebug()<<"vertex"<< name()<< "adding clique with:" << clique;
    m_cliques.insert(clique.size(), clique);
}



/**
 * @brief GraphVertex::clearPs
 */
void GraphVertex::clearPs()	{  
	myPs.clear();
}
	
/**
 * @brief GraphVertex::appendToPs
 * @param vertex
 */
void GraphVertex::appendToPs(const int &vertex ) {
    qDebug()<<"vertex"<< name()<< "appending vertex" <<  vertex << "to myPs";
	myPs.append(vertex); 
}


/**
 * @brief GraphVertex::Ps
 * @return
 */
L_int GraphVertex::Ps(void) {
	 return myPs;
}



GraphVertex::~GraphVertex() {
    qDebug()<<"vertex"<< name()<< "destroying...";
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


