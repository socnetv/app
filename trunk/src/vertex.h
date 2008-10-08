/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.49
 Written in Qt 4.4 with KDevelop   
 
                         vertex.h  -  description
                             -------------------
    copyright            : (C) 2005-2008 by Dimitris B. Kalamaras
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


#include <QString>
#include <map>
#include <QHash>
#include <QList>

using namespace std;


class QPointF;

typedef map<int,int> imap_i;
typedef QHash<int,QString> ihash_s;


class Vertex {
public:
	Vertex(int v1, int val, int nsz, QString nc, QString nl, QString lc, QPointF p,QString nsp);
	Vertex(int v1);
	~Vertex();
	int name();
	void setName (int ); 

	void addLinkTo (int target, int w);	/** Adds an outLink to target with weight w */
	void addLinkFrom(int source);
	void changeLinkWeightTo (int target, int weight);
	void removeLinkTo (int target);		/** Removes specified edge to target vertex*/
	void removeLinkFrom(int source);
	int outLinks();			/**Returns the numbers of links from this vertex */
	int inLinks();		/**Returns the number of links to this vertex */

	/** Returns true if there is a reciprocal link from this vertex */
	bool isReciprocalLinked() { return m_reciprocalLinked;}
	void setReciprocalLinked(bool reciprocal) { m_reciprocalLinked=reciprocal;}

	/** Returns true if there is an outLink from this vertex */
	bool isOutLinked() { return m_outLinked;}
	int isLinkedTo(int V);		/**Returns the weight of the link from to vertexc V, otherwise zero*/

	void setOutLinked(bool outLinked) { m_outLinked=outLinked;}
	/** Returns true if there is an outLink from this vertex */
	bool isInLinked() { return m_inLinked;}
	void setInLinked(bool inLinked) { m_inLinked=inLinked;}
	

	void setSize(int );
	int  size();
	
	void setLabel (QString);
	QString label();
	
	void setShape(QString);
	QString shape();

	void setColor(QString);
	QString color();

	void setX(int );
	int  x();

	void setY(int );
	int y();
	
	void setOutLinkColor(int, QString);
	QString outLinkColor(int);

	void setDelta (float c){ m_delta=c;} 		/** Sets vertex pair dependancy */
	float delta() { return m_delta;}		/** Returns vertex pair dependancy */
	
	void clearPs()	;
	
	void appendToPs( int vertex ) ;

	QList<int> Ps() { return myPs;}
	
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

	imap_i m_edges;			
	
protected:

private:
	QList<int> myPs;
	int m_name, m_value, m_size, m_outLinks, m_inLinks;
	bool m_inLinked, m_outLinked, m_reciprocalLinked;
	QString m_color, m_label, m_labelColor, m_shape;
	//QString *outLinkColors;
	ihash_s outLinkColors;
	//FIXME vertice coords
	
	double m_x, m_y;
	float m_delta, m_EC, m_SEC;
	float m_ODC, m_SODC, m_IDC, m_SIDC, m_CC, m_SCC, m_BC, m_SBC, m_GC, m_SGC, m_SC, m_SSC;

};

#endif
