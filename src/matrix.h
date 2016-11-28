/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.2
 Written in Qt

                          matrix.h  -  description
                             -------------------
    copyright         : (C) 2005-2016 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

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

using namespace std; //or else compiler groans for nothrow


#ifdef Q_OS_WIN32
static const QString infinity = QString::number( INFINITY) ;
#else
static const QString infinity = QString("\xE2\x88\x9E") ;
#endif


class QTextStream;

class Row {
public:
    Row (int cols=0) {
        cell=new (nothrow) float [m_cols=cols];
		Q_CHECK_PTR( cell );
        for (int i=0;i<m_cols; i++) {
            cell[i]=0;
        }
	}

    ~Row() { m_cols=0 ; delete [] cell;}
	
    Row& operator =(Row & a) {
        if (this != &a){
            if (a.m_cols!=m_cols) {
                delete [] cell;
                cell=new (nothrow) float[m_cols=a.m_cols];
                Q_CHECK_PTR( cell);
            }
            for (int i=0;i<m_cols; i++) {
                    cell[i]=a.cell[i];
            }
        }
        return *this;
    }
	
	float column ( int c ) const {
		return cell[c];
	}


    float& operator [] (const int k) { return cell[k]; }

	
	void setColumn (int index, float elem) {
		cell[index]=elem;
	}

	void clearColumn(int index){
		cell[index]=0;
	}	


    void resize(int cols) {
		delete [] cell;
        cell=new (nothrow) float[m_cols=cols];
		Q_CHECK_PTR( cell);
        for (int i=0;i<m_cols; i++) {
			cell[i]=0;
		}
    }
	
    void setSize(int cols){
        m_cols=cols;
    }

private:
	float *cell;
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

    float item( int r, int c ) ;

    void setItem(const int r, const int c, const float elem );

    //WARNING: this operator is slow! Avoid using it.
    float  operator ()  (const int r, const int c) { return  row[r].column(c);  }

    Row& operator []  (const int &r)  { return row[r]; }

    void clearItem( int r, int c ) ;

    int cols() {return m_cols;}

    int rows() {return m_rows;}

    int  size() { return m_rows * m_cols; }

    void findMinMaxValues(float&min, float&max, bool &hasRealNumbers);

    void NeighboursNearestFarthest(float&min,float&max,
                          int &imin, int &jmin,
                          int &imax, int &jmax);

    void deleteRowColumn(int i);	/* deletes row i and column i */

    bool printMatrixConsole(bool debug=true);

    void identityMatrix (int dim);

    void zeroMatrix (const int m, const int n);

    void fillMatrix (float value );

    Matrix& operator =(Matrix & a);

    void operator +=(Matrix & b);

    Matrix& operator +(Matrix & b);

    Matrix& operator *(Matrix & b);

    friend QTextStream& operator <<  (QTextStream& os, Matrix& m);

    void product( Matrix &a, Matrix & b, bool symmetry=false) ;

    Matrix & productSym( Matrix &a, Matrix & b)  ;

    Matrix & pow (int n, bool symmetry=false)  ;
    Matrix & expBySquaring2 (Matrix &Y, Matrix &X, int n, bool symmetry=false);

    Matrix& subtractFromI () ;

    void sum(Matrix &a, Matrix &b) ;

    bool ludcmp (Matrix &a, const int &n, int indx[], float &d ) ;

    void lubksb (Matrix &a, const int &n, int indx[], float b[]);

    Matrix& inverseByGaussJordanElimination(Matrix &a);

    Matrix& inverse(Matrix &a);

    void swapRows(int rowA,int rowB);		/* elementary matrix algebra */

    void multiplyScalar(const float &f);
    void multiplyRow(int row, float value);

private:
    Row *row;
    int m_Actors;
    int m_rows;
    int m_cols;

};





#endif
