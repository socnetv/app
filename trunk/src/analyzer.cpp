/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.70
 Written in Qt 4.4
 
                         analyzer.cpp  -  description
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

 
#include "analyzer.h"


void Analyzer::load ( Vertices &mgraph, Matrix &mDM, Matrix &mTM, imap_i &mindex,
					int &mgraphDiameter, float &maverGraphDistance, 
					int mreciprocalEdgesVert, int minEdgesVert, int moutEdgesVert, 
					int mtotalEdges, int mtotalVertices, 
					bool msymmetric, 
					bool dm, bool calc_centr) {

	qDebug("*** Analyzer: load() passing graph...");
	m_graph=mgraph;
	DM = &mDM;
	TM = &mTM;
	index = mindex;	//no problem!
	graphDiameter = mgraphDiameter;	//return it somehow!
	averGraphDistance = maverGraphDistance;	//return it somehow!
	symmetricAdjacencyMatrix = msymmetric;
	reciprocalEdgesVert = mreciprocalEdgesVert;
	totalVertices=mtotalVertices;
	totalEdges = mtotalEdges;
	inEdgesVert = minEdgesVert;
	outEdgesVert = moutEdgesVert;
	isCreateDistanceMatrix = dm ;
	isCalcCentralities = calc_centr; 
	
	qDebug("Analyzer: OK. Now, start()ing a new QThread!");
	if (!isRunning()) 
		start(QThread::NormalPriority);
}



/** starts the new thread 
*/

void Analyzer::run()  {
	qDebug("**** QThread/Analyzer: This is a thread, running!");
	
	if ( isCreateDistanceMatrix  ) {
		qDebug() <<  " Analyzer: Will create a new distance matrix" ;
		createDistanceMatrix();
	}
	else{
	qDebug("**** QThread/Analyzer: end of routine!");
	}
}



void Analyzer::createDistanceMatrix(){
	
		QList<Vertex*>::iterator it, it1;	
		QList<int>::iterator it2;
		int w=0, u=0,s=0;
		float d_sw=0, d_su=0;	
		reciprocalEdgesVert=0;
		outEdgesVert=0;
		inEdgesVert=0;
		maxIndexBC=0;
		maxIndexSC=0;
		maxIndexEC=0;
		
		averGraphDistance=0;
		nonZeroDistancesCounter=0;
		//The following are for CC
		fmap_i::iterator it3; 

		float CC=0, BC=0, SC=0, GC=0, EC=0, stdGC=0, stdEC=0;
		qDebug("Graph: createDistanceMatrix() - initialising variables for maximum centrality indeces");
		if (symmetricAdjacencyMatrix) {
			maxIndexBC=( totalVertices-1.0) *  (totalVertices-2.0)  / 2.0;
			maxIndexSC=( totalVertices-1.0) *  (totalVertices-2.0) / 2.0;
			maxIndexCC=1.0/(totalVertices-1.0);
			maxIndexEC=totalVertices-1.0;
			qDebug("############# maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
		}
		else {	
			maxIndexBC= ( ( outEdgesVert-1.0) *  (inEdgesVert-2.0) - (reciprocalEdgesVert-1.0))/ 2.0;
			maxIndexSC=1;
			maxIndexEC=(totalVertices-1.0);
			maxIndexCC=1.0/(totalVertices-1.0);  //FIXME This applies only on undirected graphs
			qDebug("############# maxIndexBC %f, maxIndexCC %f, maxIndexSC %f", maxIndexBC, maxIndexCC, maxIndexSC);
		}
		//float maxIndexBC-directed= (n1-1) * (n2-1)-(ns-1) , n1  vert outgoing n2 ingoing vert ns self  // all this divided by two.
		qDebug("Graph: createDistanceMatrix() - initialising variables for centrality index");
		maxCC=0; minCC=RAND_MAX; nomCC=0; denomCC=0; groupCC=0; maxNodeCC=0; minNodeCC=0; sumCC=0;
		discreteCCs.clear(); classesCC=0;
		maxBC=0; minBC=RAND_MAX; nomBC=0; denomBC=0; groupBC=0; maxNodeBC=0; minNodeBC=0; sumBC=0;
		discreteBCs.clear(); classesBC=0;
		maxSC=0; minSC=RAND_MAX; nomSC=0; denomSC=0; groupSC=0; maxNodeSC=0; minNodeSC=0; sumSC=0;
		discreteSCs.clear(); classesSC=0;
		maxGC=0; minGC=RAND_MAX; nomGC=0; denomGC=0; groupGC=0; maxNodeGC=0; minNodeGC=0; sumGC=0;
		discreteGCs.clear(); classesGC=0;
		maxEC=0; minEC=RAND_MAX; nomEC=0; denomEC=0; groupEC=0; maxNodeEC=0; minNodeEC=0; sumEC=0;
		discreteECs.clear(); classesEC=0;
		
		//Zero closeness indeces of each vertex
		if (isCalcCentralities) 
			for (it=m_graph.begin(); it!=m_graph.end(); it++) {
				(*it)->setBC( 0.0 );
				(*it)->setSC( 0.0 );
				(*it)->setGC( 0.0 );
				(*it)->setCC( 0.0 );
		}
		qDebug("MAIN LOOP: for every s in V do (solve the single source shortest path problem...");
		for (it=m_graph.begin(); it!=m_graph.end(); it++){
			
			s=index[ (*it)->name() ];
			qDebug("Source vertex s=%i of BFS algorithm has index %i. Clearing Stack ...", (*it)->name(), s);
			if (isCalcCentralities){
				qDebug("Empty stack Stack which will return vertices in order of their (non increasing) distance from S ...");
				//- Complexity linear O(n) 
				while ( !Stack.empty() )  
					Stack.pop();
				qDebug("...and for each vertex: empty list Ps of predecessors");
				//Complexity linear O(n)
 				for (it1=m_graph.begin(); it1!=m_graph.end(); it1++) 
 					(*it1)->clearPs();
			}

			qDebug("PHASE 1 (SSSP): Call BFS for source vertex %i to determine distances and shortest path counts from s to every vertex t", (*it)->name());
			BFS(s,isCalcCentralities );
			qDebug("***** FINISHED PHASE 1 (SSSP) BFS ALGORITHM. Continuing to calculate centralities");
			if (isCalcCentralities){
				qDebug("Set centrality for current source vertex %i  with index s=%i", (*it)->name(), s);
				if ( (*it)->CC() != 0 ) //Closeness centrality must be inverted 	
					CC=1.0/(*it)->CC();
				else CC=0;
				(*it)->setSCC ( CC * ( totalVertices-1.0)  );
				(*it)->setCC( CC );
				//Resolve classes Closeness centrality
				qDebug("=========Resolving CC classes...");
				resolveClasses(CC, discreteCCs, classesCC,(*it)->name() );
				sumCC+=CC;
				minmax( CC, (*it), maxCC, minCC, maxNodeCC, minNodeCC) ;
				//And graph centrality must be inverted...
				if ( (*it)->GC() != 0 ) {
					EC=(*it)->GC();		//Eccentricity Centrality is max geodesic
					GC=1.0/EC;		//Graph Centrality is inverted Eccentricity
				}
				else { GC=0; EC=0;}
				(*it)->setGC( GC );		//Set Graph Centrality 
				(*it)->setEC( EC ); 		//Set Eccentricity Centrality 
				//Resolve classes Graph centrality
				resolveClasses(GC, discreteGCs, classesGC);
				stdGC =(totalVertices-1.0)*GC ;
				(*it)->setSGC(stdGC);
				sumGC+=GC;
				minmax( GC, (*it), maxGC, minGC, maxNodeGC, minNodeGC) ;

				stdEC =EC/(totalVertices-1.0);
				(*it)->setSEC(stdEC);
				sumEC+=EC;
				minmax( EC, (*it), maxEC, minEC, maxNodeEC, minNodeEC) ;
				
				
				qDebug("PHASE 2 (ACCUMULATION): Start back propagation of dependencies. Set dependency delta[u]=0 on each vertex");
				for (it1=m_graph.begin(); it1!=m_graph.end(); it1++){
					(*it1)->setDelta(0.0);
//					qDebug("vertex %i with index %i has delta = %F", (*it1)->name(),index[(*it1)->name()], (*it1)->delta());
				}

				qDebug("Visit all vertices in reverse order of their discovery (from s = %i) to sum dependencies. Initial Stack size has %i", s, Stack.size());

				while ( !Stack.empty() ) {
					w=Stack.top(); 
					qDebug("Stack top is vertex w=%i. This is the furthest vertex from s. Popping it.", w);
					Stack.pop();
					QList<int> lst=m_graph[w]->Ps();
					qDebug("preLOOP: Checking size of predecessors list Ps[w]...  = %i ",lst.size());
					qDebug("LOOP: for every other vertex u in the list of predecessors Ps[w] of w....");
					if (lst.size() > 0) // just in case...do a sanity check
						for ( it2=lst.begin(); it2 != lst.end(); it2++ ){
							u=(*it2);
							qDebug("Selecting Ps[w] element u=%i with delta_u=%f. sigma(u)=TM(s,u)=%i, sigma(w)=TM(s,w)=%i, delta_w=%f ", u, m_graph[u]->delta(),TM->item(s,u), TM->item(s,w), m_graph[w]->delta());
							if ( TM->item(s,w) > 0) {
								//delta[u]=delta[u]+(1+delta[w])*(sigma[u]/sigma[w]) ;
								d_su=m_graph[u]->delta()+(1.0+m_graph[w]->delta() ) * ( (float)TM->item(s,u)/(float)TM->item(s,w) );
							}
							else {
								d_su=m_graph[u]->delta();
								qDebug("TM (s,w) zero, i.e. zero shortest path counts from s to w - using SAME DELTA for vertex u");
							}
							qDebug("Assigning new delta d_su = %f to u = %i", d_su, u);
							m_graph[u]->setDelta( d_su);
						}
					qDebug()<<" Adding delta_w to BC of w";
					if  (w!=s) { 
						qDebug("w!=s. For this furthest vertex we need to add its new delta %f to old BC index: %f",m_graph[w]->delta(), m_graph[w]->BC());
						d_sw = m_graph[w]->BC() + m_graph[w]->delta();
						qDebug("New BC = d_sw = %f", d_sw);
						m_graph[w]->setBC (d_sw);
					}
				}
			}
		}
		if (averGraphDistance!=0)
			averGraphDistance = averGraphDistance / (nonZeroDistancesCounter);
		
		if (isCalcCentralities) {
			for (it=m_graph.begin(); it!=m_graph.end(); it++) {

				if (symmetricAdjacencyMatrix) {
					qDebug("Betweeness centrality must be divided by two if the graph is undirected");
					(*it)->setBC ( (*it)->BC()/2.0);
				}

				BC=(*it)->BC();
				//Resolve classes Betweeness centrality
				qDebug("Resolving BC classes...");
				resolveClasses(BC, discreteBCs, classesBC);
				//Store standard Betweeness 
				qDebug("******************* BC %f maxIndex: %f", BC, maxIndexBC);
				(*it)->setSBC( BC/maxIndexBC );   
				//Find min & max BC - not using stdBC:  Wasserman & Faust, pp. 191-192
				sumBC+=BC;
				minmax( BC, (*it), maxBC, minBC, maxNodeBC, minNodeBC) ;
				//Find denominal of groupBC
				nomBC +=(maxBC - BC );

				//Resolve classes Stress centrality
				SC=(*it)->SC();
				qDebug("Resolving SC classes...");
				resolveClasses(SC, discreteSCs, classesSC);
				//Store standard Stress centrality
				(*it)->setSSC ( SC/maxIndexSC );
				//Find min & max SC - not using stdSC:  Wasserman & Faust, pp. 191-192
				sumSC+=SC;
				minmax( SC, (*it), maxSC, minSC, maxNodeSC, minNodeSC) ;
				//Find denominal of groupSC
				nomSC +=(maxSC - SC );
				
				//Find denominal of groupGC
				nomGC += maxGC-(*it)->SGC();
				//Find denominal of groupCC
				nomCC += maxCC- (*it)->SCC();
			}
			maxCC = (totalVertices-1.0)*maxCC;	//standardize minimum and maximum Closeness centrality
			minCC = (totalVertices-1.0)*minCC; 
			denomCC =  (( totalVertices-2.0) *  (totalVertices-1.0))/ (2.0*totalVertices-3.0);
			groupCC = nomCC/denomCC;	//Calculate group Closeness centrality
	
			nomBC*=2.0;
			denomBC =   (totalVertices-1.0) *  (totalVertices-1.0) * (totalVertices-2.0);
			groupBC=nomBC/denomBC;		//Calculate group Betweeness centrality
	
			denomGC =  ( ( totalVertices-2.0) *  (totalVertices-1.0) )/ (2.0*totalVertices-3.0);
			groupGC= nomGC/denomGC;		//Calculate group Graph centrality
	
			nomSC*=2.0;
			denomSC =   (totalVertices-1.0) *  (totalVertices-1.0) * (totalVertices-2.0);
			groupSC = nomSC/denomSC;	//Calculate group Stress centrality
//			calculatedCentralities=TRUE;
		}
	

}



/**
*	Breadth-First Search (BFS) method for unweighted graphs (directed or not)

	INPUT: 
		a 'source' vertex with index s and a boolean calc_centralities.
		(Implicitly, BFS uses the m_graph structure)
	
	OUTPUT: 
		For every vertex t: DM(s, t) is set to the distance of each t from s
		For every vertex t: TM(s, t) is set to the number of shortest paths between s and t
		For every vertex u: it increases SC(u) by one, when it finds a new shor. path from s to t through u.
		For source vertex s: it calculates CC(s) as the sum of its distances from every other vertex. 
		For every source s: it calculates GC(u) as the maximum distance from all other vertices.

		Also, if calc_centralities is TRUE then BFS does extra operations:
			a) each vertex u popped from Q is pushed to a stack Stack 
			b) Append each neighbor y of u to the list Ps, thus Ps stores all predecessors of y on all all shortest paths from s 
	
*/ 
void Analyzer::BFS(int s, bool calc_centralities){
	int u,w, dist_u, temp=0;

	//set distance of s from s equal to 0	
	DM->setItem(s,s,0);
	//set sigma of s from s equal to 1
	TM->setItem(s,s,1);

	//
	qDebug("BFS: Construct a queue Q of integers and push source vertex s=%i to Q as initial vertex", s);
	queue<int> Q;
//	qDebug("BFS: Q size %i", Q.size());

	Q.push(s);

	qDebug("BFS: LOOP: While Q not empty ");
	while ( !Q.empty() ) {
		qDebug("BFS: Dequeue: first element of Q is u=%i", Q.front());
		u=Q.front(); Q.pop();
		if (calc_centralities){
			qDebug("BFS: If we are to calculate centralities, we must push u=%i to global stack Stack ", u);
			Stack.push(u);
		}
		imap_f::iterator it;
		qDebug("BFS: LOOP over every edge (u,w) e E, that is all neighbors w of vertex u");
		for( it = m_graph [ u ]->m_outEdges.begin(); it != m_graph [ u ]->m_outEdges.end(); it++ ) {
			
			w=index[it->first];	
			qDebug("BFS: u=%i is connected with w=%i of index %i. ", u, it->first, w);
			qDebug("BFS: Start path discovery");
			if (	DM->item(s, w) == -1 ) { //if distance (s,w) is infinite, w found for the first time.
				qDebug("BFS: first time visiting w=%i. Enqueuing w to the end of Q", w);
				Q.push(w);
				qDebug()<<"First check if distance(s,u) = -1 (aka infinite :)) and set it to zero";
				dist_u=DM->item(s,u);
 				if (dist_u <0) dist_u=0;
				qDebug("BFS: Setting distance of w=%i from s=%i equal to distance(s,u) plus 1. New distance = %i",w,s, dist_u+1);
				DM->setItem(s, w, dist_u+1);
				averGraphDistance += dist_u+1;
				nonZeroDistancesCounter++;
				if (calc_centralities){
					qDebug()<<"Calculate CC: the sum of distances (will invert it l8r)";
					m_graph [s]->setCC (m_graph [s]->CC() + dist_u+1);
					qDebug()<<"Calculate GC: the maximum distance (will invert it l8r) - also for Eccentricity";
					if (m_graph [s]->GC() < dist_u+1 ) m_graph [s]->setGC(dist_u+1);

				}
				qDebug("BFS: Checking graphDiameter");
				if ( dist_u+1 > graphDiameter){
					graphDiameter=dist_u+1;
					qDebug("BFS: new graphDiameter = %i", graphDiameter );
				}
			}		

			qDebug("BFS: Start path counting"); 	//Is edge (u,w) on a shortest path from s to w via u?
			if ( DM->item(s,w)==DM->item(s,u)+1) {
				temp= TM->item(s,w)+TM->item(s,u);
				qDebug("BFS: Found a NEW SHORTEST PATH from s=%i to w=%i via u=%i. Setting Sigma(%i, %i) = %i",s, w, u, s, w,temp);
				if (s!=w)
					TM->setItem(s,w, temp);
				if (calc_centralities){
					qDebug("If we are to calculate centralities, we must calculate SC as well");
					m_graph[u]->setSC(m_graph[u]->SC()+1);

					qDebug("BFS: appending u=%i to list Ps[w=%i] with the predecessors of w on all shortest paths from s ", u, w);
					m_graph[w]->appendToPs(u);
				}
			}
		}
	} 	
}






/**
	minmax() facilitates the calculations of minimum and maximum centralities during createDistanceMatrix()
*/
void Analyzer::minmax(float C, Vertex *v, float &max, float &min, int &maxNode, int &minNode) {
	qDebug("MINMAX C=%f, max=%f, min=%f, name= %i", C, max, min, v->name()); 
	if (C > max ) {
		max=C;
		maxNode=v->name();
	}
	if (C < min ) {
		min=C;
		minNode=v->name();
	}
}




/** 	This method calculates the number of discrete centrality classes of all vertices
	It stores that number in a map<float,int> where the centrality value is the key.
	Called from createDistanceMatrix()
*/
void Analyzer::resolveClasses(float C, fmap_i &discreteClasses, int &classes){
Q_UNUSED(C);
Q_UNUSED(discreteClasses);
Q_UNUSED(classes);
// 	fmap_i::iterator it2;
// 	it2 = discreteClasses.find(C);    //O(logN) complexity
// 	if (it2 == discreteClasses.end() )	{
// 		classes++; 
// 		qDebug("######This is a new centrality class. Amount of classes = %i", classes);
// 		discreteClasses[C]=classes;
// 	}
}


void Analyzer::resolveClasses(float C, fmap_i &discreteClasses, int &classes, int vertex){
Q_UNUSED(C);
Q_UNUSED(discreteClasses);
Q_UNUSED(classes);
Q_UNUSED(vertex);
// 	fmap_i::iterator it2;
// 	it2 = discreteClasses.find(C);    //O(logN) complexity
// 	if (it2 == discreteClasses.end() )	{
// 		classes++; 
// 		qDebug("######Vertex %i  belongs to a new centrality class. Amount of classes = %i", vertex, classes);
// 		discreteClasses[C]=classes;
// 	}
}
