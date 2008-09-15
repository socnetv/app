/***************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.48
 Written in Qt 4.4
 
                         graph.h  -  description
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

#ifndef GRAPH_H
#define GRAPH_H

#include <QMainWindow> //for parent
#include <QObject>

#include "vertex.h"
#include "matrix.h"

#include <stack> 
//FYI: stack is a wrapper around <deque> in C++, see: www.cplusplus.com/reference/stl/stack

#include <map>
#include <QList>
#include <QTextStream>
#include "parser.h"



using namespace std;


class QPointF;


/**	This is the main class for a Graph, used in conjuction with Vertex and Matrix objects.
	
	Graph class has the interface and the various network algorithms 
	Vertex class holds each vertex data (colors, strings, statistics, etc)
	Matrix class holds the adjacency matrix of the network.
	
*/

typedef map<float,int> fmap_i;
typedef map<int,int> imap_i;

class Graph:  public QObject{
	Q_OBJECT
public: 	
    	Graph(); /** adds a new Vertex named v1 to m_graph, with:
	         * Value=val, Size=nsz, Color=nc, Label=nl, labelColor=lc, Shape=nsp
	         * at Point=p */
	int loadFile(QString, int, QString, QString, QString, bool, int maxWidth, int maxHeight);
	
	void addVertex (int v1, int val, int nsz, QString nc, QString nl, QString lc, QPointF p,QString nsp);	
	void addVertex (int v1);		//adds a new Vertex named v1 to m_graph

	int lastVertexNumber();		//returns the number of the last vertex
	int firstVertexNumber();		//returns the number of the first vertex
	
	void removeVertex (int );		//removes given vertex from m_graph
	//adds an edge between v1 and v2, weight w, colored
	void addEdge (int v1, int v2, int w, QString color, bool undirected); 
	void setEdgeWeight (int v1, int v2, int w); 		//sets the edge weight between v1 and v2
	void removeEdge (int v1, int v2);			//removes the edge between v1 and v2

	int hasVertex(int );			//Checks if a vertex exists
	int hasVertex(QString);		//Checks if a vertex with a label exists
	int hasEdge (int v1, int v2);		//Checks if edge between v1 and v2 exists. Returns weight

	void setInitVertexSize (int); //Changes the init size used by all new vertices.
	
	void setInitVertexShape (QString); //Changes the init shape used by all new vertices.
	void setVertexShape(int v, QString shape); //Changes the shape.of vertex v 
	QString shape(int v);	//returns the shape of this vertex

	void setInitVertexLabelColor(QString color); //Changes the init color used by all new vertices' labels
	void setVertexLabel(int v, QString label); //Changes the label.of vertex v 
	QString label(int);

	void setInitVertexColor (QString color);  //Changes the init color used by all new vertices

	void setVertexColor(int v, QString color); //Changes the color.of vertex v 
	void setInitEdgeColor(QString);
	void updateVertCoords(int v, int x, int y); //Updates vertex v with coords x,y

	void setEdgeColor(int s, int t, QString color);	//Changes the color of edge (s,t).

	int edgesFrom (int i) ;			//Returns the number of edges starting from v1
	int edgesTo (int i) ;			//Returns the number of edges ending to v1  
	int totalEdges ();			//Returns the sum of edges inside m_graph
	int vertices() ;			//Returns the sum of vertices inside m_graph
	int verticesWithOutEdges();		//Returns the sum of vertices having outEdges
	int verticesWithInEdges();		//Returns the sum of vertices having inEdges
	int verticesWithReciprocalEdges();	//Returns the sum of vertices having reciprocal edges
	void writeAdjacencyMatrixTo(QTextStream& os); 		//exports the adjacency matrix to a given textstream
	void writeAdjacencyMatrix(const char*, const char*);		//Writes the adjacency matrix to a given file.
	friend QTextStream& operator <<  (QTextStream& os, Graph& m);  //

	bool isSymmetric();			//Returns TRUE if symmetricAdjacencyMatrix=TRUE
	int distance( int, int);		//Returns the geodesic distance between two vertices
	int diameter();				//Returns the diameter of the graph
	void minmax(float C, Vertex *v, float &max, float &min, int &maxNode, int &minNode) ;
	void resolveClasses(float C, fmap_i &discreteClasses, int &classes);
	void resolveClasses(float C, fmap_i &discreteClasses, int &classes, int name);  

	//Creates the distance matrix and calculates the centralities, if bool is true.
	void createDistanceMatrix(bool);		
	void BFS(int, bool);			// Breadth-first search used by createDistanceMatrix()

	void writeDistanceMatrix(const char*, const char*, const char*);//Writes the distance matrix to a file

	void eccentr_JordanCenter(); // FIXME ?
	
	void makeEdgesReciprocal();

	void centralityInDegree(bool);		//Calculates the inDegree centrality of each vertex
	void centralityOutDegree(bool);		//Calculates the outDegree centrality of each vertex
	void writeCentralityInDegree();		//Writes the in-degree centralities to a file
	void writeCentralityOutDegree();	//Writes the out-degree centralities to a file

	void createUniformRandomNetwork(int, int);	//Creates a uniform random network
	void createPhysicistLatticeNetwork(int, int, double, double, double); 	//Creates a Circular lattice
	void createSameDegreeRandomNetwork(int, int); 	//Creates a random network with the same degree in all nodes
	
	void layoutCircleCentrality(double x0, double y0, double maxRadius, int CentralityType);
	void layoutLevelCentrality(double maxWidth, double maxHeight, int CentralityType);
	void clear();				//Clears m_graph
	~Graph();
	
	QList<Vertex*> m_graph;	//List of pointers to the vertices. 
	//A vertex stores all the info: outLinks, colours, centralities, positions, etc

	int m_totalEdges, m_totalVertices;

	/** index stores the index of each vertex inside m_graph. It starts at zero (0).
	This is crucial when we want to find the place of a vertex inside m_graph after adding or removing many vertices */
	imap_i index;			

	/** maps have O(logN) lookup complexity
		Consider using tr1::hashmap which has O(1) lookup, but this is not ISO C++ yet :(  
	*/
	fmap_i	discreteIDCs, discreteODCs, discreteCCs, discreteBCs, discreteSCs, discreteGCs, discreteECs;
	float meanDegree, varianceDegree;
	float minIDC, maxIDC, sumIDC, groupIDC;
	float minODC, maxODC, sumODC, groupODC;
	float minCC, maxCC, nomCC, denomCC, sumCC, groupCC, maxIndexCC;
	float minBC, maxBC, nomBC, denomBC, sumBC, groupBC, maxIndexBC;
	float minGC, maxGC, nomGC, denomGC, sumGC, groupGC, maxIndexGC;
	float minSC, maxSC, nomSC, denomSC, sumSC, groupSC, maxIndexSC;
	float minEC, maxEC, nomEC, denomEC, sumEC, groupEC, maxIndexEC;
	int classesIDC, maxNodeIDC, minNodeIDC;
	int classesODC, maxNodeODC, minNodeODC;
	int classesCC, maxNodeCC, minNodeCC;
	int classesBC, maxNodeBC, minNodeBC;
	int classesGC, maxNodeGC, minNodeGC;
	int classesSC, maxNodeSC, minNodeSC;
	int classesEC, maxNodeEC, minNodeEC;
	bool calculatedIDC, calculatedODC, calculatedCentralities;

	void setParent(QMainWindow*);
	QMainWindow* parent();
	


public slots:
	//called by Parser
	void createNode(int,int,QString, QString, QString, QPointF, QString, bool);
	void createEdge (int, int, int, QString, bool, bool, bool);
	void createEdge (int, int, int);
	void fileType(int, QString, int,int);	
	void removeDummyNode(int);
	void parserFinished();

signals:
	void addBackgrCircle(int, int, int);
	void addBackgrHLine (int);
protected: 

private:
	Matrix  SM, TM, PM, DM;
	int *test;
	stack<int> Stack;
	Parser parser;	
	bool order;
	int *eccentricities;
	int graphDiameter;
	//These are used in the calculation of standard betweeness centrality on directed graphs:
	int outEdgesVert, inEdgesVert, reciprocalEdgesVert;
	QMainWindow *m_parent;
	bool adjacencyMatrixCreated, symmetricAdjacencyMatrix, graphModified, distanceMatrixCreated;
	QString networkName, initEdgeColor, initVertexColor, initVertexLabelColor, initVertexShape;
	int initVertexSize;
};

#endif
