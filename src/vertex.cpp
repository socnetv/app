/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 1.5
 Written in Qt
 
                         vertex.cpp  -  description
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

#include "vertex.h"


#include <QtDebug>		//used for qDebug messages
#include <QPointF>
#include "graph.h"

Vertex::Vertex(	Graph* parent, 
				int v1,  int val, int size, QString color, 
				QString numColor, int numSize, 
				QString label, QString labelColor, int labelSize,
				QPointF p, 
				QString shape): parentGraph (parent)  
{ 
	m_name=v1; 
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
    outLinkColors.reserve(2000);
    m_outLinksCounter=0;
    m_inLinksCounter=0;
    m_outDegree=0;
    m_inDegree=0;
    m_localDegree=0;
    m_Eccentricity=0;
    m_DC=0; m_SDC=0; m_DP=0; m_SDP=0; m_CC=0; m_SCC=0; m_BC=0; m_SBC=0;
    m_SC=0; m_SSC=0; m_IRCC=0; m_SIRCC=0;
    m_CLC=0; m_hasCLC=false;
    m_curRelation=0;
    m_inLinked=false;
    m_outLinked=false;
    m_reciprocalLinked=false;
    m_enabled = true;
    m_outLinks.reserve(2000);
    m_inLinks.reserve(2000);
    connect (this, SIGNAL (setEdgeVisibility ( int, int, int, bool) ),
             parent, SLOT (slotSetEdgeVisibility (int, int, int, bool)) );

}

Vertex::Vertex(int v1) { 
	m_name=v1; 
	m_value=1;
	m_size=9;
	m_color="black";
	m_label="";
	m_labelColor="black";
	m_shape="circle";
    m_outLinksCounter=0;
    m_inLinksCounter=0;
    m_Eccentricity=0;
    m_DC=0; m_SDC=0; m_DP=0; m_SDP=0; m_CC=0; m_SCC=0; m_BC=0; m_SBC=0;
    m_IRCC=0; m_SIRCC=0; m_SC=0; m_SSC=0;
    m_curRelation=0;
    m_inLinked=false;
    m_outLinked=false;
    m_reciprocalLinked=false;
}


/**
* @brief Vertex::changeRelation
* @param relation
*/
void Vertex::changeRelation(int relation) {
    qDebug() << "Vertice::changeRelation() to relation " << relation
                << " from current relation " << m_curRelation;
    // first make false all edges of current relation
    filterEdgesByRelation(m_curRelation, false);
    // then make true all edges of new relation
    filterEdgesByRelation(relation, true);
    // update current relation
    m_curRelation=relation;
}


/**
 * @brief Vertex::addLinkTo
 * @param target
 * @param weight
 */
void Vertex::addLinkTo (long int target, float weight) {
    qDebug() <<"Vertex::addLinkTo() - new link "
            << name() << " -> "<< target << " weight "<< weight
               << " relation " << m_curRelation;
    // do not use [] operator. It silently creates an item if key do not exist
    m_outLinks.insertMulti(
                target, rel_w_bool(m_curRelation, pair_f_b(weight, true) ) );
    m_outLinksCounter++;
}


/**
 * @brief Vertex::setOutLinkEnabled
 * @param target
 * @param status
 */
void Vertex::setOutLinkEnabled (long int target, bool status){
    qDebug () << "Vertex::setOutLinkEnabled - set outLink to " << target
              << " as " << status
                 << ". Finding outLink...";
    QMutableHashIterator < int, rel_w_bool > it1 (m_outLinks);
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
 * @brief Vertex::addLinkFrom
 * @param source
 * @param weight
 */
void Vertex::addLinkFrom (long int source, float weight) {
//    qDebug() <<"Vertex: "<< name() << " addLinkFrom() "<< source;
    m_inLinks.insertMulti(
                source, rel_w_bool (m_curRelation, pair_f_b(weight, true) ) );
    m_inLinksCounter++;
}

void Vertex::changeLinkWeightTo(long int target, float weight){
    qDebug() << "Vertex::changeEdgeWeightTo " << target << " weight " << weight ;
    qDebug() << " *** m_outLinks.count " <<
                m_outLinks.count();
    qDebug() << "first find and remove old relation-weight pair" ;
    H_edges::iterator it1=m_outLinks.find(target);
    while (it1 != m_outLinks.end() ) {
        if ( it1.key() == target && it1.value().first == m_curRelation ) {
            it1=m_outLinks.erase(it1);
        }
        else {
            ++it1;
        }
    }
    qDebug() << " *** m_outLinks.count " <<
                m_outLinks.count();
    qDebug() << " create new relation-weight pair ";
    m_outLinks.insertMulti(
                target, rel_w_bool(m_curRelation, pair_f_b(weight, true) ) );
    qDebug() << " *** m_outLinks.count " << m_outLinks.count();
}



/**
 * @brief Vertex::removeLinkTo
 * finds and removes a link to vertex v2
 * @param v2
 */
void Vertex::removeLinkTo (long int v2) {
    qDebug() << "Vertex: removeLinkTo() - vertex " << m_name
             << " has " <<outLinks() << " out-links. Removing link to "<< v2 ;

    if (outLinks()>0) {
        qDebug () << "checking all_outLinks";
        H_edges::iterator it1=m_outLinks.find(v2);
        while (it1 != m_outLinks.end() && it1.key() == v2 ) {
            if ( it1.value().first == m_curRelation ) {
                qDebug() << " *** vertex " << m_name << " connected to "
                         << it1.key() << " relation " << it1.value().first
                         << " weight " << it1.value().second.first
                         << " enabled ? " << it1.value().second.second;
                qDebug() << " *** erasing outEdge from m_outLinks ";
                it1=m_outLinks.erase(it1);
            }
            else {
                ++it1;
            }
        }

        m_outLinksCounter--;

        qDebug() << "Vertex: vertex " <<  m_name << " now has " <<  outLinks() << " out-edges";
	}
	else {
		qDebug() << "Vertex: vertex " <<  m_name << " has no edges" ;
	}
}


/**
 * @brief Vertex::removeLinkFrom
 * @param v2
 */
void Vertex::removeLinkFrom(long int v2){
    qDebug() << "Vertex: removeLinkFrom() vertex " << m_name
             << " has " <<  inLinks() << "  in-edges. RemovingEdgeFrom " << v2 ;

    if (inLinks()>0) {
        qDebug () << "checking all_inLinks";
        H_edges::iterator it=m_inLinks.find(v2);
        while (it != m_inLinks.end() ) {
            if ( it.key() == v2 && it.value().first == m_curRelation ) {
                qDebug() << " *** vertex " << m_name << " connected from  "
                         << it.key() << " relation " << it.value().first
                         << " weight " << it.value().second.first
                         << " enabled ? " << it.value().second.second;
                qDebug() << " *** erasing inEdge from m_inLinks ";
                it=m_inLinks.erase(it);
            }
            else {
                ++it;
            }
        }
        m_inLinksCounter--;

        qDebug() << "Vertex: vertex " << m_name << " now has "
                 << inLinks() << " in-links"  ;
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
    QMutableHashIterator < int, rel_w_bool > it (m_outLinks);
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
    qDebug() << "Vertex::filterEdgesByRelation of vertex " << this->m_name
                << " relation " << relation << " to " << status;
    int target=0;
    float weight =0;
    int edgeRelation=0;
    QMutableHashIterator < int, rel_w_bool > it1 (m_outLinks);
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
 * @brief Vertex::outLinks
 * Returns the number of active outbound arcs, aka the number of
 * outLinks, from this vertex for the current relation
 * @return long int
 */
long int Vertex::outLinks() {
    m_outLinksCounter = 0;
    int relation=0;
    bool edgeStatus = false;
    H_edges::const_iterator it1=m_outLinks.constBegin();
    while (it1 != m_outLinks.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_outLinksCounter++;
            }
        }
        ++it1;
    }
    return m_outLinksCounter;
}

/**
 * @brief Vertex::returnEnabledOutLinks
 * Returns a qhash of all enabled outLinks in the active relation
 * @return  QHash<int,float>*
 */
QHash<int,float>* Vertex::returnEnabledOutLinks(){
    qDebug() << " Vertex:: returnEnabledOutLinks()";
    QHash<int,float> *enabledOutLinks = new QHash<int,float>;
    enabledOutLinks->reserve(10000);
    float m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outLinks.constBegin();
    while (it1 != m_outLinks.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                enabledOutLinks->insert(it1.key(), m_weight);
            }
        }
        ++it1;
    }
    return enabledOutLinks;
}


/**
 * @brief Vertex::inLinks
 * Returns the number of active inbound arcs, aka the number of
 * inLinks, to this vertex for the current relation
 * @return long int
 */
long int Vertex::inLinks() {
    m_inLinksCounter = 0;
    int relation=0;
    bool edgeStatus = false;
    H_edges::const_iterator it1=m_inLinks.constBegin();
    while (it1 != m_inLinks.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_inLinksCounter++;
            }
        }
        ++it1;
    }
    return m_inLinksCounter;
}




/**
 * @brief Vertex::outDegree
 * Returns the outDegree (the sum of all enabled outLinks weights) of this vertex
 * @return long int
 */
long int Vertex::outDegree() {
    qDebug() << " Vertex:: outDegree()";
    m_outDegree=0;
    float m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_outLinks.constBegin();
    while (it1 != m_outLinks.constEnd() ) {
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
 * @brief Vertex::inDegree
 * Returns the inDegree (the sum of all enabled inLinks weights) of this vertex
 * @return long int
 */
long int Vertex::inDegree() {
    qDebug() << " Vertex::inDegree()";
    m_inDegree=0;
    float m_weight=0;
    int relation = 0;
    bool edgeStatus=false;
    H_edges::const_iterator it1=m_inLinks.constBegin();
    while (it1 != m_inLinks.constEnd() ) {
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
 	localDegree is the outDegree + inDegree minus the edges counted twice.
*/
long int Vertex::localDegree(){
	long int v2=0;
    int relation = 0;
    bool edgeStatus=false;
    m_localDegree = (outDegree() + inDegree() );

    H_edges::const_iterator it1=m_outLinks.constBegin();
    while (it1 != m_outLinks.constEnd() ) {
        relation = it1.value().first;
        if ( relation == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                v2=it1.key();
                if (this->isLinkedFrom (v2) ) m_localDegree--;
            }
        }
        ++it1;
    }

	qDebug() << "Vertex:: localDegree() for " << this->name()  << "is " << m_localDegree;
	return m_localDegree;
}


/**
 * @brief Vertex::isLinkedTo
 * Checks if this vertex is outlinked to v2 and returns the weight of the link
 * only if the outLink is enabled.
 * @param v2
 * @return
 */
float Vertex::isLinkedTo(long int v2){
    qDebug()<< "Vertex::isLinkedTo()" ;
    float m_weight=0;
    bool edgeStatus=false;
    H_edges::iterator it1=m_outLinks.find(v2);
    while (it1 != m_outLinks.end() && it1.key() == v2 ) {
        if ( it1.value().first == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                qDebug()<< "***** Vertex::isLinkedTo() - relation "
                           << it1.value().first
                        <<" link "  <<  this->name()
                        << " -> " << v2 << "exists, weight "<< m_weight;
                return m_weight;
            }
            else
                qDebug()<< "Vertex::isLinkedTo() - relation "
                           << it1.value().first
                        <<" link "  <<  this->name()
                        << " -> " << v2 << "exists, weight "<< m_weight
                        << " but edgeStatus " << edgeStatus;
                return 0;
        }
        ++it1;
    }
    qDebug()<< "Vertex::isLinkedTo() - INEXISTENT LINK IN RELATION " << m_curRelation;
	return 0;
}


/**
 * @brief Vertex::isLinkedFrom
 * Checks if this vertex is inLinked from v2 and returns the weight of the link
 * only if the inLink is enabled.
 * @param v2
 * @return
 */
float Vertex::isLinkedFrom(long int v2){
    qDebug()<< "Vertex::isLinkedFrom()" ;
    float m_weight=0;
    bool edgeStatus=false;
    H_edges::iterator it1=m_inLinks.find(v2);
    while (it1 != m_inLinks.end() && it1.key() == v2) {
        if ( it1.value().first == m_curRelation ) {
            edgeStatus=it1.value().second.second;
            if ( edgeStatus == true) {
                m_weight=it1.value().second.first;
                qDebug()<< "Vertex::isLinkedFrom() - a ("  <<  this->name()
                        << ", " << v2 << ") = "<< m_weight;
                return m_weight;
            }
            else
                qDebug()<< "Vertex::isLinkedFrom() - a ("  <<  this->name()
                        << ", " << v2 << ") = "<< m_weight
                        << " but edgeStatus " << edgeStatus;
                return 0;

        }
        ++it1;
    }
    qDebug()<< "Vertex::isLinkedFrom() - a ("  <<  this->name()  << ", " << v2 << ") = 0 ";
    return 0;
}


long int Vertex::name() {
	return m_name;
}


void Vertex::setName (long int v1) {
	m_name=v1; 
}




void Vertex::setEnabled (bool flag ){
	m_enabled=flag;
}

bool Vertex::isEnabled (){
	return m_enabled;
}



void Vertex::setSize(int size) {
    qDebug() << " Vertex::setSize() : " << size;
	m_size=size;
}

int  Vertex::size(){
	return m_size;
}


void Vertex::setShape(QString shape){
	m_shape=shape;
}

QString Vertex::shape(){
	return m_shape;	
}


void Vertex::setColor(QString color){
	m_color=color;
}

QString Vertex::color(){
    return m_color;
}

QString Vertex::colorToPajek(){
    if (m_color.startsWith("#")) {
        return  ("RGB"+m_color.right( m_color.size()-1 )).toUpper()  ;
    }
    return m_color;
}


void Vertex::setNumberColor (QString color) {
	m_numberColor = color;
}


QString Vertex::numberColor(){
	return m_numberColor;
}


void Vertex::setNumberSize (int size){
	m_numberSize=size;
}


int Vertex::numberSize(){
	return m_numberSize;
}



void Vertex::setLabel(QString label){
	m_label=label;
}

QString Vertex::label(){
	return m_label;	
}

void Vertex::setLabelColor(QString labelColor){
	m_labelColor=labelColor;
}

QString Vertex::labelColor(){
	return m_labelColor;	
}


void Vertex::setLabelSize(int newSize){
	m_labelSize=newSize;
}

int Vertex::labelSize(){
	return m_labelSize;
}


void Vertex::setX(float  x){
//	qDebug("Vertex setX with %f",x);
	m_x=x;
}

float Vertex::x(){
	return m_x;	
}


void Vertex::setY(float y){
//	qDebug("Vertex setY with %f", y);
	m_y=y;
}

float	Vertex::y(){
	return m_y;	
}


QPointF Vertex::pos () { 
	return QPointF ( x(), y() ); 
}



void Vertex::setOutLinkColor(long int target, QString color){
	qDebug()<<"Vertex: update linkColor to vertex "<< target<< " color: "<< color;
	outLinkColors[target]=color;
}

//FIXME: See MW line 1965
// FIXME MULTIGRAPH
QString Vertex::outLinkColor(int target){
	if (outLinkColors.contains(target))
		return outLinkColors.value(target);	
	else return "black";
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
    m_outLinks.clear();
    outLinkColors.clear();
    clearPs();
    m_outLinks.clear();
    m_inLinks.clear();
}


