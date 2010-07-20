/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.90
 Written in Qt 4.4 with KDevelop   

                        matrix  -  description
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

#include "matrix.h"

#include <iostream>		//used for cout


Matrix::Matrix(const Matrix &b) {
	row = new Row[m_Actors=b.m_Actors];
		for (int i=0;i<m_Actors; i++) { 
			row[i].resize(m_Actors); //every Row object holds max_int=32762
		}
	for (int i=0; i<m_Actors; i++) 
		row[i]=b.row[i];
}



void Matrix::setSize (int Actors){
	for (int i=0;i<m_Actors; i++) { 
		row[i].setSize(Actors); 
	}
	m_Actors=Actors;

}
//assigment allows sane copying b=a where b,a matrices
 Matrix& Matrix::operator =(Matrix & a) {
			if (this != &a){
				if (a.m_Actors!=m_Actors) {
					delete [] row;
					row=new Row[m_Actors=a.m_Actors];
					Q_CHECK_PTR( row );
					for (int i=0;i<m_Actors; i++) { 
						row[i].resize(m_Actors); //every Row object holds max_int=32762 actors
					}
				}

				for (int i=0;i<m_Actors; i++) row[i]=a.row[i];
			}
			return *this;
}
		
//slowing  down the process
int  Matrix::operator ()  (const int r, const int c){
			return  row[r].column(c);
}
		
/**  	Outputs matrix m to a text stream
*	Used when exporting SM to a file in slotExportSM of MainWindow class.
*/
QTextStream& operator <<  (QTextStream& os, Matrix& m){
			qDebug ("Matrix: << Matrix");
			for (register int r = 0; r < m.rows(); ++r) {
				for (register int c = 0; c < m.cols(); ++c)
					os << m(r,c) << ' ';
				os << '\n';
			}
			return os;
}



void Matrix::resize (int Actors) {
		qDebug ("Matrix: resize() -- deleting old rows");
		delete [] row;
 		row = new Row [m_Actors=Actors];  
		Q_CHECK_PTR( row );
		qDebug ("Matrix: resize() -- resizing each row");
		for (int i=0;i<m_Actors; i++) { 
				row[i].resize(m_Actors); //every Row object holds max_int=32762
		}
			
}

int Matrix::item( int r, int c ){
	return row[r].column(c);	

}

void Matrix::setItem( int r, int c, int elem ) {
	 row [ r  ].setColumn(c, elem); 
	
}

void Matrix::clearItem( int r, int c ) 	{
	row[r].clearColumn(c); 
}


int Matrix::edgesFrom(int r){
	qDebug("Matrix: edgesFrom() %i = %i",r, row[r].outEdges());
	return row[r].outEdges();
}


int Matrix::edgesTo(int t){
	int m_inEdges=0;
	for (register int i = 0; i < rows(); ++i) {
		if (item(i, t) )
			m_inEdges++;
	}
	qDebug("Matrix: edgesTo() %i = %i",t, m_inEdges);
	return m_inEdges;
}


int Matrix::totalEdges(){
	int m_totalEdges=0;
	for (register int r = 0; r < rows(); ++r) {
		m_totalEdges+=edgesFrom(r);
	}
	qDebug("Matrix: totalEdges %i",m_totalEdges);
	return m_totalEdges;
}

bool Matrix::printMatrixConsole(){
	qDebug("Matrix: printMatrixConsole");
	for (register int r = 0; r < rows(); ++r) {
		for (register int c = 0; c < cols(); ++c)
			cout<< item(r,c) <<' ';
			cout<<'\n';
	}
	return true;
}



void Matrix::deleteRowColumn(int erased){
	qDebug("Matrix: deleteRowColumn(), %i", erased);
	qDebug("Matrix: mActors before %i", m_Actors);

/*
	for (register int i=0;i<m_Actors; i++) {
		for (register int j=0;j<m_Actors; j++) 
			cout<< item(i,j) <<'\t';
			cout<<'\n';	
	}*/
	--m_Actors;
	qDebug("Matrix: mActors now %i. Resizing...", m_Actors);
	for (register int i=0;i<m_Actors+1; i++) { 
		for (register int j=0;j<m_Actors+1; j++) { 
			qDebug ("Matrix i=%i, j=%i, value=%i", i, j, item(i, j) );
			if ( j==erased && item(i,erased) ){
			 	clearItem(i,j);
				qDebug (" %i connected to %i. Clearing", i, erased);
			}	
			if (i<erased && j< erased) {
				qDebug ("i, j < erased. Skipping. Item remains");
			}
			if (i<erased && j>=erased) {
				setItem( i, j, item(i,j+1) ) ;
				qDebug ("case 2");
			}			
			if (i>=erased && j<erased) {
				setItem( i, j, item(i+1,j) ) ;
				qDebug ("case 3");
			}
			if (i>=erased && j>=erased) {
				setItem( i, j, item(i+1,j+1) ) ;
				qDebug ("case 4");
			}
			if (i>=m_Actors || j>=m_Actors) {
				setItem( i, j, 0) ;
				qDebug ("case 5 (border)");
			}	
			qDebug ("Now matrix i=%i, j=%i, value=%i", i, j, item(i, j) );
		}	
	}
	for (register int i=0;i<m_Actors; i++) 
		row[i].updateOutEdges();
	
/*	for (register int i=0;i<m_Actors; i++) {
		for (register int j=0;j<m_Actors; j++) 
			cout<< item(i,j) <<'\t';
			cout<<'\n';	

	}*/
}


void Matrix::fillMatrix (int value )   {
	for (int i=0;i<m_Actors; i++) 
		for (int j=0;j<m_Actors; j++) 
			setItem(i,j, value);
}

//takes two (ActorsXActors) matrices and returns their product as a reference to this
Matrix& Matrix::product( Matrix &a, Matrix & b, bool symmetry)  {
		   	for (register int i=0;i< rows();i++)
        			for (register int j=0;j<cols();j++) {
					
					setItem(i,j,0);
            					for (register int k=0;k<m_Actors;k++)
			 				if  ( k > j && symmetry) {
								if (a.item(i,k)!=0 && b.item(j,k)!=0)
									setItem(i,j, item(i,j)+a.item(i,k)*b.item(j,k));
									
							}
							else{
								setItem(i,j, item(i,j)+a.item(i,k)*b.item(k,j));
							}
				}
			return *this;
}
		
//takes two (AXA) matrices (symmetric) and outputs an upper triangular matrix
Matrix& Matrix::productSym( Matrix &a, Matrix & b)  {
		
		   	for (register int i=0;i<m_Actors;i++)
        			for (register int j=0;j<m_Actors;j++) {
					setItem(i,j,0);
					if (i>=j) continue;
            				for (register int k=0;k<m_Actors;k++)
			 			if  ( k > j ) {
							if (a.item(i,k)!=0 && b.item(j,k)!=0)
								setItem(i,j, item(i,j)+a.item(i,k)*b.item(j,k));
						}
						else  //k <= j  && i<j
							if ( i>k ) {
								if (a.item(k,i)!=0 && b.item(k,j)!=0)
									setItem(i,j, item(i,j)+a.item(k,i)*b.item(k,j));
							}
							else {
								if (a.item(i,k)!=0 && b.item(k,j)!=0)
									setItem(i,j, item(i,j)+a.item(i,k)*b.item(k,j));
							}
				}
			return *this;
}
	
Matrix& Matrix::pow (Matrix &a, int power, bool symmetry)  {
		Matrix t=a;
		for (register int k=1; k<power; k++){
			product(a,t, symmetry);
			t=*this;  	  //SET TempMatrix EQUAL TO ProductMatrix		  

		}
		return *this;
}
		
		

