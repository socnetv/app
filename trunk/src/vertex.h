/***************************************************************************
 SocNetV: Social Networks Visualizer
 version: 1.3
 Written in Qt
 
                         vertex.h  -  description
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

typedef QList<int> ilist;


typedef QPair <float, bool> pair_f_b;
typedef QPair <int, pair_f_b > rel_w_bool;
typedef QHash < int, rel_w_bool > QHash_edges;

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

    long int name();
    void setName (long int );

    void setEnabled (bool flag );
    bool isEnabled ();

    void changeRelation(int) ;

    void addLinkTo (long int target, float weight);	/* Adds an outLink to target with weight w */
    void addLinkFrom(long int source, float weight);

    void changeLinkWeightTo (long int target, float weight);

    void setOutLinkEnabled (long int, bool);

    void removeLinkTo (long int target);		/* Removes edge to vertex t */
    void removeLinkFrom(long int source);	/* Removes edge from vertex s	*/

    long int outLinks();
    QHash<int,float>* returnEnabledOutLinks();

    long int inLinks();

    long int outDegree();
    long int inDegree();
    long int localDegree();

    void setEccentricity (float c){ m_Eccentricity=c;}		/* sets eccentricity */
    float eccentricity() { return m_Eccentricity;}		/* Returns eccentricity */

    /* Returns true if there is a reciprocal link from this vertex */
    bool isReciprocalLinked() { return m_reciprocalLinked;}
    void setReciprocalLinked(bool reciprocal) { m_reciprocalLinked=reciprocal;}

    /* Returns true if there is an outLink from this vertex */
    bool isOutLinked() { return m_outLinked;}
    void setOutLinked(bool outLinked) { m_outLinked=outLinked;}
    float isLinkedTo(long int V);	/* Returns the weight of the link to vertex V, otherwise zero*/

    /* Returns true if there is an outLink from this vertex */
    bool isInLinked() { return m_inLinked;}
    void setInLinked(bool inLinked) { m_inLinked=inLinked;}
    float isLinkedFrom (long int v);

    bool isIsolated() { return !(m_outLinked | m_inLinked) ; }
    void setIsolated(bool isolated) {m_isolated = isolated; }

    void filterEdgesByWeight(float m_threshold, bool overThreshold);
    //	void filterEdgesByColor(float m_threshold, bool overThreshold);
    void filterEdgesByRelation(int relation, bool status);

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

    void setOutLinkColor(long int, QString);
    QString outLinkColor(int);

    void setDelta (float c){ m_delta=c;} 		/* Sets vertex pair dependancy */
    float delta() { return m_delta;}		/* Returns vertex pair dependancy */

    void clearPs()	;

    void appendToPs(long  int vertex ) ;
    ilist Ps(void);

    void setDC (float c){ m_DC=c;} 	/* Sets vertex Degree Centrality*/
    void setSDC (float c ) { m_SDC=c;}	/* Sets standard vertex Degree Centrality*/
    float DC() { return m_DC;}		/* Returns vertex Degree Centrality*/
    float SDC() { return m_SDC;}		/* Returns standard vertex Degree Centrality*/

    void setCC (float c){ m_CC=c;}		/* sets vertex Closeness Centrality*/
    void setSCC (float c ) { m_SCC=c;}	/* sets standard vertex Closeness Centrality*/
    float CC() { return m_CC;}		/* Returns vertex Closeness Centrality*/
    float SCC() { return m_SCC; }		/* Returns standard vertex Closeness Centrality*/

    void setIRCC (float c){ m_IRCC=c;}		/* sets vertex IRCC */
    void setSIRCC (float c ) { m_SIRCC=c;}	/* sets standard vertex IRCC */
    float IRCC() { return m_IRCC;}		/* Returns vertex IRCC */
    float SIRCC() { return m_SIRCC; }		/* Returns standard vertex IRCC*/

    void setBC(float c){ m_BC=c;}		/* sets s vertex Betweeness Centrality*/
    void setSBC (float c ) { m_SBC=c;}	/* sets standard vertex Betweeness Centrality*/
    float BC() { return m_BC;}		/* Returns vertex Betweeness Centrality*/
    float SBC() { return m_SBC; }		/* Returns standard vertex Betweeness Centrality*/

    void setSC (float c){ m_SC=c;}  	/* sets vertex Stress Centrality*/
    void setSSC (float c ) { m_SSC=c;}	/* sets standard vertex Stress Centrality*/
    float SC() { return m_SC;}		/* Returns vertex Stress Centrality*/
    float SSC() { return m_SSC; }		/* Returns standard vertex Stress Centrality*/

    void setEC(float dist) { m_EC=dist;}	/* Sets max Geodesic Distance to all other vertices*/
    void setSEC(float c) {m_SEC=c;}
    float EC() { return m_EC;}		/* Returns max Geodesic Distance to all other vertices*/
    float SEC() { return m_SEC;}

    void setPC (float c){ m_PC=c;}		/* sets vertex Power Centrality*/
    void setSPC (float c ) { m_SPC=c;}	/* sets standard vertex Power Centrality*/
    float PC() { return m_PC;}		/* Returns vertex Power Centrality*/
    float SPC() { return m_SPC; }		/* Returns standard vertex Power Centrality*/

    void setIC (float c){ m_IC=c;}		/* sets vertex Information Centrality*/
    void setSIC (float c ) { m_SIC=c;}	/* sets standard vertex Information Centrality*/
    float IC() { return m_IC;}		/* Returns vertex Information  Centrality*/
    float SIC() { return m_SIC; }		/* Returns standard vertex Information Centrality*/

    void setDP (float c){ m_DP=c;} 	/* Sets vertex Degree Prestige */
    void setSDP (float c ) { m_SDP=c;}	/* Sets standard vertex Degree Prestige */
    float DP() { return m_DP;}		/* Returns vertex Degree Prestige */
    float SDP() { return m_SDP;}		/* Returns standard vertex Degree Prestige */

    void setPRC (float c){ m_PRC=c;}		/* sets vertex PageRank*/
    void setSPRC (float c ) { m_SPRC=c;}	/* sets standard vertex PageRank*/
    float PRC() { return m_PRC;}		/* Returns vertex PageRank */
    float SPRC() { return m_SPRC; }		/* Returns standard vertex PageRank*/

    void setPP (float c){ m_PP=c;}		/* sets vertex Proximity Prestige */
    void setSPP (float c ) { m_SPP=c;}	/* sets standard vertex Proximity Prestige */
    float PP() { return m_PP;}		/* Returns vertex Proximity Prestige */
    float SPP() { return m_SPP; }		/* Returns standard vertex Proximity Prestige */

    float CLC() { return m_CLC;	}
    void setCLC(float clucof)  { m_CLC=clucof; m_hasCLC=true; }
    bool hasCLC() { 	return m_hasCLC; }

    //hold all outbound and inboud edges of this vertex.
    QHash_edges m_outLinks, m_inLinks;
signals:
    void setEdgeVisibility ( int, int, bool);

protected:

private:

    Graph *parentGraph;
    ilist myPs;
    long int m_name,  m_outLinksCounter, m_inLinksCounter, m_outDegree, m_inDegree, m_localDegree;
    float m_Eccentricity;
    int m_value, m_size, m_labelSize, m_numberSize, m_curRelation;
    bool m_inLinked, m_outLinked, m_reciprocalLinked, m_enabled, m_hasCLC, m_isolated;
    QString m_color, m_numberColor, m_label, m_labelColor, m_shape;
    //QString *outLinkColors;
    ihash_s outLinkColors;
    //FIXME vertice coords

    double m_x, m_y;
    float m_CLC;
    float m_delta, m_EC, m_SEC;
    float m_DC, m_SDC, m_DP, m_SDP, m_CC, m_SCC, m_BC, m_SBC, m_IRCC, m_SIRCC, m_SC, m_SSC;
    float m_PC, m_SPC, m_SIC, m_IC, m_SPRC, m_PRC;
    float m_PP, m_SPP;

};

#endif
