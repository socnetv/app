/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.51
 Written in Qt 4.4 
 
                         vertex.cpp  -  description
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

#include "vertex.h"


#include <QtDebug>		//used for qDebug messages
#include <QPointF>

Vertex::Vertex(int v1, int val, int nsz, QString nc, QString nl, QString lc, QPointF p,QString nsp) { 
	m_name=v1; 
	m_value=val;
	m_size=nsz;
	m_color=nc;
	m_label=nl;
	m_labelColor=lc;
	m_shape=nsp;
	m_x=p.x();
	m_y=p.y();
	//FIXME outLinkColors list need update when we remove vertices/edges
//	outLinkColors=new  QString[1500];
	//Q_CHECK_PTR(outLinkColors);
	outLinkColors.reserve(1600);	
	m_outLinks=0;
	m_inLinks=0;
	m_ODC=0; m_SODC=0; m_IDC=0; m_SIDC=0; m_CC=0; m_SCC=0; m_BC=0; m_SBC=0; m_GC=0; m_SGC=0; m_SC=0; m_SSC=0;
	m_CLC=0; m_hasCLC=FALSE;
	
	m_inLinked=FALSE;
	m_outLinked=FALSE;
	m_reciprocalLinked=FALSE;

}

Vertex::Vertex(int v1) { 
	m_name=v1; 
	m_value=1;
	m_size=9;
	m_color="black";
	m_label="";
	m_labelColor="black";
	m_shape="circle";
	m_outLinks=0;
	m_inLinks=0;

	m_ODC=0; m_SODC=0; m_IDC=0; m_SIDC=0; m_CC=0; m_SCC=0; m_BC=0; m_SBC=0; m_GC=0; m_SGC=0; m_SC=0; m_SSC=0;
	m_inLinked=FALSE;
	m_outLinked=FALSE;
	m_reciprocalLinked=FALSE;
}


void Vertex::addLinkTo (int v2, float weight) {
	qDebug() <<"Vertex: "<< name() << " addLinkTo() "<< v2 << " of weight "<< weight;
	m_outEdges[v2]=weight;
	m_outLinks++;
}



void Vertex::addLinkFrom (int source, float weight) {
	qDebug() <<"Vertex: "<< name() << " addLinkFrom() "<< source;
	m_inEdges[source]=weight;
	m_inLinks++;

}

void Vertex::changeLinkWeightTo(int target, float weight){
	qDebug("Vertex: changeEdgeWeightTo %i", target);
	m_outEdges[target]=weight;
}


//finds and removes a link to vertice v2
void Vertex::removeLinkTo (int v2) {
	qDebug("Vertex: removeLinkTo() vertex %i has %i edges. RemovingEdgeTo %i",m_name, outDegree(),v2 );
	if (outDegree()>0) {
		m_outLinks--;
		imap_f::iterator i=m_outEdges.find(v2);
		if ( i != m_outEdges.end() ) {
			qDebug("Vertex: edge exists. Removing it");
			m_outEdges.erase(i);
			if ( m_outLinks == 0 ) setOutLinked(FALSE);
		}
		else {
			qDebug("Vertex: edge doesnt exist.");
		}
		qDebug("Vertex: vertex %i now has %i edges",m_name, outDegree() );
	}
	else {
		qDebug("Vertex: vertex  %i has no edges", m_name);
	}
}


void Vertex::removeLinkFrom(int v2){
	qDebug("Vertex: removeLinkFrom() vertex %i has %i edges. RemovingEdgeFrom %i",m_name, outDegree(),v2 );
	if (outDegree()>0) {
		m_inLinks--;
		imap_f::iterator i=m_inEdges.find(v2);
		if ( i != m_inEdges.end() ) {
			qDebug("Vertex: edge exists. Removing it");
			m_inEdges.erase(i);
			if ( m_inLinks == 0 ) setInLinked(FALSE);
		}
		else {
			qDebug("Vertex: edge doesnt exist.");
		}
		qDebug("Vertex: vertex %i now has %i edges",m_name, inDegree() );
	}
	else {
		qDebug("Vertex: vertex  %i has no edges", m_name);
	}
}


//Returns the numbers of links from this vertice
int Vertex::outDegree() { 
//	qDebug("Vertex: size %i", m_outEdges.size()); 
	//return m_outLinks;
	return m_outEdges.size(); 
}



//Returns the numbers of links to this vertice
int Vertex::inDegree() { 
//	qDebug("Vertex: size %i", m_outEdges.size()); 
	return m_inLinks; 
}



/**
 	localDegree is the outDegree + inDegree minus the edges counted twice.
*/
int Vertex::localDegree(){
	imap_f::iterator it1;
	int v2=0; 
	int m_localDegree = (outDegree() + inDegree() );
	for( it1 =  m_outEdges.begin(); it1 !=  m_outEdges.end(); it1++ ) {
		v2=it1->first;		
		if (this->isLinkedFrom (v2) ) m_localDegree--; 
	}
	qDebug("Vertex:: localDegree() for %i is  %i", this->name(), m_localDegree);
	return m_localDegree;
}


//Checks if this vertex is outlinked to v2 and returns the weight of the link
float Vertex::isLinkedTo(int v2){
	imap_f::iterator weight=m_outEdges.find(v2);
	if (weight  != m_outEdges.end()) {
		//	qDebug()<< "link to " << v2 << " weight "<<(*weight).second;
		return (*weight).second;
	}
	else 
		return 0;
}



float Vertex::isLinkedFrom(int v2){
	imap_f::iterator weight=m_inEdges.find(v2);
	if (weight  != m_inEdges.end()) {
		//	qDebug()<< "link to " << v2 << " weight "<<(*weight).second;
		return (*weight).second;
	}
	else 
		return 0;
}


int Vertex::name() {
	return m_name;
}


void Vertex::setName (int v1) {
	m_name=v1; 
}


void Vertex::setSize(int size) {
	m_size=size;
}

int  Vertex::size(){
	return m_size;
}


void Vertex::setLabel(QString label){
	m_label=label;
}

QString Vertex::label(){
	return m_label;	
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


void Vertex::setX(float  x){
	qDebug("Vertex setX with %f",x);
	m_x=x;
}

float Vertex::x(){
	return m_x;	
}


void Vertex::setY(float y){
	qDebug("Vertex setY with %f", y);
	m_y=y;
}

float	Vertex::y(){
	return m_y;	
}


QPointF Vertex::pos () { 
	return QPointF ( x(), y() ); 
}



void Vertex::setOutLinkColor(int target, QString color){
	qDebug()<<"Vertex: update linkColor to vertex "<< target<< " color: "<< color;
	outLinkColors[target]=color;
}

//FIXME: See MW line 1965
QString Vertex::outLinkColor(int target){
	if (outLinkColors.contains(target))
		return outLinkColors.value(target);	
	else return "black";
}


void Vertex::clearPs()	{  
	myPs.clear();
}
	
void Vertex::appendToPs( int vertex ) {
	qDebug("adding %i to myPs", vertex); 
	myPs.append(vertex); 
}


ilist Vertex::Ps(void) {
	 return myPs;
}

Vertex::~Vertex() {
	m_outEdges.clear();
}
