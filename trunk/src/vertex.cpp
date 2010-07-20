/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.90
 Written in Qt 4.4 
 
                         vertex.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2010 by Dimitris B. Kalamaras
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
	outLinkColors.reserve(1600);	
	m_outLinks=0;
	m_inLinks=0;
	m_ODC=0; m_SODC=0; m_IDC=0; m_SIDC=0; m_CC=0; m_SCC=0; m_BC=0; m_SBC=0; m_GC=0; m_SGC=0; m_SC=0; m_SSC=0;
	m_CLC=0; m_hasCLC=FALSE;
	
	m_inLinked=FALSE;
	m_outLinked=FALSE;
	m_reciprocalLinked=FALSE;
	m_enabled = TRUE;
	connect (this, SIGNAL (setEdgeVisibility ( int, int, bool) ), parent, SLOT (slotSetEdgeVisibility ( int, int, bool)) );
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




void Vertex::addLinkTo (unsigned long int v2, float weight) {
	//qDebug() <<"Vertex: "<< name() << " addLinkTo() "<< v2 << " of weight "<< weight;
	m_outEdges[v2]=weight;
	m_enabled_outEdges [v2]=1;
	m_outLinks++;
}



void Vertex::addLinkFrom (unsigned long int source, float weight) {
	//qDebug() <<"Vertex: "<< name() << " addLinkFrom() "<< source;
	m_inEdges[source]=weight;
	m_inLinks++;

}

void Vertex::changeLinkWeightTo(unsigned long int target, float weight){
	qDebug() << "Vertex: changeEdgeWeightTo " << target;
	m_outEdges[target]=weight;
	m_enabled_outEdges[target] = 1;
}


//finds and removes a link to vertice v2
void Vertex::removeLinkTo (unsigned long int v2) {
	qDebug() << "Vertex: removeLinkTo() vertex " << m_name << " has " <<outDegree() << " edges. RemovingEdgeTo "<< v2 ;
	if (outDegree()>0) {
		m_outLinks--;
		imap_f::iterator it=m_outEdges.find(v2);
		if ( it != m_outEdges.end() ) {
			qDebug("Vertex: edge exists. Removing it");
			m_outEdges.erase(it);
			m_enabled_outEdges[ it->first ] = 0;
			if ( m_outLinks == 0 ) setOutLinked(FALSE);
		}
		else {
			qDebug("Vertex: edge doesnt exist.");
		}
		qDebug() << "Vertex: vertex " <<  m_name << " now has " <<  outDegree() << " edges";
	}
	else {
		qDebug() << "Vertex: vertex " <<  m_name << " has no edges" ;
	}
}


void Vertex::removeLinkFrom(unsigned long int v2){
	qDebug() << "Vertex: removeLinkFrom() vertex " << m_name << " has " <<  outDegree() << "  edges. RemovingEdgeFrom " << v2 ;
	if (outDegree()>0) {
		m_inLinks--;
		imap_f::iterator i=m_inEdges.find(v2);
		if ( i != m_inEdges.end() ) {
			qDebug("Vertex: edge exists. Removing it");
			m_inEdges.erase(i);
			if ( m_inLinks == 0 ) setInLinked(FALSE);
		}
		else {
			qDebug() << "Vertex: edge doesnt exist.";
		}
		qDebug() << "Vertex: vertex " << m_name << " now has " << inDegree() << "  edges"  ;
	}
	else {
		qDebug() << "Vertex: vertex " << m_name << " has no edges";
	}
}



/**
	Called from Graph parent 
	to filter edges over or under a specified weight (m_threshold)
*/
void Vertex::filterEdgesByWeight(float m_threshold, bool overThreshold){
	qDebug() << "Vertex::filterEdgesByWeight of vertex " << this->m_name;
	imap_f::iterator it1;
	int target=0;
	float m_weight=0; 
	for( it1 =  m_outEdges.begin(); it1 !=  m_outEdges.end(); it1++ ) {
		target=it1->first;
		m_weight = it1->second; 
		if (overThreshold) {
			if ( m_weight >= m_threshold ) {
				qDebug() << "Vertex::filterEdgesByWeight(). Edge  to " << target 
				<< " has weight " << m_weight << ". It will be disabled. Emitting signal to Graph....";
				m_enabled_outEdges[target] = 0;
				emit setEdgeVisibility ( m_name, target, false ); 
			}
			else {
				qDebug() << "Vertex::filterEdgesByWeight(). Edge to " << target 
				<< " has weight " << m_weight << ". It will be enabled. Emitting signal to Graph....";
				m_enabled_outEdges[target] = 1;
				emit setEdgeVisibility ( m_name, target, true );
			}
		}
		else {
			 if ( m_weight <= m_threshold ) {
				qDebug() << "Vertex::filterEdgesByWeight(). Edge  to " << target 
				<< " has weight " << m_weight << ". It will be disabled. Emitting signal to Graph....";
				m_enabled_outEdges[target] = 0;
				emit setEdgeVisibility ( m_name, target, false );
			}
			else {
				qDebug() << "Vertex::filterEdgesByWeight(). Edge  to " << target 
				<< " has weight " << m_weight << ". It will be enabled. Emitting signal to Graph....";
				m_enabled_outEdges[target] = 1;
				emit setEdgeVisibility ( m_name, target, true );
			}	
		} 
	}
}









//Returns the numbers of links from this vertice
unsigned long int Vertex::outDegree() { 
	//return m_outLinks;
	return m_outEdges.size();		//FIXME: What if the user has filtered out links? 
}



//Returns the numbers of links to this vertice
unsigned long int Vertex::inDegree() { 
	return m_inLinks; 			//FIXME: What if the user has filtered out links?
}



/**
 	localDegree is the outDegree + inDegree minus the edges counted twice.
*/
unsigned long int Vertex::localDegree(){
	imap_f::iterator it1;
	unsigned long int v2=0; 
	unsigned long int m_localDegree = (outDegree() + inDegree() );
	for( it1 =  m_outEdges.begin(); it1 !=  m_outEdges.end(); it1++ ) {
		v2=it1->first;		
		if (this->isLinkedFrom (v2) ) m_localDegree--; 
	}
	qDebug() << "Vertex:: localDegree() for " << this->name()  << "is " << m_localDegree;
	return m_localDegree;
}


//Checks if this vertex is outlinked to v2 and returns the weight of the link
float Vertex::isLinkedTo(unsigned long int v2){
	imap_f::iterator weight=m_outEdges.find(v2);
	if (weight  != m_outEdges.end()) {
		if  ( m_enabled_outEdges[ (*weight).first ] == 1) {
			//	qDebug()<< "link to " << v2 << " weight "<<(*weight).second;
			return (*weight).second;			
		}
		else 
			return 0;
	}
	else 
		return 0;
}



float Vertex::isLinkedFrom(unsigned long int v2){
	imap_f::iterator weight=m_inEdges.find(v2);
	if (weight  != m_inEdges.end()) {
		//	qDebug()<< "link to " << v2 << " weight "<<(*weight).second;
		return (*weight).second;
	}
	else 
		return 0;
}


unsigned long int Vertex::name() {
	return m_name;
}


void Vertex::setName (unsigned long int v1) {
	m_name=v1; 
}




void Vertex::setEnabled (bool flag ){
	m_enabled=flag;
}

bool Vertex::isEnabled (){
	return m_enabled;
}



void Vertex::setSize(int size) {
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



void Vertex::setOutLinkColor(unsigned long int target, QString color){
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
	
void Vertex::appendToPs(unsigned long  int vertex ) {
	qDebug() << "adding " <<  vertex << " to myPs"; 
	myPs.append(vertex); 
}


ilist Vertex::Ps(void) {
	 return myPs;
}

Vertex::~Vertex() {
	m_outEdges.clear();
}
