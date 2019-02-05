/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 2.5
 Written in Qt
 
                         graphvertex.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2018 by Dimitris B. Kalamaras
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

#include <QtDebug>		//used for qDebug messages

#include "graph.h"
#include "graphvertex.h"

GraphVertex::GraphVertex(Graph* parent,
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
                         const QString &iconPath ): parentGraph (parent)
{ 
    qDebug() << "GraphVertex::GraphVertex() - vertex:"<<  name << "initializing...";
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
//    m_outEdgeLabels.reserve(1000);
//    m_outEdges.reserve(1000);
//    m_inEdges.reserve(1000);
//    m_neighborhoodList.reserve(1000);

    m_outEdgesCounter=0;
    m_inEdgesCounter=0;
    m_outDegree=0;
    m_inDegree=0;
    m_localDegree=0;
    m_Eccentricity=0;
    m_DC=0; m_SDC=0; m_DP=0; m_SDP=0; m_CC=0; m_SCC=0; m_BC=0; m_SBC=0;
    m_SC=0; m_SSC=0; m_IRCC=0; m_SIRCC=0;
    m_CLC=0; m_hasCLC=false;
    m_curRelation=relation;
    m_enabled = true;

    connect (this, SIGNAL (setEdgeVisibility ( int, int, int, bool) ),
             parent, SLOT (edgeVisibilitySet (int, int, int, bool)) );

}


/**
 * @brief constructor with default values
 * @param name
 */
GraphVertex::GraphVertex(const int &name) {
    qDebug() << "GraphVertex::GraphVertex() - "<<  name << " using default values";
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
* @param newRel
*/
void GraphVertex::relationSet(int newRel) {
    qDebug() << "GraphVertex::relationSet() - vertex:" << name()
             << "current relation:" << m_curRelation
             << "settting new relation: " << newRel;
    // first make false all edges of current relation
    edgeFilterByRelation(m_curRelation, false);
    // then make true all edges of new relation
    edgeFilterByRelation(newRel, true);
    // update current relation
    m_curRelation=newRel;
}


/**
 * @brief returns the vertex color to pajek format
 * @return
 */
QString GraphVertex::colorToPajek(){
    if (m_color.startsWith("#")) {
        return  ("RGB"+m_color.right( m_color.size()-1 )).toUpper()  ;
    }
    return m_color;
}



/**
 * @brief Adds an outbound edge to vertex v2 with weight w
 * @param target
 * @param weight
 */
void GraphVertex::edgeAddTo (const int &v2, const qreal &weight) {
    qDebug() <<"GraphVertex::edgeAddTo() - new outbound edge"
            << name() << " -> "<< v2 << " weight "<< weight
               << " relation " << m_curRelation;
    // do not use [] operator - silently creates an item if key do not exist
    m_outEdges.insertMulti(
                v2, pair_i_fb(m_curRelation, pair_f_b(weight, true) ) );
}


/**
 * @brief GraphVertex::setOutEdgeEnabled
 * @param target
 * @param status
 */
void GraphVertex::setOutEdgeEnabled (const int target, bool status){
    qDebug () << "GraphVertex::setOutEdgeEnabled - set outEdge to " << target
              << " as " << status
                 << ". Finding outLink...";
    QMutableHashIterator < int, pair_i_fb > it1 (m_outEdges);
    int linkTarget=0;
    qreal weight =0;
    int relation = 0;
    while ( it1.hasNext()) {
        it1.next();
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            linkTarget=it1.key();
            if ( linkTarget == target ) {
                weight = it1.value().second.first;
                qDebug() << " *** vertex " << m_name << " connected to "
                         << linkTarget << " relation " << relation
                         << " weight " << weight
                         << " status " << it1.value().second.second;
                it1.setValue(pair_i_fb(m_curRelation, pair_f_b(weight, status) ));
                emit setEdgeVisibility (m_curRelation, m_name, target, status );
            }
        }
        else {

        }
    }
}


/**
 * @brief Adds an inbound edge from vertex v1
 * @param source
 * @param weight
 */
void GraphVertex::edgeAddFrom (const int &v1, const qreal &weight) {
    qDebug() <<"GraphVertex::edgeAddFrom() - new inbound edge"
            << name() << " <- "<< v1 << " weight "<< weight
               << " relation " << m_curRelation;
    m_inEdges.insertMulti(
                v1, pair_i_fb (m_curRelation, pair_f_b(weight, true) ) );
}



void GraphVertex::changeOutEdgeWeight(const int &target, const qreal &weight){
    qDebug() << "GraphVertex::changeEdgeWeightTo " << target << " weight " << weight ;
    qDebug() << " *** m_outEdges.count " <<
                m_outEdges.count();
    qDebug() << "first find and remove old relation-weight pair" ;
    H_edges::iterator it1=m_outEdges.find(target);
    while (it1 != m_outEdges.end() ) {
        if ( it1.key() == target && it1.value().first == m_curRelation ) {
            it1=m_outEdges.erase(it1);
        }
        else {
            ++it1;
        }
    }
    qDebug() << " *** m_outEdges.count " <<
                m_outEdges.count();
    qDebug() << " create new relation-weight pair ";
    m_outEdges.insertMulti(
                target, pair_i_fb(m_curRelation, pair_f_b(weight, true) ) );
    qDebug() << " *** m_outEdges.count " << m_outEdges.count();
}



/**
 * @brief Removes outbound edge to vertex v2
 * @param v2
 */
void GraphVertex::edgeRemoveTo (const int v2) {
    qDebug() << "GraphVertex: edgeRemoveTo() - vertex " << m_name
             << " has " <<outEdges() << " out-links. Removing link to "<< v2 ;

    if (outEdges()>0) {
        qDebug() << "GraphVertex::edgeRemoveTo() - checking all_outEdges";
        H_edges::iterator it1=m_outEdges.find(v2);
        while (it1 != m_outEdges.end() && it1.key() == v2 ) {
            if ( it1.value().first == m_curRelation ) {
                qDebug() << " *** vertex " << m_name << " connected to "
                         << it1.key() << " relation " << it1.value().first
                         << " weight " << it1.value().second.first
                         << " enabled ? " << it1.value().second.second
                         << " Erasing outEdge from m_outEdges ";
                it1=m_outEdges.erase(it1);
            }
            else {
                ++it1;
            }
        }
        qDebug() << "GraphVertex::edgeRemoveTo() - vertex " <<  m_name << " now has " <<  outEdges() << " out-edges";
	}
	else {
        qDebug() << "GraphVertex::edgeRemoveTo() - vertex " <<  m_name << " has no edges" ;
	}
}


/**
 * @brief Removes the inbound edge from vertex v2
 * @param v2
 */
void GraphVertex::edgeRemoveFrom(const int v2){
    qDebug() << "GraphVertex::edgeRemoveFrom() - vertex " << m_name
             << " has " <<  inEdges() << "  in-edges. RemovingEdgeFrom " << v2 ;

    if (inEdges()>0) {
        qDebug() << "GraphVertex::edgeRemoveFrom() - checking all_inEdges";
        H_edges::iterator it=m_inEdges.find(v2);
        while (it != m_inEdges.end() ) {
            if ( it.key() == v2 && it.value().first == m_curRelation ) {
                qDebug() << " *** vertex " << m_name << " connected from  "
                         << it.key() << " relation " << it.value().first
                         << " weight " << it.value().second.first
                         << " enabled ? " << it.value().second.second
                         << " Erasing inEdge from m_inEdges ";
                it=m_inEdges.erase(it);
            }
            else {
                ++it;
            }
        }
        qDebug() << "GraphVertex::edgeRemoveFrom() - vertex " << m_name << " now has "
                 << inEdges() << " in-links"  ;
	}
	else {
        qDebug() << "GraphVertex::edgeRemoveFrom() - vertex " << m_name << " has no edges";
	}
}



/**
 * @brief Filters out edges over or under a specified weight (m_threshold)
 * @param m_threshold
 * @param overThreshold
 */
void GraphVertex::edgeFilterByWeight(qreal m_threshold, bool overThreshold){
	qDebug() << "GraphVertex::edgeFilterByWeight of vertex " << this->m_name;
	int target=0;
    qreal weight=0;
    QMutableHashIterator < int, pair_i_fb > it (m_outEdges);
    while ( it.hasNext()) {
        it.next();
        if ( it.value().first == m_curRelation ) {
            target=it.key();
            weight = it.value().second.first;
            if (overThreshold) {
                if ( weight >= m_threshold ) {
                    qDebug() << "GraphVertex::edgeFilterByWeight() - edge  to " << target
                    << " has weight " << weight
                    << ". It will be disabled. Emitting signal to Graph....";
                    it.setValue(pair_i_fb(m_curRelation, pair_f_b(weight, false) ));
                    emit setEdgeVisibility (m_curRelation, m_name, target, false );
                }
                else {
                    qDebug() << "GraphVertex::edgeFilterByWeight() - edge  to " << target
                    << " has weight " << weight << ". It will be enabled. Emitting signal to Graph....";
                    it.setValue(pair_i_fb(m_curRelation, pair_f_b(weight, true) ));
                    emit setEdgeVisibility (m_curRelation, m_name, target, true );
                }
            }
            else {
                 if ( weight <= m_threshold ) {
                    qDebug() << "GraphVertex::edgeFilterByWeight() - edge  to " << target
                    << " has weight " << weight << ". It will be disabled. Emitting signal to Graph....";
                    it.setValue(pair_i_fb(m_curRelation, pair_f_b(weight, false) ));
                    emit setEdgeVisibility (m_curRelation, m_name, target, false );
                }
                else {
                    qDebug() << "GraphVertex::edgeFilterByWeight() - edge  to " << target
                    << " has weight " << weight << ". It will be enabled. Emitting signal to Graph....";
                    it.setValue(pair_i_fb(m_curRelation, pair_f_b(weight, true) ));
                    emit setEdgeVisibility (m_curRelation, m_name, target, true );
                }
            }

        }
    }
}







/**
 * @brief Filters out unilateral (non-reciprocal) edges
   If allRelations is true, then all relations are checked
 * @param toggle
 */
void GraphVertex::edgeFilterUnilateral(const bool &toggle){
    qDebug() << "GraphVertex::edgeFilterUnilateral() of vertex " << this->m_name;
    int target=0;
    qreal weight=0;
    QMutableHashIterator < int, pair_i_fb > it (m_outEdges);
    while ( it.hasNext()) {
        it.next();
        if ( it.value().first == m_curRelation ) {
            target=it.key();
            weight = it.value().second.first;
            if (hasEdgeFrom(target)==0) {   // \todo != weight would be more precise?
                    if ( !toggle ) {
                        qDebug() << "GraphVertex::edgeFilterUnilateral() - unilateral edge to " << target
                        << " has weight " << weight
                        << ". It will be disabled. Emitting signal to Graph....";
                        it.setValue(pair_i_fb(m_curRelation, pair_f_b(weight, false) ));
                        emit setEdgeVisibility (m_curRelation, m_name, target, false );
                    }
                    else {
                        qDebug() << "GraphVertex::edgeFilterUnilateral() - unilateral edge to " << target
                        << " has weight " << weight << ". It will be enabled. Emitting signal to Graph....";
                        it.setValue(pair_i_fb(m_curRelation, pair_f_b(weight, true) ));
                        emit setEdgeVisibility (m_curRelation, m_name, target, true );
                    }
            }
        }
    }
}



/**
 * @brief Filters out all edges of a given relation
 * @param relation
 */
void GraphVertex::edgeFilterByRelation(int relation, bool status ){
    qDebug() << "GraphVertex::edgeFilterByRelation() - Vertex" << name()
                << "Setting edges of relation" << relation << "to" << status;
    int target=0;
    qreal weight =0;
    int edgeRelation=0;
    QMutableHashIterator < int, pair_i_fb > it1 (m_outEdges);
    while ( it1.hasNext()) {
        it1.next();
        edgeRelation = it1.value().first;
        if ( edgeRelation == relation ) {
            target=it1.key();
            weight = it1.value().second.first;
            qDebug() << "GraphVertex::edgeFilterByRelation() - outLink"
                     << m_name << " -> " << target
                     << " of relation" << relation
                     << "Emitting to GW to be" << status ;
            it1.setValue(pair_i_fb(relation, pair_f_b(weight, status) ));
            emit setEdgeVisibility ( relation, m_name, target, status );
        }
        else {

        }

    }
}



/**
 * @brief Returns the number of active outbound arcs, aka the number of
 * outEdges, from this vertex for the current relation
 * @return int
 */
int GraphVertex::outEdges() {
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
 * @brief Returns the number of active outbound arcs
 * Needs to have outEdges called before the call to this method
 * @return int
 */
int GraphVertex::outEdgesConst() const {
    return m_outEdgesCounter;
}


/**
 * @brief Returns a qhash of all enabled outEdges in the active relation
 * @return  QHash<int,qreal>*
 */
QHash<int,qreal> GraphVertex::outEdgesEnabledHash(const bool &allRelations){
    //qDebug() << " GraphVertex::outEdgesEnabledHash() vertex " << this->name();
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
                    //                qDebug() <<  " GraphVertex::outEdgesEnabledHash() count:"
                    //                             << enabledOutEdges->count();
                }
            }
        }
        else {
            if ( !enabledOutEdges.contains(it1.key() )) {
                edgeStatus=it1.value().second.second;
                if ( edgeStatus == true) {
                    m_weight=it1.value().second.first;
                    enabledOutEdges.insert(it1.key(), m_weight);
                    //                qDebug() <<  " GraphVertex::outEdgesEnabledHash() count:"
                    //                             << enabledOutEdges->count();
                }
            }
        }
        ++it1;
    }
//    qDebug() << " GraphVertex::outEdgesEnabledHash() vertex " << this->name()
//                << " outEdges count:"
//                 << enabledOutEdges->count();
    return enabledOutEdges;
}


/**
 * @brief  Returns a qhash of all edges to neighbors in all relations
 * @return
 */
QHash<int, qreal>* GraphVertex::outEdgesAllRelationsUniqueHash() {
    qDebug() << "GraphVertex::outEdgesAllRelationsUniqueHash() - v " << this->name();
    QHash<int,qreal> *outEdgesAll = new QHash<int,qreal>;
    qreal m_weight=0;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        if ( !outEdgesAll->contains(it1.key() )) {
                m_weight=it1.value().second.first;
                outEdgesAll->insert(it1.key(), m_weight);
                qDebug() <<  "GraphVertex::outEdgesAllRelationsUniqueHash() -"
                          << this->name() << "->" << it1.key()
                          << "relation"<< it1.value().first;

        }
        ++it1;
    }
    qDebug() << "GraphVertex::outEdgesAllRelationsUniqueHash() - v " << this->name()
                << " outEdges count:"
                 << outEdgesAll->count();
    return outEdgesAll;

}

/**
 * @brief Returns a qhash of all reciprocal edges to neighbors in the active relation
 * @return  QHash<int,qreal>*
 */
QHash<int, qreal> GraphVertex::reciprocalEdgesHash(){
    m_reciprocalEdges.clear();
    qreal m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.constBegin();
//    qDebug() << "GraphVertex::reciprocalEdgesHash() - of vertex "
//             << this->name()
//                << " - outEdges " <<  m_outEdges.count()
//                << " - Checking all edges for reciprocality";

    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                if (this->hasEdgeFrom (it1.key()) == m_weight ) {
//                    qDebug() << "GraphVertex::reciprocalEdgesHash() - of vertex "
//                             << this->name()
//                             << "Found reciprocal edge with   " << it1.key();
                    m_reciprocalEdges.insertMulti(it1.key(), m_weight);
                }
            }
        }
        ++it1;
    }

    qDebug() << "GraphVertex::reciprocalEdgesHash() - vertex" << this->name()
             <<  "reciprocalEdges:"
              << m_reciprocalEdges.count();

    return m_reciprocalEdges;
}



/**
 * @brief Returns a list of all neighbors mutually connected to this vertex in the active relation
 * Same as calling GraphVertex::reciprocalEdgesHash().keys() which returns a QList of int keys,
 * where each key is a vertex reciprocally connected to this one.
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
                if (this->hasEdgeFrom (it1.key()) == m_weight ) {
                    m_neighborhoodList << it1.key();
//                qDebug() <<  "GraphVertex::neighborhoodList() - mutually connected neighbor="
//                          << it1.key()
//                          << " m_neighborhoodList.count()"
//                          << m_neighborhoodList.count();
                }
            }
        }
        ++it1;
    }

    qDebug() << "GraphVertex::neighborhoodList() - of vertex " << this->name()
             <<  "final list"
              <<m_neighborhoodList
                 <<" count"
                 << m_neighborhoodList.count();

//    qDebug() <<  "GraphVertex::neighborhoodList() - reporting localDegree "
//                 << this->localDegree();
    return m_neighborhoodList;
}


/**
 * @brief Returns the number of active inbound arcs, aka the number of
 * inEdges, to this vertex for the current relation
 * @return int
 */
int GraphVertex::inEdges() {
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
 * @return  QHash<int,qreal>*
 */
QHash<int,qreal>* GraphVertex::inEdgesEnabledHash() {
    qDebug() << "GraphVertex::inEdgesEnabledHash()";
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

    return enabledInEdges;
}



/**
 * @brief Returns the number of active inbound arcs
 * Needs to have inEdges called before the call to this method
 * @return int
 */
int GraphVertex::inEdgesConst() const {
    return m_inEdgesCounter;
}



/**
 * @brief Returns the degreeOut (the sum of all enabled outEdges weights) of this vertex
 * @return int
 */
int GraphVertex::degreeOut() {
    qDebug() << "GraphVertex::degreeOut()";
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

int GraphVertex::outDegreeConst() {
    return m_outDegree;
}

/**
 * @brief Returns the degreeIn (the sum of all enabled inEdges weights) of this vertex
 * @return int
 */
int GraphVertex::degreeIn() {
    qDebug() << "GraphVertex::degreeIn()";
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


int GraphVertex::inDegreeConst() {
    return m_inDegree;
}


/**
    localDegree is the degreeOut + degreeIn minus the edges counted twice.
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

	qDebug() << "GraphVertex:: localDegree() for " << this->name()  << "is " << m_localDegree;
	return m_localDegree;
}


/**
 * @brief GraphVertex::hasEdgeTo
 * Checks if this vertex is outlinked to v2 and returns the weight of the edge
 * only if the outbound edge is enabled.
 * @param v2
 * @return
 */
qreal GraphVertex::hasEdgeTo(const int &v2, const bool &allRelations){
    qreal m_weight=0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.find(v2);
    while (it1 != m_outEdges.end() && it1.key() == v2 ) {
        if (!allRelations) {
            if ( it1.value().first == m_curRelation  ) {
                edgeStatus=it1.value().second.second;
                if ( edgeStatus == true) {
                    m_weight=it1.value().second.first;
//                    qDebug()<< "GraphVertex::hasEdgeTo() - "<<  this->name()
//                            << "->" << v2 << " = "<< m_weight;
                    return m_weight;
                }
                else {
//                    qDebug()<< "GraphVertex::hasEdgeTo() - "<<  this->name()
//                            << "->" << v2 << " = "<< m_weight
//                            << " but edgeStatus " << edgeStatus;
                    return 0;
                }
            }
        }
        else {
                m_weight=it1.value().second.first;
//                qDebug()<< "GraphVertex::hasEdgeTo() - "<<  this->name()
//                        << "->" << v2 << " = "<< m_weight
//                        << "relation"<<it1.value().first;
                return m_weight;
        }
        ++it1;
    }

	return 0;
}


/**
 * @brief GraphVertex::hasEdgeFrom
 * Checks if this vertex is inLinked from v2 and returns the weight of the link
 * only if the inLink is enabled.
 * @param v2
 * @return
 */
qreal GraphVertex::hasEdgeFrom(const int &v2, const bool &allRelations){
    qreal m_weight=0;
    bool edgeStatus=false;
    H_edges::iterator it1=m_inEdges.find(v2);
    while (it1 != m_inEdges.end() && it1.key() == v2) {
        if (!allRelations) {
            if ( it1.value().first == m_curRelation  ) {
                edgeStatus=it1.value().second.second;
                if ( edgeStatus == true) {
                    m_weight=it1.value().second.first;
                    qDebug()<< "GraphVertex::hasEdgeFrom() - "<<  this->name()
                            << "<-" << v2 << " = "<< m_weight;
                    return m_weight;
                }
                else {
                    qDebug()<< "GraphVertex::hasEdgeFrom() - "<<  this->name()
                            << "<-" << v2 << " = "<< m_weight
                            << " but edgeStatus " << edgeStatus;
                    return 0;
                }
            }
        }
        else {
                m_weight=it1.value().second.first;
                qDebug()<< "GraphVertex::hasEdgeFrom() - "<<  this->name()
                        << "<-" << v2 << " = "<< m_weight
                           << "relation"<<it1.value().first;
                return m_weight;



        }
        ++it1;
    }
    //qDebug()<< "GraphVertex::hasEdgeFrom() - a ("  <<  this->name()  << ", " << v2 << ") = 0 ";
    return 0;
}




/**
 * @brief Sets distance to vertex v1 to dist
 * @param v1
 * @param dist
 */
void GraphVertex::setDistance (const int &v1, const qreal &d) {
//    qDebug() <<"GraphVertex::setDistance() - dist"
//            << name() << " --> "<< v1 << " = "<< d
//               << " relation " << m_curRelation;
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
 * @brief Returns geodesic distance to vertex v1
 * If d to v1 has not been set previously, then return RAND_MAX
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
//    qDebug() <<"GraphVertex::distance() - d("
//               << name() << " --> "<< v1 << ") = "<< d;

    return d;
}

/**
 * @brief Removes all items from m_distance hash dictionary
 */
void GraphVertex::clearDistance() {
    m_distance.clear();
}






/**
 * @brief Sets shortest paths to vertex v1 to sp
 * @param v1
 * @param sp
 */
void GraphVertex::setShortestPaths (const int &v1, const int &sp) {
//    qDebug() <<"GraphVertex::setShortestPaths() - sp"
//            << name() << " --> "<< v1 << " = "<< sp
//               << " relation " << m_curRelation;
    m_shortestPaths.insert( v1, pair_i_i( m_curRelation, sp ) );
}

/**
 * @brief Returns number of shortest paths to vertex v1
 * If it has not been set previously, then return 0
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
//    qDebug() <<"GraphVertex::shortestPaths() - sp ("
//               << name() << "->"<< v1 << ") = "<< sp;

    return sp;
}

/**
 * @brief Reserves N items for the ShortestPaths hash. See QHash Algorithmic Complexity
 * Not to be used on large nets, atm.
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
 * @brief GraphVertex::cliques
 * Returns the number of cliques sized size this vertex belongs to
 * @param size
 * @return
 */
int GraphVertex::cliques (const int &ofSize)
{
    return m_cliques.values( ofSize ).size();
}

/**
 * @brief GraphVertex::cliqueAdd
 * @param clique
 */
void GraphVertex::cliqueAdd (const QList<int> &clique) {
    qDebug()<<"GraphVertex::cliqueAdd() - vertex:"
           << name()
           << "in a clique with:"
           << clique;
    m_cliques.insertMulti(clique.size(), clique);
}



/**
 * @brief GraphVertex::clearPs
 */
void GraphVertex::clearPs()	{  
	myPs.clear();
}
	
void GraphVertex::appendToPs(const int &vertex ) {
    qDebug()<<"GraphVertex::appendToPs() - vertex:"
           << name() << "adding" <<  vertex << " to myPs";
	myPs.append(vertex); 
}


L_int GraphVertex::Ps(void) {
	 return myPs;
}



GraphVertex::~GraphVertex() {
    qDebug() << " GraphVertex::~GraphVertex() - destroying my data";
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


