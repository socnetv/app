/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.50
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
	qDebug("Vertex: addLinkTo %i", v2);
// 	m_edges.append(v2);
	m_edges[v2]=weight;
	m_outLinks++;
}



void Vertex::addLinkFrom (int source) {
	qDebug("Vertex: addLinkFrom %i", source);
	m_inLinks++;

}

void Vertex::changeLinkWeightTo(int target, float weight){
	qDebug("Vertex: changeEdgeWeightTo %i", target);
// 	m_edges.append(v2);
	m_edges[target]=weight;
}


//finds and removes a link to vertice v2
void Vertex::removeLinkTo (int v2) {
	qDebug("Vertex: removeEdgeTo %i", v2);
	qDebug("Vertex: vertice %i has %i edges",m_name, outLinks() );
	if (outLinks()>0) {
		m_outLinks--;
/*		qSort(m_edges.begin(), m_edges.end());
		QList<int>::iterator i = qBinaryFind(m_edges.begin(), m_edges.end(), v2);*/
		imap_f::iterator i=m_edges.find(v2);
		if ( i != m_edges.end() ) {
			qDebug("Vertex: edge exists. Removing it");
			m_edges.erase(i);
		}
		else {
			qDebug("Vertex: edge doesnt exist.");
		}
//		sorted=true;		
		qDebug("Vertex: vertice %i now has %i edges",m_name, outLinks() );
	}
	else {
		qDebug("Vertex: vertice %i has no edges", m_name);
	}

}


void Vertex::removeLinkFrom(int v2){
	Q_UNUSED(v2);
	m_inLinks--;
}


//Returns the numbers of links from this vertice
int Vertex::outLinks() { 
//	qDebug("Vertex: size %i", m_edges.size()); 
	return m_edges.size(); 
}



//Returns the numbers of links to this vertice
int Vertex::inLinks() { 
//	qDebug("Vertex: size %i", m_edges.size()); 
	return m_inLinks; 
}

//Checks if this vertex is outlinked to v2 and returns the weight of the link
float Vertex::isLinkedTo(int v2){
	imap_f::iterator weight=m_edges.find(v2);
	if (weight  != m_edges.end()) {
		qDebug()<< (*weight).second;
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
	qDebug("Vertex: update linkColor to %i", target);
	outLinkColors[target]=color;
}

QString Vertex::outLinkColor(int target){
	if (outLinkColors.contains(target))
		return outLinkColors.value(target);	
	else return "black";
}


void Vertex::clearPs()	{ 
	qDebug("clearPs"); 
	myPs.clear();
	qDebug("clearedPs");
}
	
void Vertex::appendToPs( int vertex ) {
	qDebug("adding %i to myPs", vertex); 
	myPs.append(vertex); 
}

















Vertex::~Vertex() {
	m_edges.clear();
}
