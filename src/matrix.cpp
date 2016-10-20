/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 2.2
 Written in Qt

                        matrix  -  description
                             -------------------
    copyright            : (C) 2005-2016 by Dimitris B. Kalamaras
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



#define TINY 1.0e-20

#include <cstdlib>		//allows the use of RAND_MAX macro
#include <QDebug>
#include <QtMath>		//needed for fabs, qFloor etc
#include <QTextStream>


/**
 * @brief Matrix::Matrix
 * Default constructor - creates a Matrix of given size (default 0)
 * Use resize(int) to resize it
 * @param Actors
 */
Matrix::Matrix (int rowDim, int colDim)  : m_rows (rowDim), m_cols(colDim) {
    row = new (nothrow) Row[ m_rows ];
    Q_CHECK_PTR( row );
    for (int i=0;i<m_rows; i++) {
        row[i].resize( m_cols );
    }
}



/**
* @brief Matrix::Matrix
* Copy constructor. Creates a Matrix identical to Matrix b
* Allows Matrix a=b declaration
* Every Row object holds max_int=32762
* @param b
*/
Matrix::Matrix(const Matrix &b) {
    qDebug()<< "Matrix:: constructor";
    m_rows=b.m_rows;
    m_cols=b.m_cols ;
    row = new Row[m_rows];
    Q_CHECK_PTR( row );
    for (int i=0;i<m_rows; i++) {
        row[i].resize( m_cols );
    }
    for (int i=0; i<m_rows; i++) {
        row[i]=b.row[i];
    }
}


/**
 * @brief Matrix::~Matrix
 * Destructor
 */
Matrix::~Matrix() {
    delete [] row;
}

 /**
 * @brief Matrix::clear
 * clears data
 */
void Matrix::clear() {
    if (m_rows > 0){
        qDebug() << "Matrix::clear() deleting old rows";
        m_rows=0;
        m_cols=0;
        delete [] row;
    }
}


/**
 * @brief Matrix::resize
 * Resize this matrix to m x n
 * Called before every operation on new matrices.
 * Every Row object holds max_int=32762
 * @param Actors
 */
void Matrix::resize (const int m, const int n) {
    qDebug() << "Matrix: resize() ";
    clear();
    m_rows = m;
    m_cols = n;
    row = new (nothrow) Row [ m_rows  ];
    Q_CHECK_PTR( row );
    qDebug() << "Matrix: resize() -- resizing each row";
    for (int i=0;i<m_rows; i++) {
        row[i].resize( m_cols );  // CHECK ME
    }
}




/**
 * @brief Matrix::findMinMaxValues
 * @param maxVal
 * @param minVal
 */
void Matrix::findMinMaxValues (float & maxVal, float &minVal){
    maxVal=0;
    minVal=RAND_MAX;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            if ( item(r,c) > maxVal)
                maxVal = item(r,c) ;
            if ( item(r,c) < minVal){
                minVal= item(r,c) ;
            }
        }
    }
}




/**
 * @brief Matrix::identityMatrix
 * makes this square matrix the identity square matrix I
 * @param dim
 */
void Matrix::identityMatrix(int dim) {
    qDebug() << "Matrix: identityMatrix() -- deleting old rows";
    clear();
    m_rows=dim;
    m_cols=dim;
    row = new (nothrow) Row [m_rows];
    Q_CHECK_PTR( row );
    qDebug() << "Matrix: resize() -- resizing each row";
    for (int i=0;i<m_rows; i++) {
        row[i].resize(m_rows);
        setItem(i,i, 1);
    }
}



/**
 * @brief Matrix::zeroMatrix
 * makes this matrix the zero matrix of size mxn
 * @param m
 * @param n
 */
void Matrix::zeroMatrix(const int m, const int n) {
    qDebug() << "Matrix:: zeroMatrix() m " << m << " n " << n;
    clear();
    m_rows=m;
    m_cols=n;
    row = new (nothrow) Row [m_rows];
    Q_CHECK_PTR( row );
    qDebug() << "Matrix::zeroMatrix - resizing each row";
    for (int i=0;i<m_rows; i++) {
        row[i].resize(m_cols);
        setItem(i,i, 0);
    }

}


/**
 * @brief Matrix::item
 * returns the (r,c) matrix element
 * @param r
 * @param c
 * @return
 */
float Matrix::item( int r, int c ){
    return row[r].column(c);
}



/**
 * @brief Matrix::setItem
 * sets the (r,c) matrix element calling the setColumn method
 * @param r
 * @param c
 * @param elem
 */
void Matrix::setItem( const int r, const int c, const float elem ) {
    row [ r ].setColumn(c, elem);
}


/**
 * @brief Matrix::clearItem
 * clears the (r,c) matrix element
 * @param r
 * @param c
 */
void Matrix::clearItem( int r, int c ) 	{
    row[r].clearColumn(c);
}


/**
 * @brief Matrix::edgesFrom
 * returns the number of edges starting from r
 * @param r
 * @return
 */
int Matrix::edgesFrom(int r){
    qDebug() << "Matrix: edgesFrom() " << r << " = "<< row[r].outEdges();
    return row[r].outEdges();
}


/**
 * @brief Matrix::edgesTo
 * @param t
 * @return
 */
int Matrix::edgesTo(const int t){
    int m_inEdges=0;
    for (int i = 0; i < rows(); ++i) {
        if ( item(i, t) != 0 )
            m_inEdges++;
    }
    qDebug()<< "Matrix: edgesTo() " << t << " = " << m_inEdges;
    return m_inEdges;
}


/**
 * @brief Matrix::totalEdges
 * @return
 */
int Matrix::totalEdges(){
    int m_totalEdges=0;
    for (int r = 0; r < rows(); ++r) {
        m_totalEdges+=edgesFrom(r);
    }
    qDebug() << "Matrix: totalEdges " << m_totalEdges;
    return m_totalEdges;
}


/**
 * @brief Matrix::deleteRowColumn
 * @param erased
 */
void Matrix::deleteRowColumn(int erased){
    qDebug() << "Matrix: deleteRowColumn() : "<< erased;
    qDebug() << "Matrix: m_rows before " <<  m_rows;

    --m_rows;
    qDebug() << "Matrix: m_rows now " << m_rows << ". Resizing...";
    for (int i=0;i<m_rows+1; i++) {
        for (int j=0;j<m_rows+1; j++) {
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
            if (i>=m_rows || j>=m_rows) {
                setItem( i, j, 0) ;
                qDebug() <<"case 5 (border)";
            }
            qDebug() << "Matrix: new value (" <<  i << ", " << j << ")="<< item(i, j) ;
        }
    }
    for (int i=0;i<m_rows; i++)
        row[i].updateOutEdges();

}


/**
 * @brief Matrix::fillMatrix
 * fills a matrix with a given value
 * @param value
 */
void Matrix::fillMatrix(float value )   {
    for (int i=0;i< rows() ; i++)
        for (int j=0;j< cols(); j++)
            setItem(i,j, value);
}



/**
 * @brief Matrix::subtractFromI
 * @return
 */
Matrix& Matrix::subtractFromI ()  {
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++) {
            if (i==j)
                setItem(i,j, 1.0 - item(i,j));
            else
                setItem(i,j, item(i,j));
        }
    return *this;
}




/**
 * @brief Matrix::swapRows
 * Swaps row A with row B of this matrix
 * @param rowA
 * @param rowB
 */
void Matrix::swapRows(int rowA,int rowB){
    qDebug()<<"   swapRow() "<< rowA+1 << " with " << rowB+1;
    float *tempRow = new  (nothrow) float [ rows() ];
    Q_CHECK_PTR(tempRow);
    for ( int j=0; j<  rows(); j++) {
      tempRow[j] = item (rowB, j);
      setItem ( rowB, j, item ( rowA, j ) );
      setItem ( rowA, j,  tempRow[j] );
      }
    delete [] tempRow;
}





/**
* @brief Matrix::multiplyScalar
  * Scalar Multiplication
  * Multiplies this by float f and returns the product matrix of the same dim
  * Allows to use P.multiplyScalar(f)
  * @param f
*/
void Matrix::multiplyScalar (const float  & f) {
        qDebug()<< "Matrix::multiplyScalar() with f " << f;
        for (int i=0;i< rows();i++) {
            for (int j=0;j<cols();j++) {
                setItem(i,j, item(i,j) * f );
            }
        }
}


/**
 * @brief Matrix::multiplyRow
 * Multiply every element of row A by value
 * @param row
 * @param value
 */
void Matrix::multiplyRow(int row, float value) {
    qDebug()<<"   multiplyRow() "<< row+1 << " by value " << value;
    for ( int j=0; j<  rows(); j++) {
        setItem ( row, j,  value * item (row, j) );
        qDebug()<<"   item("<< row+1 << ","<< j+1 << ") = " <<  item(row,j);
    }
}





/**
 * @brief Matrix::product
 * Matrix Multiplication
 * Allows P = a * b where P, a and b are not the same initially.
 * Takes two matrices a and b of the same dimension
 * and returns their product as a reference to the calling object
 * NOTE: do not use it as B.product(A,B) because it will destroy B on the way.
 * @param a
 * @param b
 * @param symmetry
 * @return
 */
Matrix& Matrix::product( Matrix &a, Matrix & b, bool symmetry)  {
    qDebug()<< "Matrix::product()";
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++) {
            setItem(i,j,0);
            for (int k=0;k<m_rows;k++) {
                qDebug() << "Matrix::product() - a("<< i+1 << ","<< k+1 << ")="
                         << a.item(i,k) << "* b("<< k+1 << ","<< j+1 << ")="
                         << b.item(k,j)  << " gives "  << a.item(i,k)*b.item(k,j);
                if  ( k > j && symmetry) {
                    if (a.item(i,k)!=0 && b.item(j,k)!=0)
                        setItem(i,j, item(i,j)+a.item(i,k)*b.item(j,k));
                }
                else{
                    setItem(i,j, item(i,j)+a.item(i,k)*b.item(k,j));
                }
            }
            qDebug() << "Matrix::product() - ("<< i+1 << ","<< j+1 << ") = "
                     << item(i,j);
        }
    return *this;
}


/**
 * @brief Matrix::productSym
 * takes two ( N x N ) matrices (symmetric) and outputs an upper triangular matrix
 * @param a
 * @param b
 * @return
 */
Matrix& Matrix::productSym( Matrix &a, Matrix & b)  {
    for (int i=0;i<rows();i++)
        for (int j=0;j<cols();j++) {
            setItem(i,j,0);
            if (i>=j) continue;
            for (int k=0;k<m_rows;k++)
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


/**
 * @brief Matrix::pow
 * @param power
 * @param symmetry
 * @return
 */
Matrix& Matrix::pow2 (int power, bool symmetry)  {
    Matrix t=*this;
    for (int k=1; k<power; k++){
        product(*this, t, symmetry);
    }
    return *this;
}

//Matrix& Matrix::expBySquaring ( Matrix &a, int power) {
//    this->identityMatrix();
//    expBySquaring2( *this, a, power );

//}

//Matrix& Matrix::expBySquaring2 ( Matrix &a, Matrix &b, int power) {
//    if (power==1) {
//        return a*b;
//    }
//    else if ( power%2 == 1 ) { //odd

//    }
//    else if ( power%2 == 0 ) { //even
//    }
//}

/**
 * @brief Matrix::sum
 * Matrix addition
 * Takes two (nxn) matrices and returns their product as a reference to this
 * Same algorithm as operator +, just different interface.
 * In this case, you use something like: c.sum(a,b)
 * @param a
 * @param b
 * @return
 */
Matrix& Matrix::sum( Matrix &a, Matrix & b)  {
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            setItem(i,j, a.item(i,j)+b.item(i,j));
    return *this;
}



/**
* @brief Matrix::operator =
* Assigment allows copying a matrix onto another using b=a where b,a matrices
* Equals two matrices.
* @param a
* @return
*/
Matrix& Matrix::operator = (Matrix & a) {
    qDebug()<< "Matrix::operator asignment =";
    if (this != &a){
        if (a.m_rows!=m_rows) {
            clear();
            m_rows=a.m_rows;
            m_cols=a.m_cols;
            row=new (nothrow) Row[m_rows];
			Q_CHECK_PTR( row );
            for (int i=0;i<m_rows; i++) {
                row[i].resize(m_cols); //every Row object holds max_int=32762
			}
		}
       for (int i=0;i<m_rows; i++)
           row[i]=a.row[i];
	}
	return *this;
}


/**
* @brief Matrix::operator +
* Matrix addition: +
* Adds this matrix with b of the same dim
* Allows a+b
* @param a
* @return
*/
  Matrix& Matrix::operator +(Matrix & b) {
      qDebug()<< "Matrix::operator addition";
      for (int i=0;i< rows();i++)
          for (int j=0;j<cols();j++)
              setItem(i,j, item(i,j)+b.item(i,j));
      return *this;

 }






/**
 * @brief operator <<
 * Outputs matrix m to a text str
 * @param os
 * @param m
 * @return
 */
QTextStream& operator <<  (QTextStream& os, Matrix& m){
    qDebug() << "Matrix: << Matrix";
    int fieldWidth = 8, newFieldWidth = 8, actorNumber=1;
    float maxVal, minVal;
    m.findMinMaxValues(maxVal,minVal);
    float element;



    if (maxVal == -1 ||  maxVal==RAND_MAX )
         os << " max Value = " <<  infinity << endl;
        else
        os << " max Value = " << maxVal<< endl;
    if (minVal == -1 ||  minVal==RAND_MAX )
         os << " min Value = " <<  infinity << endl;
    else
        os << " min Value = " << minVal<< endl<<endl;
    if (maxVal > 999999 )
        fieldWidth = 14;
    else if (maxVal > 99999 )
        fieldWidth = 13;
    else if (maxVal > 9999 )
        fieldWidth = 12;
    else if  (maxVal > 999 )
        fieldWidth = 8;
    else if  (maxVal > 99 )
        fieldWidth = 7;

    os << qSetFieldWidth(fieldWidth) << right <<  QString("v |");
    for (int r = 0; r < m.cols(); ++r) {
        newFieldWidth = fieldWidth;
        actorNumber = r+1;
        if ( actorNumber > 99999)
            newFieldWidth = fieldWidth -5;
        else if ( actorNumber > 9999)
            newFieldWidth = fieldWidth -4;
        else if ( actorNumber > 999)
            newFieldWidth = fieldWidth -3;
        else if ( actorNumber > 99)
            newFieldWidth = fieldWidth -2;
        else if ( actorNumber > 9)
            newFieldWidth = fieldWidth -1;
        os << qSetFieldWidth(newFieldWidth) << right  << QString("%1").arg(actorNumber) ;
    }
    os<<endl;
    os.setFieldAlignment(QTextStream::AlignCenter);
    os.setPadChar('-');
    for (int r = 0; r < m.cols()+1; ++r) {
        if ( r > 99999)
            newFieldWidth = fieldWidth -6;
        else if ( r > 9999)
            newFieldWidth = fieldWidth -5;
        else if ( r > 999)
            newFieldWidth = fieldWidth -4;
        else if ( r > 99)
            newFieldWidth = fieldWidth -3;
        else if ( r > 9)
            newFieldWidth = fieldWidth -2 ;
        os << qSetFieldWidth(newFieldWidth) <<  QString("-") ;
    }
    os << qSetFieldWidth(1) << QString("-");
    os.setPadChar(' ');
    os<<endl;
    for (int r = 0; r < m.rows(); ++r) {
        actorNumber = r+1;
        if ( actorNumber > 99999)
            newFieldWidth = fieldWidth -5;
        else if ( actorNumber > 9999)
            newFieldWidth = fieldWidth -4;
        else if ( actorNumber > 999)
            newFieldWidth = fieldWidth -3;
        else if ( actorNumber > 99)
            newFieldWidth = fieldWidth -2;
        else if ( actorNumber > 9)
            newFieldWidth = fieldWidth -1;
        else
            newFieldWidth = fieldWidth;
        os << qSetFieldWidth(newFieldWidth) << right << QString("%1 |").arg(actorNumber) ;
        for (int c = 0; c < m.cols(); ++c) {
            element = m(r,c) ;
            newFieldWidth = fieldWidth;
            if ( element == RAND_MAX )
                newFieldWidth = fieldWidth;
            else if ( element > 9999)
                newFieldWidth = fieldWidth -5;
            else if ( element > 9999)
                newFieldWidth = fieldWidth -4;
            else if ( element > 999)
                newFieldWidth = fieldWidth -3;
            else if ( element > 99)
                newFieldWidth = fieldWidth -2;
            else if ( element > 9)
                newFieldWidth = fieldWidth -1;
            else if ( (element - floor (element) ) != 0  ) {
                if ( element *10 == qFloor(10* element)  )
                newFieldWidth = fieldWidth-1;
                else if (element *100 == qFloor(100* element)  )
                newFieldWidth = fieldWidth-1;
                else if (element *1000 == qFloor(1000* element)  )
                newFieldWidth = fieldWidth-2;
                else
                    newFieldWidth = fieldWidth-2;
            }
            else if (element < 1.0 ) {
                if ( element *10 == qFloor(10* element)  )
                newFieldWidth = fieldWidth-1;
                else if (element *100 == qFloor(100* element)  )
                newFieldWidth = fieldWidth-1;
                else if (element *1000 == qFloor(1000* element)  )
                newFieldWidth = fieldWidth-2;
                else
                    newFieldWidth = fieldWidth-2;
            }
            else
                newFieldWidth = fieldWidth;
            if ( element == -1 || element == RAND_MAX)  // we print infinity symbol instead of -1 (distances matrix).
                os << qSetFieldWidth(newFieldWidth) << right << infinity;
            else
                os << qSetFieldWidth(newFieldWidth)
                   << right << element;
        }
        os << '\n';
    }
    return os;
}




/**
 * @brief Matrix::printMatrixConsole
 * @return
 */
bool Matrix::printMatrixConsole(bool debug){
    qDebug() << "Matrix::printMatrixConsole() debug " << debug ;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            if ( item(r,c) < RAND_MAX  ) {
                QTextStream( (debug ? stderr : stdout) ) << item(r,c) << ' ';
            }
            else {
                QTextStream( (debug ? stderr : stdout) ) << "X" << ' ';
            }

//            QTextStream( (debug ? stderr : stdout) )
//                    << ( (item(r,c) < RAND_MAX ) ? item(r,c) : INFINITY  )<<' ';
        }
        QTextStream( (debug ? stderr : stdout)  ) <<'\n';
    }
    return true;
}







/**
 * @brief Matrix::inverseByGaussJordanElimination
 * Inverts given matrix A by Gauss Jordan elimination
   Input:  matrix A
   Output: matrix A becomes unit matrix
   *this becomes the invert of A and is returned back.
 * @param A
 * @return inverse matrix of A
 */
Matrix& Matrix::inverseByGaussJordanElimination(Matrix &A){
	qDebug()<< "Matrix::inverseByGaussJordanElimination()";
	int n=A.cols();
    qDebug()<<"Matrix::inverseByGaussJordanElimination() - build I size " << n
             << " This will become A^-1 in the end";

    identityMatrix( n );

	int l=0, m_pivotLine=0;
	float m_pivot=0, temp_pivot=0, elim_coef=0;

    for ( int j=0; j< n; j++) { // for n, it is the last diagonal element of A
	    l=j+1;
	    m_pivotLine=-1;
	    m_pivot = A.item(j,j);
	    qDebug() << "inverseByGaussJordanElimination() at column " << j+1
		    << " Initial pivot " << m_pivot ;
        for ( int i=l; i<n; i++) {
            temp_pivot = A.item(i,j);
            if ( qFabs( temp_pivot ) > qFabs ( m_pivot ) ) {
                qDebug() << " A("<< i+1 << ","<< j+1  << ") = " <<  temp_pivot
                         << " absolutely larger than current pivot "<< m_pivot
                         << ". Marking new pivot line: " << i+1;
                m_pivotLine=i;
                m_pivot = temp_pivot ;
            }
        }
        if ( m_pivotLine != -1 ) {
            A.swapRows(m_pivotLine,j);
            swapRows(m_pivotLine,j);
        }


	    qDebug()<<"   multiplyRow() "<< j+1 << " by value " << 1/m_pivot ;
        for ( int k=0; k<  rows(); k++) {
            A.setItem ( j, k,  (1/m_pivot) * A.item (j, k) );
            setItem ( j, k,  (1/m_pivot) * item (j, k) );
            qDebug()<<"   A.item("<< j+1 << ","<< k+1 << ") = " <<  A.item(j,k);
            qDebug()<<"   item("<< j+1 << ","<< k+1 << ") = " <<  item(j,k);
        }

	    qDebug() << "eliminate variables FromRowsBelow()" << j+1 ;
        for ( int i=0; i<  rows(); i++) {
		 qDebug()<<"   Eliminating item("<< i+1 << ","<< j+1 << ") = "
			 <<  A.item(i,j) << " while at column j="<<j+1;
		 if ( A.item(i,j)==0 ){
		    qDebug()<< " ...already eliminated - continue";
		    continue;
		}
		 if ( i == j){
		     qDebug()<< " ...skip pivotline - continue";
		    continue;
		}
		elim_coef=A.item (i, j);
        for ( int k=0; k<  cols(); k++) {
		    qDebug()<<"   A.item("<< i+1 << ","<< k+1 << ") = " <<  A.item(i,k)
			    << " will be subtracted by " << " A.item ("<< i+1
			    << ", "<< j+1 << ") x A.item(" << j+1 << ","<<k+1
			    <<") =" << elim_coef * A.item(j,k) ;
		    A.setItem ( i, k,   A.item (i, k) -  elim_coef * A.item(j, k)  );
		    qDebug()<<"   A.item("<< i+1 << ","<< k+1 << ") = " <<  A.item(i,k);

		    qDebug()<<"   item("<< i+1 << ","<< k+1 << ") = " <<  item(i,k)
			    << " will be subtracted by " << " A.item ("<< i+1
			    << ", "<< j+1 << ") x item(" << j+1 << ","<<k+1
			    <<") =" << elim_coef * item(j,k)  <<  " = "
			    << elim_coef << " x " << item(j,k) ;

		    setItem ( i, k,   item (i, k) -  elim_coef * item(j, k)  );
		    qDebug()<<"   item("<< i+1 << ","<< k+1 << ") = " <<  item(i,k);

		}
	    }

	}
	return *this;
}



/**
 * @brief Matrix::ludcmp(Matrix &a, const int &n, int *indx, float *d)
 * Given matrix a, it replaces a by the LU decomposition of a rowwise permutation of itself.
 * Used in combination with lubksb to solve linear equations or invert a matrix.
 * @param a: input matrix n x n and output arranged as in Knuth's equation (2.3.14)
 * @param n: input size of matrix
 * @param indx: output vector, records the row permutation effected by the partial pivoting
 * @param d: output as ±1 depending on whether the number of row interchanges was even or odd
 * @return:
 *
 * Code adapted from Knuth's Numerical Recipes in C, pp 46
 *
 */
bool Matrix::ludcmp (Matrix &a, const int &n, int indx[], float &d) {
    qDebug () << "Matrix::ludcmp () - decomposing matrix a to L*U";
    int i=0, j=0, imax=0, k;
    float big,temp;
    //vv=vector<float>(1,n);
    float *vv;            // vv stores the implicit scaling of each row
    vv=new (nothrow) float [n];
    Q_CHECK_PTR( vv );

//    QTextStream stream(stdout);
//    stream << "a = LU = " << a ;

    d=1.0;               // No row interchanges yet.

    qDebug () << "Matrix::ludcmp() - loop over row to get scaling info" ;
    for (i=0;i<n;i++) {  // Loop over rows to get the implicit scaling information.
        qDebug () << "Matrix::ludcmp() - row i " <<  i+1;
        big=0;
        for (j=0;j<n;j++) {
            if ((temp=fabs( a[i][j] ) ) > big)
                big=temp;
        }
        if (big == 0)  //       No nonzero largest element.
        {
            qDebug() << "Matrix::ludcmp() - Singular matrix in routine ludcmp";
            return false;
        }
        vv[i]=1.0/big;  //  Save the scaling.
        qDebug () << "Matrix::ludcmp() - big element in row i " << i+1 << " is "<< big << " row scaling vv[i] " << vv[i];
    }

    qDebug () << "Matrix::ludcmp() - Start Crout's LOOP over columns";

    for (j=0;j<n;j++) //     This is the loop over columns of Crout’s method.
    {
        qDebug () << "Matrix::ludcmp() - COLUMN j " <<  j+1 << " search largest pivot";
        big=0;  //      Initialize for the search for largest pivot element.
        imax = j;

        for (i=j;i<n;i++)
        {
            if ( ( temp = vv[i] * fabs( a[i][j] ) ) > big)
            {   //  Is the figure of merit for the pivot better than the best so far?
                big=temp;
                imax=i;
                qDebug () << "Matrix::ludcmp() - found new largest pivot at row " <<  imax+1 << " big " << temp;
            }
        }

        qDebug () << "Matrix::ludcmp() - check for row interchange ";
        if (j != imax) //          Do we need to interchange rows?
        {
            qDebug () << "Matrix::ludcmp() - interchanging rows " << imax+1 << " and " << j+1;
            for ( k=0; k<n; k++ ) { //            Yes, do so...
                temp=a[imax][k];
                a[imax][k] = a[j][k];
                a[j][k] = temp;
            }
            d = -(d);  //..and change the parity of d.
            vv[imax]=vv[j];  //         Also interchange the scale factor.
            qDebug () << "Matrix::ludcmp() - imax  " << imax+1  << " vv[imax]" << vv[imax] << "new parity d " << d;
        }
        indx[j]=imax;
        qDebug () << "Matrix::ludcmp() - indx[j]=imax=" <<  indx[j] +1;
        if ( a[j][j] == 0 ) {
            a[j][j] = TINY; // For some apps, on singular matrices, it is desirable to substitute TINY for zero.
            qDebug () << "Matrix::ludcmp() - WARNING singular matrix set a[j][j]=TINY ";
        }

        for (i=j+1;i<n;i++) {
            //     Now, divide by the pivot element.
            temp=a[i][j] /=  a[j][j] ;
            qDebug () << "Matrix::ludcmp() - j " << j+1<< " dividing by pivot " << a.item(j,j) << " temp  = " << temp;
            for (k=j+1;k<n;k++) {       //reduce remaining submatrix
                a[i][k] -= ( temp * a[j][k] );
                qDebug () << "Matrix::ludcmp() - lower a["<< i+1 << "][" << k+1 <<"] = " << a[i][k];
            }
        }


//           stream << endl << "at j " << j+1 << " matrix a = LU = " << a ;
    }  // Go back for the next column in the reduction.


 //free_vector(vv,1,n);
    qDebug () << "delete vector vv";
    delete[] vv;


//   stream << "final a = LU = " << a ;


    return true;

}




/**
 * @brief Matrix::lubksb(float **a, int n, int *indx, float b[])
 *
 * Solves the set of n linear equations A·X = b, where A nxn matrix
 * decomposed as L·U (L lower triangular and U upper triangular)
 * by forward substitution and  backsubstitution.
 *
 * Given A = L·U we have
 * A · x = (L · U) · x = L · (U · x) = b
 * So, this routine first solves
 * L · y = b
 * for the vector y by forward substitution and then solves
 * U · x = y
 * for the vector x using backsubstitution

 * @param a: input matrix a as the LU decomposition of A, returned by the routine ludcmp
 * @param n: input size of matrix
 * @param indx: input vector, records the row permutation, returned by the routine ludcmp
 * @param b: input array as the right-hand side vector B, and ouput with the solution vector X
 * @return:
 *
 * a, n, and indx are not modified by this routine and can be left in place for
 * successive calls with different right-hand sides b.
 * This routine takes into account the possibility that b will begin with many
 * zero elements, so it is efficient for use in matrix inversion.

* Code adapted from Knuth's Numerical Recipes in C, pp 47
 *
 */
void Matrix::lubksb(Matrix &a, const int &n, int indx[], float b[])
{
    qDebug () << "Matrix::lubksb() - ";
    int i, j, ii=0,ip;
    float sum;
    for ( i=0;i<n;i++) {  // When ii is set to a positive value, it will become the
        ip=indx[i];       // index of the first nonvanishing element of b. We now
        sum=b[ip];        // do the forward substitution, equation (2.3.6). The
        b[ip]=b[i];       // only new wrinkle is to unscramble the permutation
        if (ii != 0 )           // as we go.
            for ( j=(ii-1);j<=i-1;j++)
                sum -= a[i][j]*b[j];
        else if (sum !=0 )     // A nonzero element was encountered, so from now on we
            ii=i+1;         //  will have to do the sums in the loop above.
        qDebug() << "Matrix::lubksb() "<< "i " << i  << " ip=indx[i] " << ip <<  " b[ip] " << b[ip] << " b[i] " << b[i] <<  "sum " << sum ;
        b[i]=sum;
    }
    for ( i=(n-1);i>=0;i--) {  // Now we do the backsubstitution, equation (2.3.7).
        sum=b[i];
        qDebug() << "Matrix::lubksb() backsubstitution: "<< "i " << i  << " b[i] " << b[i] <<  "sum " << sum ;
        for ( j=i+1;j<n;j++)
            sum -= a[i][j]*b[j];
        b[i]=sum/a[i][i]; //  Store a component of the solution vector X. All done!
        qDebug() << "Matrix::lubksb() backsubstitution: "<< "i " << i  <<  "sum " << sum << " a[i][i] " << a[i][i]   << " b[i] " << b[i] ;
    }
}

/**
 * @brief Matrix::inverse
 * @param a
 * @return
 */
Matrix& Matrix::inverse(Matrix &a)
{
    int i,j, n=a.rows();
    float d, col[n];

    int indx[n];
    qDebug () << "Matrix::inverse() - inverting matrix a - size " << n;
    if (n==0) {
        return (*this);
    }
    if ( ! ludcmp(a,n,indx,d) )
    { //  Decompose the matrix just once.
        qDebug () << "Matrix::inverse() - matrix a singular - RETURN";
        return *this;
    }

    qDebug () << "Matrix::inverse() - find inverse by columns";
    for ( j=0; j<n; j++) {    //    Find inverse by columns.
        for( i=0; i<n; i++)
            col[i]=0;
        col[j]=1.0;

        qDebug () << "Matrix::inverse() - call lubksb";
        lubksb(a,n,indx,col);

        for( i=0; i<n; i++) {
             (*this)[i][j] = col[i];
        }

    }
        qDebug () << "Matrix::inverse() - finished!";

    return *this;
}

