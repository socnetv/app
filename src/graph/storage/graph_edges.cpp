/**
 * @file graph_edges.cpp
 * @brief Implements edge storage and CRUD operations for the Graph class
 *        (create/add/remove, enable/disable, existence queries, weights).
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

//
// Create/add/remove
//

/**
 * @brief Checks if edge (v1,v2) already exists, then creates it and signals the UI to draw it.
 *
 * This is the main entry point for edge creation, called from:
 *   - Parser::signalCreateEdge (when loading a network file)
 *   - MainWindow (when user clicks "add link" button)
 *   - GraphicsWidget (when user middle-clicks two nodes)
 *
 * Edge type semantics:
 *   - EdgeType::Undirected  : stores both v1->v2 and v2->v1 internally (via edgeAdd),
 *                             draws a single undirected edge in the UI.
 *   - EdgeType::Directed    : stores only v1->v2. If v2->v1 already exists,
 *                             upgrades both to EdgeType::Reciprocated.
 *   - EdgeType::Reciprocated: both directions exist with equal weight.
 *
 * Mixed-section Pajek files (*Arcs followed by *Edges):
 *   Some Pajek files declare directed arcs in an *Arcs section AND undirected edges
 *   in a subsequent *Edges section. The same node pair (v1,v2) may appear in both.
 *   When *Arcs are processed first, v1->v2 is stored as a directed arc.
 *   When *Edges is processed later, edgeExists(v1,v2) returns non-zero, so we must
 *   NOT silently drop the request. If v2->v1 is still missing, we store the reverse
 *   arc and upgrade the existing forward arc visual to Reciprocated.
 *
 *   In that special case:
 *   - We store the reverse arc v2->v1 using the undirected edge's weight/color/label.
 *     The forward arc (from *Arcs) is left untouched to preserve its original data.
 *   - We emit signalDrawEdge(v2, v1, ..., Reciprocated) — note the swapped arguments.
 *     GraphicsWidget::drawEdge() with Reciprocated looks up the existing arc via
 *     createEdgeName(targetNum, sourceNum). Since createEdgeName() is order-sensitive
 *     ("rel:v1>v2"), passing (v2,v1) makes targetNum=v1, sourceNum=v2, so the lookup
 *     resolves to createEdgeName(v1,v2) which matches the key stored during *Arcs
 *     processing. No new GraphicsEdge is created; setDirectionType() is called instead.
 *   - drawArrows is forced true: a reciprocated edge always shows arrows on both ends
 *     regardless of what the undirected request's drawArrows value was.
 *   - We do NOT set m_graphIsDirected: adding the missing reverse makes the pair more
 *     symmetric, not less. Directedness is determined by the overall graph state after
 *     full import completes.
 *   - We return true because a new arc was genuinely added to the graph.
 *
 * @param v1         Source node number
 * @param v2         Target node number
 * @param weight     Edge weight
 * @param color      Edge color
 * @param type       Edge type: EdgeType::Undirected, Directed, or Reciprocated
 * @param drawArrows Whether to draw arrowheads in the UI
 * @param bezier     Whether to draw the edge as a bezier curve
 * @param label      Edge label (optional)
 * @param signalMW   Whether to signal MainWindow after modifying graph state
 * @return true if a new edge (or reverse arc) was created, false if fully skipped
 */
bool Graph::edgeCreate(const int &v1,
                       const int &v2,
                       const qreal &weight,
                       const QString &color,
                       const int &type,
                       const bool &drawArrows,
                       const bool &bezier,
                       const QString &label,
                       const bool &signalMW)
{
    //
    // GUARD: Check if v1->v2 already exists.
    //
    // This can legitimately happen in mixed Pajek files that have both an *Arcs
    // section and an *Edges section. The *Arcs section is processed first,
    // creating directed arcs. When the *Edges section is processed later,
    // some pairs may already be present as directed arcs.
    //
    if (edgeExists(v1, v2))
    {
        //
        // Special case: caller wants an Undirected edge but v1->v2 was
        // previously stored as a directed arc (e.g. from a *Arcs section in
        // a mixed Pajek file). An undirected edge requires BOTH directions to
        // be stored internally. Check if the reverse v2->v1 is also present.
        //
        if (type == EdgeType::Undirected && !edgeExists(v2, v1))
        {
            // v2->v1 is missing. Store it to complete the undirected pair.
            //
            // Weight/color/label policy:
            //   We use the undirected edge's values for the reverse arc.
            //   The forward arc (from *Arcs) is left untouched to preserve
            //   its original explicitly declared data. The two directions may
            //   therefore differ in metadata, which is acceptable: we prefer
            //   not to silently overwrite explicitly declared arc data.
            //
            // Visual update:
            //   We emit Reciprocated with swapped arguments (v2,v1) so that
            //   drawEdge()'s internal lookup createEdgeName(targetNum,sourceNum)
            //   resolves to createEdgeName(v1,v2), matching the key under which
            //   the original forward arc was stored during *Arcs processing.
            //   This upgrades the existing GraphicsEdge to bidirectional without
            //   creating any duplicate visual object.
            //   drawArrows is forced true since a reciprocated edge always
            //   shows arrows on both ends.
            //
            // Directedness:
            //   m_graphIsDirected is NOT set here. Adding the missing reverse
            //   makes the pair more symmetric. Directedness is determined by
            //   the overall graph state after full import completes.
            //
            // qDebug() << "edgeCreate(): v1->v2 exists as directed arc but request"
            //             " is Undirected and reverse v2->v1 is missing."
            //             " Storing reverse arc and upgrading visual to Reciprocated:"
            //          << v1 << "<->" << v2;

            // Store the reverse arc in the data model.
            edgeAdd(v2, v1, weight, EdgeType::Directed, label,
                    ((weight == 0) ? "blue" : color));

            // Upgrade the existing forward arc visual to bidirectional.
            // Arguments are swapped (v2,v1) intentionally — see comment above.
            emit signalDrawEdge(v2, v1, weight, label,
                                ((weight == 0) ? "blue" : color),
                                EdgeType::Reciprocated,
                                true, // force drawArrows for reciprocated
                                bezier, initEdgeWeightNumbers);

            // Update edge count and notify MainWindow since a new arc was added.
            setModStatus(ModStatus::EdgeCount, signalMW);

            // Return true: a new arc was genuinely added to the graph.
            return true;
        }

        // v1->v2 exists and either:
        //   a) the request is not Undirected, or
        //   b) v2->v1 also already exists (pair is already complete).
        // Nothing to do.
        return false;
    }

    //
    // v1->v2 does not exist yet. Proceed with normal edge creation.
    //

    if (type == EdgeType::Undirected)
    {
        // Undirected edge: edgeAdd stores BOTH v1->v2 and v2->v1 internally.
        // The UI draws a single undirected edge between the two nodes.
        // qDebug() << "edgeCreate(): Creating new UNDIRECTED edge:"
        //          << v1 << "-" << v2
        //          << "weight" << weight
        //          << "label" << label;

        edgeAdd(v1, v2, weight, type, label,
                ((weight == 0) ? "blue" : color));

        emit signalDrawEdge(v1, v2, weight, label,
                            ((weight == 0) ? "blue" : color),
                            type,
                            drawArrows, bezier, initEdgeWeightNumbers);
    }
    else if (edgeExists(v2, v1))
    {
        // v1->v2 does not exist, but v2->v1 already exists.
        // Adding v1->v2 now makes the relationship reciprocal (bidirectional).
        // Upgrade the edge type to Reciprocated so both the data model and
        // the UI reflect the bidirectional relationship.
        // qDebug() << "edgeCreate(): Creating new RECIPROCAL edge:"
        //          << v1 << "->" << v2
        //          << "weight" << weight
        //          << "label" << label
        //          << "(reverse v2->v1 already exists)";

        edgeAdd(v1, v2, weight, EdgeType::Reciprocated, label, color);

        emit signalDrawEdge(v1, v2, weight, label, color,
                            EdgeType::Reciprocated,
                            drawArrows, bezier, initEdgeWeightNumbers);

        m_graphIsDirected = true;
    }
    else
    {
        // Neither v1->v2 nor v2->v1 exists.
        // Create a plain directed arc from v1 to v2.
        // qDebug() << "edgeCreate(): Creating new DIRECTED edge:"
        //          << v1 << "->" << v2
        //          << "weight" << weight
        //          << "label" << label;

        edgeAdd(v1, v2, weight, EdgeType::Directed, label,
                ((weight == 0) ? "blue" : color));

        emit signalDrawEdge(v1, v2, weight, label,
                            ((weight == 0) ? "blue" : color),
                            EdgeType::Directed,
                            drawArrows, bezier, initEdgeWeightNumbers);

        m_graphIsDirected = true;
        m_graphIsSymmetric = false;
    }

    // Persist the color of the last created edge so that new edges drawn
    // interactively by the user on the canvas inherit the same color
    // as the edges loaded from the file.
    initEdgeColor = color;

    // Update edge count and notify MainWindow if needed.
    setModStatus(ModStatus::EdgeCount, signalMW);

    return true;
}

/**
 * @brief Called from WebCrawler when it finds an new link
 * Calls edgeCreate() method with initEdgeColor
 * @param source
 * @param target
 */
void Graph::edgeCreateWebCrawler(const int &source, const int &target)
{
    //    qDebug()<< " will create edge from" << source << "to" << target ;
    qreal weight = 1.0;
    bool drawArrows = true;
    bool bezier = false;

    edgeCreate(source, target, weight, initEdgeColor, EdgeType::Directed, drawArrows, bezier);
}

/**
 * @brief Adds a directed arc from v1 to v2 into the internal graph data structures.
 *
 * This is the low-level storage function. It does NOT signal the UI.
 * All UI signaling is handled by the caller (edgeCreate / signalDrawEdge).
 *
 * Internally, each vertex maintains two adjacency lists:
 *   - OutEdges: arcs going OUT from this vertex
 *   - InEdges:  arcs coming IN to this vertex
 *
 * For a directed arc v1->v2:
 *   - v1's OutEdges gains v2 (with weight, color, label)
 *   - v2's InEdges  gains v1 (with weight)
 *
 * For an undirected edge (EdgeType::Undirected), the edge is stored as
 * two symmetric directed arcs:
 *   - v1->v2 (OutEdge of v1, InEdge of v2)
 *   - v2->v1 (OutEdge of v2, InEdge of v1)
 * This matches SocNetV's internal convention: undirected edges are always
 * represented as a pair of reciprocal directed arcs in the adjacency structure.
 *
 * For EdgeType::Reciprocated, the reverse arc v2->v1 already exists in the
 * data model (created earlier as a directed arc). No additional storage is
 * needed here; the caller is responsible for updating type semantics if required.
 *
 * Weight handling:
 *   If weight != 1 and weight != 0, the graph is marked as weighted.
 *   Weight == 0 is treated as a special "null" edge (drawn in blue by convention).
 *
 * @param v1     Source node number (external node number, not internal index)
 * @param v2     Target node number (external node number, not internal index)
 * @param weight Edge weight
 * @param type   EdgeType::Directed, Undirected, or Reciprocated
 * @param label  Edge label (optional, stored on the out-arc of v1)
 * @param color  Edge color (optional, stored on the out-arc of v1)
 */
void Graph::edgeAdd(const int &v1,
                    const int &v2,
                    const qreal &weight,
                    const int &type,
                    const QString &label,
                    const QString &color)
{
    // Resolve external node numbers to internal m_graph indices.
    // vpos[] maps external node number -> index in m_graph vector.
    int source = vpos[v1];
    int target = vpos[v2];

    // qDebug() << "edgeAdd(): Adding arc from vertex" << v1 << "[idx" << source << "]"
    //          << "to vertex" << v2 << "[idx" << target << "]"
    //          << "weight" << weight << "type" << type << "label" << label;

    // Store the forward arc v1->v2.
    // OutEdge on v1: carries weight, color, and label (full arc metadata).
    // InEdge  on v2: carries weight only (no label/color needed on the receiving end).
    m_graph[source]->addOutEdge(v2, weight, color, label);
    m_graph[target]->addInEdge(v1, weight);

    // If weight is non-trivial (neither the default 1 nor the null-edge 0),
    // mark the graph as weighted so algorithms use weight values.
    if (weight != 1 && weight != 0)
    {
        setWeighted(true);
    }

    if (type == EdgeType::Reciprocated)
    {
        // The reverse arc v2->v1 already exists in the data model
        // (it was created in a prior edgeAdd call as a directed arc).
        // Nothing additional needs to be stored here.
        // The caller (edgeCreate) is responsible for any type-upgrade
        // semantics at the GraphVertex level if needed in the future.
    }
    else if (type == EdgeType::Undirected)
    {
        // Undirected edge: store the reverse arc v2->v1 as well,
        // so that both vertices see each other in their adjacency lists.
        // SocNetV represents undirected edges as two symmetric directed arcs.
        //
        // Note: only weight is stored on the reverse arc.
        // Color and label are not duplicated on the reverse direction.
        // qDebug() << "edgeAdd(): Edge is Undirected — also adding reverse arc"
        //          << v2 << "->" << v1;

        m_graph[target]->addOutEdge(v1, weight);
        m_graph[source]->addInEdge(v2, weight);
    }
}

/**
 * @brief Toggles the status of outbound edge source -> target at source vertex
 * @param v1
 * @param v2
 * @param toggle
 * @return
 */
void Graph::edgeOutboundStatusSet(const int &source, const int &target, const bool &toggle)
{

    m_graph[vpos[source]]->setOutEdgeEnabled(target, toggle);
}

/**
 * @brief Toggles the status of inbound edge target <- source at target vertex
 * @param v1
 * @param v2
 * @param toggle
 * @return
 */
void Graph::edgeInboundStatusSet(const int &target, const int &source, const bool &toggle)
{

    m_graph[vpos[target]]->setInEdgeEnabled(source, toggle);
}

/**
 * @brief Removes the directed arc v1->v2 or, if the graph is undirected, the edge v1 <->v2
 *
 * Emits signal to GW to delete the graphics item.
 *
 * @param v1
 * @param v2
 * @param removeReverse if true also removes the reverse edge
 */
void Graph::edgeRemove(const int &v1,
                       const int &v2,
                       const bool &removeReverse)
{
    qDebug() << "Graph::edgeRemove() - edge" << v1 << "[" << vpos[v1]
             << "] -->" << v2 << " to be removed. removeReverse:" << removeReverse;
    m_graph[vpos[v1]]->removeOutEdge(v2);
    m_graph[vpos[v2]]->removeInEdge(v1);

    if (isUndirected() || removeReverse)
    { // remove reverse edge too
        m_graph[vpos[v2]]->removeOutEdge(v1);
        m_graph[vpos[v1]]->removeInEdge(v2);
        m_graphIsSymmetric = true;
    }
    else
    {
        if (edgeExists(v2, v1) != 0)
        {
            m_graphIsSymmetric = false;
        }
    }

    emit signalRemoveEdge(v1, v2, (isDirected() || removeReverse));

    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Removes a SelectedEdge
 * @param selectedEdge
 * @param removeReverse
 */
void Graph::edgeRemoveSelected(SelectedEdge &selectedEdge,
                               const bool &removeReverse)
{
    qDebug() << "Graph::edgeRemoveSelected()" << selectedEdge;
    edgeRemove(selectedEdge.first, selectedEdge.second, removeReverse);
}

/**
 * @brief Removes all selected edges
 */
void Graph::edgeRemoveSelectedAll()
{
    qDebug() << "Graph::edgeRemoveSelectedAll()";

    foreach (SelectedEdge edgeToRemove, getSelectedEdges())
    {
        qDebug() << "Graph::edgeRemoveSelectedAll() - About to remove" << edgeToRemove;
        edgeRemoveSelected(edgeToRemove, true);
    }
}

//
// Existence / symmetry / counts
//

/**
 * @brief Checks if there is an edge from v1 to v2 and returns the weight, if the edge exists.
 * 
 * Complexity:  O(logN) for vpos retrieval + O(1) for QList index retrieval + O(logN) for checking edge(v2)
 * 
 * @param v1
 * @param v2
 * @param reciprocated: if true, checks if the edge is reciprocated (v1<->v2) with the same weight
 * @return zero if edge or reciprocated edge does not exist or non-zero if arc /reciprocated edge exists
 */
qreal Graph::edgeExists(const int &v1, const int &v2, const bool &checkReciprocal)
{

    edgeWeightTemp = m_graph[vpos[v1]]->hasEdgeTo(v2);
    //    qDebug() << "Checking if edge exists:" << v1 << "->" << v2 << "=" << edgeWeightTemp  ;

    if (!checkReciprocal)
    {
        return edgeWeightTemp;
    }
    else if (edgeWeightTemp != 0)
    {
        edgeReverseWeightTemp = m_graph[vpos[v2]]->hasEdgeTo(v1);
        //        qDebug() << "Checking if reverse edge exists: " << v2 << "->" << v1 << "=" << edgeWeightTemp  ;
        if (edgeWeightTemp == edgeReverseWeightTemp)
        {
            return edgeWeightTemp;
        }
    }
    return 0;
}

/**
 * @brief Checks if there is an edge from v1 to v2, even weight = 0 and returns the weight, if the edge exists
 * or RAND_MAX if the edge does not exist at all.
 *
 * This is only used in GraphML saving if the user has selected the Settings option to save zero-weight edges
 *
 * @see https://github.com/socnetv/app/issues/151
 *
 * @param v1
 * @param v2
 */
qreal Graph::edgeExistsVirtual(const int &v1, const int &v2)
{

    qreal m_weight = RAND_MAX;
    bool edgeStatus = false;
    H_edges::const_iterator it1;
    GraphVertex *source = m_graph[vpos[v1]];
    H_edges source_outEdges = source->m_outEdges;

    it1 = source_outEdges.constFind(v2);
    while (it1 != source_outEdges.constEnd() && it1.key() == v2)
    {
        if (it1.value().first == m_curRelation)
        {
            edgeStatus = it1.value().second.second;
            if (edgeStatus == true)
            {
                m_weight = it1.value().second.first;
            }
        }
        ++it1;
    }

    return m_weight;
}

/**
 * @brief Returns TRUE if edge(v1, v2) is symmetric, i.e. (v1,v2) == (v2,v1).
 * @param v1
 * @param v2
 * @return
 */
bool Graph::edgeSymmetric(const int &v1, const int &v2)
{
    if ((edgeExists(v1, v2, true)) != 0)
    {
        qDebug() << "Edge" << v1 << "->" << v2 << "is symmetric";
        return true;
    }
    else
    {
        qDebug() << "Edge" << v1 << "->" << v2 << "is not symmetric";
        return false;
    }
}

/**
 * @brief Returns the number of enabled ties in the current relation.
 *
 * IMPORTANT: Naming vs semantics
 *  - Internally, SocNetV stores adjacency as directed arcs in each vertex's out-edges.
 *  - For an UNDIRECTED graph, each undirected edge is represented as TWO symmetric arcs
 *    (v1->v2 and v2->v1). Therefore, summing outEdgesCount() over all vertices yields
 *    2*E, and we must divide by 2 to return the logical undirected edge count E.
 *  - For a DIRECTED graph, summing outEdgesCount() over all vertices yields A (the arc
 *    count), and we return it as-is.
 *
 * Caching:
 *  - m_totalEdges caches the *internal* count (sum of enabled out-arcs).
 *  - edgesEnabled() returns the *logical* count:
 *      - E for undirected graphs
 *      - A for directed graphs
 *
 * TODO / THINK: Self-loops (v->v)
 *  - A self-loop contributes exactly 1 outbound arc in outEdgesCount().
 *  - In an UNDIRECTED graph, dividing m_totalEdges by 2 assumes every tie is a symmetric pair.
 *    A loop is NOT a symmetric pair, so it would be mishandled by the /2 rule.
 *  - Decide on a loop policy:
 *      (a) forbid loops in undirected graphs (and filter them out here), or
 *      (b) count loops separately and adjust the formula to: E = (nonLoopArcs/2) + loopArcs
 *          (where loopArcs is the number of enabled v->v arcs).
 *
 * @return int Logical enabled ties: E (undirected) or A (directed)
 */
int Graph::edgesEnabled()
{
    int enabledEdges = 0;

    if (calculatedEdges)
    {
        enabledEdges = (isUndirected()) ? (m_totalEdges / 2) : m_totalEdges;
        return enabledEdges;
    }

    // Compute internal tie count from scratch: sum enabled outbound arcs for current relation.
    m_totalEdges = 0;
    VList::const_iterator it;
    for (it = m_graph.cbegin(); it != m_graph.cend(); ++it)
    {
        m_totalEdges += (*it)->outEdgesCount();
    }

    calculatedEdges = true;

    // Convert internal arc count to logical tie count.
    enabledEdges = (isUndirected()) ? (m_totalEdges / 2) : m_totalEdges;
    return enabledEdges;
}

/**
 * @brief Returns the number of outbound edges (arcs) from vertex v1
 * @param v1
 * @return
 */
int Graph::vertexEdgesOutbound(int v1)
{
    qDebug("Graph: vertexEdgesOutbound()");
    return m_graph[vpos[v1]]->outEdgesCount();
}

/**
 * @brief Returns the number of inbound edges (arcs) to vertex v1
 * @param v1
 * @return int
 */
int Graph::vertexEdgesInbound(int v1)
{
    qDebug("Graph: vertexEdgesInbound()");
    return m_graph[vpos[v1]]->inEdgesCount();
}

//
// Weight core
//

/**
 * @brief Changes the weight of the edge from vertex v1 to v2 (and optionally of the reverse edge)
 *
 * @param v1
 * @param v2
 * @param weight
 * @param undirected
 */
void Graph::edgeWeightSet(const int &v1, const int &v2,
                          const qreal &weight, const bool &undirected)
{
    qDebug() << "Changing the weight of edge" << v1 << "[" << vpos[v1]
             << "]->" << v2 << "[" << vpos[v2] << "]" << " to new weight " << weight;
    m_graph[vpos[v1]]->setOutEdgeWeight(v2, weight);
    if (undirected)
    {
        qDebug() << "Changing the weight of the reverse edge too";
        m_graph[vpos[v2]]->setOutEdgeWeight(v1, weight);
    }
    emit setEdgeWeight(v1, v2, weight);
    setModStatus(ModStatus::EdgeCount);
}

/**
 * @brief Returns the weight of the edge v1->v2
 * @param v1
 * @param v2
 * @return qreal
 */
qreal Graph::edgeWeight(const int &v1, const int &v2) const
{
    return m_graph[vpos[v1]]->hasEdgeTo(v2);
}

/**
 * @brief Changes the direction type of an existing edge
 *
 * @param v1
 * @param v2
 * @param weight
 */
void Graph::edgeTypeSet(const int &v1,
                        const int &v2,
                        const qreal &weight,
                        const int &dirType)
{

    qDebug() << "Changing the direction type of edge: " << v1
             << "->" << v2 << "new edgeType:" << dirType;

    if (dirType != EdgeType::Directed)
    {

        // check if reverse edge exists
        qreal revEdgeWeight = edgeExists(v2, v1);

        if (revEdgeWeight == 0)
        {
            // Reverse edge does not exist, add it
            qDebug() << "reverse  edge" << v1 << " <- " << v2 << " does not exist - Adding it...";
            // Note: Even if dirType=EdgeType::Undirected we add the opposite edge as EdgeType::Reciprocated
            edgeAdd(v2, v1, weight, EdgeType::Reciprocated, "", initEdgeColor);
        }
        else
        {
            // Reverse edge does exist
            if (dirType == EdgeType::Undirected)
            {
                // Make the edge weights equal
                // TOFIX: how do we decide which of the two weights to keep?
                qDebug() << "Graph::edgeTypeSet(): opposite  " << v1
                         << " <- " << v2 << " exists - equaling weights.";
                if (weight != revEdgeWeight)
                {
                    edgeWeightSet(v2, v1, weight);
                }
            }
            else
            {
                // if dirType is EdgeType::Reciprocated we don't need  to equalize weights
            }
        }
        emit signalEdgeType(v1, v2, dirType);
    }
}
