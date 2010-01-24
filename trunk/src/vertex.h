/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.81
 Written in Qt 4.4   
 
                         vertex.h  -  description
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

#ifndef VERTEX_H
#define VERTEX_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>
#include <map>

using namespace std;

class QPointF;
class Graph;

typedef map<int,float> imap_f;
typedef QHash<int,QString> ihash_s;
typedef QHash<int,int> ihash_i;  
typedef QList<int> ilist;



class Vertex : public QObject{
	Q_OBJECT

public:

	Vertex(	Graph* parent, 
				int v1,  int val, int size, QString color, 
				QString numColor, int numSize, 
				QString label, QString labelColor, int labelSize,
				QPointF p, 
				QString shape);

	Vertex(int v1);

	~Vertex();

	unsigned long int name();
	void setName (unsigned long int ); 

	void addLinkTo (unsigned long int target, float weight);	/** Adds an outLink to target with weight w */
	void addLinkFrom(unsigned long int source, float weight);

	void changeLinkWeightTo (unsigned long int target, float weight);

	void removeLinkTo (unsigned long int target);		/** Removes edge to vertex t */
	void removeLinkFrom(unsigned long int source);	/** Removes edge from vertex s	*/
	
	unsigned long int outDegree();	/**Returns the number of outward directed graph edges from this vertex   */
	unsigned long int inDegree();		/**Returns the number of inward directed graph edges from this vertex   */
	unsigned long int localDegree();
	
	/** Returns true if there is a reciprocal link from this vertex */
	bool isReciprocalLinked() { return m_reciprocalLinked;}
	void setReciprocalLinked(bool reciprocal) { m_reciprocalLinked=reciprocal;}

	/** Returns true if there is an outLink from this vertex */
	bool isOutLinked() { return m_outLinked;}
	float isLinkedTo(unsigned long int V);		/**Returns the weight of the link from to vertexc V, otherwise zero*/

	void filterEdgesByWeight(float m_threshold, bool overThreshold);
//	void filterEdgesByColor(float m_threshold, bool overThreshold);
	
	void setOutLinked(bool outLinked) { m_outLinked=outLinked;}
	/** Returns true if there is an outLink from this vertex */
	bool isInLinked() { return m_inLinked;}
	void setInLinked(bool inLinked) { m_inLinked=inLinked;}
	float isLinkedFrom (unsigned long int v);
	

	void setSize(int );
	int  size();
	
	
	void setShape(QString);
	QString shape();

	void setColor(QString);
	QString color();

	void setNumberColor (QString);
	QString numberColor();

	void setNumberSize (int);
	int numberSize();
	

	void setLabel (QString);
	QString label();

	void setLabelColor (QString);
	QString labelColor();
	
	void setLabelSize(int);
	int labelSize();


	void setX(float );
	float x();

	void setY(float );
	float y();
	
	QPointF pos ();


	void setOutLinkColor(unsigned long int, QString);
	QString outLinkColor(int);

	void setDelta (float c){ m_delta=c;} 		/** Sets vertex pair dependancy */
	float delta() { return m_delta;}		/** Returns vertex pair dependancy */
	
	void clearPs()	;
	
	void appendToPs(unsigned long  int vertex ) ;
	ilist Ps(void);
	
	void setODC (float c){ m_ODC=c;} 	/** Sets vertex Out-Degree Centrality*/
	void setSODC (float c ) { m_SODC=c;}	/** Sets standard vertex Out-Degree Centrality*/
	float ODC() { return m_ODC;}		/** Returns vertex Out-Degree Centrality*/
	float SODC() { return m_SODC; }	/** Returns standard vertex Out-Degree Centrality*/

	void setIDC (float c){ m_IDC=c;} 	/** Sets vertex In-Degree Centrality*/
	void setSIDC (float c ) { m_SIDC=c;}	/** Sets standard vertex In-Degree Centrality*/
	float IDC() { return m_IDC;}		/**Returns vertex In-Degree Centrality*/
	float SIDC() { return m_SIDC; }	/**Returns standard vertex In-Degree Centrality*/


	void setCC (float c){ m_CC=c;}		/**sets vertex Closeness Centrality*/
	void setSCC (float c ) { m_SCC=c;}	/**sets standard vertex Closeness Centrality*/
	float CC() { return m_CC;}		/**Returns vertex Closeness Centrality*/
	float SCC() { return m_SCC; }	/**Returns standard vertex Closeness Centrality*/

	void setBC(float c){ m_BC=c;}		/**sets s vertex Betweeness Centrality*/
	void setSBC (float c ) { m_SBC=c;}	/**sets standard vertex Betweeness Centrality*/
	float BC() { return m_BC;}		/**Returns vertex Betweeness Centrality*/
	float SBC() { return m_SBC; }	/**Returns standard vertex Betweeness Centrality*/

	void setGC (float c){ m_GC=c;}		/**sets vertex Graph Centrality*/
	void setSGC (float c ) { m_SGC=c;}	/**sets standard vertex Graph Centrality*/
	float GC() { return m_GC;}		/**Returns vertex Graph Centrality*/
	float SGC() { return m_SGC; }	/**Returns standard vertex Graph Centrality*/

	void setSC (float c){ m_SC=c;}  	/**sets vertex Stress Centrality*/
	void setSSC (float c ) { m_SSC=c;}	/**sets standard vertex Stress Centrality*/
	float SC() { return m_SC;}		/**Returns vertex Stress Centrality*/
	float SSC() { return m_SSC; }	/**Returns standard vertex Stress Centrality*/

	void setEC(float dist) { m_EC=dist;}/** Sets max Geodesic Distance to all other vertices*/
	void setSEC(float c) {m_SEC=c;}
	float EC() { return m_EC;}		/** Returns max Geodesic Distance to all other vertices*/
	float SEC() { return m_SEC;}

	float CLC() { return m_CLC;	}
	void setCLC(float clucof)  { m_CLC=clucof; m_hasCLC=TRUE; }
	bool hasCLC() { 	return m_hasCLC; }

	imap_f m_outEdges;			//holds all edges starting from this vertex.
	imap_f m_inEdges;			//holds all edges starting from this vertex.
	ihash_i m_enabled_outEdges;
signals:
	void setEdgeVisibility ( int, int, bool);
		
protected:

private:
	Graph *parentGraph;
	ilist myPs;
	unsigned long int m_name,  m_outLinks, m_inLinks; 
	int m_value, m_size, m_labelSize, m_numberSize;
	bool m_inLinked, m_outLinked, m_reciprocalLinked, m_hasCLC;
	QString m_color, m_numberColor, m_label, m_labelColor, m_shape;
	//QString *outLinkColors;
	ihash_s outLinkColors;
	//FIXME vertice coords
	
	double m_x, m_y;
	float m_CLC;
	float m_delta, m_EC, m_SEC;
	float m_ODC, m_SODC, m_IDC, m_SIDC, m_CC, m_SCC, m_BC, m_SBC, m_GC, m_SGC, m_SC, m_SSC;

};

#endif
