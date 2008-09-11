/****************************************************************************
 SocNetV: Social Networks Visualiser 
 version: 0.47
 Written in Qt 4.4 with KDevelop   
 
C++ implementation: layout.cpp

 Description: 

(C) 2006, 2008 Dimitris Kalamaras 
email: dimitris_kalamaras@gmail.com

*/

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

#include <netlayout.h>

//obsolete
void NetLayout::springEmbedder (vector<vector<double> > &p, vector<vector<double> > &pp, int iter, Matrix &SM, int wd, int ht){
//	QValueVector<int>::iterator it;
//	for ( it = xx.begin(); it != xx.end(); ++it )
	double cr=2155.00;			//repulsion constant (Frep)
	double cs=5.00;			//control parameter for the strength of the spring (Fspring)
	double l=100.00;		//the natural length of the spring (Fspring)
	int N=pp.size();
	double sumFsX=0, sumFsY=0, sumFrX=0, sumFrY=0, t1=0, t2=0;
	double delta=0.05;
	qDebug("iter = %i ", iter);
 	qDebug("cr = %f ", cr);
 	qDebug("fc.length = %i", N);
	vector<double> ul(2);
	iter=10;
	//unit length vectors pointing from Pu to Pv, and Pv to Pu respectively
	//frep is the repulsive force between every pair of non-adjacent vertices
	//fsp is the complementary spring force between every pair of adjacent vertices
	vector<vector<double> > ulvpupv(1, ul), ulvpvpu(1, ul), freppupv(1, ul), fsppupv(1, ul), fv(1, ul);	
	vector<double> ed(N);
	vector<vector<double> > edpvpu(N, ed);		//Euclidean distance between positions of Pv and Pu.
	for (register int i=0; i< (iter+2); i++) {
		qDebug ("ITERATION = %i ", i+1);
		for (register int v=0; v< N; v++) {
			sumFsX=0; sumFsY=0; sumFrX=0; sumFrY=0;
			for (register int u=0; u< N; u++) {
				qDebug("u=%i, v=%i", u+1, v+1);
				if (u==v ) continue;
				qDebug ("Old coords of v=%i were: X=%f, Y=%f", v+1, p[v][0], p[v][1]);
				qDebug ("Old coords of u=%i were: X=%f, Y=%f", u+1, p[u][0], p[u][1]);
				edpvpu[v][u]=sqrt( pow ((p[u][0]-p[v][0]), 2) + pow((p[u][0]-p[v][0]),2)  );
				
				if ( SM.item(u,v)!=1 ) { 			//Frep valid only for non-adjacent vertices
					qDebug("u=%i,v=%i non-adjacent. Calculating Frep", u+1, v+1);
					
					qDebug ("edpvpu[%i][%i] = %f", v+1, u+1, edpvpu[v][u]);
					ulvpupv[0][0]=  (p[v][0]-p[u][0])  / edpvpu[v][u];
					ulvpupv[0][1]=  (p[v][1]-p[u][1])  / edpvpu[v][u];
					qDebug ("ulvpupvX = %f", ulvpupv[0][0]) ;
					qDebug ("ulvpupvY = %f", ulvpupv[0][1]) ;
					freppupv[0][0] = ( cr / pow(edpvpu[v][u],2) ) * ulvpupv[0][0];
					freppupv[0][1] = ( cr / pow(edpvpu[v][u],2) ) * ulvpupv[0][1];
					qDebug ("freppupvX[%i][%i] = %f", u+1, v+1, freppupv[0][0]) ;
					qDebug ("freppupvY[%i][%i] = %f", u+1, v+1, freppupv[0][1]) ;
					sumFrX+=freppupv[0][0];
					sumFrY+=freppupv[0][1];
					qDebug ("sumFrepX=%f; sumFrepY=%f", sumFrX, sumFrY );
				}
				if (SM.item(u,v)==1) {
					qDebug("u=%i,v=%i adjacent. Calculating Fspring", u+1, v+1);
					qDebug ("edpvpu[%i][%i] = %f", v+1, u+1, edpvpu[v][u]);
					ulvpvpu[0][0]=  (p[u][0]-p[v][0])  / edpvpu[v][u];
					ulvpvpu[0][1]=  (p[u][1]-p[v][1])  / edpvpu[v][u];
					qDebug ("ulvpupvX = %f", ulvpvpu[0][0]) ;
					qDebug ("ulvpupvY = %f", ulvpvpu[0][1]) ;
					fsppupv[0][0]=cs *  log (edpvpu[v][u] / l)  * ulvpvpu[0][0] ;
					fsppupv[0][1]=cs *  log (edpvpu[v][u] / l)  * ulvpvpu[0][1] ;
					qDebug ("log = %f", log (edpvpu[v][u] / l) );
					qDebug ("fsppupvX = %f", fsppupv[0][0]) ;
					qDebug ("fsppupvY = %f", fsppupv[0][1]) ;
					sumFsX+=fsppupv[0][0];
					sumFsY+=fsppupv[0][1];
					qDebug ("sumFsX=%f; sumFsY=%f;",  sumFsX, sumFsY );
				}
			}
			qDebug ("sumFsX=%f; sumFsY=%f; sumFrX=%f; sumFrY=%f;", sumFsX, sumFsY, sumFrX, sumFrY );
			t1=pp[v][0]+ delta*(sumFsX + sumFrX);
			t2=pp[v][1]+ delta*(sumFsY + sumFrY);
			if ( t1 > 0 && t2  >0 && t1 < wd &&  t2 < ht  ) {
				pp[v][0]= t1;
				pp[v][1]= t2;
			}
		}
		for (register int v=0; v< N; v++) {
			p[v][0]= pp[v][0];
			p[v][1]= pp[v][1];
			qDebug ("NEW coords of v=%i will be: X=%f, Y=%f", v+1, p[v][0], p[v][1]);
		}
	}
}



void NetLayout::FR (vector<vector<double> > &p, vector<vector<double> > &pp, int iter, Matrix &SM, int wd, int ht){
	double l=100.00;	//the natural length of the spring 
	int N=pp.size();
	double sumFaX=0, sumFaY=0, sumFrX=0, sumFrY=0, t1=0, t2=0, threshold=0, temp=0;
	double delta=0.05;
	qDebug("iter = %i ", iter);
 	qDebug("fc.length = %i", N);
	int rect=50;
	iter=10;
	//ulvpupv, ulvpvpu: unit length vectors pointing from Pu to Pv, and Pv to Pu respectively
	//Fr is the repulsive force between every pair of vertices
	//Fa is the attracting force between every pair of adjacent vertices
	vector<double> ul(2);
	vector<vector<double> > ulvpupv(1, ul), ulvpvpu(1, ul), frpupv(1, ul), fapupv(1, ul), fv(1, ul);	
	vector<double> ed(N);
	vector<vector<double> > edpvpu(N, ed);		//Euclidean distance between positions of Pv and Pu.

	for (register int v=0; v< N; v++) 
			for (register int u=0; u< N; u++) 
				temp+=sqrt( pow ((p[u][0]-p[v][0]), 2) + pow((p[u][0]-p[v][0]),2)  );
	for (register int i=0; i< (iter+2); i++) {
		temp=temp/pow((double)N,(int)2);
		threshold=temp;
		temp=0;
		qDebug ("ITERATION = %i ", i+1);
		for (register int v=0; v< N; v++) {
			sumFaX=0; sumFaY=0; sumFrX=0; sumFrY=0;
			for (register int u=0; u< N; u++) {
				qDebug("u=%i, v=%i", u+1, v+1);
				if (u==v ) continue;
				qDebug ("Old coords of v=%i were: X=%f, Y=%f", v+1, p[v][0], p[v][1]);
				qDebug ("Old coords of u=%i were: X=%f, Y=%f", u+1, p[u][0], p[u][1]);
				edpvpu[v][u]=sqrt( pow ((p[u][0]-p[v][0]), 2) + pow((p[u][0]-p[v][0]),2)  );
				temp+=edpvpu[v][u];
				if (edpvpu[v][u] < threshold)  {
					qDebug("u=%i,v=%i and euclidean distance < threshold. Calculating Fr", u+1, v+1);
					qDebug ("edpvpu[%i][%i] = %f", v+1, u+1, edpvpu[v][u]);
					ulvpupv[0][0]=  (p[v][0]-p[u][0])  / edpvpu[v][u];
					ulvpupv[0][1]=  (p[v][1]-p[u][1])  / edpvpu[v][u];
					qDebug ("ulvpupvX = %f", ulvpupv[0][0]) ;
					qDebug ("ulvpupvY = %f", ulvpupv[0][1]) ;
// 					if ( edpvpu[v][u] == 0)  edpvpu[v][u]=1;
					frpupv[0][0] = ( pow(l,2) / edpvpu[v][u] ) * ulvpupv[0][0];	// pow(l,2)
					frpupv[0][1] = ( pow(l,2) / edpvpu[v][u] ) * ulvpupv[0][1];	// pow(l,2)
					qDebug ("frpupvX[%i][%i] = %f", u+1, v+1, frpupv[0][0]) ;
					qDebug ("frpupvY[%i][%i] = %f", u+1, v+1, frpupv[0][1]) ;
					sumFrX+=frpupv[0][0];
					sumFrY+=frpupv[0][1];
					qDebug ("sumFrX=%f; sumFrY=%f", sumFrX, sumFrY );
				}
				if (SM.item(u,v)==1) {
					qDebug("u=%i,v=%i are also adjacent. Calculating Fa", u+1, v+1);
					qDebug ("edpvpu[%i][%i] = %f", v+1, u+1, edpvpu[v][u]);
					ulvpvpu[0][0]=  (p[u][0]-p[v][0])  / edpvpu[v][u];
					ulvpvpu[0][1]=  (p[u][1]-p[v][1])  / edpvpu[v][u];
					qDebug ("ulvpvpuX = %f", ulvpvpu[0][0]) ;
					qDebug ("ulvpvpuY = %f", ulvpvpu[0][1]) ;
					fapupv[0][0]=( pow( edpvpu[v][u],2) / l)  * ulvpvpu[0][0] ;
					fapupv[0][1]=( pow( edpvpu[v][u],2) / l)  * ulvpvpu[0][1] ;
					qDebug ("pow = %f", pow (edpvpu[v][u],2) );
					qDebug ("fapupvX = %f", fapupv[0][0]) ;
					qDebug ("fapupvY = %f", fapupv[0][1]) ;
					sumFaX+=fapupv[0][0];
					sumFaY+=fapupv[0][1];
					qDebug ("sumFaX=%f; sumFaY=%f;",  sumFaX, sumFaY );
				}
			}
			qDebug ("sumFaX=%f; sumFaY=%f; sumFrX=%f; sumFrY=%f;", sumFaX, sumFaY, sumFrX, sumFrY );
			t1=pp[v][0]+ delta*(sumFaX + sumFrX);
			t2=pp[v][1]+ delta*(sumFaY + sumFrY);
			if ( t1 > 0 && t2  >0 && t1 < wd &&  t2 < ht  ) {
				pp[v][0]= t1;
				pp[v][1]= t2;
			}
			else if ( t1 < 0 && t2 <0 ) {
					pp[v][0]= rect; pp[v][1]= rect; 
			}
			else if ( t1 > wd && t2 > ht ) {
				pp[v][0]= wd-rect; pp[v][1]= ht-rect; 
			}
			else if ( t1 < 0 ) pp[v][0]= rect;
			else if ( t1 > wd ) pp[v][0]= wd-rect;
			else if ( t2 < 0 ) pp[v][1]= rect;
			else if ( t2 > ht ) pp[v][1]= ht-rect;
			
		}
		for (register int v=0; v< N; v++) {
			p[v][0]= pp[v][0];
			p[v][1]= pp[v][1];
			qDebug ("NEW coords of v=%i will be: X=%f, Y=%f", v+1, p[v][0], p[v][1]);
		}
		

	}
}


	

