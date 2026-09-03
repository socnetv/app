/**
 * @file matrix.h
 * @brief Declares the Matrix class for handling adjacency and sociomatrix data structures in network analysis.
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


#ifndef MATRIX_H
#define MATRIX_H

#include <QtGlobal>
#include <QString>  //for static const QString declares below
#include <functional>   // std::function, for the optional cancelCheck callback
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


/**
 * @brief General-purpose dense matrix (adjacency, distance, similarity, sociomatrix, etc.),
 * used throughout SocNetV wherever a network needs to be represented or analyzed as an N x M
 * grid of numbers.
 *
 * Storage: 2D access on a 1D array, via a precomputed row-pointer index. Every cell lives in
 * one single contiguous allocation (`m_data`, row-major: row 0's cells, then row 1's, and so
 * on) - one allocation total for the whole grid, rather than one per row. A second, much
 * smaller array (`m_rowPtr`) records where each row starts inside `m_data` - one pointer per
 * row, computed once whenever the matrix is built or resized (see rebuildRowPtr()). Reading
 * or writing cell (r,c) is then: look up row r's starting address in `m_rowPtr` (one array
 * lookup), then step c cells forward from it (one pointer offset) - no multiplication needed
 * at access time, no matter how large the matrix is.
 *
 * `operator[]` (the `a[i][j]` syntax) returns a raw `qreal*` into `m_data` via `m_rowPtr`,
 * because the LU-decomposition/inversion code (`ludcmp()`, `lubksb()`, `inverse()`) needs to
 * modify cells in place with compound assignment (`a[i][j] -= ...`), which `item()`/
 * `setItem()` (a plain value-returning getter and a separate setter) can't express.
 * Everything else in the codebase reads/writes exclusively through `item()`/`setItem()`.
 */


class Matrix {
public:
    /**default constructor - default rows = cols = 0 */
    Matrix (int rowDim=0, int colDim=0)  ;

    Matrix(const Matrix &b) ;	/* Copy constructor allows Matrix a=b  */

    ~Matrix();

    void clear();

    void resize (const int m, const int n) ;

    // m_rowPtr[r] is the address of row r's first cell inside m_data (see rebuildRowPtr()
    // below). So m_rowPtr[r][c] is: fetch that address, then step c cells forward.
    qreal item( int r, int c ) { return m_rowPtr[r][c]; }

    void setItem(const int r, const int c, const qreal elem ) { m_rowPtr[r][c] = elem; }

    // Returns a raw pointer to the start of row r, so a[i][j] indexing keeps working
    // for ludcmp()/lubksb()/inverse() without a wrapper row type.
    qreal* operator []  (const int &r)  { return m_rowPtr[r]; }

    void clearItem( int r, int c ) { m_rowPtr[r][c] = 0; }

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


    Matrix& operator =(const Matrix & a);

    void sum(Matrix &a, Matrix &b) ;

    void operator +=(Matrix & b);

    Matrix operator +(Matrix & b);

    Matrix operator -(Matrix & b);

    Matrix operator *(Matrix & b);
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

    Matrix pow (int n, bool symmetry=false)  ;
    Matrix expBySquaring2 (Matrix &Y, Matrix &X, int n, bool symmetry=false);

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
            const qreal eps, const int &maxIter,
            std::function<bool()> cancelCheck = nullptr,
            qreal *lambdaMax = nullptr);

    Matrix& degreeMatrix();

    Matrix& laplacianMatrix();

    Matrix& transpose();

    Matrix& cocitationMatrix();


    Matrix& inverseByGaussJordanElimination(Matrix &a);

    bool inverse(Matrix &a, std::function<bool()> cancelCheck = nullptr);

    bool solve(qreal b[]);

    bool ludcmp (Matrix &a, const int &n, int indx[], qreal &d, std::function<bool()> cancelCheck = nullptr) ;

    void lubksb (Matrix &a, const int &n, int indx[], qreal b[]);


    Matrix& distancesMatrix(const int &metric,
                            const QString varLocation,
                            const bool &diagonal,
                            const bool &considerWeights,
                            std::function<bool()> cancelCheck = nullptr);

    Matrix& similarityMatrix(Matrix &AM,
                               const int &measure,
                               const QString varLocation="Rows",
                               const bool &diagonal=false,
                               const bool &considerWeights=true,
                               std::function<bool()> cancelCheck = nullptr);


    Matrix& pearsonCorrelationCoefficients(Matrix &AM,
                                          const QString &varLocation="Rows",
                                           const bool &diagonal=false,
                                           std::function<bool()> cancelCheck = nullptr);


    friend QTextStream& operator <<  (QTextStream& os, Matrix& m);
    bool printHTMLTable(QTextStream& os,
                        const bool markDiag=false,
                        const bool &plain=false,
                        const bool &printInfinity=true);
    bool printMatrixConsole(bool debug=true);

    bool illDefined();

private:
    // Builds m_rowPtr[]: a lookup table of row-start pointers into m_data, one entry per
    // row, so item()/setItem()/operator[] never have to recompute a row's address from
    // scratch. Concretely: m_rowPtr is an array of m_rows pointers; entry r is set to
    // "m_data, advanced by r whole rows" (r*m_cols cells). Looking up cell (r,c) then
    // becomes "read m_rowPtr[r] to get row r's address, then step c cells forward from
    // it" - one array read plus one cheap offset, regardless of how large the matrix is,
    // versus recomputing r*m_cols (a real multiplication) on every single access.
    //
    // Must be called once after every place that gives m_data a new address or a new
    // m_cols - the constructor, the copy constructor, resize(), identityMatrix(),
    // zeroMatrix(), deleteRowColumn(), and the branch of operator=() that reallocates -
    // because every entry in the old m_rowPtr[] would otherwise point at stale memory or
    // use the wrong row width. Complexity: O(m_rows) - one pointer computed per row, not
    // per cell, which is what keeps this cheap even for large matrices.
    //
    // This function only allocates and fills; it does not free a previous m_rowPtr array.
    // Callers that already have one from an earlier allocation must free it themselves
    // first (via clear(), or a direct delete[] - see deleteRowColumn()) before calling this.
    void rebuildRowPtr() {
        m_rowPtr = new (nothrow) qreal*[m_rows];
        Q_CHECK_PTR( m_rowPtr );
        for (int i=0; i<m_rows; i++) {
            m_rowPtr[i] = m_data + static_cast<size_t>(i) * m_cols;
        }
    }

    qreal *m_data;   // the actual N*M cells, one allocation, row-major.
    qreal **m_rowPtr; // m_rowPtr[r] == m_data + r*m_cols, precomputed for every row.
    int m_rows;
    int m_cols;

};





#endif
