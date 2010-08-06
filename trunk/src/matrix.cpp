/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.90
 Written in Qt 4.4

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

//constructor - every Row object holds max_int=32762
Matrix::Matrix(const Matrix &b) {
	row = new Row[m_Actors=b.m_Actors];
		for (register int i=0;i<m_Actors; i++) {
			row[i].resize(m_Actors);
		}
	for (register int i=0; i<m_Actors; i++)
		row[i]=b.row[i];
}


// set the size of the matrix
void Matrix::setSize (int Actors){
	for (register int i=0;i<m_Actors; i++) {
		row[i].setSize(Actors); 
	}
	m_Actors=Actors;

}


//assigment allows copying a matrix onto another using b=a where b,a matrices
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


 //WARNING: this operator is slow! Avoid using it.
float  Matrix::operator ()  (const int r, const int c){
	return  row[r].column(c);
}


/**  	Outputs matrix m to a text stream
*	Used when exporting SM to a file in slotExportSM of MainWindow class.
*/
QTextStream& operator <<  (QTextStream& os, Matrix& m){
	qDebug() << "Matrix: << Matrix";
	for (register int r = 0; r < m.rows(); ++r) {
		for (register int c = 0; c < m.cols(); ++c)
			os << m(r,c) << ' ';
		os << '\n';
	}
	return os;
}



//Resize this matrix. Every Row object holds max_int=32762
void Matrix::resize (int Actors) {
	qDebug() << "Matrix: resize() -- deleting old rows";
	delete [] row;
	row = new Row [m_Actors=Actors];
	Q_CHECK_PTR( row );
	qDebug() << "Matrix: resize() -- resizing each row";
	for (register int i=0;i<m_Actors; i++) {
	    row[i].resize(m_Actors);
	}
}



// makes this matrix the identity matrix I
void Matrix::identityMatrix(int Actors) {
	qDebug() << "Matrix: identityMatrix() -- deleting old rows";
	delete [] row;
	row = new Row [m_Actors=Actors];
	Q_CHECK_PTR( row );
	qDebug() << "Matrix: resize() -- resizing each row";
	for (int i=0;i<m_Actors; i++) {
		row[i].resize(m_Actors);
		setItem(i,i, 1);
	}

}



// returns the (r,c) matrix element
float Matrix::item( int r, int c ){
	return row[r].column(c);
}

// sets the (r,c) matrix element calling the setColumn method
void Matrix::setItem( int r, int c, float elem ) {
	 row [ r ].setColumn(c, elem);

}

// clears the (r,c) matrix element
void Matrix::clearItem( int r, int c ) 	{
	row[r].clearColumn(c); 
}


//returns the number of edges starting from r
int Matrix::edgesFrom(int r){
	qDebug() << "Matrix: edgesFrom() " << r << " = "<< row[r].outEdges();
	return row[r].outEdges();
}


int Matrix::edgesTo(int t){
	int m_inEdges=0;
	for (register int i = 0; i < rows(); ++i) {
		if ( item(i, t) != 0 )
			m_inEdges++;
	}
	qDebug()<< "Matrix: edgesTo() " << t << " = " << m_inEdges;
	return m_inEdges;
}


int Matrix::totalEdges(){
	int m_totalEdges=0;
	for (register int r = 0; r < rows(); ++r) {
		m_totalEdges+=edgesFrom(r);
	}
	qDebug() << "Matrix: totalEdges " << m_totalEdges;
	return m_totalEdges;
}


bool Matrix::printMatrixConsole(){
	qDebug() << "Matrix: printMatrixConsole";
	for (register int r = 0; r < rows(); ++r) {
		for (register int c = 0; c < cols(); ++c)
			cout<< item(r,c) <<' ';
			cout<<'\n';
	}
	return true;
}



void Matrix::deleteRowColumn(int erased){
	qDebug() << "Matrix: deleteRowColumn() : "<< erased;
	qDebug() << "Matrix: mActors before " <<  m_Actors;

	--m_Actors;
	qDebug() << "Matrix: mActors now " << m_Actors << ". Resizing...";
	for (register int i=0;i<m_Actors+1; i++) { 
		for (register int j=0;j<m_Actors+1; j++) { 
		    qDebug() << "Matrix: (" <<  i << ", " << j << ")="<< item(i, j) ;
			if ( j==erased && item(i,erased) ){
			 	clearItem(i,j);
				qDebug() << i << "  connected to " << erased << ". Clearing..." ;
			}	
			if (i<erased && j< erased) {
				qDebug() << "i, j < erased. Skipping. Item remains";
			}
			if (i<erased && j>=erased) {
				setItem( i, j, item(i,j+1) ) ;
				qDebug() << "case 2";
			}			
			if (i>=erased && j<erased) {
				setItem( i, j, item(i+1,j) ) ;
				qDebug() <<"case 3";
			}
			if (i>=erased && j>=erased) {
				setItem( i, j, item(i+1,j+1) ) ;
				qDebug() <<"case 4";
			}
			if (i>=m_Actors || j>=m_Actors) {
				setItem( i, j, 0) ;
				qDebug() <<"case 5 (border)";
			}	
			qDebug() << "Matrix: new value (" <<  i << ", " << j << ")="<< item(i, j) ;
		}	
	}
	for (register int i=0;i<m_Actors; i++) 
		row[i].updateOutEdges();
	
}


// fills a matrix with a given valut
void Matrix::fillMatrix (float value )   {
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
	
Matrix& Matrix::pow (int power, bool symmetry)  {
	Matrix t=*this;
	for (register int k=1; k<power; k++){
		product(*this, t, symmetry);
	}
	return *this;
}


//takes two (nxn) matrices and returns their product as a reference to this
Matrix& Matrix::sum( Matrix &a, Matrix & b)  {
	for (register int i=0;i< rows();i++)
		for (register int j=0;j<cols();j++)
		    setItem(i,j, a.item(i,j)*b.item(i,j));
	return *this;
}



Matrix& Matrix::subtractFromI ()  {
	for (register int i=0;i< rows();i++)
	    for (register int j=0;j<cols();j++) {
		if (i==j)
		    setItem(i,j, 1.0 - item(i,j));
		else
		    setItem(i,j, item(i,j));
	    }
	return *this;
}


/* Switch row A with row B of this matrix */
void Matrix::switchRows(int rowA,int rowB){
     qDebug()<<"   switchRows "<< rowA+1 << " with " << rowB+1;
    float *tempRow = new  float [ rows() ];
    for ( register int j=0; j<  rows(); j++) {
	  tempRow[j] = item (rowB, j);
	  setItem ( rowB, j, item ( rowA, j ) );
	  setItem ( rowA, j,  tempRow[j] );
      }
    delete [] tempRow;
}


/* Multiply every elememt of row A by value */
void Matrix::multiplyRow(int row, float value) {
    qDebug()<<"   multiplyRow "<< row+1 << " by value " << value;
    for ( register int j=0; j<  rows(); j++) {
	  setItem ( row, j,  value * item (row, j) );
    }
}




/* Subtract given row from every row below of this one in this Matrix */
void Matrix::subtractRowFromRowsBelow(int row) {
    qDebug() << "subtractRowFromRowsBelow(): " << row+1;
    for ( register int i=row+1; i<  rows(); i++) {
	for ( register int j=0; j<  rows(); j++) {
	    qDebug()<<"   item("<< i+1 << ","<< j+1 << ") = " <<  item(i,j) << " will be subtracted by " << item (i, j) * item(row, row) ;
	    setItem ( i, j,   item (i, j) -  item (i, j) * item(row, row)  );
	    qDebug()<<"   item("<< i+1 << ","<< j+1 << ") = " <<  item(i,j);
	}
    }
}


/* Subtract given row from every row above of this one in this Matrix */
void Matrix::subtractRowFromRowsAbove(int row) {
    for ( register int i=row-1; i >  0; i-- ) {
	for ( register int j=rows(); j > 0; j--) {
	    qDebug()<<"   item("<< i+1 << ","<< j+1 << ") = " <<  item(i,j) << " will be subtracted by " << item (i, j) * item(row, row) ;
	    setItem ( i, j,   item (i, j) -  item (i, j) * item(row, row)  );
	    qDebug()<<"   item("<< i+1 << ","<< j+1 << ") = " <<  item(i,j);
	}
    }
}


Matrix& Matrix::inverseByGaussJordanElimination(Matrix &A){
	qDebug()<< "Matrix::inverseByGaussJordanElimination()";
	int n=A.cols();
	qDebug()<<"Matrix::inverseByGaussJordanElimination() - starting with the identity Matrix; this will become A^-1 in the end";
	identityMatrix( n );
	int l=0, m_pivotLine=0;
	float m_pivot=0;

	for ( register int j=0; j< n-1; j++) { // for n, it is the last diagonal element of A
	    l=j+1;
	    m_pivotLine=0;
	    m_pivot = A.item(j,j);
	    qDebug() << "inverseByGaussJordanElimination(). initial pivot " << m_pivot ;
	    for ( register int i=l; i<n; i++) {
		if ( A.item(i,j) > m_pivot ) { // find the pivot
		    qDebug() << " initial A("<< i+1 << ","<< j+1  << ") = " <<  A.item(i,j) << " is larger than last pivot "<< m_pivot << " new pivot line...";
		    m_pivotLine=i;
		    m_pivot = A.item(i,j) ;
		}
	    }
	    A.switchRows(m_pivotLine,j);
	    switchRows(m_pivotLine,j);
	    A.multiplyRow(m_pivotLine,1/m_pivot);
	    multiplyRow(m_pivotLine,1/m_pivot);
	    A.subtractRowFromRowsBelow(m_pivotLine);
	    subtractRowFromRowsBelow(m_pivotLine);
	    A.subtractRowFromRowsAbove(m_pivotLine);
	    subtractRowFromRowsAbove(m_pivotLine);
	}

//	qDebug() << "inverseByGaussJordanElimination(). Now A is an upper triangular matrix...start the back propagation routine" ;
//	for ( register int j=n-1;  j > 0; j--) {
//		l=j-1;
//		for ( register int i=l; i<=0; i--) {
//		    qDebug()<<" initial A(" << i+1 << ","<< j+1 << ") = " <<  A.item(i,j);
//		    setItem(i,j, -A.item(i,j)/A.item(j,j) ) ;
//		    A.setItem(i,j, -A.item(i,j)/A.item(j,j) );
//		    qDebug()<<" final A("<< i+1 << ","<< j+1 << ") = " <<  A.item(i,j);;
//		}
//	 }
	 return *this;
}
