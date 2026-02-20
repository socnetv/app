/**
 * @file graph_type_strings.cpp
 * @brief Implements Graph helper methods for mapping internal enum/int types to user-facing strings (and back), plus small string utilities.
 * @author Dimitris B. Kalamaras
 * @copyright
 *   Copyright (C) 2005-2025 by Dimitris B. Kalamaras.
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
 * @brief Helper method, return the human readable name of matrix type.
 * @param matrix
 */
QString Graph::graphMatrixTypeToString(const int &matrixType) const
{
    QString matrixStr;

    switch (matrixType)
    {

    case MATRIX_ADJACENCY:
        matrixStr = "Adjacency Matrix";
        break;
    case MATRIX_DISTANCES:
        matrixStr = "Distances Matrix";
        break;
    case MATRIX_DEGREE:
        matrixStr = "Degree Matrix";
        break;
    case MATRIX_LAPLACIAN:
        matrixStr = "Laplacian Matrix";
        break;
    case MATRIX_ADJACENCY_INVERSE:
        matrixStr = "Adjacency Inverse";
        break;

    case MATRIX_GEODESICS:
        matrixStr = "Geodesics Matrix";
        break;
    case MATRIX_REACHABILITY:
        matrixStr = "Reachability Matrix";
        break;
    case MATRIX_ADJACENCY_TRANSPOSE:
        matrixStr = "Adjacency Transpose";
        break;
    case MATRIX_COCITATION:
        matrixStr = "Cocitation Matrix";
        break;
    case MATRIX_DISTANCES_EUCLIDEAN:
        matrixStr = "Euclidean distance matrix";
        break;
    case MATRIX_DISTANCES_MANHATTAN:
        matrixStr = "Manhattan distance matrix";
        break;
    case MATRIX_DISTANCES_JACCARD:
        matrixStr = "Jaccard distance matrix";
        break;
    case MATRIX_DISTANCES_HAMMING:
        matrixStr = "Hamming distance matrix";
        break;
    default:
        matrixStr = "-";
        break;
    }
    return matrixStr;
}

/**
 * @brief Helper method, return the matrix type of human readable matrix name .
 * @param matrix
 * @return
 */
int Graph::graphMatrixStrToType(const QString &matrix) const
{
    if (matrix.contains("Hamming", Qt::CaseInsensitive))
    {
        return MATRIX_DISTANCES_HAMMING;
    }
    else if (matrix.contains("Jaccard", Qt::CaseInsensitive))
    {
        return MATRIX_DISTANCES_JACCARD;
    }
    else if (matrix.contains("Manhattan", Qt::CaseInsensitive))
    {
        return MATRIX_DISTANCES_MANHATTAN;
    }
    else if (matrix.contains("Euclidean", Qt::CaseInsensitive))
    {
        return MATRIX_DISTANCES_EUCLIDEAN;
    }
    else if (matrix.contains("Cocitation", Qt::CaseInsensitive))
    {
        return MATRIX_COCITATION;
    }
    else if (matrix.contains("Adjacency Transpose", Qt::CaseInsensitive))
    {
        return MATRIX_ADJACENCY_TRANSPOSE;
    }
    else if (matrix.contains("Reachability", Qt::CaseInsensitive))
    {
        return MATRIX_REACHABILITY;
    }
    else if (matrix.contains("Geodesics", Qt::CaseInsensitive))
    {
        return MATRIX_GEODESICS;
    }
    else if (matrix.contains("Adjacency Inverse", Qt::CaseInsensitive))
    {
        return MATRIX_ADJACENCY_INVERSE;
    }
    else if (matrix.contains("Laplacian", Qt::CaseInsensitive))
    {
        return MATRIX_LAPLACIAN;
    }
    else if (matrix.contains("Degree", Qt::CaseInsensitive))
    {
        return MATRIX_DEGREE;
    }
    else if (matrix.contains("Adjacency", Qt::CaseInsensitive))
    {
        return MATRIX_ADJACENCY;
    }
    else if (matrix.contains("Distances", Qt::CaseInsensitive))
    {
        return MATRIX_DISTANCES;
    }
    else
    {
        return -1;
    }
}

/**
 * @brief Helper method, return the human readable name of metric type.
 * @param metric
 */
QString Graph::graphMetricTypeToString(const int &metricType) const
{
    QString metricStr;
    switch (metricType)
    {
    case METRIC_SIMPLE_MATCHING:
        metricStr = "Simple / Exact matching";
        break;
    case METRIC_JACCARD_INDEX:
        metricStr = "Jaccard Index";
        break;
    case METRIC_HAMMING_DISTANCE:
        metricStr = "Hamming distance";
        break;
    case METRIC_COSINE_SIMILARITY:
        metricStr = "Cosine similarity";
        break;
    case METRIC_EUCLIDEAN_DISTANCE:
        metricStr = "Euclidean distance";
        break;
    case METRIC_MANHATTAN_DISTANCE:
        metricStr = "Manhattan distance";
        break;
    case METRIC_PEARSON_COEFFICIENT:
        metricStr = "Pearson Correlation Coefficient";
        break;
    case METRIC_CHEBYSHEV_MAXIMUM:
        metricStr = "Chebyshev distance";
        break;
    default:
        metricStr = "-";
        break;
    }
    return metricStr;
}

/**
 * @brief Helper method, return the identifier of a metric.
 * @param metricStr
 */
int Graph::graphMetricStrToType(const QString &metricStr) const
{
    int metric = METRIC_SIMPLE_MATCHING;
    if (metricStr.contains("Simple", Qt::CaseInsensitive))
        metric = METRIC_SIMPLE_MATCHING;
    else if (metricStr.contains("Jaccard", Qt::CaseInsensitive))
        metric = METRIC_JACCARD_INDEX;
    else if (metricStr.contains("None", Qt::CaseInsensitive))
        metric = METRIC_NONE;
    else if (metricStr.contains("Hamming", Qt::CaseInsensitive))
        metric = METRIC_HAMMING_DISTANCE;
    else if (metricStr.contains("Cosine", Qt::CaseInsensitive))
        metric = METRIC_COSINE_SIMILARITY;
    else if (metricStr.contains("Euclidean", Qt::CaseInsensitive))
        metric = METRIC_EUCLIDEAN_DISTANCE;
    else if (metricStr.contains("Manhattan", Qt::CaseInsensitive))
        metric = METRIC_MANHATTAN_DISTANCE;
    else if (metricStr.contains("Pearson ", Qt::CaseInsensitive))
        metric = METRIC_PEARSON_COEFFICIENT;
    else if (metricStr.contains("Chebyshev", Qt::CaseInsensitive))
        metric = METRIC_CHEBYSHEV_MAXIMUM;
    return metric;
}

/**
 * @brief  Helper method, return the human readable name of clustering method type.
 * @return
 */
QString Graph::graphClusteringMethodTypeToString(const int &methodType) const
{
    QString methodStr;
    switch (methodType)
    {
    case Clustering::Single_Linkage:
        methodStr = "Single-linkage (minimum)";
        break;
    case Clustering::Complete_Linkage:
        methodStr = "Complete-linkage (maximum)";
        break;
    case Clustering::Average_Linkage:
        methodStr = "Average-linkage (UPGMA)";
        break;
    default:
        break;
    }
    return methodStr;
}

/**
 * @brief Helper method, return clustering method type from the human readable name of it.
 * @param method
 * @return
 */
int Graph::graphClusteringMethodStrToType(const QString &method) const
{
    int methodType = Clustering::Average_Linkage;
    if (method.contains("Single", Qt::CaseInsensitive))
    {
        methodType = Clustering::Single_Linkage;
    }
    else if (method.contains("Complete", Qt::CaseInsensitive))
    {
        methodType = Clustering::Complete_Linkage;
    }
    else if (method.contains("Average", Qt::CaseInsensitive))
    {
        methodType = Clustering::Average_Linkage;
    }
    return methodType;
}

/**
 * @brief Helper method, returns a nice qstring where all html special chars are encoded
 * @param str
 * @return
 */
QString Graph::htmlEscaped(QString str) const
{
    str = str.simplified();
    if (str.contains('&'))
    {
        str = str.replace('&', "&amp;");
    }
    if (str.contains('<'))
    {
        str = str.replace('<', "&lt;");
    }
    if (str.contains('>'))
    {
        str = str.replace('>', "&gt;");
    }
    if (str.contains('\"'))
    {
        str = str.replace('\"', "&quot;");
    }
    if (str.contains('\''))
    {
        str = str.replace('\'', "&apos;");
    }
    return str;
}
