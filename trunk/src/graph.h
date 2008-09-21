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

#include <stack>  //FYI: stack is a wrapper around <deque> in C++, see: www.cplusplus.com/reference/stl/stack

#include <map>
#include <QList>
#include <QTextStream>
#include "parser.h"



using namespace std;


class QPointF;


/**	This is the main class for a Graph, used in conjuction with Vertex, Parser and Matrix objects.
	
	Graph class has the interface and the various network algorithms 
	Vertex class holds each vertex data (colors, strings, statistics, etc)
	Matrix class holds the adjacency matrix of the network.
	Parser class loads files of networks.
	
*/

typedef map<float,int> fmap_i;
typedef map<int,int> imap_i;

class Graph:  public QObject{
	Q_OBJECT

public slots:
	void createVertex(int,int,QString, QString, QString, QPointF, QString); 	//Main vertex creation call
	void createVertex(int i, QPointF p); 					//Called by GW
	void createVertex(int i, int canvasWidth, int canvasHeight); 		//Called by MW
	void createEdge (int, int, int, QString, bool, bool, bool);				//
	void createEdge (int, int, int);							//
	void fileType(int, QString, int,int);	
	void removeDummyNode(int);
	void parserFinished();


signals:
	void addBackgrCircle(int, int, int);
	void addBackgrHLine (int);
	void updateProgressDialog(int );
	void drawNode( int ,int,  QString, QString,QString, QPointF, QString, bool);

public: 	
	/**INIT AND CLEAR*/
    	Graph(); // adds a new Vertex named v1 to m_graph, with:
	         // Value=val, Size=nsz, Color=nc, Label=nl, labelColor=lc, Shape=nsp
	         // at Point=p */

	void clear();				//Clears m_graph
	~Graph();				//destroy

	void setParent(QMainWindow*);
	QMainWindow* parent();
	
	void setShowLabels(bool toggle);

	/**FILES (READ AND WRITE)*/
	int loadFile(QString, int, QString, QString, QString, bool, int maxWidth, int maxHeight);	//Almost universal network loader. :)
	

	/** VERTICES */
	int lastVertexNumber();						//Returns the number of the last vertex
	int firstVertexNumber();					//Returns the number of the first vertex

	int hasVertex(int );						//Checks if a vertex exists
	int hasVertex(QString);						//Checks if a vertex with a label exists
	void removeVertex (int );					//removes given vertex from m_graph

	void setInitVertexSize (int); 					//Changes the init size used by all new vertices.
	void setVertexSize(int v, int );				//Changes the size.of vertex v 

	void setInitVertexShape (QString); 				//Changes the init shape used by all new vertices.
	void setVertexShape(int v, QString shape); 			//Changes the shape.of vertex v 
	QString shape(int v);						//returns the shape of this vertex

	void setInitVertexColor (QString color);  			//Changes the init color used by all new vertices
	void setVertexColor(int v, QString color); 			//Changes the color.of vertex v 

	void setInitVertexLabelColor(QString color); 			//Changes the init color used by all new vertices' labels
	void setVertexLabel(int v, QString label); 			//Changes the label.of vertex v 
	QString label(int);			

	void updateVertCoords(int v, int x, int y);			 //Updates vertex v with coords x,y

	int vertices() ;						//Returns the sum of vertices inside m_graph

	int edgesFrom (int i) ;						//Returns the number of edges starting from v1
	int edgesTo (int i) ;						//Returns the number of edges ending to v1  

	int verticesWithOutEdges();					//Returns the sum of vertices having outEdges
	int verticesWithInEdges();					//Returns the sum of vertices having inEdges
	int verticesWithReciprocalEdges();				//Returns the sum of vertices having reciprocal edges


	/**EDGES*/
	int hasEdge (int v1, int v2);					//Checks if edge between v1 and v2 exists. Returns weight
	void removeEdge (int v1, int v2);				//removes the edge between v1 and v2
	void setEdgeWeight (int v1, int v2, int w); 			//sets the edge weight between v1 and v2
	void setInitEdgeColor(QString);

	void setEdgeColor(int s, int t, QString color);			//Changes the color of edge (s,t).

	int totalEdges ();						//Returns the sum of edges inside m_graph

	bool isSymmetric();						//Returns TRUE if symmetricAdjacencyMatrix=TRUE
	void makeEdgesReciprocal();					//Symmetrize all edges so that the network is undirected.


	/**PRINT OUT*/
	void writeAdjacencyMatrixTo(QTextStream& os);	 		//Exports the adjacency matrix to a given textstream
	void writeAdjacencyMatrix(const char*, const char*);		//Writes the adjacency matrix to a given file.
	void writeDistanceMatrix(const char*, const char*, const char*);//Writes the distance matrix to a file
	void writeCentralityInDegree();					//Writes the in-degree centralities to a file
	void writeCentralityOutDegree();				//Writes the out-degree centralities to a file

	friend QTextStream& operator <<  (QTextStream& os, Graph& m);  	//


	/**DISTANCES & CENTRALITIES*/
	int distance( int, int);				//Returns the geodesic distance between two vertices
	int diameter();						//Returns the diameter of the graph

	void createDistanceMatrix(bool);			//Creates the distance matrix and calculates the centralities, if bool is true.

	void centralityInDegree(bool);				//Calculates the inDegree centrality of each vertex
	void centralityOutDegree(bool);				//Calculates the outDegree centrality of each vertex

	void eccentr_JordanCenter(); 				// FIXME ?


	/**LAYOUTS*/	
	void layoutCircleCentrality(double x0, double y0, double maxRadius, int CentralityType);
	void layoutLevelCentrality(double maxWidth, double maxHeight, int CentralityType);


	/**RANDOM NETWORKS*/
	void createUniformRandomNetwork(int, int);				//Creates a uniform random network
	void createPhysicistLatticeNetwork(int, int, double, double, double); 	//Creates a Circular lattice
	void createSameDegreeRandomNetwork(int, int); 				//Creates a random network with the same degree in all nodes


	/** List of pointers to the vertices. A vertex stores all the info: links, colours, etc */
	QList<Vertex*> m_graph;			


	/** index stores the index of each vertex inside m_graph. It starts at zero (0).
	This is crucial when we want to find the place of a vertex inside m_graph after adding or removing many vertices */
	imap_i index;			
	/** maps have O(logN) lookup complexity
		Consider using tr1::hashmap which has O(1) lookup, but this is not ISO C++ yet :(  
	*/


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



protected: 

private:
	QMainWindow *m_parent;		//stores the parent of this class.
	Parser parser;		//file loader threaded class.

	/** private member functions */
	void addVertex (int v1, int val, int nsz, QString nc, QString nl, QString lc, QPointF p,QString nsp);	
	void addEdge (int v1, int v2, int w, QString color, bool undirected); 		//adds an edge between v1 and v2, weight w, colored
	void minmax(float C, Vertex *v, float &max, float &min, int &maxNode, int &minNode) ;
	void BFS(int, bool);			// Breadth-first search used by createDistanceMatrix()
	void resolveClasses(float C, fmap_i &discreteClasses, int &classes);
	void resolveClasses(float C, fmap_i &discreteClasses, int &classes, int name);  

	/** used in createDistanceMatrix() */
	fmap_i	discreteIDCs, discreteODCs, discreteCCs, discreteBCs, discreteSCs, discreteGCs, discreteECs;
	int *eccentricities;
	bool calculatedIDC, calculatedODC, calculatedCentralities;
	Matrix  TM, DM;
	stack<int> Stack;

	/** General & initialisation variables */
	int m_totalEdges, m_totalVertices, graphDiameter, initVertexSize;
	int outEdgesVert, inEdgesVert, reciprocalEdgesVert;
	
	bool order, initShowLabels;
	bool adjacencyMatrixCreated, symmetricAdjacencyMatrix, graphModified, distanceMatrixCreated;

	QString networkName, initEdgeColor, initVertexColor, initVertexLabelColor, initVertexShape;

};

#endif
