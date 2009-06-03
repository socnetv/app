/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.70
 Written in Qt 4.4
 
                         analyzer.h  -  description
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

#ifndef __ANALYZER_H__
#define __ANALYZER_H__


using namespace std;

#include <QThread>
#include <stack>  //FYI: stack is a wrapper around <deque> in C++, see: www.cplusplus.com/reference/stl/stack
#include <queue>		//for BFS queue Q

#include "vertex.h"
#include "matrix.h"
#include "parser.h"

typedef map<float,int> fmap_i;		// used by discrete*
typedef map<int,int> imap_i;		// used by index
typedef QList<Vertex*>  Vertices;

/** 	
	Main class for distance matrix creation 
*/
class Analyzer :  public QThread {
	Q_OBJECT
public:
	void load (Vertices &, Matrix &, Matrix &, imap_i &, int &, float &, int , int, int, int, int, bool, bool, bool);
	void createDistanceMatrix ();
	void BFS(int, bool);				//Breadth-first search 
	void minmax(float C, Vertex *v, float &max, float &min, int &maxNode, int &minNode) ;	//helper
	void resolveClasses(float C, fmap_i &discreteClasses, int &classes);			//helper
	void resolveClasses(float C, fmap_i &discreteClasses, int &classes, int name);  	//helper

signals:

protected:
	void run();
private:
	Vertices m_graph;
	imap_i index;	
	Matrix  *TM, *DM;
	stack<int> Stack;

	bool isCreateDistanceMatrix, isCalcCentralities, symmetricAdjacencyMatrix;
	int totalEdges, totalVertices, nonZeroDistancesCounter;
	int outEdgesVert, inEdgesVert, reciprocalEdgesVert, graphDiameter ;
	float averGraphDistance;
	
	
	
	fmap_i	discreteIDCs, discreteODCs, discreteCCs, discreteBCs, discreteSCs, discreteGCs, discreteECs;
	float meanDegree, varianceDegree;
	float minIDC, maxIDC, sumIDC, groupIDC;
	float minODC, maxODC, sumODC, groupODC;
	float minCC, maxCC, nomCC, denomCC, sumCC, groupCC, maxIndexCC;
	float minBC, maxBC, nomBC, denomBC, sumBC, groupBC, maxIndexBC;
	float minGC, maxGC, nomGC, denomGC, sumGC, groupGC, maxIndexGC;
	float minSC, maxSC, nomSC, denomSC, sumSC, groupSC, maxIndexSC;
	float minEC, maxEC, nomEC, denomEC, sumEC, groupEC, maxIndexEC;
	float minCLC, maxCLC, averageCLC;
	int maxNodeCLC, minNodeCLC;
	int classesIDC, maxNodeIDC, minNodeIDC;
	int classesODC, maxNodeODC, minNodeODC;
	int classesCC, maxNodeCC, minNodeCC;
	int classesBC, maxNodeBC, minNodeBC;
	int classesGC, maxNodeGC, minNodeGC;
	int classesSC, maxNodeSC, minNodeSC;
	int classesEC, maxNodeEC, minNodeEC;

 	
};



#endif 
