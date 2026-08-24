/**
 * @file matrix.cpp
 * @brief Implements the Matrix class for handling adjacency and sociomatrix data structures in network analysis.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2026 by Dimitris B. Kalamaras.
 *   This file is part of SocNetV (Social Network Visualizer).
 * @license
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 3 or later.
 *   For more details, see <http://www.gnu.org/licenses/>.
 * @see https://socnetv.org
 */


#include "matrix.h"

#define TINY 1.0e-20

// Fix #269: relative-tolerance threshold for ludcmp()'s pivot-magnitude singularity check.
// The checked value is already scaled by its row's own maximum, so this is a dimensionless
// relative test - not tied to the matrices' raw value range.
#define RELATIVE_SINGULARITY_TOLERANCE 1.0e-12

#include <cstdlib>		//allows the use of RAND_MAX macro
#include <QDebug>
#include <QLoggingCategory>
#include <QtMath>		//needed for fabs, qFloor etc
#include <QTextStream>

Q_LOGGING_CATEGORY(lcMatrix, "socnetv.matrix")


/**
 * @brief Constructs a rowDim x colDim matrix, all cells zero-initialized. Defaults to 0x0
 * (an empty matrix) - use resize()/zeroMatrix()/identityMatrix() to size it later.
 * @param rowDim
 * @param colDim
 */
Matrix::Matrix (int rowDim, int colDim)  : m_rows (rowDim), m_cols(colDim) {
    m_data = new (nothrow) qreal[ static_cast<size_t>(m_rows) * m_cols ]();
    Q_CHECK_PTR( m_data );
    rebuildRowPtr();
}



/**
 * @brief Copy constructor: creates a Matrix identical to (an independent copy of) b.
 * Allows the `Matrix a = b` declaration form.
 * @param b
 */
Matrix::Matrix(const Matrix &b) {
    qCDebug(lcMatrix)<< "Matrix:: constructor";
    m_rows=b.m_rows;
    m_cols=b.m_cols ;
    const size_t n = static_cast<size_t>(m_rows) * m_cols;
    m_data = new (nothrow) qreal[n];
    Q_CHECK_PTR( m_data );
    for (size_t i=0; i<n; i++) {
        m_data[i]=b.m_data[i];
    }
    rebuildRowPtr();
}


/**
 * @brief Destructor: frees the data buffer and the row-pointer index.
 */
Matrix::~Matrix() {
    if ( rows() ) {
        delete [] m_data;
        delete [] m_rowPtr;
    }
}


 /**
 * @brief Frees this matrix's data buffer and row-pointer index and resets it to 0x0. Called
 * at the start of resize()/identityMatrix()/zeroMatrix()/operator= before they allocate a
 * fresh buffer at the new size.
 */
void Matrix::clear() {
    if (m_rows > 0){
        qCDebug(lcMatrix) << "Matrix::clear() deleting old rows";
        m_rows=0;
        m_cols=0;
        delete [] m_data;
        delete [] m_rowPtr;
    }
}


/**
 * @brief Resizes this matrix to m x n, discarding any previous contents. All cells start
 * zero-initialized.
 * @param m New row count.
 * @param n New column count.
 */
void Matrix::resize (const int m, const int n) {
    qCDebug(lcMatrix) << "Matrix: resize() ";
    clear();
    m_rows = m;
    m_cols = n;
    m_data = new (nothrow) qreal[ static_cast<size_t>(m_rows) * m_cols ]();
    Q_CHECK_PTR( m_data );
    rebuildRowPtr();
}




/**
 * @brief Scans every cell of this matrix (including the diagonal) and reports the smallest
 * and largest values found, plus whether any cell has a fractional (non-integer) part -
 * used by report writers to decide display precision and, for distance matrices, whether
 * the max value is RAND_MAX (meaning some pair is unreachable).
 * Complexity: O(rows()*cols()).
 * @param min Output: the smallest value found.
 * @param max Output: the largest value found.
 * @param hasRealNumbers Output: true if any cell has a non-zero fractional part.
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
 * @brief Like findMinMaxValues(), but skips the diagonal (r==c) and also reports which
 * pair of distinct vertices achieved the min/max - used to find the closest and farthest
 * pair of nodes in a distance/dissimilarity matrix (e.g. by hierarchical clustering, which
 * repeatedly needs "which two clusters are nearest right now").
 * Complexity: O(rows()*cols()).
 * @param min Output: the smallest off-diagonal value found.
 * @param max Output: the largest off-diagonal value found.
 * @param imin Output: row of the cell where the minimum was found.
 * @param jmin Output: column of the cell where the minimum was found.
 * @param imax Output: row of the cell where the maximum was found.
 * @param jmax Output: column of the cell where the maximum was found.
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
    qCDebug(lcMatrix) << "Matrix::identityMatrix() -- deleting old rows";
    clear();
    m_rows=dim;
    m_cols=dim;
    m_data = new (nothrow) qreal[ static_cast<size_t>(m_rows) * m_cols ]();
    Q_CHECK_PTR( m_data );
    rebuildRowPtr();
    for (int i=0;i<m_rows; i++) {
        setItem(i,i, 1);
    }
}



/**
 * @brief Makes this matrix the zero matrix of size mxn
 * @param m
 * @param n
 */
void Matrix::zeroMatrix(const int m, const int n) {
    qCDebug(lcMatrix) << "Matrix::zeroMatrix() m " << m << " n " << n;
    clear();
    m_rows=m;
    m_cols=n;
    m_data = new (nothrow) qreal[ static_cast<size_t>(m_rows) * m_cols ]();
    Q_CHECK_PTR( m_data );
    rebuildRowPtr();
}


// item()/setItem()/clearItem() are now inline in matrix.h.







/**
 * @brief Deletes row erased and column erased from this (square) matrix, shifting every
 * later row/column back by one to close the gap. Complexity: O(rows()*cols()) - rebuilds
 * the whole matrix into a new, smaller buffer (see the comment inside).
 * @param erased row/col to delete
 */
void Matrix::deleteRowColumn(int erased){
    qCDebug(lcMatrix) << "Matrix:deleteRowColumn() - will delete row and column"
             << erased
             << "m_rows before" <<  m_rows;

    // Removing a column changes every row's stride, so the smaller matrix has to be built
    // into a fresh buffer rather than shrunk in place.
    const int oldRows = m_rows;
    const int newRows = m_rows - 1;
    qreal *newData = new (nothrow) qreal[ static_cast<size_t>(newRows) * newRows ];
    Q_CHECK_PTR( newData );

    for (int i=0; i<newRows; i++) {
        const int srcI = (i < erased) ? i : i + 1;
        for (int j=0; j<newRows; j++) {
            const int srcJ = (j < erased) ? j : j + 1;
            newData[static_cast<size_t>(i) * newRows + j] =
                m_data[static_cast<size_t>(srcI) * oldRows + srcJ];
        }
    }

    delete [] m_data;
    delete [] m_rowPtr;
    m_data = newData;
    m_rows = newRows;
    m_cols = newRows;
    rebuildRowPtr();

    qCDebug(lcMatrix) << "Matrix:deleteRowColumn() - finished, new matrix:";

}


/**
 * @brief Fills every cell of this matrix with the given value. Complexity: O(rows()*cols()).
 * @param value
 */
void Matrix::fillMatrix(qreal value )   {
    for (int i=0;i< rows() ; i++)
        for (int j=0;j< cols(); j++)
            setItem(i,j, value);
}



/**
 * @brief Replaces this matrix with I - this (the identity matrix minus this matrix), in
 * place. Complexity: O(rows()*cols()).
 * @return this, now holding I - this.
 */
Matrix& Matrix::subtractFromI ()  {
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++) {
            if (i==j)
                setItem(i,j, 1.0 - item(i,j));
            else
                setItem(i,j, -item(i,j));
        }
    return *this;
}




/**
 * @brief Swaps row rowA with row rowB of this matrix, element by element.
 * Used by inverseByGaussJordanElimination() and ludcmp() during partial pivoting (moving
 * the row with the largest pivot candidate into the current position improves numerical
 * stability of the elimination). Complexity: O(cols()).
 * @param rowA
 * @param rowB
 */
void Matrix::swapRows(int rowA,int rowB){
    qCDebug(lcMatrix)<<"   swapRow() "<< rowA+1 << " with " << rowB+1;
    qreal *tempRow = new  (nothrow) qreal [ cols() ];
    Q_CHECK_PTR(tempRow);
    for ( int j=0; j<  cols(); j++) {
      tempRow[j] = item (rowB, j);
      setItem ( rowB, j, item ( rowA, j ) );
      setItem ( rowA, j,  tempRow[j] );
      }
    delete [] tempRow;
}





/**
 * @brief Multiplies every cell of this matrix, in place, by scalar f. Allows P.multiplyScalar(f).
 * Complexity: O(rows()*cols()).
 * @param f
 */
void Matrix::multiplyScalar (const qreal  & f) {
        qCDebug(lcMatrix)<< "Matrix::multiplyScalar() with f " << f;
        for (int i=0;i< rows();i++) {
            for (int j=0;j<cols();j++) {
                setItem(i,j, item(i,j) * f );
            }
        }
}


/**
 * @brief Multiplies every element of the given row by value, in place. Complexity: O(cols()).
 * @param row
 * @param value
 */
void Matrix::multiplyRow(int row, qreal value) {
    qCDebug(lcMatrix)<<"   multiplyRow() "<< row+1 << " by value " << value;
    for ( int j=0; j<  cols(); j++) {
        setItem ( row, j,  value * item (row, j) );
        qCDebug(lcMatrix)<<"   item("<< row+1 << ","<< j+1 << ") = " <<  item(row,j);
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
    qCDebug(lcMatrix)<< "Matrix::operator asignment =";
    if (this != &a){
        // Both dimensions must match before reusing the existing buffer - a shared buffer
        // can't have rows of inconsistent width the way independent row objects could.
        if (a.m_rows!=m_rows || a.m_cols!=m_cols) {
            clear();
            m_rows=a.m_rows;
            m_cols=a.m_cols;
            m_data=new (nothrow) qreal[ static_cast<size_t>(m_rows) * m_cols ];
            Q_CHECK_PTR( m_data );
            rebuildRowPtr();
        }
        const size_t n = static_cast<size_t>(m_rows) * m_cols;
        for (size_t i=0;i<n; i++)
            m_data[i]=a.m_data[i];
    }
    return *this;
}



/**
 * @brief Matrix addition: sets this matrix to a + b, cell by cell. Same result as
 * operator+(), just a different calling interface: c.sum(a,b) instead of c = a + b.
 * Complexity: O(rows()*cols()).
 * @param a
 * @param b
 */
void Matrix::sum( Matrix &a, Matrix & b)  {
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            setItem(i,j, a.item(i,j)+b.item(i,j));
}





/**
 * @brief Adds matrix b to this matrix, in place, cell by cell. Allows A += B.
 * Complexity: O(rows()*cols()).
 * @param b
 */
void Matrix::operator +=(Matrix & b) {
    qCDebug(lcMatrix)<< "Matrix::operator +=";
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            setItem(i,j, item(i,j)+b.item(i,j));
}


/**
 * @brief Matrix addition, operator +. Adds this matrix and b (same dimensions) and returns
 * the sum S. Allows S = A + B. Complexity: O(rows()*cols()).
 * @param b
 * @return Matrix S
 */
Matrix& Matrix::operator +(Matrix & b) {
    Matrix *S = new Matrix(rows(), cols());
    qCDebug(lcMatrix)<< "Matrix::operator +";
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            S->setItem(i,j, item(i,j)+b.item(i,j));
    return *S;
}


/**
 * @brief Matrix subtraction, operator -. Subtracts b (same dimensions) from this matrix and
 * returns the result S. Allows S = A - B. Complexity: O(rows()*cols()).
 * @param b
 * @return Matrix S
 */
Matrix& Matrix::operator -(Matrix & b) {
    Matrix *S = new Matrix(rows(), cols() );
    qCDebug(lcMatrix)<< "Matrix::operator -";
    for (int i=0;i< rows();i++)
        for (int j=0;j<cols();j++)
            S->setItem(i,j, item(i,j)-b.item(i,j));
    return *S;
}



/**
 * @brief Matrix multiplication, operator *. Allows P = A * B, where A is this (m x n) and
 * B is b (n x p); returns the m x p product P.
 * Complexity: O(m*n*p).
 * @param b
 * @return Matrix P
 */
Matrix& Matrix::operator *(Matrix & b) {

    qCDebug(lcMatrix)<< "Matrix::operator *";

    Matrix *P = new Matrix(rows(), b.cols());

    if ( cols() != b.rows() ) {
        qCDebug(lcMatrix)<< "Matrix::product() - ERROR! Non compatible input matrices:"
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
 * @brief Multiplies (right) this m x n matrix with given n x p matrix b, replacing this
 * matrix's own contents with the m x p product. Allows A *= B. Complexity: O(m*n*p).
 * @param b
 */
void Matrix::operator *=(Matrix & b) {

    qCDebug(lcMatrix)<< "Matrix::operator *";

    if ( cols() != b.rows() ) {
        qCDebug(lcMatrix)<< "Matrix::product() - ERROR! Non compatible input matrices:"
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
 * @brief Matrix Multiplication. Given two matrices A (mxn) and B (nxp), computes their
 * product and stores it into the calling matrix, which becomes an m x p matrix.
 * Allows P.product(A, B).
 * @param A
 * @param B
 * @param symmetry If true, the result is assumed symmetric (P(i,j)==P(j,i)): only the
 * upper triangle (i<=j) is actually computed, and each computed value is mirrored
 * directly into its (j,i) counterpart instead of being recomputed - roughly half the
 * multiply-accumulate work of the general case. Used where A and B are already known to
 * be symmetric (e.g. cocitationMatrix(), which multiplies a matrix by its own transpose).
 * Complexity: O(m*n*p), or roughly half that with symmetry=true.
 */
void Matrix::product(Matrix &A, Matrix & B, bool symmetry)  {
    qCDebug(lcMatrix)<< "Matrix::product() - symmetry" << symmetry;

    if (A.cols() != B.rows() ) {
        qCDebug(lcMatrix)<< "Matrix::product() - ERROR! Non compatible input matrices:"
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
    *this = *P;

    //this->printMatrixConsole();
}


/**
 * @brief OBSOLETE - no caller found anywhere in the codebase. Was intended to take two
 * (N x N) symmetric matrices a and b and write an upper-triangular product into this matrix
 * (the lower triangle, i>=j, left at zero). Unlike inverseByGaussJordanElimination() (also
 * uncalled, but explicitly kept for cross-checking matrix inversion), there is no stated
 * reason to keep this one - a real candidate for removal. Complexity: O(N^3).
 * @param a
 * @param b
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
 * @brief Returns the n-th power of this matrix (X^n), via exponentiation by squaring
 * (see expBySquaring2()). Used by the walks-matrix code (XM = AM.pow(length)): entry
 * (i,j) of AM^n counts the number of walks of length n from vertex i to vertex j.
 * @param n
 * @param symmetry Passed straight through to expBySquaring2()/product() - see product()'s
 * own @param symmetry for what it does.
 * @return This matrix, raised to the n-th power.
 * Complexity: O(log(n)) matrix multiplications, each O(rows()^3) - see expBySquaring2().
 */
Matrix& Matrix::pow (int n, bool symmetry)  {
    if (rows()!= cols()) {
        qCDebug(lcMatrix)<< "Matrix::pow() - Error. This works only for square matrix";
        return *this;
    }
    qCDebug(lcMatrix)<< "Matrix::pow() ";
    Matrix X, Y; //auxilliary matrices
    qCDebug(lcMatrix)<< "Matrix::pow() - creating X = this";
    X=*this; //X = this
    //X.printMatrixConsole(true);
    qCDebug(lcMatrix)<< "Matrix::pow() - creating Y = I";
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
 * For n > 4 it is more efficient than naively multiplying the base with itself repeatedly:
 * O(log(n)) matrix multiplications instead of O(n), each multiplication itself O(rows()^3).
 */
Matrix& Matrix::expBySquaring2 (Matrix &Y, Matrix &X,  int n, bool symmetry) {
    if (n==1) {
        qCDebug(lcMatrix) <<"Matrix::expBySquaring2() - n = 1. Computing PM = X*Y where "
                   "X = " ;
        //X.printMatrixConsole();
        //Y.printMatrixConsole();
        Matrix *PM = new Matrix(rows(), cols());
        PM->product(X, Y, symmetry);
        //PM->printMatrixConsole();
        return *PM;
    }
    else if ( n%2 == 0 ) { //even
        qCDebug(lcMatrix)<<"Matrix::expBySquaring2() - even n =" << n
               << "Computing PM = X * X";
        Matrix PM(rows(), cols());
        PM.product(X,X,symmetry);
        //PM.printMatrixConsole();
        return expBySquaring2 ( Y, PM, n/2 );
    }
    else  { //odd
        qCDebug(lcMatrix)<<"Matrix::expBySquaring2() - odd n =" << n
               << "First compute PM = X * Y";
        Matrix PM(rows(), cols());
        Matrix PM2(rows(), cols());
        PM.product(X,Y,symmetry);
        //PM.printMatrixConsole();
        qCDebug(lcMatrix)<<"Matrix::expBySquaring2() - odd n =" << n
               << "Now compute PM2 = X * X";
        PM2.product(X,X,symmetry);
        //PM2.printMatrixConsole();
        return expBySquaring2 ( PM, PM2, (n-1)/2 );
    }
}



/**
 * @brief Calculates the matrix-by-vector product Ax of this matrix (or the left product
 * xA, if leftMultiply is true). Used by powerIteration()'s inner loop.
 * Complexity: O(rows()*cols()).
 * @param in input array/vector, cols() elements (rows(), if leftMultiply).
 * @param out output array, rows() elements (cols(), if leftMultiply).
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
 *
 * Meaning: start from any vector x, repeatedly multiply by the matrix and rescale back to
 * unit length - the vector converges to the eigenvector for the matrix's largest eigenvalue
 * (lambda_max), which is exactly the vector eigenvector centrality reports.
 *
 * We use C arrays instead of std::vectors or anything else,
 * as we know from start the size (n) of vectors x and tmp
 * This approach is faster than using std::vector when n > 1000
 * @note Uses n = rows() throughout (for tmp's size, and as both the row and column count
 * passed to productByVector()/distanceEuclidean()/distanceManhattan()) - correct only for
 * square matrices. Unlike pow(), this method does not check rows()==cols() itself; it
 * relies on the caller (centralityEigenvector()) only ever passing a square (adjacency)
 * matrix.
 * Complexity: O(maxIter * n^2) - each iteration is one O(n^2) productByVector() call plus
 * a handful of O(n) passes; iterates until the vector's Manhattan distance to its previous
 * value drops below eps, or maxIter is reached.
 * @param x
 * @param xsum
 * @param xmax
 * @param xmaxi
 * @param xmin
 * @param xmini
 * @param eps
 * @param maxIter
 * @param cancelCheck Optional callback checked once per iteration; if it returns true, the
 * loop stops early (x/xsum/xmax/xmin reflect the last completed iteration, not a full result).
 * Defaults to nullptr (never cancels), so existing callers are unaffected.
 * @param lambdaMax Optional out param: the largest eigenvalue itself, read off from the
 * pre-normalization vector length on the final iteration (norm(Ax) ~= lambda_max once x has
 * converged to unit length). Callers that only need the eigenvector (e.g.
 * centralityEigenvector()) can leave this nullptr; callers that need to validate a convergence
 * bound like Katz's alpha < 1/lambda_max need it.
 */
void Matrix::powerIteration (
        qreal x[],
        qreal &xsum,
        qreal &xmax,
        int &xmaxi,
        qreal &xmin,
        int &xmini,
        const qreal eps,
        const int &maxIter,
        std::function<bool()> cancelCheck,
        qreal *lambdaMax) {

    qCDebug(lcMatrix) << "Matrix::powerIteration() - maxIter"
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
        if (cancelCheck && cancelCheck()) {
            qCDebug(lcMatrix) << "Matrix::powerIteration() - canceled at iteration" << iter;
            break;
        }

        qCDebug(lcMatrix) << "Matrix::powerIteration() - iteration"
                 << iter ;

        // calculate the matrix-by-vector product Ax and
        // store the result to vector tmp
        productByVector(x, tmp, false);

        qCDebug(lcMatrix) << "Matrix::powerIteration() - tmp = Ax ="
                 << tmp;

        // calculate the euclidean length of the resulting vector
        // which will be the denominator in the vector normalization
        norm = distanceEuclidean(tmp, n);

        qCDebug(lcMatrix) << "Matrix::powerIteration() - norm" << norm;

        // norm should never be zero, but in case there is
        // numerical error, we set it to 1
        if (!norm) {
            qCDebug(lcMatrix) << "### Matrix::powerIteration() - norm = 0 !!!";
            norm = 1;
        }


        // normalize vector tmp to unit vector for next iteration
        xsum = 0;
        for(int i = 0; i < n; i++) {
           tmp[i] = tmp[i] / norm;
        }
        qCDebug(lcMatrix) << "Matrix::powerIteration() - tmp / norm "
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


        qCDebug(lcMatrix) << "Matrix::powerIteration() - end of iteration"
                 << iter << "\n"
                 << "x" << x  << "\n"
                 << "distance from previous x " << distance
                 << "sum" << xsum
                 << "xmax" << xmax
                 << "xmin" << xmin;

        iter ++;
        if (iter > maxIter)
            break;

    } while ( distance > eps);

    // norm is ||Ax|| from the final iteration, with x already unit-length from the previous
    // one - at convergence this approximates the dominant eigenvalue (lambda_max).
    if (lambdaMax)
        *lambdaMax = norm;

     delete [] tmp;
}



/**
 * @brief Returns the transpose of this matrix (T(i,j) = this(j,i)). Allows T = A.transpose().
 * Complexity: O(rows()*cols()).
 * @return Matrix T
 */
Matrix& Matrix::transpose() {
    Matrix *T = new Matrix(cols(), rows());
    //T->zeroMatrix(cols(), rows());
    qCDebug(lcMatrix)<< "Matrix::transpose()";
    for (int i=0;i< cols();i++) {
        for (int j=0;j<rows();j++) {
            T->setItem(i,j, item(j,i));

        }
    }
    return *T;
}





/**
 * @brief Returns the cocitation matrix of this matrix (C = A * A^T). Allows
 * T = A.cocitationMatrix(). C(i,j) counts how many nodes both i and j point to (or, read the
 * other way with the transpose on the other side, how many nodes point to both i and j) -
 * the basis of bibliometric cocitation/coupling analysis.
 * Complexity: O(rows()^3) - transpose() is O(N^2), but the product() call that follows
 * dominates at O(N^3).
 * @return Matrix T
 */
Matrix& Matrix::cocitationMatrix() {
    Matrix *T = new Matrix(cols(), rows());
    qCDebug(lcMatrix)<< "Matrix::cocitationMatrix() this transpose";
    //this->transpose().printMatrixConsole();
    T->product(this->transpose(),*this, true);
    return *T;
}




/**
 * @brief Returns the degree matrix of this matrix: a diagonal matrix where S(i,i) is the
 * sum of row i (i.e. vertex i's degree, if this is an adjacency matrix). Allows
 * S = A.degreeMatrix(). Used by laplacianMatrix(). Complexity: O(rows()*cols()).
 * @return Matrix S
 */
Matrix& Matrix::degreeMatrix() {
    Matrix *S = new Matrix(rows(), cols());
    qCDebug(lcMatrix)<< "Matrix::degreeMatrix()";
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
 * @brief Returns the Laplacian of this matrix: an N x N matrix L = D - A, where D is this
 * matrix's degreeMatrix(). Allows S = A.laplacianMatrix(). Complexity: O(rows()*cols()).
 * @return Matrix S
 */
Matrix& Matrix::laplacianMatrix() {
    Matrix *S = new Matrix(rows(), cols());
    //S->zeroMatrix(rows(), cols());
    qCDebug(lcMatrix)<< "Matrix::laplacianMatrix()";
    *S = (this->degreeMatrix()) - *this;
    return *S;
}







/**
 * @brief Inverts matrix A by Gauss-Jordan elimination with partial pivoting: starts this
 * matrix as the identity, then applies the same row operations to both A and this that
 * drive A to the identity - by the time A has become the identity, this matrix has become
 * A's inverse. Input: matrix A. Output: A becomes the identity matrix; this matrix becomes
 * A's inverse and is returned. Complexity: O(n^3).
 * @note Unreachable in the current codebase - createMatrixAdjacencyInverse()'s only caller
 * always passes "lu" (Matrix::inverse()/ludcmp(), below), never "gauss". Kept rather than
 * removed: matrix inversion is numerically sensitive code, and a second independent
 * implementation is useful for cross-checking even while unused.
 * @param A
 * @return This matrix, now holding A's inverse.
 */
Matrix& Matrix::inverseByGaussJordanElimination(Matrix &A){
	qCDebug(lcMatrix)<< "Matrix::inverseByGaussJordanElimination()";
	int n=A.cols();
    qCDebug(lcMatrix)<<"Matrix::inverseByGaussJordanElimination() - build I size " << n
             << " This will become A^-1 in the end";

    identityMatrix( n );

	int l=0, m_pivotLine=0;
    qreal m_pivot=0, temp_pivot=0, elim_coef=0;

    for ( int j=0; j< n; j++) { // for n, it is the last diagonal element of A
	    l=j+1;
	    m_pivotLine=-1;
	    m_pivot = A.item(j,j);
	    qCDebug(lcMatrix) << "inverseByGaussJordanElimination() at column " << j+1
		    << " Initial pivot " << m_pivot ;
        for ( int i=l; i<n; i++) {
            temp_pivot = A.item(i,j);
            if ( qFabs( temp_pivot ) > qFabs ( m_pivot ) ) {
                qCDebug(lcMatrix) << " A("<< i+1 << ","<< j+1  << ") = " <<  temp_pivot
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


	    qCDebug(lcMatrix)<<"   multiplyRow() "<< j+1 << " by value " << 1/m_pivot ;
        for ( int k=0; k<  rows(); k++) {
            A.setItem ( j, k,  (1/m_pivot) * A.item (j, k) );
            setItem ( j, k,  (1/m_pivot) * item (j, k) );
            qCDebug(lcMatrix)<<"   A.item("<< j+1 << ","<< k+1 << ") = " <<  A.item(j,k);
            qCDebug(lcMatrix)<<"   item("<< j+1 << ","<< k+1 << ") = " <<  item(j,k);
        }

	    qCDebug(lcMatrix) << "eliminate variables FromRowsBelow()" << j+1 ;
        for ( int i=0; i<  rows(); i++) {
		 qCDebug(lcMatrix)<<"   Eliminating item("<< i+1 << ","<< j+1 << ") = "
			 <<  A.item(i,j) << " while at column j="<<j+1;
		 if ( A.item(i,j)==0 ){
		    qCDebug(lcMatrix)<< " ...already eliminated - continue";
		    continue;
		}
		 if ( i == j){
		     qCDebug(lcMatrix)<< " ...skip pivotline - continue";
		    continue;
		}
		elim_coef=A.item (i, j);
        for ( int k=0; k<  cols(); k++) {
		    qCDebug(lcMatrix)<<"   A.item("<< i+1 << ","<< k+1 << ") = " <<  A.item(i,k)
			    << " will be subtracted by " << " A.item ("<< i+1
			    << ", "<< j+1 << ") x A.item(" << j+1 << ","<<k+1
			    <<") =" << elim_coef * A.item(j,k) ;
		    A.setItem ( i, k,   A.item (i, k) -  elim_coef * A.item(j, k)  );
		    qCDebug(lcMatrix)<<"   A.item("<< i+1 << ","<< k+1 << ") = " <<  A.item(i,k);

		    qCDebug(lcMatrix)<<"   item("<< i+1 << ","<< k+1 << ") = " <<  item(i,k)
			    << " will be subtracted by " << " A.item ("<< i+1
			    << ", "<< j+1 << ") x item(" << j+1 << ","<<k+1
			    <<") =" << elim_coef * item(j,k)  <<  " = "
			    << elim_coef << " x " << item(j,k) ;

		    setItem ( i, k,   item (i, k) -  elim_coef * item(j, k)  );
		    qCDebug(lcMatrix)<<"   item("<< i+1 << ","<< k+1 << ") = " <<  item(i,k);

		}
	    }

	}
	return *this;
}



/**
 * @brief Given matrix a, it replaces a by the LU decomposition of a rowwise permutation of itself.
 * Used in combination with lubksb to solve linear equations or invert a matrix.
 * Complexity: O(n^3).
 * @param a: input matrix n x n and output arranged as in Knuth's equation (2.3.14)
 * @param n: input size of matrix
 * @param indx: output vector, records the row permutation effected by the partial pivoting
 * @param d: output as ±1 depending on whether the number of row interchanges was even or odd
 * @param cancelCheck Optional callback, checked once per outer-loop iteration in both the O(n^2)
 * scaling pass and the O(n^3) Crout's-method pass; if it returns true, decomposition stops early
 * and this returns false (same as the singular-matrix case - callers must not distinguish the two
 * from this return value alone; inverse()'s post-cancelCheck() logic is what does that, not this).
 * Defaults to nullptr (never cancels), so existing callers are unaffected. Added because ludcmp()
 * is the dominant O(n^3) cost inverse() wraps - without this, inverse()'s own per-column
 * cancelCheck (see below) can never fire in time, since it's only reached after ludcmp() already
 * finished (see WS15 P1, roadmap_ws15_cancellation_progress_unification.md).
 * @return:
 *
 * Code adapted from Knuth's Numerical Recipes in C, pp 46
 *
 */
bool Matrix::ludcmp (Matrix &a, const int &n, int indx[], qreal &d, std::function<bool()> cancelCheck) {
    qCDebug(lcMatrix) << "Matrix::ludcmp () - decomposing matrix a to L*U";
    int i=0, j=0, imax=0, k;
    qreal big,temp;
    //vv=vector<qreal>(1,n);
    qreal *vv;            // vv stores the implicit scaling of each row
    vv=new (nothrow) qreal [n];
    Q_CHECK_PTR( vv );

//    QTextStream stream(stdout);
//    stream << "a = LU = " << a ;

    d=1.0;               // No row interchanges yet.

    qCDebug(lcMatrix) << "Matrix::ludcmp() - loop over row to get scaling info" ;
    for (i=0;i<n;i++) {  // Loop over rows to get the implicit scaling information.
        if (cancelCheck && cancelCheck()) {
            qCDebug(lcMatrix) << "Matrix::ludcmp() - canceled at scaling row" << i;
            delete[] vv;
            return false;
        }
        qCDebug(lcMatrix) << "Matrix::ludcmp() - row i " <<  i+1;
        big=0;
        for (j=0;j<n;j++) {
            if ((temp=fabs( a[i][j] ) ) > big)
                big=temp;
        }
        if (big == 0)  //       No nonzero largest element.
        {
            qCDebug(lcMatrix) << "Matrix::ludcmp() - Singular matrix in routine ludcmp";
            return false;
        }
        vv[i]=1.0/big;  //  Save the scaling.
        qCDebug(lcMatrix) << "Matrix::ludcmp() - big element in row i " << i+1 << " is "<< big << " row scaling vv[i] " << vv[i];
    }

    qCDebug(lcMatrix) << "Matrix::ludcmp() - Start Crout's LOOP over columns";

    for (j=0;j<n;j++) //     This is the loop over columns of Crout’s method.
    {
        if (cancelCheck && cancelCheck()) {
            qCDebug(lcMatrix) << "Matrix::ludcmp() - canceled at Crout column" << j;
            delete[] vv;
            return false;
        }
        qCDebug(lcMatrix) << "Matrix::ludcmp() - COLUMN j " <<  j+1 << " search largest pivot";
        big=0;  //      Initialize for the search for largest pivot element.
        imax = j;

        for (i=j;i<n;i++)
        {
            if ( ( temp = vv[i] * fabs( a[i][j] ) ) > big)
            {   //  Is the figure of merit for the pivot better than the best so far?
                big=temp;
                imax=i;
                qCDebug(lcMatrix) << "Matrix::ludcmp() - found new largest pivot at row " <<  imax+1 << " big " << temp;
            }
        }

        qCDebug(lcMatrix) << "Matrix::ludcmp() - check for row interchange ";
        if (j != imax) //          Do we need to interchange rows?
        {
            qCDebug(lcMatrix) << "Matrix::ludcmp() - interchanging rows " << imax+1 << " and " << j+1;
            for ( k=0; k<n; k++ ) { //            Yes, do so...
                temp=a[imax][k];
                a[imax][k] = a[j][k];
                a[j][k] = temp;
            }
            d = -(d);  //..and change the parity of d.
            vv[imax]=vv[j];  //         Also interchange the scale factor.
            qCDebug(lcMatrix) << "Matrix::ludcmp() - imax  " << imax+1  << " vv[imax]" << vv[imax] << "new parity d " << d;
        }
        indx[j]=imax;
        qCDebug(lcMatrix) << "Matrix::ludcmp() - indx[j]=imax=" <<  indx[j] +1;
        // Fix #269: 'big' here is the chosen pivot's magnitude, already scaled by its row's
        // own maximum (vv[j] = 1/rowMax) - a dimensionless, relative quantity, not a raw
        // value. A singular (or numerically indistinguishable from singular) matrix drives
        // this toward zero without necessarily hitting it exactly - relying only on an exact
        // a[j][j]==0 check (as before) misses that case, silently substituting TINY and
        // dividing by it later, which is exactly what produced the reported 1e+20 garbage
        // inverse entries on a genuinely singular input. Checking the relative pivot
        // magnitude here catches both the exact-zero and near-zero cases.
        if ( big < RELATIVE_SINGULARITY_TOLERANCE ) {
            qCDebug(lcMatrix) << "Matrix::ludcmp() - Singular matrix: pivot magnitude" << big
                               << "at column" << j+1 << "below relative tolerance";
            delete[] vv;
            return false;
        }

        for (i=j+1;i<n;i++) {
            //     Now, divide by the pivot element.
            temp=a[i][j] /=  a[j][j] ;
            qCDebug(lcMatrix) << "Matrix::ludcmp() - j " << j+1<< " dividing by pivot " << a.item(j,j) << " temp  = " << temp;
            for (k=j+1;k<n;k++) {       //reduce remaining submatrix
                a[i][k] -= ( temp * a[j][k] );
                qCDebug(lcMatrix) << "Matrix::ludcmp() - lower a["<< i+1 << "][" << k+1 <<"] = " << a[i][k];
            }
        }


//           stream << "\n" << "at j " << j+1 << " matrix a = LU = " << a ;
    }  // Go back for the next column in the reduction.


 //free_vector(vv,1,n);
    qCDebug(lcMatrix) << "delete vector vv";
    delete[] vv;


//   stream << "final a = LU = " << a ;


    return true;

}




/**
 * @brief Solves the set of n linear equations A·X = b, where A nxn matrix
 * decomposed as L·U (L lower triangular and U upper triangular)
 * by forward substitution and  backsubstitution.
 * Complexity: O(n^2) - cheap compared to ludcmp()'s O(n^3) decomposition, which is exactly
 * why ludcmp() is only run once and lubksb() can then be reused per right-hand side.
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
    qCDebug(lcMatrix) << "Matrix::lubksb() - ";
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
        qCDebug(lcMatrix) << "Matrix::lubksb() "<< "i " << i  << " ip=indx[i] " << ip <<  " b[ip] " << b[ip] << " b[i] " << b[i] <<  "sum " << sum ;
        b[i]=sum;
    }
    for ( i=(n-1);i>=0;i--) {  // Now we do the backsubstitution, equation (2.3.7).
        sum=b[i];
        qCDebug(lcMatrix) << "Matrix::lubksb() backsubstitution: "<< "i " << i  << " b[i] " << b[i] <<  "sum " << sum ;
        for ( j=i+1;j<n;j++)
            sum -= a[i][j]*b[j];
        b[i]=sum/a[i][i]; //  Store a component of the solution vector X. All done!
        qCDebug(lcMatrix) << "Matrix::lubksb() backsubstitution: "<< "i " << i  <<  "sum " << sum << " a[i][i] " << a[i][i]   << " b[i] " << b[i] ;
    }
}



/**
 * @brief Computes and returns the inverse of matrix a, into this matrix. Allows b.inverse(a).
 * Decomposes a once via ludcmp() (LU decomposition with partial pivoting), then solves n
 * separate systems - one per column of the identity matrix - via lubksb(), each giving one
 * column of the inverse. This is the "lu" method createMatrixAdjacencyInverse() actually
 * uses (as opposed to inverseByGaussJordanElimination()'s "gauss" method, which has no
 * caller). If a is singular, ludcmp() returns false and this matrix is left unmodified
 * (see the weak-singularity-detection finding, #269, in roadmap_ws5_matrices_modernization.md).
 * Complexity: O(n^3) for the one-time ludcmp() decomposition, plus O(n) calls to lubksb()
 * at O(n^2) each (one per column) - O(n^3) overall, same order as the decomposition itself.
 * @param a
 * @param cancelCheck Optional callback, also forwarded to ludcmp() (see its own doc) since
 * ludcmp()'s one-time O(n^3) decomposition is what this method actually spends most of its time
 * in - checked there once per outer-loop iteration, and here once per column, before that
 * column's lubksb() call; if it returns true, the loop stops early and this matrix holds only the
 * columns already solved (the rest are left at whatever resize()/identityMatrix() initialized them
 * to - not a valid inverse). Defaults to nullptr (never cancels), so existing callers are
 * unaffected. Callers must check their own cancellation flag after calling this, not infer it from
 * the return value - ludcmp() returns false identically for "canceled" and "singular", and this
 * method has no way to tell those apart either.
 * @return true if a's inverse was fully computed into this matrix; false if a is singular
 * (Fix #269: now a real relative-tolerance pivot check in ludcmp(), not a weak after-the-fact
 * scan for nonzero entries) or cancelCheck fired, in which case this matrix is left
 * unmodified/partial - callers must not treat its contents as valid without checking the
 * return value first.
 */
bool Matrix::inverse(Matrix &a, std::function<bool()> cancelCheck)
{
    int i,j, n=a.rows();
    qreal d;
    //qreal *col = new qreal[n];
    qreal *col = new  (nothrow) qreal [ n ];
    Q_CHECK_PTR(col);

    //int indx[n];
    int *indx = new  (nothrow) int [ n ];
    Q_CHECK_PTR(indx);

    qCDebug(lcMatrix) << "Matrix::inverse() - inverting matrix a - size " << n;
    if (n==0) {
        delete[] col;
        delete[] indx;
        return true;
    }
    if ( ! ludcmp(a,n,indx,d,cancelCheck) )
    { //  Decompose the matrix just once.
        qCDebug(lcMatrix) << "Matrix::inverse() - matrix a singular or canceled - RETURN";
        delete[] col;
        delete[] indx;
        return false;
    }

    qCDebug(lcMatrix) << "Matrix::inverse() - find inverse by columns";
    bool completed = true;
    for ( j=0; j<n; j++) {    //    Find inverse by columns.
        if (cancelCheck && cancelCheck()) {
            qCDebug(lcMatrix) << "Matrix::inverse() - canceled at column" << j;
            completed = false;
            break;
        }

        for( i=0; i<n; i++)
            col[i]=0;
        col[j]=1.0;

        qCDebug(lcMatrix) << "Matrix::inverse() - call lubksb";
        lubksb(a,n,indx,col);

        for( i=0; i<n; i++) {
             (*this)[i][j] = col[i];
        }

    }
        qCDebug(lcMatrix) << "Matrix::inverse() - finished!";

    delete[] col;
    delete[] indx;
    return completed;
}





/**
 * @brief Solves the linear system A*x = b, where A is this matrix, in place: b is
 * overwritten with the solution vector x. Allows A.solve(b). Works on a private copy of
 * this matrix (ludcmp() would otherwise decompose - and so overwrite - the caller's own
 * data), via the same ludcmp()+lubksb() pair inverse() uses. Complexity: O(n^3), dominated
 * by the one-time ludcmp() decomposition (lubksb() itself is only O(n^2)).
 * @param b Right-hand-side vector on input, solution vector x on output.
 * @return false if A is singular (b is left unmodified) or the working copy couldn't be
 * allocated; true on success.
 */
bool Matrix::solve(qreal b[])
{

    Matrix *A = new Matrix(this->rows(), this->cols());

    *A = *this;

    int n=rows();
    qreal d;

//    int indx[n];
    int *indx = new  (nothrow) int [ n ];
    if (!indx) {
        delete A;
        return false;
    }
    Q_CHECK_PTR(indx);

    qCDebug(lcMatrix) << "Matrix::solve() - solving A x  - size " << n;
    if (n==0) {
        return false;
    }
    if ( ! ludcmp(*A,n,indx,d) )
    { //  Decompose the matrix just once.
        delete A;
        delete[] indx;
        qCDebug(lcMatrix) << "Matrix::solve() - matrix a singular - RETURN";
        return false ;
    }

    lubksb(*A, n, indx, b);

    return true;
}


/**
 * @brief Computes a dissimilarities matrix T: T(i,k) is how different variable i and
 * variable k are, under the chosen metric, treating either this matrix's rows, its columns,
 * or both (concatenated) as the "variables" being compared. Backs the Distances dialog's
 * Euclidean/Manhattan/Jaccard/Hamming/Chebyshev options (see graph_reports.cpp's
 * MATRIX_DISTANCES_* cases).
 * @param metric One of the METRIC_* constants declared at the top of matrix.h (Jaccard,
 * Hamming, Euclidean, Manhattan, or Chebyshev - simple matching and Pearson are handled by
 * similarityMatrix()/pearsonCorrelationCoefficients() instead, not here).
 * @param varLocation "Rows", "Columns", or "Both" - which axis holds the variables being compared.
 * @param diagonal If true, i==k / k==j comparisons are included; if false, they're skipped
 * (a variable is never compared against itself).
 * @param considerWeights Currently unused (Q_UNUSED) - accepted for interface symmetry with
 * similarityMatrix()/pearsonCorrelationCoefficients(), which do use it.
 * Complexity: O(N^2 * M), where N is the number of variables being compared and M is the
 * length of each variable's sample (the other axis) - a triple-nested loop, effectively
 * O(N^3) when varLocation is "Rows" or "Columns" (M==N there).
 * @return Matrix T, the dissimilarities matrix.
 */
Matrix& Matrix::distancesMatrix(const int &metric,
                        const QString varLocation,
                        const bool &diagonal,
                        const bool &considerWeights,
                        std::function<bool()> cancelCheck) {
    Q_UNUSED(considerWeights);

    Matrix *T = new Matrix(cols(), rows());

    qCDebug(lcMatrix)<< "Matrix::distancesMatrix() -"
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

        QList<qreal> mean (N,0); // holds mean values

        qCDebug(lcMatrix)<< "Matrix::distancesMatrix() - input matrix:";
        //this->printMatrixConsole();

        for (int i = 0 ; i < N ; i++ ) {
            if (cancelCheck && cancelCheck()) {
                return *T;
            }
            sum = 0 ;
            for (int k = i ; k < N ; k++ ) {
                distTemp = 0;
                ties = 0;
                max = 0;
                for (int j = 0 ; j < N ; j++ ) {


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

        QList<qreal> mean (N,0); // holds mean values

        qCDebug(lcMatrix)<< "Matrix::distancesMatrix() -"
                <<"input matrix";
        //printMatrixConsole(true);

        for (int i = 0 ; i < N ; i++ ) {
            if (cancelCheck && cancelCheck()) {
                return *T;
            }
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

        QList<qreal> mean (N,0); // holds mean values

        //create augmented matrix (concatenated rows and columns) from input matrix
        for (int i = 0 ; i < N  ; i++ ) {
            for (int j = 0 ; j < N  ; j++ ) {
                CM.setItem(j,i, item(i,j));
                CM.setItem(j + N,i, item(j,i));
            }
        }

        qCDebug(lcMatrix)<< "Matrix::distancesMatrix() -"
                <<"input matrix";
        //CM.printMatrixConsole(true);


        for (int i = 0 ; i < N ; i++ ) {
            if (cancelCheck && cancelCheck()) {
                return *T;
            }

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
    qCDebug(lcMatrix) << "Matrix::distancesMatrix() - FINISHED - Returning matrix:";
    //T->printMatrixConsole();
    return *T;
}




/**
 * @brief Computes a pairwise similarity matrix SCM: SCM(i,k) is how alike variable i and
 * variable k are, under the chosen matching measure, treating either AM's rows, its columns,
 * or both (concatenated) as the "variables" being compared. The mirror image of
 * distancesMatrix() (similarity instead of dissimilarity) - backs the Similarity dialog's
 * simple-matching/Jaccard/Hamming/Cosine options.
 * @param AM Input matrix whose rows/columns/both are being compared.
 * @param measure One of the METRIC_* constants (simple matching, Jaccard, Hamming, Cosine -
 * Pearson is handled separately by pearsonCorrelationCoefficients()).
 * @param varLocation "Rows", "Columns", or "Both" - which axis holds the variables being compared.
 * @param diagonal If true, i==k comparisons are included; if false, a variable is never
 * compared against itself.
 * @param considerWeights Whether edge weights factor into the match/mismatch decision.
 * @return Matrix SCM, N x N (N = number of variables being compared), with a similarity
 * score for every pair.
 * Complexity: O(N^2 * M), same shape as distancesMatrix() - see its complexity note.
 */
Matrix& Matrix::similarityMatrix(Matrix &AM,
                                   const int &measure,
                                   const QString varLocation,
                                   const bool &diagonal,
                                   const bool &considerWeights,
                                   std::function<bool()> cancelCheck){

    Q_UNUSED(considerWeights);

    qCDebug(lcMatrix)<< "Matrix::similarityMatrix() -"
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

        QList<qreal> mean (N,0); // holds mean values

        qCDebug(lcMatrix)<< "Matrix::similarityMatrix() -"
                <<"input matrix";
        //AM.printMatrixConsole(true);

        for (int i = 0 ; i < N ; i++ ) {
            if (cancelCheck && cancelCheck()) {
                return *this;
            }
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


                qCDebug(lcMatrix) << "matches("<<i+1<<","<<k+1<<") =" << matches

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

        QList<qreal> mean (N,0); // holds mean values

        qCDebug(lcMatrix)<< "Matrix::similarityMatrix() -"
                <<"input matrix";
        //AM.printMatrixConsole(true);

        for (int i = 0 ; i < N ; i++ ) {
            if (cancelCheck && cancelCheck()) {
                return *this;
            }
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
                qCDebug(lcMatrix) << "matches("<<i+1<<","<<k+1<<") =" << matches

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

        QList<qreal> mean (N,0); // holds mean values


        //create augmented matrix (concatenated rows and columns) from input matrix
        for (int i = 0 ; i < N  ; i++ ) {
            for (int j = 0 ; j < N  ; j++ ) {
                CM.setItem(j,i, AM.item(i,j));
                CM.setItem(j + N,i, AM.item(j,i));
            }
        }
        qCDebug(lcMatrix)<< "Matrix::similarityMatrix() -"
                <<"input matrix";
        //CM.printMatrixConsole(true);


        for (int i = 0 ; i < N ; i++ ) {

            if (cancelCheck && cancelCheck()) {
                return *this;
            }

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

                qCDebug(lcMatrix) << "matches("<<i+1<<","<<k+1<<") =" << matches

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
 * @brief Computes the Pearson product-moment correlation coefficient between every pair of
 * variables (AM's rows or its columns, per varLocation), where each variable's "sample" is
 * the sequence of values across the other axis. r ranges -1 (perfect negative correlation)
 * to +1 (perfect positive correlation), with 0 meaning no linear correlation.
 * @param AM Input matrix whose rows or columns are being compared.
 * @param varLocation "Rows" or "Columns" - which axis holds the variables being compared.
 * @param diagonal If true, i==k comparisons are included (always r=1, trivially); if false,
 * a variable is never compared against itself.
 * @return Matrix N x N (N = number of variables being compared) of Pearson r values.
 * Complexity: O(N^2 * M), same shape as distancesMatrix() - see its complexity note.
 */
Matrix& Matrix::pearsonCorrelationCoefficients(Matrix &AM,
                                               const QString &varLocation,
                                               const bool &diagonal,
                                               std::function<bool()> cancelCheck){
    qCDebug(lcMatrix)<< "Matrix::pearsonCorrelationCoefficients() -"
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

        QList<qreal> mean (N,0); // holds mean values
        QList<qreal> sigma(N,0);
        qCDebug(lcMatrix)<< "Matrix::pearsonCorrelationCoefficients() -"
                <<"input matrix";
        //AM.printMatrixConsole(true);

        for (int i = 0 ; i < N ; i++ ) {
            if (cancelCheck && cancelCheck()) {
                return *this;
            }

            for (int k = i ; k < N ; k++ ) {

                qCDebug(lcMatrix) << "comparing rows i"<<i+1<<"k"<<k+1;

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

                    qCDebug(lcMatrix) << "AM.item(i,j)=AM.item("<<i+1<<","<<j+1<<") = "<<AM.item(i,j)
                             << " mean(i)=mean("<<i+1<<") = "<<mean[i]
                                << "AM.item(k,j)=AM.item("<<k+1<<","<<j+1<<") = "<<AM.item(k,j)
                                << " mean(k)=mean("<<k+1<<") = "<<mean[k];

                    if (!diagonal && (i==j ) ) {
                        qCDebug(lcMatrix) << "skipping because i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
                        continue;
                    }
                    if (!diagonal && (k==j) ) {
                        qCDebug(lcMatrix) << "skipping because i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
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


                qCDebug(lcMatrix) << "covariance("<<i+1<<","<<k+1<<") =" << covariance
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

        QList<qreal> mean (N,0); // holds mean values
        QList<qreal> sigma(N,0);

        qCDebug(lcMatrix)<< "Matrix::pearsonCorrelationCoefficients() -"
                <<"input matrix";
        //AM.printMatrixConsole(true);


        for (int i = 0 ; i < N ; i++ ) {
            if (cancelCheck && cancelCheck()) {
                return *this;
            }

            for (int k = i ; k < N ; k++ ) {

                qCDebug(lcMatrix) << "comparing columns i"<<i+1<<"k"<<k+1;

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
                        qCDebug(lcMatrix) << "skipping because i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
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


                qCDebug(lcMatrix) << "covariance("<<i+1<<","<<k+1<<") =" << covariance
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

        QList<qreal> mean (N,0); // holds mean values
        QList<qreal> sigma(N,0);


        //create augmented matrix (concatenated rows and columns) from input matrix
        for (int i = 0 ; i < N  ; i++ ) {
            for (int j = 0 ; j < N  ; j++ ) {
                CM.setItem(j,i, AM.item(i,j));
                CM.setItem(j + N,i, AM.item(j,i));
            }
        }

        qCDebug(lcMatrix)<< "Matrix::pearsonCorrelationCoefficients() -"
                <<"input matrix";
        //CM.printMatrixConsole(true);


        for (int i = 0 ; i < N ; i++ ) {  //a column
            if (cancelCheck && cancelCheck()) {
                return *this;
            }

            for (int k = i ; k < N ; k++ ) {  // next column

               qCDebug(lcMatrix) << "comparing augmented columns i"<<i+1<<"k"<<k+1;

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
                    qCDebug(lcMatrix) << "CM.item(j,i)=CM.item("<<j+1<<","<<i+1<<") = "<<CM.item(j,i)
                             << " mean(i)=mean("<<i+1<<") = "<<mean[i]
                                << "CM.item(j,k)=CM.item("<<j+1<<","<<k+1<<") = "<<CM.item(j,k)
                                << " mean(k)=mean("<<k+1<<") = "<<mean[k];


                    if (!diagonal) {
                        if ( (i==j || k==j )) {
                            qCDebug(lcMatrix) << "skipping because i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
                            continue;
                        }
                        if ( j>=N && ( (i+N)==j || (k+N)==j )) {
                            qCDebug(lcMatrix) << "skipping because j>=N and i"<<i+1<<"k"<<k+1 <<"j"<<j+1;
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

                qCDebug(lcMatrix) << "final covariance("<<i+1<<","<<k+1<<") =" << covariance
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

    qCDebug(lcMatrix) << "Printing matrix m to provided output stream...";

    int actorNumber=1, fieldWidth = 13;
    qreal maxVal, minVal, maxAbsVal, element;
    bool hasRealNumbers=false;

    m.findMinMaxValues(minVal, maxVal, hasRealNumbers);

    maxAbsVal = ( fabs(minVal) > fabs(maxVal) ) ? fabs(minVal) : fabs(maxVal) ;


    os << qSetFieldWidth(0) << Qt::endl ;

    os << "- Values:        "
       << ( (hasRealNumbers) ? ("real numbers (printed decimals 3)") : ("integers only" ) ) << Qt::endl;

    os << "- Max value:  ";

    if ( maxVal==RAND_MAX )
        os <<  infinity << " (=not connected nodes, in distance matrix)";
    else
        os <<   maxVal;

    os << qSetFieldWidth(0) << Qt::endl ;

    os << "- Min value:   ";

    if ( minVal==RAND_MAX )
        os << infinity;
    else
        os << minVal;


    os << qSetFieldWidth(0) << Qt::endl << Qt::endl;

    os << qSetFieldWidth(7) << Qt::fixed << Qt::right << "v"<< qSetFieldWidth(3) << "" ;

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

        os << Qt::fixed << actorNumber;
    }

    os << qSetFieldWidth(0) << Qt::endl;

    os << qSetFieldWidth(7)<< Qt::endl;

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


        os << Qt::fixed << actorNumber
            << qSetFieldWidth(3) <<"" ;

        for (int c = 0; c < m.cols(); ++c) {
            element = m.item(r,c) ;
            os << qSetFieldWidth(fieldWidth) << Qt::fixed << Qt::right;
            if ( element == RAND_MAX)  // we print inf symbol instead of RAND_MAX (distances matrix).
                os << Qt::fixed << Qt::right << qSetFieldWidth(fieldWidth) << infinity ;
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
        os << qSetFieldWidth(0) << Qt::endl;
    }
    return os;
}





/**
 * @brief Writes this matrix as an HTML table to os, one row of table cells per matrix row.
 * @warning Do not use on a network with disabled/deleted nodes - row/column headers are
 * generated from a running counter, not the real vertex number, so they go out of sync
 * with actual actor numbers once any vertex has been deleted.
 * @param os Output stream to write the HTML table to.
 * @param markDiag If true, diagonal cells get distinct styling.
 * @param plain If true, skip HTML styling/highlighting (a plain table).
 * @param printInfinity If true, RAND_MAX cells print as the infinity symbol (unreachable/no
 * edge) instead of the raw number.
 * @return true on success.
 * Complexity: O(rows()*cols()).
 */
bool Matrix::printHTMLTable(QTextStream& os,
                            const bool markDiag,
                            const bool &plain,
                            const bool &printInfinity){
    qCDebug(lcMatrix) << "Matrix::printHTMLTable()";
    int elementLabel=0, rowCount = 0;
    qreal maxVal, minVal, element;
    bool hasRealNumbers=false;

    findMinMaxValues(minVal, maxVal, hasRealNumbers);

    //maxAbsVal = ( fabs(minVal) > fabs(maxVal) ) ? fabs(minVal) : fabs(maxVal) ;

    os <<  ( (hasRealNumbers) ? qSetRealNumberPrecision(3) : qSetRealNumberPrecision(0) ) ;

    if (plain) {
        os << "<pre>";

        // print first/header row
        os << "<span class=\"header\">" << qSetFieldWidth(5) << Qt::right << "A/A";
        os << Qt::fixed << qSetFieldWidth(10) << Qt::right ;
        for (int r = 0; r < cols(); ++r) {
            elementLabel = r+1;
            os << elementLabel;
        }
        os << qSetFieldWidth(0) << "</span>"<< Qt::endl;

        for (int r = 0; r < rows(); ++r) {
            elementLabel = r+1;
            rowCount++;

            os << "<span class=\"header\">" << qSetFieldWidth(5) << Qt::right;
            os << elementLabel;
            os << qSetFieldWidth(0) << "</span>";

            for (int c = 0; c < cols(); ++c) {
                element = item(r,c) ;
                os << Qt::fixed << qSetFieldWidth(10) << Qt::right;
                if (  element == RAND_MAX)  // print inf symbol instead of RAND_MAX (distances matrix).
                    os << infinity;
                else {
                    os << element ;

                }
               // os << "";
            }

            os << qSetFieldWidth(0) << Qt::endl;
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
            os << Qt::fixed << Qt::right;
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


    os << qSetFieldWidth(0) << Qt::endl ;


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
 * @brief Prints this matrix as plain text, one line per row, cells right-aligned to a fixed
 * width. Cells >= RAND_MAX (unreachable/no edge) print as "x" instead of the raw number.
 * A quick way to eyeball a matrix's contents while debugging.
 * Complexity: O(rows()*cols()).
 * @param debug If true, print to stderr; if false, print to stdout.
 * @return true.
 */
bool Matrix::printMatrixConsole(bool debug){
    qCDebug(lcMatrix) << "Matrix::printMatrixConsole() - debug " << debug
             << "matrix rows" << rows()<< "cols"<< cols();
    QTextStream out ( (debug ? stderr : stdout) );

    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            if ( item(r,c) < RAND_MAX  ) {
                out <<  qSetFieldWidth(12) << qSetRealNumberPrecision(3)
                    << Qt::forcepoint << Qt::fixed << Qt::right
                        << item(r,c);
            }
            else {
                out <<  qSetFieldWidth(12) << qSetRealNumberPrecision(3)
                    << Qt::forcepoint << Qt::fixed << Qt::right
                        << "x";
            }

//            QTextStream( (debug ? stderr : stdout) )
//                    << ( (item(r,c) < RAND_MAX ) ? item(r,c) : INFINITY  )<<' ';
        }
        out << qSetFieldWidth(0)<< Qt::endl;
    }
    return true;
}


/**
 * @brief Checks whether this matrix is "ill-defined": whether any cell holds RAND_MAX, the
 * sentinel value used elsewhere in the codebase for "infinite"/unreachable (e.g. a distance
 * matrix entry for a disconnected pair).
 * @return true if at least one cell is RAND_MAX or greater; false otherwise.
 * Complexity: O(rows()*cols()) worst case, but returns as soon as one such cell is found.
 */
bool Matrix::illDefined(){
    qCDebug(lcMatrix) << "Matrix::illDefined() " ;

    for (int r = 0; r < rows(); ++r) {
        for (int c = 0; c < cols(); ++c) {
            if ( item(r,c) < RAND_MAX  ) {
            }
            else {
                qCDebug(lcMatrix) << "Matrix::illDefined() - matrix ill-defined: TRUE" ;
                return true;

            }
        }
    }
    return false;
}


