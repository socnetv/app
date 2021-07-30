/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.4
 Written in Qt

                          matrix.h  -  description
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

#ifndef MATRIX_H
#define MATRIX_H

#include <QtGlobal>
#include <QString>  //for static const QString declares below
#include <utility>      // std::pair, std::make_pair
#include <vector>

using namespace std; //or else compiler groans for nothrow

class QTextStream;


#ifdef Q_OS_WIN32
static const QString infinity = "\u221E" ;
#else
static const QString infinity = QString("\xE2\x88\x9E") ;
#endif


static const int METRIC_NONE = -1;
static const int METRIC_SIMPLE_MATCHING  = 0;
static const int METRIC_JACCARD_INDEX = 1;
static const int METRIC_HAMMING_DISTANCE = 2;
static const int METRIC_COSINE_SIMILARITY  = 3;
static const int METRIC_EUCLIDEAN_DISTANCE = 4;
static const int METRIC_MANHATTAN_DISTANCE= 5;
static const int METRIC_PEARSON_COEFFICIENT = 6;
static const int METRIC_CHEBYSHEV_MAXIMUM= 7;





class MatrixRow {
public:
    MatrixRow (int cols=0) {
        cell=new (nothrow) qreal [m_cols=cols];
		Q_CHECK_PTR( cell );
        for (int i=0;i<m_cols; i++) {
            cell[i]=0;
        }
	}

    ~MatrixRow() {  if (m_cols) delete [] cell; m_cols=0 ; }
	
    MatrixRow& operator =(MatrixRow & a) {
        if (this != &a){
            if (a.m_cols!=m_cols) {
                delete [] cell;
                cell=new (nothrow) qreal[m_cols=a.m_cols];
                Q_CHECK_PTR( cell);
            }
            for (int i=0;i<m_cols; i++) {
                    cell[i]=a.cell[i];
            }
        }
        return *this;
    }
	
    qreal column ( int c ) const {
		return cell[c];
	}


    qreal& operator [] (const int k) { return cell[k]; }

	
    void setColumn (int index, qreal elem) {
		cell[index]=elem;
	}

	void clearColumn(int index){
		cell[index]=0;
	}	


    void resize(int cols) {
		delete [] cell;
        cell=new (nothrow) qreal[m_cols=cols];
		Q_CHECK_PTR( cell);
        for (int i=0;i<m_cols; i++) {
			cell[i]=0;
		}
    }
	
    void setSize(int cols){
        m_cols=cols;
    }

private:
    qreal *cell;
    int m_cols;
};




class Matrix {
public:
    /**default constructor - default rows = cols = 0 */
    Matrix (int rowDim=0, int colDim=0)  ;

    Matrix(const Matrix &b) ;	/* Copy constructor allows Matrix a=b  */

    ~Matrix();

    void clear();

    void resize (const int m, const int n) ;

    qreal item( int r, int c ) ;

    void setItem(const int r, const int c, const qreal elem );

    //WARNING: operator() is slow! Avoid using it.
    qreal  operator ()  (const int r, const int c) { return  row[r].column(c);  }

    MatrixRow& operator []  (const int &r)  { return row[r]; }

    void clearItem( int r, int c ) ;

    int cols() {return m_cols;}

    int rows() {return m_rows;}

    int  size() { return m_rows * m_cols; }

    void findMinMaxValues(qreal&min, qreal&max, bool &hasRealNumbers);

    void NeighboursNearestFarthest(qreal&min,qreal&max,
                          int &imin, int &jmin,
                          int &imax, int &jmax);

    void deleteRowColumn(int i);	/* deletes row i and column i */

    void identityMatrix (int dim);

    void zeroMatrix (const int m, const int n);

    void fillMatrix (qreal value );

    Matrix& subtractFromI () ;


    Matrix& operator =(Matrix & a);

    void sum(Matrix &a, Matrix &b) ;

    void operator +=(Matrix & b);

    Matrix& operator +(Matrix & b);

    Matrix& operator -(Matrix & b);

    Matrix& operator *(Matrix & b);
    void operator *=(Matrix & b);

    void product( Matrix &A, Matrix & B, bool symmetry=false) ;

    Matrix & productSym( Matrix &a, Matrix & b)  ;

    void swapRows(int rowA,int rowB);

    void multiplyScalar(const qreal &f);
    void multiplyRow(int row, qreal value);

    void productByVector (
            qreal in[],
            qreal out[],
            const bool &leftMultiply=false);

    Matrix & pow (int n, bool symmetry=false)  ;
    Matrix & expBySquaring2 (Matrix &Y, Matrix &X, int n, bool symmetry=false);

    qreal distanceManhattan(
            qreal x[],
            qreal y[],
            int n);
    qreal distanceEuclidean(
            qreal x[],
            int n);

    void powerIteration (
            qreal x[] ,
            qreal &xsum,
            qreal &xmax,
            int &xmaxi,
            qreal &xmin,
            int &xmini,
            const qreal eps, const int &maxIter);

    Matrix& degreeMatrix();

    Matrix& laplacianMatrix();

    Matrix& transpose();

    Matrix& cocitationMatrix();


    Matrix& inverseByGaussJordanElimination(Matrix &a);

    Matrix& inverse(Matrix &a);

    bool solve(qreal b[]);

    bool ludcmp (Matrix &a, const int &n, int indx[], qreal &d ) ;

    void lubksb (Matrix &a, const int &n, int indx[], qreal b[]);


    Matrix& distancesMatrix(const int &metric,
                            const QString varLocation,
                            const bool &diagonal,
                            const bool &considerWeights);
    
    Matrix& similarityMatrix(Matrix &AM,
                               const int &measure,
                               const QString varLocation="Rows",
                               const bool &diagonal=false,
                               const bool &considerWeights=true);


    Matrix& pearsonCorrelationCoefficients(Matrix &AM,
                                          const QString &varLocation="Rows",
                                           const bool &diagonal=false);


    friend QTextStream& operator <<  (QTextStream& os, Matrix& m);
    bool printHTMLTable(QTextStream& os,
                        const bool markDiag=false,
                        const bool &plain=false,
                        const bool &printInfinity=true);
    bool printMatrixConsole(bool debug=true);

    bool illDefined();

private:
    MatrixRow *row;
    int m_rows;
    int m_cols;

};





#endif
