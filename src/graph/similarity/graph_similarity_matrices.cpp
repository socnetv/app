/**
 * @file graph_similarity_matrices.cpp
 * @brief Implements similarity and dissimilarity matrix construction
 *        methods for the Graph class (matching, Pearson, and related metrics).
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

#include "graph.h"

#include <QDebug>

/**
 * @brief Calls Matrix:distancesMatrix to compute the dissimilarities matrix DSM
 * of the variables (rows, columns, both) in given input matrix using the
 * user defined metric
 * @param INPUT_MATRIX
 * @param DSM
 * @param metric
 * @param varLocation
 * @param diagonal
 * @param considerWeights
 */
void Graph::createMatrixDissimilarities(Matrix &INPUT_MATRIX,
                                        Matrix &DSM,
                                        const int &metric,
                                        const QString &varLocation,
                                        const bool &diagonal,
                                        const bool &considerWeights)
{
    qDebug() << "Graph::createMatrixDissimilarities() -metric" << metric;

    DSM = INPUT_MATRIX.distancesMatrix(metric, varLocation, diagonal, considerWeights);

    //    qDebug()<<"Graph::createMatrixDissimilarities() - matrix DSM:";
    // DSM.printMatrixConsole(true);
}

/**
 * @brief Calls Matrix:similarityMatrix to compute the similarity matrix SCM
 * of the variables (rows, columns, both) in given input matrix using the
 * selected matching measure.
 *
 * @param AM
 * @param SCM
 * @param rows
 */
void Graph::createMatrixSimilarityMatching(Matrix &AM,
                                           Matrix &SCM,
                                           const int &measure,
                                           const QString &varLocation,
                                           const bool &diagonal,
                                           const bool &considerWeights)
{
    qDebug() << "Graph::createMatrixSimilarityMatching()";

    QString pMsg = tr("Computing Similarity coefficients matrix. \nPlease wait...");
    emit signalProgressBoxCreate(1, pMsg);
    SCM.similarityMatrix(AM, measure, varLocation, diagonal, considerWeights);
    emit signalProgressBoxUpdate(1);
    emit signalProgressBoxKill();
}

/**
 * @brief
 * The Pearson product-moment correlation coefficient (PPMCC, PCC or Pearson's r)
 * is a measure of the linear dependence between two variables X and Y.
 *
 * As a normalized version of the covariance, the PPMCC is computed with the formula:
 *  r =\frac{\sum ^n _{i=1}(x_i - \bar{x})(y_i - \bar{y})}{\sqrt{\sum ^n _{i=1}(x_i - \bar{x})^2} \sqrt{\sum ^n _{i=1}(y_i - \bar{y})^2}}
 *
 * It gives a value between +1 and −1 inclusive, where 1 is total positive linear
 * correlation, 0 is no linear correlation, and −1 is total negative linear correlation.
 *
 * In SNA, Pearson correlations can be used to track the similarity between actors,
 * in terms of structural equivalence.
 *
 * This method creates an actor by actor NxN matrix PCC where the (i,j) element
 * is the Pearson correlation coefficient of actor i and actor j.
 * If the input matrix is the adjacency matrix, the PCC of two nodes measures
 * how related (similar, inverse or not related at all) their patterns of ties tend to be.
 * A positive value means there is strong linear association of the two actors,
 * while a negative value means the inverse. For instance a value of -1 means
 * the two actors have exactly opposite ties to other actors, while a value of 1
 * means the actors have identical patterns of ties to other actors
 * (they are connected to the same actors).
 *
 * The correlation measure of similarity is particularly useful when the data on ties are valued

 * @param AM
 * @param PCC
 * @param rows
 */
void Graph::createMatrixSimilarityPearson(Matrix &AM,
                                          Matrix &PCC,
                                          const QString &varLocation,
                                          const bool &diagonal)
{
    qDebug() << "Graph::createMatrixSimilarityPearson()";

    PCC.pearsonCorrelationCoefficients(AM, varLocation, diagonal);

    qDebug() << "Graph::createMatrixSimilarityPearson() - matrix PCC";
    // PCC.printMatrixConsole(true);
}
