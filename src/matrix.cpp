/***************************************************************************
 SocNetV: Social Network Visualizer 
 version: 2.9
 Written in Qt

                        matrix  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
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

#include "matrix.h"



#define TINY 1.0e-20

#include <cstdlib>		//allows the use of RAND_MAX macro
#include <QDebug>
#include <QtMath>		//needed for fabs, qFloor etc
#include <QTextStream>


/**
 * @brief Matrix::Matrix
 * Default constructor - creates a Matrix of given dimension (0x0)
 * Use resize(m,n) or zeromatrix(m,n) to resize it
 * @param Actors
 */
Matrix::Matrix (int rowDim, int colDim)  : m_rows (rowDim), m_cols(colDim) {
    row = new (nothrow) MatrixRow[ m_rows ];
    Q_CHECK_PTR( row );
    for (int i=0;i<m_rows; i++) {
        row[i].resize( m_cols );
    }
}



/**
* @brief Matrix::Matrix
* Copy constructor. Creates a Matrix identical to Matrix b
* Allows Matrix a=b declaration
* Every MatrixRow object holds max_int=32762
* @param b
*/
Matrix::Matrix(const Matrix &b) {
    qDebug()<< "Matrix:: constructor";
    m_rows=b.m_rows;
    m_cols=b.m_cols ;
    row = new MatrixRow[m_rows];
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
    if ( rows() )
        delete [] row;
}


 /**
 * @brief Clears data
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
 * @brief Resizes this matrix to m x n
 * Called before every operation on new matrices.
 * Every MatrixRow object holds max_int=32762
 * @param Actors
 */
void Matrix::resize (const int m, const int n) {
    qDebug() << "Matrix: resize() ";
    clear();
    m_rows = m;
    m_cols = n;
    row = new (nothrow) MatrixRow [ m_rows  ];
    Q_CHECK_PTR( row );
    qDebug() << "Matrix: resize() -- resizing each row";
    for (int i=0;i<m_rows; i++) {
        row[i].resize( m_cols );  // CHECK ME
    }
}




/**
 * @brief finds Min-Max values in current Matrix
 * @param min value in the matrix
 * @param max value
 * Complexity: O(n^2)
 */
void Matrix::findMinMaxValues (qreal &min, qreal & max, bool &hasRealNumbers){
    max=0;
    min=RAND_MAX;
    hasRealNumbers = false;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            if ( fmod (item(r,c), 1.0)  != 0 )  {
                hasRealNumbers = true;
            }
            if ( item(r,c) > max) {
                max = item(r,c) ;
            }
            if ( item(r,c) < min){
                min = item(r,c) ;
            }
        }
    }
}



/**
 * @brief Like Matrix::findMinMaxValues only it skips r==c
 *
 * @param min value. If (r,c) = minimum, it mean that neighbors r and c are the nearest in the matrix/network
 * @param max value
 * Complexity: O(n^2)
 */
void Matrix::NeighboursNearestFarthest (qreal &min, qreal & max,
                               int &imin, int &jmin,
                               int &imax, int &jmax){
    max=0;
    min=RAND_MAX;
    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            if (r==c) continue;
            if ( item(r,c) > max) {
                max = item(r,c) ;
                imax = r; jmax=c;
            }
            if ( item(r,c) < min){
                min = item(r,c) ;
                imin = r; jmin=c;
            }
        }
    }
}



/**
 * @brief Makes this square matrix the identity square matrix I
 * @param dim
 */
void Matrix::identityMatrix(int dim) {
    qDebug() << "Matrix::identityMatrix() -- deleting old rows";
    clear();
    m_rows=dim;
    m_cols=dim;
    row = new (nothrow) MatrixRow [m_rows];
    Q_CHECK_PTR( row );
    //qDebug() << "Matrix: resize() -- resizing each row";
    for (int i=0;i<m_rows; i++) {
        row[i].resize(m_rows);
        setItem(i,i, 1);
    }
}



/**
 * @brief Makes this matrix the zero matrix of size mxn
 * @param m
 * @param n
 */
void Matrix::zeroMatrix(const int m, const int n) {
    qDebug() << "Matrix::zeroMatrix() m " << m << " n " << n;
    clear();
    m_rows=m;
    m_cols=n;
    row = new (nothrow) MatrixRow [m_rows];
    Q_CHECK_PTR( row );
    //qDebug() << "Matrix::zeroMatrix - resizing each row";
    for (int i=0;i<m_rows; i++) {
        row[i].resize(m_cols);
        for (int j=0;j<m_cols; j++) {
            setItem(i,j, 0);
        }

    }

}


/**
 * @brief Returns the (r,c) matrix element
 * @param r
 * @param c
 * @return
 */
qreal Matrix::item( int r, int c ){
    return row[r].column(c);
}



/**
 * @brief Sets the (r,c) matrix element calling the setColumn method
 * @param r
 * @param c
 * @param elem
 */
void Matrix::setItem( const int r, const int c, const qreal elem ) {
    row [ r ].setColumn(c, elem);
}



/**
 * @brief Clears the (r,c) matrix element
 * @param r
 * @param c
 */
void Matrix::clearItem( int r, int c ) 	{
    row[r].clearColumn(c);
}







/**
 * @brief Deletes row and column and shifts rows and cols accordingly
 * @param erased row/col to delete
 */
void Matrix::deleteRowColumn(int erased){
    qDebug() << "Matrix:deleteRowColumn() - will delete row and column"
             << erased
             << "m_rows before" <<  m_rows;

    --m_rows;
    m_cols = m_rows;
    qDebug() << "Matrix:deleteRowColumn() - m_rows now " << m_rows << ". Resizing...";
    for (int i=0;i<m_rows+1; i++) {
        for (int j=0;j<m_rows+1; j++) {
//            qDebug() << "Matrix:deleteRowColumn() -"
//                        <<"item ("<< i+1 << "," << j+1 << ") ="<< item(i, j) ;
            if (i>=m_rows || j>=m_rows) {
                setItem( i, j, RAND_MAX) ;
//                qDebug() << "Matrix:deleteRowColumn() -"
//                         <<"both i,j>=m_rows, corner case (will be deleted). Setting to RAND_MAX."
//                        << "New item value (" <<  i+1 << ", " << j+1 << ") ="<< item(i, j) ;
            }
            else if (i<erased && j< erased) {
//                qDebug() << "Matrix:deleteRowColumn() -"
//                         << "i, j < erased. Skipping. Item unchanged.";
                continue;
            }
            else if (i<erased && j>=erased) {
                setItem( i, j, item(i,j+1) ) ;
//                qDebug() << "Matrix:deleteRowColumn() -"
//                            <<"j>=erased, shifting column left"
//                           << "New item value (" <<  i+1 << ", " << j+1 << ") ="<< item(i, j) ;
            }
            else if (i>=erased && j<erased) {
                setItem( i, j, item(i+1,j) ) ;
//                qDebug() << "Matrix:deleteRowColumn() -"
//                         <<"i>=erased, shifting rows up."
//                        << "New item value (" <<  i+1 << ", " << j+1 << ") ="<< item(i, j) ;
            }
            else if (i>=erased && j>=erased) {
                setItem( i, j, item(i+1,j+1) ) ;
//                qDebug() << "Matrix:deleteRowColumn() -"
//                         <<"both i,j>=erased, shifting row up and column left."
//                        << "New item value (" <<  i+1 << ", " << j+1 << ") ="<< item(i, j) ;
            }

        }
        row[i].setSize(m_cols);
    }
    qDebug() << "Matrix:deleteRowColumn() - finished, new matrix:";
    //printMatrixConsole(true); // @TODO comment out to release

}


/**
 * @brief Fills a matrix with a given value
 * @param value
 */
void Matrix::fillMatrix(qreal value )   {
    for (int i=0;i< rows() ; i++)
        for (int j=0;j< cols(); j++)
            setItem(i,j, value);
}



/**
 * @brief Subtracts this matrix from I and returns
 *
 * @return I-this to this matrix
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
 * @brief Swaps row A with row B of this matrix
 * @param rowA
 * @param rowB
 */
void Matrix::swapRows(int rowA,int rowB){
    qDebug()<<"   swapRow() "<< rowA+1 << " with " << rowB+1;
    qreal *tempRow = new  (nothrow) qreal [ rows() ];
    Q_CHECK_PTR(tempRow);
    for ( int j=0; j<  rows(); j++) {
      tempRow[j] = item (rowB, j);
      setItem ( rowB, j, item ( rowA, j ) );
      setItem ( rowA, j,  tempRow[j] );
      }
    delete [] tempRow;
}





/**
* @brief Scalar Multiplication. Multiplies this by qreal f
*  and returns the product matrix of the same dim
  * Allows to use P.multiplyScalar(f)
  * @param f
*/
void Matrix::multiplyScalar (const qreal  & f) {
        qDebug()<< "Matrix::multiplyScalar() with f " << f;
        for (int i=0;i< rows();i++) {
            for (int j=0;j<cols();j++) {
                setItem(i,j, item(i,j) * f );
            }
        }
}


/**
 * @brief Multiply every element of row by value
 * @param row
 * @param value
 */
void Matrix::multiplyRow(int row, qreal value) {
    qDebug()<<"   multiplyRow() "<< row+1 << " by value " << value;
    for ( int j=0; j<  rows(); j++) {
        setItem ( row, j,  value * item (row, j) );
        qDebug()<<"   item("<< row+1 << ","<< j+1 << ") = " <<  item(row,j);
    }
}








/**
* @brief Matrix equality/assignment , operator =
* Allows copying a matrix onto another using b=a where b,a matrices
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
            row=new (nothrow) MatrixRow[m_rows];
            Q_CHECK_PTR( row );
            for (int i=0;i<m_rows; i++) {
                row[i].resize(m_cols); //every MatrixRow object holds max_int=32762
            }
        }
       for (int i=0;i<m_rows; i++)
           row[i]=a.row[i];
    }
    return *this;
}



/**
 * @brief Matrix addition
 * Takes two (nxn) matrices and returns their sum as a reference to this
 * Same algorithm as operator +, just different interface.
 * In this case, you use something like: c.sum(a,b)
 * @param a
 * @param b
 * @return
 */
void Matrix::sum( Matrix &a, Matrix & b)  {
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            setItem(i,j, a.item(i,j)+b.item(i,j));
}





/**
* @brief Matrix::operator +=
* Matrix add another matrix: +=
* Adds to this matrix another matrix B of the same dim and returns to this
* Allows A+=B
* @param b
* @return this
*/
void Matrix::operator +=(Matrix & b) {
    qDebug()<< "Matrix::operator +=";
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            setItem(i,j, item(i,j)+b.item(i,j));
}


/**
  * @brief Matrix addition, operator +
  * Adds this matrix and B of the same dim and returns the sum S
  * Allows S = A+B
  * @param b
  * @return Matrix S
*/
Matrix& Matrix::operator +(Matrix & b) {
    Matrix *S = new Matrix(rows(), cols());
    qDebug()<< "Matrix::operator +";
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            S->setItem(i,j, item(i,j)+b.item(i,j));
    return *S;
}


/**
  * @brief Matrix subtraction, operator -
  * Subtract this matrix - B of the same dim and returns the result S
  * Allows S = A-B
  * @param b
  * @return Matrix S
*/
Matrix& Matrix::operator -(Matrix & b) {
    Matrix *S = new Matrix(rows(), cols() );
    qDebug()<< "Matrix::operator -";
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            S->setItem(i,j, item(i,j)-b.item(i,j));
    return *S;
}



/**
 * @brief Matrix multiplication, operator *
 * Multiplies (right) this matrix with given matrix b.
 * Allows P = A * B where A,B of same dimension
* and returns product as a reference to the calling object
* @param b
* @param symmetry
* @return
*/
Matrix& Matrix::operator *(Matrix & b) {

    qDebug()<< "Matrix::operator *";

    Matrix *P = new Matrix(rows(), cols());

    if ( cols() != b.rows() ) {
        qDebug()<< "Matrix::product() - ERROR! Non compatible input matrices:"
                   " this("
                << rows() << "," << cols()
                << ") and b(" << b.rows() << ","<< b.cols();
        return *P;
    }

    for (int i=0;i< rows();i++)
        for (int j=0;j<b.cols();j++) {
            P->setItem(i,j,0);
            for (int k=0;k< cols();k++) {
                    P->setItem(i,j, P->item(i,j) + item(i,k)*b.item(k,j) );

            }
        }
    return *P;
}



/**
* @brief Multiplies (right) this m x n matrix with given n x p matrix b
* and returns the product in the calling matrix which becomes an m x p matrix.
* This convenience operator *= allows A *= B
* @param b
* @param symmetry
* @return
*/
void Matrix::operator *=(Matrix & b) {

    qDebug()<< "Matrix::operator *";

    if ( cols() != b.rows() ) {
        qDebug()<< "Matrix::product() - ERROR! Non compatible input matrices:"
                   " this("
                << rows() << "," << cols()
                << ") and b(" << b.rows() << ","<< b.cols();
        return;
    }

    Matrix *P = new Matrix(rows(), b.cols());

    for (int i=0;i< rows();i++) {
        for (int j=0;j<b.cols();j++) {
            P->setItem(i,j,0);
            for (int k=0;k < cols();k++) {
                    P->setItem(i,j, P->item(i,j) + item(i,k)*b.item(k,j) );
            }
        }
    }
    *this = *P;
}




/**
 * @brief Matrix Multiplication. Given two matrices A (mxn) and B (nxp)
 * computes their product and stores it to the calling matrix which becomes
 * an m x p matrix
 * Allows P.product(A, B)
 * @param A
 * @param B
 * @param symmetry
 * @return i x k matrix
 */
void Matrix::product(Matrix &A, Matrix & B, bool symmetry)  {
    qDebug()<< "Matrix::product() - symmetry" << symmetry;

    if (A.cols() != B.rows() ) {
        qDebug()<< "Matrix::product() - ERROR! Non compatible input matrices:"
                   " a("
                << A.rows() << "," << A.cols()
                << ") and b(" << B.rows() << ","<< B.cols();
        return;
    }

    Matrix *P = new Matrix(A.rows(), B.cols());

    qreal prod = 0;

    for (int i=0;i< A.rows();i++) {
        for (int j=0;j<B.cols();j++) {
            if (symmetry && i > j ) continue;
            prod = 0;
            for (int k=0;k<A.cols();k++) {
                prod += A.item(i,k)*B.item(k,j);
            }
            P->setItem(i,j, prod);
            if (symmetry) {
               P->setItem(j,i, prod );
            }
        }
    }
//    qDebug() << "Matrix::product() - this";
    *this = *P;

    //this->printMatrixConsole();
}


/**
 * @brief Takes two ( N x N ) matrices (symmetric) and outputs an upper triangular matrix
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
 * @brief Returns the n-nth power of this matrix
 * @param n
 * @param symmetry
 * @return Matrix
 */
Matrix& Matrix::pow (int n, bool symmetry)  {
    if (rows()!= cols()) {
        qDebug()<< "Matrix::pow() - Error. This works only for square matrix";
        return *this;
    }
    qDebug()<< "Matrix::pow() ";
    Matrix X, Y; //auxilliary matrices
    qDebug()<< "Matrix::pow() - creating X = this";
    X=*this; //X = this
    //X.printMatrixConsole(true);
    qDebug()<< "Matrix::pow() - creating Y = I";
    Y.identityMatrix( rows() ); // y=I
    //Y.printMatrixConsole(true);
    return expBySquaring2 (Y, X, n, symmetry);

}



/**
 * @brief Recursive algorithm implementing "Exponentiation by squaring".
 * Also known as Fast Modulo Multiplication, this algorithm allows
 * fast computation of a large power n of square matrix X
 * @param Y must be the Identity matrix  on first call
 * @param X the matrix to be powered
 * @param n the power
 * @param symmetry
 * @return Matrix&

 * On first call, parameters must be: Y=I, X the orginal matrix to power and n the power.
 * Returns the power of matrix X to this object.
 * For n > 4 it is more efficient than naively multiplying the base with itself repeatedly.
 */
Matrix& Matrix::expBySquaring2 (Matrix &Y, Matrix &X,  int n, bool symmetry) {
    if (n==1) {
        qDebug() <<"Matrix::expBySquaring2() - n = 1. Computing PM = X*Y where "
                   "X = " ;
        //X.printMatrixConsole();
        //qDebug() <<"Matrix::expBySquaring2() - n = 1. And Y = ";
        //Y.printMatrixConsole();
        Matrix *PM = new Matrix(rows(), cols());
        PM->product(X, Y, symmetry);
        //qDebug()<<"Matrix::expBySquaring2() - n = 1. PM = X*Y ="  ;
        //PM->printMatrixConsole();
        return *PM;
    }
    else if ( n%2 == 0 ) { //even
        qDebug()<<"Matrix::expBySquaring2() - even n =" << n
               << "Computing PM = X * X";
        Matrix PM(rows(), cols());
        PM.product(X,X,symmetry);
        //qDebug()<<"Matrix::expBySquaring2() - even n =" << n << ". PM = X * X = " ;
        //PM.printMatrixConsole();
        return expBySquaring2 ( Y, PM, n/2 );
    }
    else  { //odd
        qDebug()<<"Matrix::expBySquaring2() - odd n =" << n
               << "First compute PM = X * Y";
        Matrix PM(rows(), cols());
        Matrix PM2(rows(), cols());
        PM.product(X,Y,symmetry);
        //qDebug()<<"Matrix::expBySquaring2() - odd n =" << n << ". PM = X * Y = " ;
        //PM.printMatrixConsole();
        qDebug()<<"Matrix::expBySquaring2() - odd n =" << n
               << "Now compute PM2 = X * X";
        PM2.product(X,X,symmetry);
        //qDebug()<<"Matrix::expBySquaring2() - odd n =" << n << ". PM2 = X * X = " ;
        //PM2.printMatrixConsole();
        return expBySquaring2 ( PM, PM2, (n-1)/2 );
    }
}



/**
 * @brief Calculates the matrix-by-vector product Ax of this matrix
 * Default product: Ax
 * if leftMultiply=true then it returns the left product xA
 * @param in input array/vector
 * @param out output array
 * @param leftMultiply
 */
void Matrix::productByVector (
        qreal in[],
        qreal out[],
        const bool &leftMultiply) {

    int n = rows();
    int m = cols();

    for(int i = 0; i < n; i++) {
         out[i] = 0;
         for (int j = 0; j < m; j++) {
             if (leftMultiply) {
              // dot product of row vector b with j-th column in A
              out[i] += item (j, i) * in[j];
             }
             else {
               // dot product of i-th row in A with the column vector b
               out[i] += item (i, j) * in[j];
             }

         }
    }
}


/**
 * @brief Helper function, takes to vectors and returns their
 * Manhattan distance (also known as l1 norm, Taxicab or L1 distance)
 * which is the sum of the absolute differences
 * of their coordinates.
 * @param x
 * @param y
 * @return
 */
qreal Matrix::distanceManhattan(
        qreal x[],
        qreal y[],
        int n) {
    qreal norm = 0;
    for(int i = 0; i < n; i++) {
        norm += fabs (x[i] - y[i]);
    }
    return norm;
}


/**
 * @brief Helper function, computes the Euclideian length (also known as L2 distance)
 * of a vector:
   if x = (x1 x2 ... xn), then ||x|| = square_root(x1*x1 + x2*x2 + ... + xn*xn)
 * @param x
 * @param n
 * @return
 */
qreal Matrix::distanceEuclidean(
        qreal x[],
        int n) {
    qreal norm = 0;
    for (int i = 0; i < n; i++) {
         norm += x[i] * x[i];
    }
    norm = sqrt(norm);
    return norm;
}


/**
 * @brief Implementation of the Power method which computes the
 * leading eigenvector x of this matrix, that is the eigenvector
 * corresponding to the largest positive eigenvalue.
 * In the process, it also computes min and max values.
 * Used by Eigenvector Centrality (EVC).
 * We use C arrays instead of std::vectors or anything else,
 * as we know from start the size (n) of vectors x and tmp
 * This approach is faster than using std::vector when n > 1000
 * @param x
 * @param xsum
 * @param xmax
 * @param xmaxi
 * @param xmin
 * @param xmini
 * @param eps
 * @param maxIter
 */
void Matrix::powerIteration (
        qreal x[],
        qreal &xsum,
        qreal &xmax,
        int &xmaxi,
        qreal &xmin,
        int &xmini,
        const qreal eps,
        const int &maxIter) {

    qDebug() << "Matrix::powerIteration() - maxIter"
             << maxIter
             <<"initial x"
            << x;

    int n = rows();
    qreal norm = 0, distance=0;

    qreal *tmp;
    tmp=new (nothrow) qreal [n];
    Q_CHECK_PTR( tmp );

    xsum = 0;
    int iter = 0;

    do {
        qDebug() << "Matrix::powerIteration() - iteration"
                 << iter ;

        // calculate the matrix-by-vector product Ax and
        // store the result to vector tmp
        productByVector(x, tmp, false);

        qDebug() << "Matrix::powerIteration() - tmp = Ax ="
                 << tmp;

        // calculate the euclidean length of the resulting vector
        // which will be the denominator in the vector normalization
        norm = distanceEuclidean(tmp, n);

        qDebug() << "Matrix::powerIteration() - norm" << norm;

        // norm should never be zero, but in case there is
        // numerical error, we set it to 1
        if (!norm) {
            qDebug() << "### Matrix::powerIteration() - norm = 0 !!!";
            norm = 1;
        }


        // normalize vector tmp to unit vector for next iteration
        xsum = 0;
        for(int i = 0; i < n; i++) {
           tmp[i] = tmp[i] / norm;
        }
        qDebug() << "Matrix::powerIteration() - tmp / norm "
                 << tmp;

        // calculate the manhattan distance between the new and prev vectors
        distance = distanceManhattan (tmp, x, n);


        xmax = 0 ;
        xmin = RAND_MAX;
        for(int i = 0; i < n; i++) {
           x[i] = tmp[i];
           xsum += x[i];
           if (x[i] > xmax) {
               xmax = x[i] ;
               xmaxi = i+1;
           }
           if (x[i] < xmin) {
               xmin = x[i] ;
               xmini = i+1;
           }
        }


        qDebug() << "Matrix::powerIteration() - end of iteration"
                 << iter << endl
                 << "x" << x  << endl
                 << "distance from previous x " << distance
                 << "sum" << xsum
                 << "xmax" << xmax
                 << "xmin" << xmin;

        iter ++;
        if (iter > maxIter)
            break;

    } while ( distance > eps);

     delete [] tmp;
}



/**
  * @brief Returns the Transpose of this matrix
  * Allows T = A.transpose()
  * @param b
  * @return Matrix T
*/

Matrix& Matrix::transpose() {
    Matrix *T = new Matrix(cols(), rows());
    //T->zeroMatrix(cols(), rows());
    qDebug()<< "Matrix::transpose()";
    for (int i=0;i< cols();i++) {
        for (int j=0;j<rows();j++) {
            T->setItem(i,j, item(j,i));

        }
    }
    return *T;
}





/**
  * @brief Returns the Cocitation Matrix of this matrix (C = A * A^T)
  * Allows T = A.cocitationMatrix()
  * @param b
  * @return Matrix T
*/

Matrix& Matrix::cocitationMatrix() {
    Matrix *T = new Matrix(cols(), rows());
    qDebug()<< "Matrix::cocitationMatrix() this transpose";
    //this->transpose().printMatrixConsole();
    T->product(this->transpose(),*this, true);
    return *T;
}




/**
  * @brief Returns the Degree Matrix of this matrix.
  * The Degree Matrix is diagonal matrix which contains information about the degree
  * of each graph vertex (row of the adjacency matrix)
  * Allows S = A.degreeMatrix()
  * @param b
  * @return Matrix S
*/

Matrix& Matrix::degreeMatrix() {
    Matrix *S = new Matrix(rows(), cols());
    qDebug()<< "Matrix::degreeMatrix()";
    qreal degree=0;
    for (int i=0;i< rows();i++) {
        degree = 0;
        for (int j=0;j<cols();j++) {
            degree += item(i,j);

        }
        S->setItem(i,i, degree);
    }
    return *S;
}



/**
  * @brief Returns the Laplacian of this matrix.
  * The Laplacian is a NxN matrix L = D - A where D is the degree matrix of A
  * Allows S = A.laplacianMatrix()
  * @param b
  * @return Matrix S
*/

Matrix& Matrix::laplacianMatrix() {
    Matrix *S = new Matrix(rows(), cols());
    //S->zeroMatrix(rows(), cols());
    qDebug()<< "Matrix::laplacianMatrix()";
    *S = (this->degreeMatrix()) - *this;
    return *S;
}







/**
 * @brief Inverts given matrix A by Gauss Jordan elimination
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
    qreal m_pivot=0, temp_pivot=0, elim_coef=0;

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
 * @brief Given matrix a, it replaces a by the LU decomposition of a rowwise permutation of itself.
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
bool Matrix::ludcmp (Matrix &a, const int &n, int indx[], qreal &d) {
    qDebug () << "Matrix::ludcmp () - decomposing matrix a to L*U";
    int i=0, j=0, imax=0, k;
    qreal big,temp;
    //vv=vector<qreal>(1,n);
    qreal *vv;            // vv stores the implicit scaling of each row
    vv=new (nothrow) qreal [n];
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
 * @brief Solves the set of n linear equations A·X = b, where A nxn matrix
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
 * @param b: input array as the right-hand side vector B, and output with the solution vector X
 * @return:
 *
 * a, n, and indx are not modified by this routine and can be left in place for
 * successive calls with different right-hand sides b.
 * This routine takes into account the possibility that b will begin with many
 * zero elements, so it is efficient for use in matrix inversion.

* Code adapted from Knuth's Numerical Recipes in C, pp 47
 *
 */
void Matrix::lubksb(Matrix &a, const int &n, int indx[], qreal b[])
{
    qDebug () << "Matrix::lubksb() - ";
    int i, j, ii=0,ip;
    qreal sum;
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
 * @brief Computes and returns the inverse of given matrix a
 * Allows b.inverse(a)
 * @param a
 * @return
 */
Matrix& Matrix::inverse(Matrix &a)
{
    int i,j, n=a.rows();
    qreal d;
    //qreal *col = new qreal[n];
    qreal *col = new  (nothrow) qreal [ n ];
    Q_CHECK_PTR(col);

    //int indx[n];
    int *indx = new  (nothrow) int [ n ];
    Q_CHECK_PTR(indx);

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





/**
 * @brief Computes and returns the solution of the set of n linear equations A·x = b
 * Allows A.solve(b)
 *
 * @param b vector
 * @return
 */
bool Matrix::solve(qreal b[])
{

    Matrix *A = new Matrix(this->rows(), this->cols());

    *A = *this;


    int n=rows();
    qreal d;

//    int indx[n];
    int *indx = new  (nothrow) int [ n ];
    Q_CHECK_PTR(indx);

    qDebug () << "Matrix::solve() - solving A x  - size " << n;
    if (n==0) {
        return false;
    }
    if ( ! ludcmp(*A,n,indx,d) )
    { //  Decompose the matrix just once.
        qDebug () << "Matrix::solve() - matrix a singular - RETURN";
        return false ;
    }

    qDebug () << "Matrix::solve() - call lubksb";
    lubksb(*A, n, indx, b);
    qDebug () << "Matrix::solve() - finished!";

    return true;
}


/**
 * @brief Computes the dissimilarities matrix of the variables (rows, columns, both)
 * of this matrix using the user defined metric
 * @param metric
 * @param varLocation
 * @param diagonal
 * @param considerWeights
 * @return
 */
Matrix& Matrix::distancesMatrix(const int &metric,
                        const QString varLocation,
                        const bool &diagonal,
                        const bool &considerWeights) {
    Q_UNUSED(considerWeights);

    Matrix *T = new Matrix(cols(), rows());

    qDebug()<< "Matrix::distancesMatrix() -"
            <<"metric"<< metric
            << "varLocation"<< varLocation
            << "diagonal"<<diagonal;

    int N = 0;
    qreal sum = 0;
    qreal distance = 0;
    qreal distTemp = 0;
    qreal ties = 0;
    qreal max = 0 ; // for Chebyshev metric
    if (varLocation=="Rows") {

        N = rows() ;

        QVector<qreal> mean (N,0); // holds mean values

        qDebug()<< "Matrix::distancesMatrix() - input matrix:";
        //this->printMatrixConsole();

        for (int i = 0 ; i < N ; i++ ) {
            sum = 0 ;
            for (int k = i ; k < N ; k++ ) {
                distTemp = 0;
                ties = 0;
                max = 0;
                for (int j = 0 ; j < N ; j++ ) {

//                    qDebug() <<  "(i,j)" << i<< ","<<j << "(k,j)" << k<<","<<j;

                    if (!diagonal && (i==j || k==j)) {
                        continue;
                    }

                    switch (metric) {
                    case METRIC_JACCARD_INDEX:
                        if (item(i,j) == item(k,j)  && (item(i,j) != 0 && item(i,j) != RAND_MAX)) {
                            distTemp++;
                        }
                        if ( ( item(i,j) != 0 && item(i,j) != RAND_MAX ) ||
                             ( item(k,j) != 0 && item(k,j) != RAND_MAX )) {
                           ties++;
                        }
                        break;
                    case METRIC_HAMMING_DISTANCE:
                        if (item(i,j) != item(k,j) ) {
                            distTemp++;
                        }
                        break;
                    case METRIC_EUCLIDEAN_DISTANCE:
                        if (item(i,j) == RAND_MAX || item(k,j) == RAND_MAX || distTemp == RAND_MAX) {
                            distTemp = RAND_MAX;
                        }
                        else {
//                            qDebug() <<  item(i,j) << "-" << item(k,j) <<"^2";
                            distTemp += ( item(i,j) - item(k,j) )*( item(i,j) - item(k,j) ); //compute (x - y)^2
                        }
                        break;
                    case METRIC_MANHATTAN_DISTANCE:
                        if (item(i,j) == RAND_MAX || item(k,j) == RAND_MAX || distTemp == RAND_MAX ) {
                            distTemp = RAND_MAX;
                        }
                        else {
                            distTemp += fabs( item(i,j) - item(k,j) ); //compute |x - y|
                        }
                        break;
                    case METRIC_CHEBYSHEV_MAXIMUM:
                        if (item(i,j) == RAND_MAX || item(k,j) == RAND_MAX || distTemp == RAND_MAX) {
                            distTemp = RAND_MAX;
                            max = RAND_MAX;
                        }
                        else {
                            distTemp =  fabs( item(i,j) - item(k,j) );
                            max = ( distTemp  > max ) ? distTemp : max;
                            distTemp = max;
                        }
                        break;
                    default:
                        break;
                    }

                }

                switch (metric) {
                case METRIC_JACCARD_INDEX:
                    if (ties!=0)
                        distance =  1 -  distTemp/  (  ties ) ;
                    else
                        distance = 1;
                    break;
                case METRIC_HAMMING_DISTANCE:
                    distance = distTemp;
                    break;
                case METRIC_EUCLIDEAN_DISTANCE:
                     distance = (distTemp == RAND_MAX) ? distTemp : sqrt(distTemp);
                     break;
                case METRIC_MANHATTAN_DISTANCE:
                    distance = distTemp ;
                    break;
                case METRIC_CHEBYSHEV_MAXIMUM:
                    distance = distTemp ;
                    break;
                default:
                    break;
                }


//                qDebug() << "distTemp("<<i+1<<","<<k+1<<") =" << distTemp
//                         << "matchRatio("<<i+1<<","<<k+1<<") =" << distance;

                T->setItem(i,k, distance);
                T->setItem(k,i, distance);

                sum += distance;
            }
            //compute mean match value
            mean[i] = sum / ( N ) ;

        }

    }
    else if (varLocation=="Columns") {

        N = rows() ;

        QVector<qreal> mean (N,0); // holds mean values

        qDebug()<< "Matrix::distancesMatrix() -"
                <<"input matrix";
        //printMatrixConsole(true);

        for (int i = 0 ; i < N ; i++ ) {
            sum = 0 ;
            for (int k = i ; k < N ; k++ ) {
                distTemp = 0;
                ties = 0;
                max = 0;
                for (int j = 0 ; j < N ; j++ ) {

                    if (!diagonal && (i==j || k==j))
                        continue;


                    switch (metric) {
                    case METRIC_JACCARD_INDEX:
                        if (item(j,i) == item(j,k)  && (item(j,i) != 0 && item(j,i) != RAND_MAX)) {
                            distTemp++;
                        }
                        if ( ( item(j,i) != 0 && item(j,i) != RAND_MAX ) ||
                             ( item(j,k) != 0 && item(j,k) != RAND_MAX )) {
                           ties++;
                        }
                        break;
                    case METRIC_HAMMING_DISTANCE:
                        if (item(j,i) != item(j,k) ) {
                            distTemp++;
                        }
                        break;
                    case METRIC_EUCLIDEAN_DISTANCE:
                        if (item(j,i) == RAND_MAX || item(j,k) == RAND_MAX || distTemp == RAND_MAX) {
                            distTemp = RAND_MAX;
                        }
                        else {
                            distTemp += ( item(j,i) - item(j,k) )*( item(j,i) - item(j,k) ); //compute (x - y)^2
                        }
                        break;
                    case METRIC_MANHATTAN_DISTANCE:
                        if (item(j,i) == RAND_MAX || item(j,k) == RAND_MAX || distTemp == RAND_MAX ) {
                            distTemp = RAND_MAX;
                        }
                        else {
                            distTemp += fabs( item(j,i) - item(j,k) ); //compute |x - y|
                        }
                        break;
                    case METRIC_CHEBYSHEV_MAXIMUM:
                        if (item(j,i) == RAND_MAX || item(j,k) == RAND_MAX || distTemp == RAND_MAX) {
                            distTemp = RAND_MAX;
                            max = RAND_MAX;
                        }
                        else {
                            distTemp =  fabs( item(j,i) - item(j,k) );
                            max = ( distTemp  > max ) ? distTemp : max;
                            distTemp = max;
                        }
                        break;
                    default:
                        break;
                    }
                }

                switch (metric) {
                case METRIC_JACCARD_INDEX:
                    if (ties!=0)
                        distance =  1 -  distTemp/  (  ties ) ;
                    else
                        distance = 1;
                    break;
                case METRIC_HAMMING_DISTANCE:
                    distance = distTemp;
                    break;
                case METRIC_EUCLIDEAN_DISTANCE:
                     distance = (distTemp == RAND_MAX) ? distTemp : sqrt(distTemp);
                     break;
                case METRIC_MANHATTAN_DISTANCE:
                    distance = distTemp ;
                    break;
                case METRIC_CHEBYSHEV_MAXIMUM:
                    distance = distTemp ;
                    break;
                default:
                    break;
                }


//                qDebug() << "distTemp("<<i+1<<","<<k+1<<") =" << distTemp

//                         << "distance("<<i+1<<","<<k+1<<") =" << distance;

                T->setItem(i,k, distance);
                T->setItem(k,i, distance);

                sum += distance;
            }
            //compute mean match value
            mean[i] = sum / ( N ) ;

        }

    }
    else if (varLocation=="Both") {
        Matrix CM;
        N = rows() ;
        int M = N * 2; // CM will have double rows

        CM.zeroMatrix(M,N);

        QVector<qreal> mean (N,0); // holds mean values

        //create augmented matrix (concatenated rows and columns) from input matrix
        for (int i = 0 ; i < N  ; i++ ) {
            for (int j = 0 ; j < N  ; j++ ) {
                CM.setItem(j,i, item(i,j));
                CM.setItem(j + N,i, item(j,i));
            }
        }

        qDebug()<< "Matrix::distancesMatrix() -"
                <<"input matrix";
        //CM.printMatrixConsole(true);


        for (int i = 0 ; i < N ; i++ ) {

            for (int k = i ; k < N ; k++ ) {

                distTemp = 0;
                ties = 0;
                max = 0;
                for (int j = 0 ; j < M ; j++ ) {

                    if (!diagonal) {
                        if ( (i==j || k==j ))
                            continue;
                        if ( j>=N && ( (i+N)==j || (k+N)==j ))
                            continue;
                    }

                    switch (metric) {
                    case METRIC_JACCARD_INDEX:
                        if (CM.item(j,i) == CM.item(j,k)  && (CM.item(j,i) != 0 && CM.item(j,i) != RAND_MAX)) {
                            distTemp++;
                        }
                        if ( ( CM.item(j,i) != 0 && CM.item(j,i) != RAND_MAX ) ||
                             ( CM.item(j,k) != 0 && CM.item(j,k) != RAND_MAX )) {
                           ties++;
                        }
                        break;
                    case METRIC_HAMMING_DISTANCE:
                        if (  CM.item(j,i) != CM.item(j,k) ) {
                            distTemp++;
                        }
                        break;
                    case METRIC_EUCLIDEAN_DISTANCE:
                        if ( CM.item(j,i) == RAND_MAX || CM.item(j,k) == RAND_MAX || distTemp == RAND_MAX) {
                            distTemp = RAND_MAX;
                        }
                        else {
                            distTemp += ( CM.item(j,i) - CM.item(j,k) )*( CM.item(j,i) - CM.item(j,k) ); //compute (x - y)^2
                        }
                        break;
                    case METRIC_MANHATTAN_DISTANCE:
                        if ( CM.item(j,i) == RAND_MAX || CM.item(j,k) == RAND_MAX || distTemp == RAND_MAX ) {
                            distTemp = RAND_MAX;
                        }
                        else {
                            distTemp += fabs( CM.item(j,i) - CM.item(j,k) ); //compute |x - y|
                        }
                        break;
                    case METRIC_CHEBYSHEV_MAXIMUM:
                        if ( CM.item(j,i) == RAND_MAX || CM.item(j,k) == RAND_MAX || distTemp == RAND_MAX) {
                            distTemp = RAND_MAX;
                            max = RAND_MAX;
                        }
                        else {
                            distTemp =  fabs( CM.item(j,i) - CM.item(j,k) );
                            max = ( distTemp  > max ) ? distTemp : max;
                            distTemp = max;
                        }
                        break;
                    default:
                        break;
                    }

                }


                switch (metric) {
                case METRIC_JACCARD_INDEX:
                    if (ties!=0)
                        distance =  1 -  distTemp/  (  ties ) ;
                    else
                        distance = 1;
                    break;
                case METRIC_HAMMING_DISTANCE:
                    distance = distTemp;
                    break;
                case METRIC_EUCLIDEAN_DISTANCE:
                     distance = (distTemp == RAND_MAX) ? distTemp : sqrt(distTemp);
                     break;
                case METRIC_MANHATTAN_DISTANCE:
                    distance = distTemp ;
                    break;
                case METRIC_CHEBYSHEV_MAXIMUM:
                    distance = distTemp ;
                    break;
                default:
                    break;
                }



//                qDebug() << "distTemp("<<i+1<<","<<k+1<<") =" << distTemp

//                         << "matchRatio("<<i+1<<","<<k+1<<") =" << distance;

                T->setItem(i,k, distance);
                T->setItem(k,i, distance);

                sum += distance;

            }
            //compute mean match value
            mean[i] = sum / ( N ) ;

        }
    }
    else {

    }
    qDebug() << "Matrix::distancesMatrix() - FINISHED - Returning matrix:";
    //T->printMatrixConsole();
    return *T;
}




/**
 * @brief  Computes the pair-wise matching score of the rows, columns
 * or both of the given matrix AM, based on the given matching measure
 * and returns the similarity matrix.
 * @param AM Matrix
 * @return Matrix nxn with matching scores for every pair of rows/columns of AM
 */

Matrix& Matrix::similarityMatrix(Matrix &AM,
                                   const int &measure,
                                   const QString varLocation,
                                   const bool &diagonal,
                                   const bool &considerWeights){

    Q_UNUSED(considerWeights);

    qDebug()<< "Matrix::similarityMatrix() -"
            <<"measure"<< measure
            << "varLocation"<< varLocation;

    int N = 0;
    qreal sum = 0;
    qreal matchRatio = 0;
    qreal matches = 0;
    qreal ties = 0;
    qreal magn_i=0, magn_k=0;
    if (varLocation=="Rows") {

        N = AM.rows() ;

        this->zeroMatrix(N,N);

        QVector<qreal> mean (N,0); // holds mean values

        qDebug()<< "Matrix::similarityMatrix() -"
                <<"input matrix";
        //AM.printMatrixConsole(true);

        for (int i = 0 ; i < N ; i++ ) {
            sum = 0 ;
            for (int k = i ; k < N ; k++ ) {
                matches = 0;
                ties = 0;
                magn_i=0; magn_k=0;
                for (int j = 0 ; j < N ; j++ ) {

                    if (!diagonal && (i==j || k==j))
                        continue;

                    switch (measure) {
                    case METRIC_SIMPLE_MATCHING :
                        if (AM.item(i,j) == AM.item(k,j) ) {
                            matches++;
                        }
                        ties++;
                        break;
                    case METRIC_JACCARD_INDEX:
                        if (AM.item(i,j) == AM.item(k,j)  && AM.item(i,j) != 0) {
                            matches++;
                        }
                        if (AM.item(i,j) != 0  || AM.item(k,j)  ) {
                           ties++;
                        }
                        break;
                    case METRIC_HAMMING_DISTANCE:
                        if (AM.item(i,j) != AM.item(k,j) ) {
                            matches++;
                        }
                        break;
                    case METRIC_COSINE_SIMILARITY:
                        matches += AM.item(i,j) * AM.item(k,j); //compute x * y
                        magn_i  += AM.item(i,j) * AM.item(i,j); //compute |x|^2
                        magn_k  += AM.item(k,j) * AM.item(k,j); //compute |y|^2
                        break;
                    case METRIC_EUCLIDEAN_DISTANCE:
                        matches += ( AM.item(i,j) - AM.item(k,j) )*( AM.item(i,j) - AM.item(k,j) ); //compute (x - y)^2
                        break;
                    default:
                        break;
                    }

                }

                switch (measure) {
                case METRIC_SIMPLE_MATCHING :
                    matchRatio=   matches/  ( ( ties  ) ) ;
                    break;
                case METRIC_JACCARD_INDEX:
                    matchRatio=   matches/  ( ( ties ) ) ;

                    break;
                case METRIC_HAMMING_DISTANCE:
                    matchRatio = matches;
                    break;
                case METRIC_COSINE_SIMILARITY:
                    // sigma(i,j) = cos(theta) = x * y / |x| * |y|
                    if ( !magn_i  || ! magn_k ) {
                        // Note that cosine similarity is undefined when
                        // one or both vertices has degree zero. By convention,
                        // in this case we take sigma(i,j) = 0
                        matchRatio = 0;
                    }
                    else
                        matchRatio = matches / sqrt( magn_i  * magn_k );
                    break;
                case METRIC_EUCLIDEAN_DISTANCE:
                    matchRatio = sqrt(matches);
                    break;
                default:
                    break;
                }


                qDebug() << "matches("<<i+1<<","<<k+1<<") =" << matches

                         << "matchRatio("<<i+1<<","<<k+1<<") =" << matchRatio;
                setItem(i,k, matchRatio);
                setItem(k,i, matchRatio);

                sum += matchRatio;
            }
            //compute mean match value
            mean[i] = sum / ( N ) ;

        }

    }
    else if (varLocation=="Columns") {

        N = AM.rows() ;

        this->zeroMatrix(N,N);

        QVector<qreal> mean (N,0); // holds mean values

        qDebug()<< "Matrix::similarityMatrix() -"
                <<"input matrix";
        //AM.printMatrixConsole(true);

        for (int i = 0 ; i < N ; i++ ) {
            sum = 0 ;
            for (int k = i ; k < N ; k++ ) {
                matches = 0;
                ties = 0;
                magn_i=0; magn_k=0;
                for (int j = 0 ; j < N ; j++ ) {

                    if (!diagonal && (i==j || k==j))
                        continue;

                    switch (measure) {
                    case METRIC_SIMPLE_MATCHING :
                        if (AM.item(j,i) == AM.item(j,k) ) {
                            matches++;
                        }
                        ties++;
                        break;
                    case METRIC_JACCARD_INDEX:
                        if (AM.item(j,i) == AM.item(j,k)  && AM.item(j,i) != 0) {
                            matches++;
                        }
                        if (AM.item(j,i) != 0  || AM.item(j,k) !=0 ) {
                           ties++;
                        }

                        break;
                    case METRIC_HAMMING_DISTANCE:
                        if (AM.item(j,i) != AM.item(j,k) ) {
                            matches++;
                        }
                        break;
                    case METRIC_COSINE_SIMILARITY:
                        matches += AM.item(j,i) * AM.item(j,k); //compute x * y
                        magn_i  += AM.item(j,i) * AM.item(j,i); //compute |x|^2
                        magn_k  += AM.item(j,k) * AM.item(j,k); //compute |y|^2
                        break;
                    case METRIC_EUCLIDEAN_DISTANCE:
                        matches += ( AM.item(j,i) - AM.item(j,k) )*( AM.item(j,i) - AM.item(j,k) ); //compute (x - y)^2
                        break;
                    default:
                        break;
                    }


                }

                switch (measure) {
                case METRIC_SIMPLE_MATCHING :
                    matchRatio=   matches/  ( ( ties  ) ) ;
                    break;
                case METRIC_JACCARD_INDEX:
                    matchRatio=   matches/  ( ( ties ) ) ;

                    break;
                case METRIC_HAMMING_DISTANCE:
                    matchRatio = matches;
                    break;
                case METRIC_COSINE_SIMILARITY:
                    // sigma(i,j) = cos(theta) = x * y / |x| * |y|
                    if ( !magn_i  || ! magn_k ) {
                        // Note that cosine similarity is undefined when
                        // one or both vertices has degree zero. By convention,
                        // in this case we take sigma(i,j) = 0
                        matchRatio = 0;
                    }
                    else
                        matchRatio = matches / sqrt( magn_i  * magn_k );
                    break;
                case METRIC_EUCLIDEAN_DISTANCE:
                    matchRatio = sqrt(matches);
                    break;
                default:
                    break;
                }
                qDebug() << "matches("<<i+1<<","<<k+1<<") =" << matches

                         << "matchRatio("<<i+1<<","<<k+1<<") =" << matchRatio;
                setItem(i,k, matchRatio);
                setItem(k,i, matchRatio);

                sum += matchRatio;
            }
            //compute mean match value
            mean[i] = sum / ( N ) ;

        }

    }
    else if (varLocation=="Both") {
        Matrix CM;
        N = AM.rows() ;
        int M = N * 2; // CM will have double rows

        this->zeroMatrix(N,N);
        CM.zeroMatrix(M,N);

        QVector<qreal> mean (N,0); // holds mean values


        //create augmented matrix (concatenated rows and columns) from input matrix
        for (int i = 0 ; i < N  ; i++ ) {
            for (int j = 0 ; j < N  ; j++ ) {
                CM.setItem(j,i, AM.item(i,j));
                CM.setItem(j + N,i, AM.item(j,i));
            }
        }
        qDebug()<< "Matrix::similarityMatrix() -"
                <<"input matrix";
        //CM.printMatrixConsole(true);


        for (int i = 0 ; i < N ; i++ ) {

            for (int k = i ; k < N ; k++ ) {

                matches = 0;
                ties = 0;
                magn_i=0; magn_k=0;
                for (int j = 0 ; j < M ; j++ ) {

                    if (!diagonal) {
                        if ( (i==j || k==j ))
                        continue;
                        if ( j>=N && ( (i+N)==j || (k+N)==j ))
                        continue;
                    }
                    switch (measure) {
                    case METRIC_SIMPLE_MATCHING :
                        if (CM.item(j,i) == CM.item(j,k) ) {
                            matches++;
                        }
                        ties++;
                        break;
                    case METRIC_JACCARD_INDEX:
                        if (CM.item(j,i) == CM.item(j,k)  && CM.item(j,i) != 0) {
                            matches++;
                        }
                        if (CM.item(j,i) != 0  || CM.item(j,k) !=0 ) {
                           ties++;
                        }
                        break;
                    case METRIC_HAMMING_DISTANCE:
                        if (CM.item(j,i) != CM.item(j,k) ) {
                            matches++;
                        }
                        break;
                    case METRIC_COSINE_SIMILARITY:
                        matches += CM.item(j,i) * CM.item(j,k); //compute x * y
                        magn_i  += CM.item(j,i) * CM.item(j,i); //compute |x|^2
                        magn_k  += CM.item(j,k) * CM.item(j,k); //compute |y|^2
                        break;
                    case METRIC_EUCLIDEAN_DISTANCE:
                        matches += ( CM.item(j,i) - CM.item(j,k) )*( CM.item(j,i) - CM.item(j,k) ); //compute (x - y)^2
                        break;
                    default:
                        break;
                    }


                }

                switch (measure) {
                case METRIC_SIMPLE_MATCHING :
                    matchRatio=   matches/  ( ( ties  ) ) ;
                    break;
                case METRIC_JACCARD_INDEX:
                    matchRatio=   matches/  ( ( ties ) ) ;

                    break;
                case METRIC_HAMMING_DISTANCE:
                    matchRatio = matches;
                    break;
                case METRIC_COSINE_SIMILARITY:
                    // sigma(i,j) = cos(theta) = x * y / |x| * |y|
                    if ( !magn_i  || ! magn_k ) {
                        // Note that cosine similarity is undefined when
                        // one or both vertices has degree zero. By convention,
                        // in this case we take sigma(i,j) = 0
                        matchRatio = 0;
                    }
                    else
                        matchRatio = matches / sqrt( magn_i  * magn_k );
                    break;
                case METRIC_EUCLIDEAN_DISTANCE:
                    matchRatio = sqrt(matches);
                    break;
                default:
                    break;
                }

                qDebug() << "matches("<<i+1<<","<<k+1<<") =" << matches

                         << "matchRatio("<<i+1<<","<<k+1<<") =" << matchRatio;
                setItem(i,k, matchRatio);
                setItem(k,i, matchRatio);

                sum += matchRatio;

            }
            //compute mean match value
            mean[i] = sum / ( N ) ;

        }
    }
    else {

    }

    return *this;

}




/**
 * @brief  Computes the Pearson Correlation Coefficient of the rows or the columns
 * of the given matrix AM
 * @param AM Matrix
 * @return Matrix nxn with PPC values for every pair of rows/columns of AM
 */
Matrix& Matrix::pearsonCorrelationCoefficients(Matrix &AM,
                                               const QString &varLocation,
                                               const bool &diagonal){
    qDebug()<< "Matrix::pearsonCorrelationCoefficients() -"
            << "varLocation"<< varLocation;

    int N = 0;
    qreal sumi = 0;
    qreal sumk = 0;
    qreal varianceTimesNi = 0; // = sqrDeviationsFromMean
    qreal varianceTimesNk = 0; // = sqrDeviationsFromMean
    qreal covariance = 0;
    qreal pcc = 0;


    if (varLocation=="Rows") {

        N = AM.rows() ;

        this->zeroMatrix(N,N);

        QVector<qreal> mean (N,0); // holds mean values
        QVector<qreal> sigma(N,0);
        qDebug()<< "Matrix::pearsonCorrelationCoefficients() -"
                <<"input matrix";
        //AM.printMatrixConsole(true);

        for (int i = 0 ; i < N ; i++ ) {

            for (int k = i ; k < N ; k++ ) {

                qDebug() << "comparing rows i"<<i+1<<"k"<<k+1;

                // compute mean and variance  values
                sumi = 0;
                sumk = 0;
                for (int j = 0 ; j < N ; j++ ) {
                    if (!diagonal && ( i==j || k==j ) )
                        continue;
                    sumi += AM.item(i,j);
                    sumk += AM.item(k,j);
                }
                mean[i] = sumi / ( (diagonal) ? (qreal) N : (qreal) (N-2) ) ;
                mean[k] = sumk / ( (diagonal) ? (qreal) N : (qreal) (N-2) ) ;
                varianceTimesNi = 0;
                varianceTimesNk = 0;
                for (int j = 0 ; j < N ; j++ ) {
                    if (!diagonal && ( i==j || k==j ) )
                        continue;
                    varianceTimesNi +=  ( AM.item(i,j)  - mean[i] ) *  ( AM.item(i,j)  - mean[i] );
                    varianceTimesNk +=  ( AM.item(k,j)  - mean[k] ) *  ( AM.item(k,j)  - mean[k] );
                }
                sigma[i] = sqrt (varianceTimesNi); //actually this is sigma * sqrt (N)
                sigma[k] = sqrt (varianceTimesNk); //actually this is sigma * sqrt (N)


                covariance = 0;

                for (int j = 0 ; j < N ; j++ ) {

                    qDebug() << "AM.item(i,j)=AM.item("<<i+1<<","<<j+1<<") = "<<AM.item(i,j)
                             << " mean(i)=mean("<<i+1<<") = "<<mean[i]
                                << "AM.item(k,j)=AM.item("<<k+1<<","<<j+1<<") = "<<AM.item(k,j)
                                << " mean(k)=mean("<<k+1<<") = "<<mean[k];

                    if (!diagonal && (i==j ) ) {
                        qDebug() << "skipping because i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
                        continue;
                    }
                    if (!diagonal && (k==j) ) {
                        qDebug() << "skipping because i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
                        continue;
                    }
                    else
                        covariance  +=  ( AM.item(i,j)  - mean[i] ) * ( AM.item(k,j)  - mean[k] ) ;
                }


                if ( ( sigma[i] != 0 ) && ( sigma[k] != 0  ) ) {
                    pcc =   covariance   /  (( sigma[i] ) * ( sigma[k] )) ;
                }
                else {
                    pcc = 0;
                }


                qDebug() << "covariance("<<i+1<<","<<k+1<<") =" << covariance
                         << "sigma["<<i+1<<"]" << sigma[i]
                         << "sigma["<<k+1<<"]" << sigma[k]
                         << "pcc("<<i+1<<","<<k+1<<") =" << pcc;
                setItem(i,k, pcc);
                setItem(k,i, pcc);
            }

        }

    }
    else if (varLocation=="Columns") {


        N = AM.rows() ;

        this->zeroMatrix(N,N);

        QVector<qreal> mean (N,0); // holds mean values
        QVector<qreal> sigma(N,0);

        qDebug()<< "Matrix::pearsonCorrelationCoefficients() -"
                <<"input matrix";
        //AM.printMatrixConsole(true);


        for (int i = 0 ; i < N ; i++ ) {

            for (int k = i ; k < N ; k++ ) {

                qDebug() << "comparing columns i"<<i+1<<"k"<<k+1;

                // compute mean and variance  values
                sumi = 0;
                sumk = 0;
                for (int j = 0 ; j < N ; j++ ) {
                    if (!diagonal && ( i==j || k==j ) )
                        continue;
                    sumi += AM.item(j,i);
                    sumk += AM.item(j,k);
                }
                mean[i] = sumi / ( (diagonal) ? (qreal) N : (qreal) (N-2) ) ;
                mean[k] = sumk / ( (diagonal) ? (qreal) N : (qreal) (N-2) ) ;
                varianceTimesNi = 0;
                varianceTimesNk = 0;
                for (int j = 0 ; j < N ; j++ ) {
                    if (!diagonal && ( i==j || k==j ) )
                        continue;
                    varianceTimesNi +=  ( AM.item(j,i)  - mean[i] ) *  ( AM.item(j,i)  - mean[i] );
                    varianceTimesNk +=  ( AM.item(j,k)  - mean[k] ) *  ( AM.item(j,k)  - mean[k] );
                }
                sigma[i] = sqrt (varianceTimesNi); //actually this is sigma * sqrt (N)
                sigma[k] = sqrt (varianceTimesNk); //actually this is sigma * sqrt (N)

                covariance = 0;
                for (int j = 0 ; j < N ; j++ ) {
                    if (!diagonal && (i==j || k==j) ) {
                        qDebug() << "skipping because i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
                        continue;
                    }
                    covariance  +=  ( AM.item(j,i)  - mean[i] ) * ( AM.item(j,k)  - mean[k] ) ;
                }


                if ( ( sigma[i] != 0 ) && ( sigma[k] != 0  ) ) {
                    pcc =   covariance   /  (( sigma[i] ) * ( sigma[k] )) ;
                }
                else {
                    pcc = 0;
                }


                qDebug() << "covariance("<<i+1<<","<<k+1<<") =" << covariance
                         << "sigma["<<i+1<<"]" << sigma[i]
                         << "sigma["<<k+1<<"]" << sigma[k]
                         << "pcc("<<i+1<<","<<k+1<<") =" << pcc;

                setItem(i,k, pcc);
                setItem(k,i, pcc);
            }

        }

    }
    else if (varLocation=="Both") {
        Matrix CM;

        N = AM.rows() ;

        int M = N * 2; // CM will have double rows


        this->zeroMatrix(N,N);

        CM.zeroMatrix(M,N);

        QVector<qreal> mean (N,0); // holds mean values
        QVector<qreal> sigma(N,0);


        //create augmented matrix (concatenated rows and columns) from input matrix
        for (int i = 0 ; i < N  ; i++ ) {
            for (int j = 0 ; j < N  ; j++ ) {
                CM.setItem(j,i, AM.item(i,j));
                CM.setItem(j + N,i, AM.item(j,i));
            }
        }

        qDebug()<< "Matrix::pearsonCorrelationCoefficients() -"
                <<"input matrix";
        //CM.printMatrixConsole(true);


        for (int i = 0 ; i < N ; i++ ) {  //a column

            for (int k = i ; k < N ; k++ ) {  // next column

               qDebug() << "comparing augmented columns i"<<i+1<<"k"<<k+1;

                // compute mean and variance  values
                sumi = 0;
                sumk = 0;
                for (int j = 0 ; j < M ; j++ ) {
                    if (!diagonal && ( i==j || k==j || (i+N)==j || (k+N)==j  ) )
                        continue;
                    sumi += CM.item(j,i);
                    sumk += CM.item(j,k);
                }
                mean[i] = sumi / ( (diagonal) ? (qreal) M : (qreal) (M-4) ) ;
                mean[k] = sumk / ( (diagonal) ? (qreal) M : (qreal) (M-4) ) ;
                varianceTimesNi = 0;
                varianceTimesNk = 0;
                for (int j = 0 ; j < M; j++ ) {
                    if (!diagonal && ( i==j || k==j || (i+N)==j || (k+N)==j  ) )
                        continue;
                    varianceTimesNi +=  ( CM.item(j,i)  - mean[i] ) *  ( CM.item(j,i)  - mean[i] );
                    varianceTimesNk +=  ( CM.item(j,k)  - mean[k] ) *  ( CM.item(j,k)  - mean[k] );
                }
                sigma[i] = sqrt (varianceTimesNi); //actually this is sigma * sqrt (N)
                sigma[k] = sqrt (varianceTimesNk); //actually this is sigma * sqrt (N)

                covariance = 0;

                for (int j = 0 ; j < M ; j++ ) {
                    qDebug() << "CM.item(j,i)=CM.item("<<j+1<<","<<i+1<<") = "<<CM.item(j,i)
                             << " mean(i)=mean("<<i+1<<") = "<<mean[i]
                                << "CM.item(j,k)=CM.item("<<j+1<<","<<k+1<<") = "<<CM.item(j,k)
                                << " mean(k)=mean("<<k+1<<") = "<<mean[k];


                    if (!diagonal) {
                        if ( (i==j || k==j )) {
                            qDebug() << "skipping because i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
                            continue;
                        }
                        if ( j>=N && ( (i+N)==j || (k+N)==j )) {
                            qDebug() << "skipping because j>=N and i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
                            continue;
                        }
                    }

                    covariance  +=  ( CM.item(j,i)  - mean[i] ) * ( CM.item(j,k)  - mean[k] ) ;
                }

                if ( ( sigma[i] != 0 ) && ( sigma[k] != 0  ) ) {
                    pcc =   covariance   /  (( sigma[i] ) * ( sigma[k] )) ;
                }
                else {
                    pcc = 0;
                }

                qDebug() << "final covariance("<<i+1<<","<<k+1<<") =" << covariance
                         << "sigma["<<i+1<<"]" << sigma[i]
                         << "sigma["<<k+1<<"]" << sigma[k]
                         << "pcc("<<i+1<<","<<k+1<<") =" << pcc;

                setItem(i,k, pcc);
                setItem(k,i, pcc);
            }

        }
    }
    else {

    }

    return *this;

}



/**
 * @brief Prints matrix m to given textstream
 * @param os
 * @param m
 * @return
 */
QTextStream& operator <<  (QTextStream& os, Matrix& m){
    qDebug() << "Matrix: << Matrix";
    int actorNumber=1, fieldWidth = 13;
    qreal maxVal, minVal, maxAbsVal, element;
    bool hasRealNumbers=false;

    m.findMinMaxValues(minVal, maxVal, hasRealNumbers);

    maxAbsVal = ( fabs(minVal) > fabs(maxVal) ) ? fabs(minVal) : fabs(maxVal) ;


    os << qSetFieldWidth(0) << endl ;

    os << "- Values:        "
       << ( (hasRealNumbers) ? ("real numbers (printed decimals 3)") : ("integers only" ) ) << endl;

    os << "- Max value:  ";

    if ( maxVal==RAND_MAX )
        os <<  infinity << " (=not connected nodes, in distance matrix)";
    else
        os <<   maxVal;

    os << qSetFieldWidth(0) << endl ;

    os << "- Min value:   ";

    if ( minVal==RAND_MAX )
        os << infinity;
    else
        os << minVal;


    os << qSetFieldWidth(0) << endl << endl;

    os << qSetFieldWidth(7) << fixed << right << "v"<< qSetFieldWidth(3) << "" ;

    os <<  ( (hasRealNumbers) ? qSetRealNumberPrecision(3) : qSetRealNumberPrecision(0) ) ;

    // Note: In the case of Distance Matrix,
    // if there is DM(i,j)=RAND_MAX (not connected), we always use fieldWidth  = 13
    if ( maxAbsVal  > 999)
        fieldWidth  = 13 ;
    else if  ( maxAbsVal > 99)
        fieldWidth  = 10 ;
    else if ( maxAbsVal > 9   )
        fieldWidth  = 9 ;
    else
        fieldWidth  = 8 ;

    // print first/header row
    for (int r = 0; r < m.cols(); ++r) {
        actorNumber = r+1;

        if ( actorNumber > 999)
            os << qSetFieldWidth(fieldWidth-3) ;
        else if  ( actorNumber > 99)
            os << qSetFieldWidth(fieldWidth-2) ;
        else if ( actorNumber > 9)
            os << qSetFieldWidth(fieldWidth-1) ;
        else
            os << qSetFieldWidth(fieldWidth) ;

        os <<  fixed << actorNumber;
    }

    os << qSetFieldWidth(0) << endl;

    os << qSetFieldWidth(7)<< endl;

    // print rows
    for (int r = 0; r < m.rows(); ++r) {
        actorNumber = r+1;

        if ( actorNumber > 999)
            os << qSetFieldWidth(4) ;
        else if  ( actorNumber > 99)
            os << qSetFieldWidth(5) ;
        else if ( actorNumber > 9)
            os << qSetFieldWidth(6) ;
        else
            os << qSetFieldWidth(7) ;


        os <<  fixed << actorNumber
            << qSetFieldWidth(3) <<"" ;

        for (int c = 0; c < m.cols(); ++c) {
            element = m(r,c) ;
            os << qSetFieldWidth(fieldWidth) << fixed << right;
            if ( element == RAND_MAX)  // we print inf symbol instead of RAND_MAX (distances matrix).
                os << fixed << right << qSetFieldWidth(fieldWidth) << infinity ;
            else {
                if ( element > 999)
                    os << qSetFieldWidth(fieldWidth-3) ;
                else if  ( element > 99)
                    os << qSetFieldWidth(fieldWidth-2) ;
                else if ( element > 9)
                    os << qSetFieldWidth(fieldWidth-1) ;
                else
                    os << qSetFieldWidth(fieldWidth) ;
                os <<  element;
            }
        }
        os << qSetFieldWidth(0) << endl;
    }
    return os;
}





/**
 * @brief  Prints this matrix as HTML table
 * This has the problem that the real actorNumber != elementLabel i.e. when we
 * have deleted a node/vertex
 * @param os
 * @param debug
 * @return
 */
bool Matrix::printHTMLTable(QTextStream& os,
                            const bool markDiag,
                            const bool &plain,
                            const bool &printInfinity){
    qDebug() << "Matrix::printHTMLTable()";
    int elementLabel=0, rowCount = 0;
    qreal maxVal, minVal, element;
    bool hasRealNumbers=false;

    findMinMaxValues(minVal, maxVal, hasRealNumbers);

    //maxAbsVal = ( fabs(minVal) > fabs(maxVal) ) ? fabs(minVal) : fabs(maxVal) ;

    os <<  ( (hasRealNumbers) ? qSetRealNumberPrecision(3) : qSetRealNumberPrecision(0) ) ;

    if (plain) {
        os << "<pre>";

        // print first/header row
        os << "<span class=\"header\">" << qSetFieldWidth(5) << right << "A/A";
        os <<  fixed << qSetFieldWidth(10) << right ;
        for (int r = 0; r < cols(); ++r) {
            elementLabel = r+1;
            os << elementLabel;
        }
        os << qSetFieldWidth(0) << "</span>"<< endl;

        for (int r = 0; r < rows(); ++r) {
            elementLabel = r+1;
            rowCount++;

            os << "<span class=\"header\">" << qSetFieldWidth(5) << right;
            os << elementLabel;
            os << qSetFieldWidth(0) << "</span>";

            for (int c = 0; c < cols(); ++c) {
                element = item(r,c) ;
                os << fixed << qSetFieldWidth(10) << right;
                if (  element == RAND_MAX)  // print inf symbol instead of RAND_MAX (distances matrix).
                    os << infinity;
                else {
                    os << element ;

                }
               // os << "";
            }

            os << qSetFieldWidth(0) << endl;
        }

        os << "</pre>";
        return true;
    }

    os << "<table  border=\"1\" cellspacing=\"0\" cellpadding=\"0\" class=\"stripes\">"
            << "<thead>"
            << "<tr>"
            << "<th>"
            << ("<sub>Actor</sup>/<sup>Actor</sup>")
            << "</th>";


    // print first/header row
    for (int r = 0; r < cols(); ++r) {
        elementLabel = r+1;
        os << "<th>"
                << elementLabel
                << "</th>";

    }
    os << "</tr>"
            << "</thead>"
            << "<tbody>";

    // print rows
    rowCount = 0;
    for (int r = 0; r < rows(); ++r) {
        elementLabel = r+1;
        rowCount++;
        os << "<tr class=" << ((rowCount%2==0) ? "even" :"odd" )<< ">";

        os <<"<td class=\"header\">"
               << elementLabel
               << "</td>";

        for (int c = 0; c < cols(); ++c) {
            element = item(r,c) ;
            os << fixed << right;
            os <<"<td" << ((markDiag && r==c)? " class=\"diag\">" : ">");
            if ( ( element == RAND_MAX ) && printInfinity) {
                // print inf symbol instead of RAND_MAX (distances matrix).
                os << infinity;
            }
            else {
                os << element ;
            }
            os << "</td>";
        }

        os <<"</tr>";
    }
    os << "</tbody></table>";


    os << qSetFieldWidth(0) << endl ;


    os << "<p>"
       << "<span class=\"info\">"
       << ("Values: ")
       <<"</span>"
       << ( (hasRealNumbers) ? ("real numbers (printed decimals 3)") : ("integers only" ) )
       << "<br />"
       << "<span class=\"info\">"
       << ("- Max value: ")
       <<"</span>"
       << ( ( maxVal==RAND_MAX ) ?
                ( (printInfinity) ? infinity : QString::number(maxVal) ) +
                " (=not connected nodes, in distance matrix)" : QString::number(maxVal) )
       << "<br />"
       << "<span class=\"info\">"
       << ("- Min value: ")
       <<"</span>"
       << ( ( minVal==RAND_MAX ) ?
                ( (printInfinity) ? infinity : QString::number(minVal) ) +
                + " (usually denotes unconnected nodes, in distance matrix)" : QString::number(minVal ) )
       << "</p>";

    return true;
}




/**
 * @brief  Prints this matrix to stderr or stdout
 * @return
 */
bool Matrix::printMatrixConsole(bool debug){
    qDebug() << "Matrix::printMatrixConsole() - debug " << debug
             << "matrix rows" << rows()<< "cols"<< cols();
    QTextStream out ( (debug ? stderr : stdout) );

    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            if ( item(r,c) < RAND_MAX  ) {
                out <<  qSetFieldWidth(12) << qSetRealNumberPrecision(3)
                     <<  forcepoint << fixed<<right
                        << item(r,c);
            }
            else {
                out <<  qSetFieldWidth(12) << qSetRealNumberPrecision(3)
                     <<  forcepoint << fixed<<right
                        << "x";
            }

//            QTextStream( (debug ? stderr : stdout) )
//                    << ( (item(r,c) < RAND_MAX ) ? item(r,c) : INFINITY  )<<' ';
        }
        out <<qSetFieldWidth(0)<< endl;
    }
    return true;
}


/**
 * @brief  Checks if matrix is ill-defined (contains at least an inf element)
 * @return
 */
bool Matrix::illDefined(){
    qDebug() << "Matrix::illDefined() " ;

    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            if ( item(r,c) < RAND_MAX  ) {
            }
            else {
                qDebug() << "Matrix::illDefined() - matrix ill-defined: TRUE" ;
                return true;

            }
        }
    }
    return false;
}


