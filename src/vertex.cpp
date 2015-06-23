/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 1.9
 Written in Qt
 
                         vertex.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
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

#include "vertex.h"


#include <QtDebug>		//used for qDebug messages
#include "graph.h"

Vertex::Vertex(Graph* parent,
               const long &name,
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
               const QString &shape): parentGraph (parent)
{ 
    qDebug() << "Vertex::Vertex() - "<<  name << " setting values";
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
	m_x=p.x();
	m_y=p.y();
	//FIXME outLinkColors list need update when we remove vertices/edges
//	outLinkColors=new  QString[1500];
    //Q_CHECK_PTR(outLinkColors);
    //outLinkColors.reserve(2000);
    m_outEdges.reserve(100);
    m_inEdges.reserve(100);
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
    m_reciprocalLinked=false;
    m_enabled = true;

    connect (this, SIGNAL (setEdgeVisibility ( int, int, int, bool) ),
             parent, SLOT (slotSetEdgeVisibility (int, int, int, bool)) );

}

// constructor with default values
Vertex::Vertex(const long int &name) {
    qDebug() << "Vertex::Vertex() - "<<  name << " using default values";
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
    m_reciprocalLinked=false;
}


/**
* @brief Vertex::changeRelation
* @param relation
*/
void Vertex::changeRelation(int relation) {
    qDebug() << "Vertex::changeRelation() - Current: " << m_curRelation
                << " new: " << relation;
    // first make false all edges of current relation
    filterEdgesByRelation(m_curRelation, false);
    // then make true all edges of new relation
    filterEdgesByRelation(relation, true);
    // update current relation
    m_curRelation=relation;
}


QString Vertex::colorToPajek(){
    if (m_color.startsWith("#")) {
        return  ("RGB"+m_color.right( m_color.size()-1 )).toUpper()  ;
    }
    return m_color;
}



/**
 * @brief Vertex::addEdgeTo
 * Adds an outLink to target with weight w
 * @param target
 * @param weight
 */
void Vertex::addEdgeTo (const long &v2, const float &weight) {
    qDebug() <<"Vertex::addEdgeTo() - new link "
            << name() << " -> "<< v2 << " weight "<< weight
               << " relation " << m_curRelation;
    // do not use [] operator - silently creates an item if key do not exist
    m_outEdges.insertMulti(
                v2, rel_w_bool(m_curRelation, pair_f_b(weight, true) ) );
}


/**
 * @brief Vertex::setOutEdgeEnabled
 * @param target
 * @param status
 */
void Vertex::setOutEdgeEnabled (long int target, bool status){
    qDebug () << "Vertex::setOutEdgeEnabled - set outLink to " << target
              << " as " << status
                 << ". Finding outLink...";
    QMutableHashIterator < int, rel_w_bool > it1 (m_outEdges);
    int linkTarget=0;
    float weight =0;
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
                it1.setValue(rel_w_bool(m_curRelation, pair_f_b(weight, status) ));
                emit setEdgeVisibility (m_curRelation, m_name, target, status );
            }
        }
        else {

        }
    }
}


/**
 * @brief Vertex::addEdgeFrom
 * @param source
 * @param weight
 */
void Vertex::addEdgeFrom (const long int &v1, const float &weight) {
//    qDebug() <<"Vertex: "<< name() << " addEdgeFrom() "<< source;
    m_inEdges.insertMulti(
                v1, rel_w_bool (m_curRelation, pair_f_b(weight, true) ) );
}


void Vertex::changeOutEdgeWeight(long int target, float weight){
    qDebug() << "Vertex::changeEdgeWeightTo " << target << " weight " << weight ;
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
                target, rel_w_bool(m_curRelation, pair_f_b(weight, true) ) );
    qDebug() << " *** m_outEdges.count " << m_outEdges.count();
}



/**
 * @brief Vertex::removeEdgeTo
 * finds and removes a link to vertex v2
 * @param v2
 */
void Vertex::removeEdgeTo (long int v2) {
    qDebug() << "Vertex: removeEdgeTo() - vertex " << m_name
             << " has " <<outEdges() << " out-links. Removing link to "<< v2 ;

    if (outEdges()>0) {
        qDebug () << "checking all_outEdges";
        H_edges::iterator it1=m_outEdges.find(v2);
        while (it1 != m_outEdges.end() && it1.key() == v2 ) {
            if ( it1.value().first == m_curRelation ) {
                qDebug() << " *** vertex " << m_name << " connected to "
                         << it1.key() << " relation " << it1.value().first
                         << " weight " << it1.value().second.first
                         << " enabled ? " << it1.value().second.second;
                qDebug() << " *** erasing outEdge from m_outEdges ";
                it1=m_outEdges.erase(it1);
            }
            else {
                ++it1;
            }
        }
        qDebug() << "Vertex: vertex " <<  m_name << " now has " <<  outEdges() << " out-edges";
	}
	else {
		qDebug() << "Vertex: vertex " <<  m_name << " has no edges" ;
	}
}


/**
 * @brief Vertex::removeEdgeFrom
 * @param v2
 */
void Vertex::removeEdgeFrom(long int v2){
    qDebug() << "Vertex: removeEdgeFrom() vertex " << m_name
             << " has " <<  inEdges() << "  in-edges. RemovingEdgeFrom " << v2 ;

    if (inEdges()>0) {
        qDebug () << "checking all_inEdges";
        H_edges::iterator it=m_inEdges.find(v2);
        while (it != m_inEdges.end() ) {
            if ( it.key() == v2 && it.value().first == m_curRelation ) {
                qDebug() << " *** vertex " << m_name << " connected from  "
                         << it.key() << " relation " << it.value().first
                         << " weight " << it.value().second.first
                         << " enabled ? " << it.value().second.second;
                qDebug() << " *** erasing inEdge from m_inEdges ";
                it=m_inEdges.erase(it);
            }
            else {
                ++it;
            }
        }
        qDebug() << "Vertex: vertex " << m_name << " now has "
                 << inEdges() << " in-links"  ;
	}
	else {
		qDebug() << "Vertex: vertex " << m_name << " has no edges";
	}
}



/**
 * @brief Vertex::filterEdgesByWeight
   Called from Graph parent
    to filter edges over or under a specified weight (m_threshold)
 * @param m_threshold
 * @param overThreshold
 */
void Vertex::filterEdgesByWeight(float m_threshold, bool overThreshold){
	qDebug() << "Vertex::filterEdgesByWeight of vertex " << this->m_name;
	int target=0;
    float weight=0;
    QMutableHashIterator < int, rel_w_bool > it (m_outEdges);
    while ( it.hasNext()) {
        it.next();
        if ( it.value().first == m_curRelation ) {
            target=it.key();
            weight = it.value().second.first;
            if (overThreshold) {
                if ( weight >= m_threshold ) {
                    qDebug() << "Vertex::filterEdgesByWeight(). Edge  to " << target
                    << " has weight " << weight
                    << ". It will be disabled. Emitting signal to Graph....";
                    it.setValue(rel_w_bool(m_curRelation, pair_f_b(weight, false) ));
                    emit setEdgeVisibility (m_curRelation, m_name, target, false );
                }
                else {
                    qDebug() << "Vertex::filterEdgesByWeight(). Edge to " << target
                    << " has weight " << weight << ". It will be enabled. Emitting signal to Graph....";
                    it.setValue(rel_w_bool(m_curRelation, pair_f_b(weight, true) ));
                    emit setEdgeVisibility (m_curRelation, m_name, target, true );
                }
            }
            else {
                 if ( weight <= m_threshold ) {
                    qDebug() << "Vertex::filterEdgesByWeight(). Edge  to " << target
                    << " has weight " << weight << ". It will be disabled. Emitting signal to Graph....";
                    it.setValue(rel_w_bool(m_curRelation, pair_f_b(weight, false) ));
                    emit setEdgeVisibility (m_curRelation, m_name, target, false );
                }
                else {
                    qDebug() << "Vertex::filterEdgesByWeight(). Edge  to " << target
                    << " has weight " << weight << ". It will be enabled. Emitting signal to Graph....";
                    it.setValue(rel_w_bool(m_curRelation, pair_f_b(weight, true) ));
                    emit setEdgeVisibility (m_curRelation, m_name, target, true );
                }
            }

        }
    }
}




/**
 * @brief Vertex::filterEdgesByRelation
 * Called from Graph to filter out all edges of a given relation
 * @param relation
 */
void Vertex::filterEdgesByRelation(int relation, bool status ){
    qDebug() << "Vertex::filterEdgesByRelation() - Vertex " << this->m_name
                << " filtering edges of relation " << relation << " to " << status;
    int target=0;
    float weight =0;
    int edgeRelation=0;
    QMutableHashIterator < int, rel_w_bool > it1 (m_outEdges);
    while ( it1.hasNext()) {
        it1.next();
        edgeRelation = it1.value().first;
        if ( edgeRelation == relation ) {
            target=it1.key();
            weight = it1.value().second.first;
            qDebug() << "*** outLink " << m_name << " -> " << target
                        << "  - emitting to GW to be " << status ;
            it1.setValue(rel_w_bool(relation, pair_f_b(weight, status) ));
            emit setEdgeVisibility ( relation, m_name, target, status );
        }
        else {

        }

    }
}



/**
 * @brief Vertex::outEdges
 * Returns the number of active outbound arcs, aka the number of
 * outEdges, from this vertex for the current relation
 * @return long int
 */
long int Vertex::outEdges() {
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
 * @brief Vertex::outEdgesConst
 * Returns the number of active outbound arcs
 * Needs to have outEdges called before the call to this method
 * @return long int
 */
long int Vertex::outEdgesConst() const {
    return m_outEdgesCounter;
}


/**
 * @brief Vertex::returnEnabledOutEdges
 * Returns a qhash of all enabled outEdges in the active relation
 * @return  QHash<int,float>*
 */
QHash<int,float>* Vertex::returnEnabledOutEdges(){
    //qDebug() << " Vertex::returnEnabledOutEdges() vertex " << this->name();
    QHash<int,float> *enabledOutEdges = new QHash<int,float>;
    float m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                enabledOutEdges->insert(it1.key(), m_weight);
//                qDebug() <<  " Vertex::returnEnabledOutEdges() count:"
//                             << enabledOutEdges->count();
            }
        }
        ++it1;
    }
//    qDebug() << " Vertex::returnEnabledOutEdges() vertex " << this->name()
//                << " outEdges count:"
//                 << enabledOutEdges->count();
    return enabledOutEdges;
}



/**
 * @brief Vertex::allReciprocalEdges
 * Returns a qhash of all reciprocal edges to neighbors in the active relation
 * @return  QHash<int,float>*
 */
QHash<int,float>* Vertex::returnReciprocalEdges(){
//    qDebug() << "Vertex::returnReciprocalEdges() - vertex " << this->name();
    QHash<int,float> *reciprocalEdges = new QHash<int,float>;
    float m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outEdges.constBegin();
    while (it1 != m_outEdges.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                if (this->hasEdgeFrom (it1.key()) )
                    reciprocalEdges->insertMulti(it1.key(), m_weight);
            }
        }
        ++it1;
    }

    qDebug() << "Vertex::returnReciprocalEdges() - vertex " << this->name()
             <<  " = "
              << reciprocalEdges->count();

    return reciprocalEdges;
}



/**
 * @brief Vertex::neighborhood
 * Returns a qhash of all neighbors in the active relation
 * @return  QHash<int,float>*
 */
//QHash<int,float>* Vertex::neighborhood(){
//    qDebug() << " Vertex::neighborhood() vertex " << this->name();
//    QHash<int,float> *neighbors = new QHash<int,float>;
//    float m_weight=0;
//    int relation = 0;
//    bool edgeStatus=false;
//    H_edges::const_iterator it1=m_outEdges.constBegin();
//    while (it1 != m_outEdges.constEnd() ) {
//        relation = it1.value().first;
//        if ( relation == m_curRelation ) {
//            edgeStatus=it1.value().second.second;
//            if ( edgeStatus == true) {
//                m_weight=it1.value().second.first;
//                if (this->hasEdgeFrom (it1.key()) )
//                    neighbors->insertMulti(it1.key(), m_weight);
//                qDebug() <<  " Vertex::returnReciprocalEdges() count:"
//                             << reciprocalEdges->count();
//            }
//        }
//        ++it1;
//    }


//    qDebug() <<  " Vertex::returnReciprocalEdges() total "
//                 << reciprocalEdges->count();

//    qDebug() <<  " Vertex::returnReciprocalEdges() localDegree "
//                 << this->localDegree();
//    return reciprocalEdges;
//}


/**
 * @brief Vertex::inEdges
 * Returns the number of active inbound arcs, aka the number of
 * inEdges, to this vertex for the current relation
 * @return long int
 */
long int Vertex::inEdges() {
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
 * @brief Vertex::inEdgesConst
 * Returns the number of active inbound arcs
 * Needs to have inEdges called before the call to this method
 * @return long int
 */
long int Vertex::inEdgesConst() const {
    return m_inEdgesCounter;
}



/**
 * @brief Vertex::outDegree
 * Returns the outDegree (the sum of all enabled outEdges weights) of this vertex
 * @return long int
 */
long int Vertex::outDegree() {
    qDebug() << "Vertex::outDegree()";
    m_outDegree=0;
    float m_weight=0;
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

long int Vertex::outDegreeConst() {
    return m_outDegree;
}

/**
 * @brief Vertex::inDegree
 * Returns the inDegree (the sum of all enabled inEdges weights) of this vertex
 * @return long int
 */
long int Vertex::inDegree() {
    qDebug() << "Vertex::inDegree()";
    m_inDegree=0;
    float m_weight=0;
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


long int Vertex::inDegreeConst() {
    return m_inDegree;
}


/**
 	localDegree is the outDegree + inDegree minus the edges counted twice.
*/
long int Vertex::localDegree(){
	long int v2=0;
    int relation = 0;
    bool edgeStatus=false;
    m_localDegree = (outDegree() + inDegree() );

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

	qDebug() << "Vertex:: localDegree() for " << this->name()  << "is " << m_localDegree;
	return m_localDegree;
}


/**
 * @brief Vertex::hasEdgeTo
 * Checks if this vertex is outlinked to v2 and returns the weight of the link
 * only if the outLink is enabled.
 * @param v2
 * @return
 */
float Vertex::hasEdgeTo(long int v2){
//    qDebug()<< "Vertex::hasEdgeTo() " << name() << " -> " << v2 ;
    float m_weight=0;
    bool edgeStatus=false;
    H_edges::iterator it1=m_outEdges.find(v2);
    while (it1 != m_outEdges.end() && it1.key() == v2 ) {
        if ( it1.value().first == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
//                qDebug()<< "***** Vertex::hasEdgeTo() - relation "
//                           << it1.value().first
//                        <<" link "  <<  this->name()
//                        << " -> " << v2 << "exists, weight "<< m_weight;
                return m_weight;
            }
            else
                qDebug()<< "Vertex::hasEdgeTo() - relation "
                           << it1.value().first
                        <<" link "  <<  this->name()
                        << " -> " << v2 << "exists, weight "<< m_weight
                        << " but edgeStatus " << edgeStatus;
                return 0;
        }
        ++it1;
    }
   // qDebug()<< "Vertex::hasEdgeTo() - INEXISTENT LINK IN RELATION " << m_curRelation;
	return 0;
}


/**
 * @brief Vertex::hasEdgeFrom
 * Checks if this vertex is inLinked from v2 and returns the weight of the link
 * only if the inLink is enabled.
 * @param v2
 * @return
 */
float Vertex::hasEdgeFrom(long int v2){
    qDebug()<< "Vertex::hasEdgeFrom()" ;
    float m_weight=0;
    bool edgeStatus=false;
    H_edges::iterator it1=m_inEdges.find(v2);
    while (it1 != m_inEdges.end() && it1.key() == v2) {
        if ( it1.value().first == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                qDebug()<< "Vertex::hasEdgeFrom() - a ("  <<  this->name()
                        << ", " << v2 << ") = "<< m_weight;
                return m_weight;
            }
            else
                qDebug()<< "Vertex::hasEdgeFrom() - a ("  <<  this->name()
                        << ", " << v2 << ") = "<< m_weight
                        << " but edgeStatus " << edgeStatus;
                return 0;

        }
        ++it1;
    }
    qDebug()<< "Vertex::hasEdgeFrom() - a ("  <<  this->name()  << ", " << v2 << ") = 0 ";
    return 0;
}



int Vertex::cliques (const int &size)
{
    int count = 0;
    foreach (int value, m_cliques) {
        if ( value == size ) {
            count ++;
        }
    }
    return count ;
}

bool Vertex::addClique (const QString &clique, const int &size) {
    QStringList members = clique.split(",");
    switch (size) {
    case 2:
    {
        m_cliques.insert( clique, size);
        break;
    }
    case 3:
    {
        if (! m_cliques.contains( clique) &&
            ! m_cliques.contains( QString::number (this->name()) +
                                  "," + members[2] +
                                  "," + members[1] ) )
        {
            m_cliques.insert( clique, size);
            return true ;
        }
        else
            return false;
        break;
    }
    case 4:
    {
        if (! m_cliques.contains( clique) &&
                ! m_cliques.contains(  QString::number (this->name()) +
                                       "," + members[1] +
                                       "," + members[3] +
                                       "," + members[2] ) &&
                ! m_cliques.contains(  QString::number (this->name()) +
                                       "," + members[2] +
                                       "," + members[1] +
                                       "," + members[3] ) &&
                ! m_cliques.contains(  QString::number (this->name()) +
                                       "," + members[2] +
                                       "," + members[3] +
                                       "," + members[1] ) &&
                ! m_cliques.contains(  QString::number (this->name()) +
                                       "," + members[3] +
                                       "," + members[1] +
                                       "," + members[2] ) &&
                ! m_cliques.contains(  QString::number (this->name()) +
                                       "," + members[3] +
                                       "," + members[2] +
                                       "," + members[1] )
                )
        {
            m_cliques.insert( clique, size);
            return true ;
        }
        else
        {
            return false;
        }

        break;
    }
    };
    return false;
}




void Vertex::clearPs()	{  
	myPs.clear();
}
	
void Vertex::appendToPs(long  int vertex ) {
	qDebug() << "adding " <<  vertex << " to myPs"; 
	myPs.append(vertex); 
}


ilist Vertex::Ps(void) {
	 return myPs;
}



Vertex::~Vertex() {
    qDebug() << " Vertex:: destroying my data";
    m_outEdges.clear();
    outLinkColors.clear();
    clearPs();
    m_outEdges.clear();
    m_inEdges.clear();
}


