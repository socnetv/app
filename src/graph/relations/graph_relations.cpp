/**
 * @file graph_relations.cpp
 * @brief Implements relation management methods for the Graph class (current
 *        relation selection, add/remove/rename, relation-based operations).
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

/**
 * @brief Changes the current relation, and optionally emits signals to MW/GW (default: true)
 *
 * Forces all enabled vertices to disable edges in the old relation and enable edges of the new relation
 *
 * Then, if updateUI==true (default), it emits signals to MW and GW
 * to update the MW UI and toggle the edges on the GW, respectivelly.
 *
 * Called from Parser, Graph methods and when the user selects a relation in the MW combo box.
 *
 * @param relNum int
 * @param updateUI bool
 */
void Graph::relationSet(int relNum, const bool &updateUI)
{

    qDebug() << "++ Request to change graph to relation:" << relNum
             << " - current relation:" << m_curRelation
             << "updateUI:" << updateUI;

    //
    // Perform checks for requested new relation number
    //
    if (m_curRelation == relNum)
    {
        // Same as current, don't do nothing
        qDebug() << "++ Requested relation is the current one - END";
        return;
    }

    if (relNum < 0)
    {
        // negative, don't do nothing
        qDebug() << "++ Requested relation is negative - END ";
        return;
    }
    else if (relNum == RAND_MAX)
    {
        // Set relation to the last existing relation
        relNum = relations() - 1;
    }
    else if (relNum > relations() - 1)
    {
        // Invalid relation, abort
        qDebug() << "++ Invalid relation - END ";
        return;
    }

    //
    // Force enabled vertices to disable all edges
    // in the old relation and enable edges in the new relation
    //
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        qDebug() << "++ changing relation of vertex"
                 << (*it)->number()
                 << "to" << relNum;
        if (!(*it)->isEnabled())
            continue;
        (*it)->setRelation(relNum);
    }

    //
    // now store the selected relation
    //
    m_curRelation = relNum;

    //
    // Check if isWeighted so that multiple-relation networks are properly loaded.
    //
    isWeighted();

    //
    // Check if we need to update the UI
    //
    if (updateUI)
    {
        qDebug() << "++ Signaling to update UI and GW and setting graph mod status to edge count changed.";
        // Notify MW to change combo box relation name
        emit signalRelationChangedToMW(m_curRelation);
        // notify GW to disable/enable the on screen edges.
        emit signalRelationChangedToGW(m_curRelation);
        // update graph mod status
        setModStatus(ModStatus::EdgeCount);
    }
}

/**
 * @brief Changes graph to previous relation
 */
void Graph::relationPrev()
{
    qDebug() << "Changing to the previous relation...";
    int relNum = m_curRelation;
    if (m_curRelation > 0)
    {
        --relNum;
        relationSet(relNum);
        // editFilterNodesIsolatesAct->setChecked(false);
    }
}

/**
 * @brief Changes graph to next relation
 */
void Graph::relationNext()
{
    qDebug() << "Changing to the next relation...";
    int relNum = m_curRelation;
    if (relations() > 0 && relNum < relations())
    {
        ++relNum;
        relationSet(relNum);
        // editFilterNodesIsolatesAct->setChecked(false);
    }
}

/**
 * @brief Adds a new relation to the graph
 *
 * Adds a new relation named relName, emitting signal to MW UI, and
 * optionally changing current graph relation to the new one.
 * Called by file parser and various Graph methods, i.e clear() etc.
 *
 * @param relName
 */
void Graph::relationAdd(const QString &relName, const bool &changeRelation)
{

    qDebug() << "Adding new relation named:" << relName;

    // Add the new relation to our relations list
    m_relationsList << relName;

    // Emit signal for the new relation to be added to the MW UI combo...
    emit signalRelationAddToMW(relName);

    // Check if we need to change to the new relation...
    if (changeRelation)
    {
        relationSet();
    }

    emit statusMessage((tr("Added a new relation named: %1."))
                           .arg(relName));
}

/**
 * @brief Gets the current relation number
 *
 * @return int
 */
int Graph::relationCurrent()
{
    return m_curRelation;
}

/**
 * @brief Gets the current relation name
 *
 * @return string
 */
QString Graph::relationCurrentName() const
{
    //    qDebug() << "Returning the current relation name...";
    return m_relationsList.value(m_curRelation);
}

/**
 * @brief Renames current relation to newName, optionally emitting a signal to MW
 * @param newName
 */
void Graph::relationCurrentRename(const QString &newName, const bool &signalMW)
{

    //
    // Check if new name is the same
    //
    if (!m_relationsList.isEmpty() && newName == m_relationsList[m_curRelation])
    {
        qDebug() << "The new name of the relation is the same as the current name. Nothing to do. Returning.";
        return;
    }

    //
    // Check if new name is empty
    //
    if (newName.isEmpty())
    {
        qDebug() << "The new name of the relation is empty. Nothing to do. Returning.";
        return;
    }

    //
    // Rename current relation to newName
    //
    qDebug() << "Renaming current relation:"
             << m_curRelation << "to:" << newName
             << " - signalMW:" << signalMW;

    m_relationsList[m_curRelation] = newName;

    //
    // Check if we need to emit a signal
    //
    if (signalMW)
    {
        emit signalRelationRenamedToMW(newName);
    }
}

/**
 * @brief Overload. Renames current relation to newName, without emitting any signal to MW
 *
 * @param newName
 */
void Graph::relationCurrentRename(const QString &newName)
{
    relationCurrentRename(newName, false);
}

/**
 * @brief Returns the count of relationships in this Graph
 *
 * @return int
 */
int Graph::relations()
{
    // qDebug () << " relations count " << m_relationsList.size();
    return m_relationsList.size();
}

/**
 * @brief Clears relationships in this Graph
 */
void Graph::relationsClear()
{
    int oldRelationsCounter = m_relationsList.size();
    m_relationsList.clear();
    m_curRelation = 0;
    qDebug() << "Cleared" << oldRelationsCounter << "relation(s)"
             << "Emitting signalRelationsClear()";
    emit signalRelationsClear();
}

/**
 * @brief Creates a new symmetric relation by keeping only strong-ties (mutual links)
 * in the current relation. In the new relation, two actors are connected only if
 * they are mutually connected in the current relation.
 * @param allRelations
 */
void Graph::addRelationSymmetricStrongTies(const bool &allRelations)
{

    qDebug() << "Creating new relation using strong ties only."
             << "initial relations" << relations();

    int y = 0, v2 = 0, v1 = 0, weight;
    qreal invertWeight = 0;

    VList::const_iterator it;

    QHash<int, qreal> outEdgesAll;
    QHash<int, qreal>::const_iterator it1;

    QHash<QString, qreal> *strongTies = new QHash<QString, qreal>;

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        v1 = (*it)->number();
        qDebug() << "Graph::addRelationSymmetricStrongTies() - v" << v1
                 << "iterate over outEdges in all relations";
        outEdgesAll = (*it)->outEdgesEnabledHash(allRelations); // outEdgesAllRelationsUniqueHash();
        it1 = outEdgesAll.cbegin();
        while (it1 != outEdgesAll.cend())
        {
            v2 = it1.key();
            weight = it1.value();
            y = vpos[v2];
            qDebug() << ""
                     << v1 << "->" << v2 << "=" << weight << "Checking opposite.";
            invertWeight = m_graph[y]->hasEdgeTo(v1, allRelations);
            if (invertWeight == 0)
            {
                qDebug() << v1 << "<-" << v2 << " does not exist. Weak tie. Continue.";
            }
            else
            {
                if (!strongTies->contains(QString::number(v1) + "--" + QString::number(v2)) &&
                    !strongTies->contains(QString::number(v2) + "--" + QString::number(v1)))
                {
                    qDebug() << v1 << "--" << v2 << " exists. Strong Tie. Adding";
                    strongTies->insert(QString::number(v1) + "--" + QString::number(v2), 1);
                }
                else
                {
                    qDebug() << v1 << "--" << v2 << " exists. Strong Tie already found. Continue";
                }
            }
            ++it1;
        }
    }

    relationAdd("Strong Ties", true);

    QHash<QString, qreal>::const_iterator it2;
    it2 = strongTies->constBegin();
    QStringList vertices;
    qDebug() << "creating strong tie edges...";
    while (it2 != strongTies->constEnd())
    {
        vertices = it2.key().split("--");
        //        qDebug() << "tie " <<it2.key()
        //                 << "vertices.at(0)" << vertices.at(0)
        //                 << "vertices.at(1)" << vertices.at(1);
        v1 = (vertices.at(0)).toInt();
        v2 = (vertices.at(1)).toInt();
        //        qDebug() << "calling edgeCreate for" << v1 << "--"<<v2;
        edgeCreate(v1, v2, 1, initEdgeColor, EdgeType::Undirected, true, false,
                   QString(), false);
        ++it2;
    }

    // delete outEdgesAll;
    delete strongTies;
    m_graphIsSymmetric = true;

    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Creates a new symmetric relation by connecting those actors
 * that are cocitated by others.
 * In the new relation, an edge will exist between actor i and actor j
 * only if C(i,j) > 0, where C the Cocitation Matrix.
 * Thus the actor pairs cited by more common neighbors will appear
 * with a stronger tie between them than pairs those cited by fewer
 * common neighbors. The resulting relation is symmetric.
 */
void Graph::relationAddCocitation()
{
    qDebug() << "Graph::relationAddCocitation()"
             << "initial relations" << relations();

    int v1 = 0, v2 = 0, i = 0, j = 0, weight;
    bool dropIsolates = false;

    createMatrixAdjacency();

    Matrix *CT = new Matrix(AM.rows(), AM.cols());
    *CT = AM.cocitationMatrix();

    // CT->printMatrixConsole(true);

    VList::const_iterator it, it1;

    relationAdd("Cocitation", true);

    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        if (!(*it)->isEnabled() || ((*it)->isIsolated() && dropIsolates))
        {
            continue;
        }
        v1 = (*it)->number();
        j = 0;
        for (it1 = m_graph.cbegin(); it1 != m_graph.cend(); it1++)
        {
            qDebug() << "Graph::relationAddCocitation() - (i,j)" << i + 1 << j + 1;
            if (!(*it1)->isEnabled() || ((*it1)->isIsolated() && dropIsolates))
            {
                continue;
            }
            v2 = (*it1)->number();

            if (v1 == v2)
            {
                j++;
                qDebug() << "Graph::relationAddCocitation() - skipping self loop" << v1 << v2;
                continue;
            }
            if ((weight = CT->item(i, j)) != 0)
            {
                qDebug() << "Graph::relationAddCocitation() - creating edge"
                         << v1 << "<->" << v2
                         << "because CT(" << i + 1 << "," << j + 1 << ") = " << weight;
                edgeCreate(v1, v2, weight, initEdgeColor,
                           EdgeType::Undirected, true, false,
                           QString(), false);
            }

            j++;
        }
        i++;
    }

    m_graphIsSymmetric = true;

    setModStatus(ModStatus::EdgeCount);
    qDebug() << "Graph::relationAddCocitation()"
             << "final relations" << relations();
}
