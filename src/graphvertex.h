/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.2
 Written in Qt
 
                         graphvertex.h  -  description
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

#ifndef GRAPHVERTEX_H
#define GRAPHVERTEX_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QMultiHash>
#include <QList>
#include <QPointF>
#include <QPair>

#include <map>

using namespace std;

class Graph;


typedef QList<int> L_int;

typedef QHash<int,QString> H_IntToStr;
typedef QHash <QString, int> H_StrToInt;

typedef QPair <qreal, bool> pair_f_b;
typedef QPair <int, pair_f_b > pair_i_fb;
typedef QMultiHash < int, pair_i_fb > H_edges;

typedef QPair <int, qreal > pair_i_f;
typedef QHash < int, pair_i_f > H_distance;

typedef QPair <int, int> pair_i_i;
typedef QHash < int, pair_i_i > H_shortestPaths;


class GraphVertex : public QObject{
    Q_OBJECT

public:

    GraphVertex(Graph* parentGraph,
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
           const QString &iconPath,
           const int &edgesEstimate = 2000);

    GraphVertex(const int &name);

    ~GraphVertex();

    int number() const;
    void setNumber(const int &number);

    void setEnabled(const bool &status);
    bool isEnabled () const;

    void setSize(const int &size );
    int size() const;

    void setShape(const QString &shape, const QString &iconPath = QString());
    QString shape() const;
    QString shapeIconPath();

    void setColor(const QString &color);
    QString color() const;
    QString colorToPajek();

    void setNumberColor (const QString &color);
    QString numberColor() const;

    void setNumberSize (const int &size);
    int numberSize() const;

    void setNumberDistance (const int &distance);
    int numberDistance() const;

    void setLabel (const QString &label);
    QString label() const;

    void setLabelColor (const QString &labelColor);
    QString labelColor() const;

    void setLabelSize(const int &size);
    int labelSize() const;

    void setLabelDistance (const int &distance);
    int labelDistance() const;

    void setX(const qreal &x);
    qreal x() const;

    void setY(const qreal &y);
    qreal y() const;

    void setPos (QPointF &p);
    QPointF pos () const;

    void set_dispX (qreal x);
    void set_dispY (qreal y);
    QPointF & disp();

    void setRelation(int newRel) ;

    void addOutEdge (const int &v2, const qreal &weight, const QString &color=QString(), const QString &label=QString());
    qreal hasEdgeTo(const int &v, const bool &allRelations=false);
    void removeOutEdge (const int target);
    void setOutEdgeWeight (const int &target, const qreal &weight);
    void setOutEdgeEnabled (const int &target, bool);
    void setOutLinkColor(const int &v2, const QString &color);
    QString outLinkColor(const int &v2);
    void setOutEdgeLabel(const int &v2, const QString &label);
    QString outEdgeLabel(const int &v2) const;

    void addInEdge(const int &v1, const qreal &weight);
    qreal hasEdgeFrom (const int &v, const bool &allRelations=false);
    void removeInEdge(const int source);
    void setInEdgeWeight (const int &source, const qreal &weight);
    void setInEdgeEnabled (const int &source, bool);

    int outEdgesCount();
    int outEdgesCountConst() const ;

    int inEdgesCount();
    int inEdgesCountConst() const ;

    bool isOutLinked();
    bool isInLinked();

    void setIsolated(bool isolated);
    bool isIsolated();

    QHash<int, qreal> outEdgesEnabledHash(const bool &allRelations=false);
    QHash<int,qreal>* outEdgesAllRelationsUniqueHash();
    QHash<int,qreal>* inEdgesEnabledHash();
    QHash<int,qreal> reciprocalEdgesHash();
    QList<int> neighborhoodList();

    int degreeOut();
    int outDegreeConst();
    int degreeIn();
    int inDegreeConst();
    int localDegree();

    void setEnabledEdgesByRelation(const int relation, const bool status);
    void setEnabledUnilateralEdges(const bool &status=false);

    qreal distance(const int &v1) ;
    void setDistance (const int &v1, const qreal &d) ;
    void reserveDistance(const int &N);
    void clearDistance();

    int shortestPaths(const int &v1) ;
    void setShortestPaths(const int &v1, const int &sp) ;
    void reserveShortestPaths(const int &N);
    void clearShortestPaths();

    void setEccentricity(const qreal &c);
    qreal eccentricity();

    void setDelta(const qreal &c);
    qreal delta();

    void clearPs()	;

    void appendToPs(const int &vertex ) ;
    L_int Ps(void);

    //used in reciprocity report
    void setOutEdgesReciprocated(int outEdgesSym=-1) { m_outEdgesSym = (outEdgesSym!=-1) ?  outEdgesSym :  m_outEdgesSym+1;  }
    int outEdgesReciprocated() { return m_outEdgesSym; }

    void setOutEdgesNonSym(int outEdgesNonSym=-1) { m_outEdgesNonSym = (outEdgesNonSym!=-1) ?  outEdgesNonSym :  m_outEdgesNonSym+1;  }
    int outEdgesNonSym() { return m_outEdgesNonSym;  }

    void setInEdgesNonSym(int inEdgesNonSym=-1) { m_inEdgesNonSym = (inEdgesNonSym!=-1) ?  inEdgesNonSym :  m_inEdgesNonSym+1;  }
    int inEdgesNonSym() { return m_inEdgesNonSym; }

    void setDC (const qreal &c){ m_DC=c;} 	/* Sets vertex Degree Centrality*/
    void setSDC (const qreal &c ) { m_SDC=c;}	/* Sets standard vertex Degree Centrality*/
    qreal DC() { return m_DC;}          /* Returns vertex Degree Centrality*/
    qreal SDC() { return m_SDC;}		/* Returns standard vertex Degree Centrality*/

    void setDistanceSum (const qreal &c) { m_distanceSum = c; }
    qreal distanceSum () { return m_distanceSum; }
    void setCC (const qreal &c){ m_CC=c;}		/* sets vertex Closeness Centrality*/
    void setSCC (const qreal &c ) { m_SCC=c;}	/* sets standard vertex Closeness Centrality*/
    qreal CC() { return m_CC;}		/* Returns vertex Closeness Centrality*/
    qreal SCC() { return m_SCC; }		/* Returns standard vertex Closeness Centrality*/

    void setIRCC (const qreal &c){ m_IRCC=c;}		/* sets vertex IRCC */
    void setSIRCC (const qreal &c ) { m_SIRCC=c;}	/* sets standard vertex IRCC */
    qreal IRCC() { return m_IRCC;}		/* Returns vertex IRCC */
    qreal SIRCC() { return m_SIRCC; }		/* Returns standard vertex IRCC*/

    void setBC(const qreal &c){ m_BC=c;}		/* sets s vertex Betweenness Centrality*/
    void setSBC (const qreal &c ) { m_SBC=c;}	/* sets standard vertex Betweenness Centrality*/
    qreal BC() { return m_BC;}		/* Returns vertex Betweenness Centrality*/
    qreal SBC() { return m_SBC; }		/* Returns standard vertex Betweenness Centrality*/

    void setSC (const qreal &c){ m_SC=c;}  	/* sets vertex Stress Centrality*/
    void setSSC (const qreal &c ) { m_SSC=c;}	/* sets standard vertex Stress Centrality*/
    qreal SC() { return m_SC;}		/* Returns vertex Stress Centrality*/
    qreal SSC() { return m_SSC; }		/* Returns standard vertex Stress Centrality*/

    void setEC(const qreal &dist) { m_EC=dist;}	/* Sets max Geodesic Distance to all other vertices*/
    void setSEC(const qreal &c) {m_SEC=c;}
    qreal EC() { return m_EC;}		/* Returns max Geodesic Distance to all other vertices*/
    qreal SEC() { return m_SEC;}

    void setPC (const qreal &c){ m_PC=c;}		/* sets vertex Power Centrality*/
    void setSPC (const qreal &c ) { m_SPC=c;}	/* sets standard vertex Power Centrality*/
    qreal PC() { return m_PC;}		/* Returns vertex Power Centrality*/
    qreal SPC() { return m_SPC; }		/* Returns standard vertex Power Centrality*/

    void setIC (const qreal &c){ m_IC=c;}		/* sets vertex Information Centrality*/
    void setSIC (const qreal &c ) { m_SIC=c;}	/* sets standard vertex Information Centrality*/
    qreal IC() { return m_IC;}		/* Returns vertex Information  Centrality*/
    qreal SIC() { return m_SIC; }		/* Returns standard vertex Information Centrality*/

    void setDP (const qreal &c){ m_DP=c;} 	/* Sets vertex Degree Prestige */
    void setSDP (const qreal &c ) { m_SDP=c;}	/* Sets standard vertex Degree Prestige */
    qreal DP() { return m_DP;}		/* Returns vertex Degree Prestige */
    qreal SDP() { return m_SDP;}		/* Returns standard vertex Degree Prestige */

    void setPRP (const qreal &c){ m_PRC=c;}		/* sets vertex PageRank*/
    void setSPRP (const qreal &c ) { m_SPRC=c;}	/* sets standard vertex PageRank*/
    qreal PRP() { return m_PRC;}		/* Returns vertex PageRank */
    qreal SPRP() { return m_SPRC; }		/* Returns standard vertex PageRank*/

    void setPP (const qreal &c){ m_PP=c;}		/* sets vertex Proximity Prestige */
    void setSPP (const qreal &c ) { m_SPP=c;}	/* sets standard vertex Proximity Prestige */
    qreal PP() { return m_PP;}		/* Returns vertex Proximity Prestige */
    qreal SPP() { return m_SPP; }		/* Returns standard vertex Proximity Prestige */

    qreal CLC() { return m_CLC;	}
    void setCLC(const qreal &clucof)  { m_CLC=clucof; m_hasCLC=true; }
    bool hasCLC() {  return m_hasCLC; }

    void setEVC (const qreal &c){ m_EVC=c;} 	/* Sets vertex Degree Centrality*/
    void setSEVC (const qreal &c ) { m_SEVC=c;}	/* Sets standard vertex Degree Centrality*/
    qreal EVC() { return m_EVC;}		/* Returns vertex Degree Centrality*/
    qreal SEVC() { return m_SEVC;}		/* Returns standard vertex Degree Centrality*/


    int cliques (const int &ofSize);

    void cliqueAdd (const QList<int> &clique);

    void clearCliques() {  m_cliques.clear();    }

    //Hashes of all outbound and inbound edges of this vertex.
    H_edges m_outEdges, m_inEdges;

    //Hash dictionary of this vertex pair-wise distances to all other vertices for each relationship
    //The key is the relationship
    //The value is a QPair < int target, qreal weight >
    H_distance m_distance;

    H_shortestPaths m_shortestPaths;

signals:
    void signalSetEdgeVisibility (const int &relation,
                                  const int &name,
                                  const int &target,
                                  const bool &visible,
                                  const bool &preserveReverseEdge=false,
                                  const int &edgeWeight=1,
                                  const int &reverseEdgeWeight=1
                                  );

protected:

private:
    Graph *m_graph;
    int m_number,  m_outEdgesCounter, m_inEdgesCounter, m_outDegree, m_inDegree, m_localDegree;
    int m_outEdgesNonSym, m_inEdgesNonSym, m_outEdgesSym;
    int m_value, m_size, m_labelSize, m_numberSize, m_numberDistance, m_labelDistance;
    int m_curRelation;
    bool m_reciprocalLinked, m_enabled, m_hasCLC, m_isolated;
    double m_x, m_y;
    qreal m_Eccentricity, m_CLC;
    qreal m_delta, m_EC, m_SEC;
    qreal m_DC, m_SDC, m_DP, m_SDP, m_CC, m_SCC, m_BC, m_SBC, m_IRCC, m_SIRCC, m_SC, m_SSC;
    qreal m_PC, m_SPC, m_SIC, m_IC, m_SPRC, m_PRC;
    qreal m_PP, m_SPP, m_EVC, m_SEVC;
    qreal m_distanceSum;

    QString m_color, m_numberColor, m_label, m_labelColor, m_shape, m_iconPath;
    QPointF m_disp;

    QHash<int,qreal> m_reciprocalEdges;
    L_int myPs;
    QMultiHash <int, L_int> m_cliques;
    L_int m_neighborhoodList;
    H_IntToStr m_outLinkColors, m_outEdgeLabels;

    //FIXME vertex coords



};

#endif
