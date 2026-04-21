/**
 * @file parser_graphml.cpp
 * @brief GraphML parsers for SocNetV
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

#include "parser.h"
#include "global.h"

SOCNETV_USE_NAMESPACE

#include <QTextCodec>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QFileInfo>

/**
 * @brief Parses the data as GraphML (not GML) formatted network.
 *
 * @param rawData
 * @return bool
 */
bool Parser::parseAsGraphML(const QByteArray &rawData)
{

    qDebug() << "Parsing data as GraphML formatted...";

    totalNodes = 0;
    totalLinks = 0;
    nodeHash.clear();
    relationsList.clear();

    bool_key = false;
    bool_node = false;
    bool_edge = false;
    key_id = "";
    key_name = "";
    key_type = "";
    key_value = "";
    initNodeCustomIcon = "";
    initEdgeWeight = 1;
    edgeWeight = 1;
    edgeColor = "black";
    arrows = true;
    edgeDirType = EdgeType::Directed;

    // Create a xml parser
    QXmlStreamReader xml;

    // Prepare the user selected codec, if needed
    QByteArray userSelectedCodec = m_textCodecName.toLatin1();

    // Add raw data into xml parser
    xml.addData(rawData);

    qDebug() << "Testing if XML document encoding is the same as the userSelectedCodec:" << userSelectedCodec;

    xml.readNext();
    if (xml.isStartDocument())
    {
        qDebug() << "XML document version"
                 << xml.documentVersion()
                 << "encoding" << xml.documentEncoding()
                 << "userSelectedCodec"
                 << m_textCodecName;
        if (xml.documentEncoding().toString() != m_textCodecName)
        {
            qDebug() << "Conflicting encodings. "
                     << " Re-reading data with userSelectedCodec" << userSelectedCodec;
            xml.clear();

            QTextCodec *codec = QTextCodec::codecForName(userSelectedCodec);
            QString decodedData = codec->toUnicode(rawData);
            xml.addData(decodedData);

            //                QTextStream in(&rawData);
            //                in.setAutoDetectUnicode(false);
            //                QString decodedData = in.readAll();
            //                // QTextStream no longer supports setCodec
            //                in.setEncoding()
            //                QTextStream in(&rawData);
        }
        else
        {
            qDebug() << "Testing XML: OK";
            xml.clear();
            xml.addData(rawData);
        }
    }

    while (!xml.atEnd())
    {
        xml.readNext();
        qDebug() << "xml.token " << xml.tokenString();
        if (xml.isStartDocument())
        {
            qDebug() << "xml startDocument" << " version "
                     << xml.documentVersion()
                     << " encoding " << xml.documentEncoding();
        }

        if (xml.isStartElement())
        {
            qDebug() << "element name " << xml.name().toString();

            if (xml.name().toString() == "graphml")
            {
                qDebug() << "GraphML start. NamespaceUri is "
                         << xml.namespaceUri().toString()
                         << "Calling readGraphML()";
                if (!readGraphML(xml))
                {
                    // return false;
                    break;
                }
            }
            else
            { // not a GraphML doc, return false.
                xml.raiseError(
                    QObject::tr("not a GraphML file."));
                qDebug() << "### Error in startElement "
                         << " The file is not an GraphML version 1.0 file ";
                errorMessage = tr("Invalid GraphML file. "
                                  "XML at startElement but element name not graphml.");
                break;
            }
        }
        else if (xml.tokenString() == "Invalid")
        {
            xml.raiseError(
                QObject::tr("invalid GraphML or encoding."));
            qDebug() << "### Cannot find startElement"
                     << " The file is not valid GraphML or has invalid encoding";
            errorMessage = tr("Invalid GraphML file. "
                              "XML tokenString at line %1 invalid.")
                               .arg(xml.lineNumber());
            break;
        }
    } // end while

    // clear our mess - remove every hash element...
    keyFor.clear();
    keyName.clear();
    keyType.clear();
    keyDefaultValue.clear();
    nodeHash.clear();
    edgeMissingNodesList.clear();

    // if there was an error return false with error string
    if (xml.hasError())
    {
        qDebug() << "### xmls has error! "
                    "Returning false with errorString"
                 << xml.errorString();
        errorMessage =
            tr("Invalid GraphML file. "
               "XML has error at line %1, token name %2:\n\n%3")
                .arg(xml.lineNumber())
                .arg(xml.name().toString())
                .arg(xml.errorString());
        xml.clear();
        return false;
    }

    xml.clear();

    qDebug() << "signaling to change to the first relation...";
    if (m_parseSink)
    {
        m_parseSink->setRelation(0);
    }

    qDebug() << "Finished OK. Returning.";
    return true;
}

/**
 * @brief Checks the xml token name and calls the appropriate function.
 *
 * @param xml
 * @return bool
 */
bool Parser::readGraphML(QXmlStreamReader &xml)
{
    qDebug() << "Reading graphml token/element...";
    bool_node = false;
    bool_edge = false;
    bool_key = false;
    // Q_ASSERT(xml.isStartElement() && xml.name().toString() == "graph");

    while (!xml.atEnd())
    { // start reading until QXmlStreamReader end().

        xml.readNext(); // read next token

        qDebug() << "line:" << xml.lineNumber();

        if (xml.isStartElement())
        { // new token (graph, node, or edge) here
            qDebug() << "isStartElement() : "
                     << xml.name().toString();
            if (xml.name().toString() == "graph") // graph definition token
                readGraphMLElementGraph(xml);

            else if (xml.name().toString() == "key")
            { // key definition token
                QXmlStreamAttributes xmlStreamAttr = xml.attributes();
                readGraphMLElementKey(xmlStreamAttr);
            }
            else if (xml.name().toString() == "default") // default key value token
                readGraphMLElementDefaultValue(xml);

            else if (xml.name().toString() == "node") // graph definition token
                readGraphMLElementNode(xml);

            else if (xml.name().toString() == "data") // data definition token
                readGraphMLElementData(xml);

            else if (xml.name().toString() == "ShapeNode")
            {
                bool_node = true;
            }
            else if ((xml.name().toString() == "Geometry" || xml.name().toString() == "Fill" || xml.name().toString() == "BorderStyle" || xml.name().toString() == "NodeLabel" || xml.name().toString() == "Shape") && bool_node)
            {
                readGraphMLElementNodeGraphics(xml);
            }

            else if (xml.name().toString() == "edge")
            { // edge definition token
                QXmlStreamAttributes xmlStreamAttr = xml.attributes();
                readGraphMLElementEdge(xmlStreamAttr);
            }

            else if (xml.name().toString() == "BezierEdge")
            {
                bool_edge = true;
            }

            else if ((xml.name().toString() == "Path" || xml.name().toString() == "LineStyle" || xml.name().toString() == "Arrows" || xml.name().toString() == "EdgeLabel") && bool_edge)
            {
                readGraphMLElementEdgeGraphics(xml);
            }

            else
                readGraphMLElementUnknown(xml);
        }

        if (xml.isEndElement())
        { // token ends here
            qDebug() << " element ends here: "
                     << xml.name().toString();
            if (xml.name().toString() == "node") // node definition end
                endGraphMLElementNode(xml);
            else if (xml.name().toString() == "edge") // edge definition end
                endGraphMLElementEdge(xml);
        }

        if (xml.hasError())
        {
            qDebug() << "xml has error:" << xml.errorString();
            return false;
        }
    }

    // Check if we need to create any edges with missing nodes
    createMissingNodeEdges();

    return true;
}

/**
 * @brief Reads a graph definition
 *
 * Called at Graph element
 *
 * @param xml
 */
void Parser::readGraphMLElementGraph(QXmlStreamReader &xml)
{
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    QString defaultDirection = xmlStreamAttr.value("edgedefault").toString();
    qDebug() << "Parsing graph element - edgedefault "
             << defaultDirection;
    if (defaultDirection == "undirected")
    {
        qDebug() << "this is an undirected graph ";
        edgeDirType = EdgeType::Undirected;
        arrows = false;
    }
    else
    {
        qDebug() << "this is a directed graph ";
        edgeDirType = EdgeType::Directed;
        arrows = true;
    }
    // store graph id
    networkName = xmlStreamAttr.value("id").toString();
    // add it as relation
    relationsList << networkName;
    qDebug() << "Signaling to add new relation:" << networkName;
    if (m_parseSink)
    {
        m_parseSink->addNewRelation(networkName);
    }
    int lastRelationIndex = relationsList.size() - 1;
    if (lastRelationIndex > 0)
    {
        totalNodes = 0;
        qDebug() << "last relation index:"
                 << lastRelationIndex
                 << "signaling to change to the new relation";
        if (m_parseSink)
        {
            m_parseSink->setRelation(lastRelationIndex);
        }
    }
    qDebug() << "graph id:" << networkName;
}

/**
 * @brief Reads a key definition
 *
 * called at key element
 *
 * @param xmlStreamAttr
 */
void Parser::readGraphMLElementKey(QXmlStreamAttributes &xmlStreamAttr)
{
    key_id = xmlStreamAttr.value("id").toString();
    qDebug() << "Reading key element - key id" << key_id;
    key_what = xmlStreamAttr.value("for").toString();
    keyFor[key_id] = key_what;
    qDebug() << "key for " << key_what;

    if (xmlStreamAttr.hasAttribute("attr.name"))
    { // to be enabled in later versions..
        key_name = xmlStreamAttr.value("attr.name").toString();
        keyName[key_id] = key_name;
        qDebug() << "key attr.name" << key_name;
    }
    if (xmlStreamAttr.hasAttribute("attr.type"))
    {
        key_type = xmlStreamAttr.value("attr.type").toString();
        keyType[key_id] = key_type;
        qDebug() << "key attr.type" << key_type;
    }
    else if (xmlStreamAttr.hasAttribute("yfiles.type"))
    {
        key_type = xmlStreamAttr.value("yfiles.type").toString();
        keyType[key_id] = key_type;
        qDebug() << "key yfiles.type" << key_type;
    }
}

/**
 * @brief Reads default key values
 *
 * Called at a default element (usually nested inside key element)
 *
 * @param xml
 */
void Parser::readGraphMLElementDefaultValue(QXmlStreamReader &xml)
{

    key_value = xml.readElementText();
    key_name = keyName.value(key_id);
    keyDefaultValue[key_id] = key_value; // key_id is already stored

    qDebug() << "Reading default key values - key default value is"
             << key_value;

    if (keyFor.value(key_id) == "node")
    {
        if (key_name == "size")
        {
            qDebug() << "key default value" << key_value << "is for node size";
            conv_OK = false;
            initNodeSize = key_value.toInt(&conv_OK);
            if (!conv_OK)
                initNodeSize = 8;
        }
        if (key_name == "shape")
        {
            qDebug() << "key default value" << key_value << "is for nodes shape";
            initNodeShape = key_value;
        }
        if (key_name == "custom-icon")
        {
            qDebug() << "key default value" << key_value << "is for node custom-icon path";
            initNodeCustomIcon = key_value;
            initNodeCustomIcon = fileDirPath + "/" + initNodeCustomIcon;
            qDebug() << "initNodeCustomIcon full path:" << initNodeCustomIcon;
            if (QFileInfo::exists(initNodeCustomIcon))
            {
                qDebug() << "custom icon file exists!";
            }
            else
            {
                qDebug() << "custom icon file does not exists!";
                xml.raiseError(
                    QObject::tr(" Default custom icon for nodes does not exist in the filesystem. \nThe declared icon file was: \n%1").arg(initNodeCustomIcon));
            }
        }
        if (key_name == "color")
        {
            qDebug() << "key default value" << key_value << "is for nodes color";
            initNodeColor = key_value;
        }
        if (key_name == "label.color")
        {
            qDebug() << "key default value" << key_value << "is for node labels color";
            initNodeLabelColor = key_value;
        }
        if (key_name == "label.size")
        {
            qDebug() << "key default value" << key_value << "is for node labels size";
            conv_OK = false;
            initNodeLabelSize = key_value.toInt(&conv_OK);
            if (!conv_OK)
                initNodeLabelSize = 8;
        }
    }
    else if (keyFor.value(key_id) == "edge")
    {
        if (key_name == "weight")
        {
            qDebug() << "key default value" << key_value << "is for edges weight";
            conv_OK = false;
            initEdgeWeight = key_value.toDouble(&conv_OK);
            if (!conv_OK)
                initEdgeWeight = 1;
        }
        if (key_name == "color")
        {
            qDebug() << "key default value" << key_value << "is for edges color";
            initEdgeColor = key_value;
        }
    }
}

/**
 * @brief Reads basic node attributes and sets the nodeNumber.
 *
 * called at the start of a node element
 *
 * @param xml
 */
void Parser::readGraphMLElementNode(QXmlStreamReader &xml)
{
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    node_id = (xmlStreamAttr.value("id")).toString();
    totalNodes++;

    //    qDebug()<< "reading node id"<<  node_id
    //           << "index" << totalNodes
    //           << "added to nodeHash"
    //           << "gwWidth, gwHeight "<< gwWidth<< "," <<gwHeight;

    nodeHash[node_id] = totalNodes;

    // copy default node attribute values.
    // Some might change when reading element data, some will stay the same...
    nodeColor = initNodeColor;
    nodeShape = initNodeShape;
    nodeIconPath = initNodeCustomIcon;
    nodeSize = initNodeSize;
    nodeNumberSize = initNodeNumberSize;
    nodeNumberColor = initNodeNumberColor;
    nodeLabel = node_id;
    nodeLabelSize = initNodeLabelSize;
    nodeLabelColor = initNodeLabelColor;
    nodeCustomAttributes = initNodeCustomAttributes;
    bool_node = true;
    randX = rand() % gwWidth;
    randY = rand() % gwHeight;
}

/**
 * @brief Signals to create a new node
 *
 * called at the end of a node element
 *
 * @param xml
 */
void Parser::endGraphMLElementNode(QXmlStreamReader &xml)
{
    Q_UNUSED(xml);
    //@todo this check means we cannot have different nodes between relations.
    if (relationsList.size() > 1)
    {
        qDebug() << "multirelational data"
                    "skipping node creation. Node should have been created in earlier relation";
        bool_node = false;
        return;
    }

    qDebug() << "signaling to create a new node"
             << totalNodes << "id " << node_id
             << " label " << nodeLabel << "at pos:" << QPointF(randX, randY);

    if (nodeShape == "custom")
    {
        if (m_parseSink)
        {
            m_parseSink->createNode(totalNodes,
                                    nodeSize,
                                    nodeColor,
                                    nodeNumberColor,
                                    nodeNumberSize,
                                    nodeLabel,
                                    nodeLabelColor,
                                    nodeLabelSize,
                                    QPointF(randX, randY),
                                    nodeShape,
                                    (nodeIconPath.isEmpty() ? initNodeCustomIcon : nodeIconPath),
                                    false,
                                    nodeCustomAttributes);
        }
    }
    else
    {
        if (m_parseSink)
        {
            m_parseSink->createNode(totalNodes,
                                    nodeSize,
                                    nodeColor,
                                    nodeNumberColor,
                                    nodeNumberSize,
                                    nodeLabel,
                                    nodeLabelColor,
                                    nodeLabelSize,
                                    QPointF(randX, randY),
                                    nodeShape,
                                    QString(),
                                    false,
                                    nodeCustomAttributes);
        }
    }

    bool_node = false;
}

/**
 * @brief Reads basic edge creation properties.
 *
 * called at the start of an edge element
 *
 * @param xmlStreamAttr
 */
void Parser::readGraphMLElementEdge(QXmlStreamAttributes &xmlStreamAttr)
{

    edge_source = xmlStreamAttr.value("source").toString();
    edge_target = xmlStreamAttr.value("target").toString();
    edge_directed = xmlStreamAttr.value("directed").toString();

    //    qDebug()<< "Parsing edge id: "
    //            <<	xmlStreamAttr.value("id").toString()
    //                << "edge_source " << edge_source
    //                << "edge_target " << edge_target
    //                << "directed " << edge_directed;

    missingNode = false;
    edgeWeight = initEdgeWeight;
    edgeColor = initEdgeColor;
    edgeLabel = "";
    edgeCustomAttributes.clear();
    bool_edge = true;

    if (edge_directed == "false" || edge_directed.contains("false", Qt::CaseInsensitive))
    {
        edgeDirType = EdgeType::Undirected;
        qDebug() << "Edge is UNDIRECTED";
    }
    else
    {
        edgeDirType = EdgeType::Directed;
        qDebug() << "Edge is DIRECTED";
    }
    if (!nodeHash.contains(edge_source))
    {
        qDebug() << "source node id "
                 << edge_source
                 << "for edge from " << edge_source << " to " << edge_target
                 << "DOES NOT EXIST!"
                 << "Inserting into edgesMissingNodesHash";
        edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                     QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
        missingNode = true;
    }
    if (!nodeHash.contains(edge_target))
    {
        qDebug() << "target node id "
                 << edge_target
                 << "for edge from " << edge_source << " to " << edge_target
                 << "DOES NOT EXIST!"
                 << "Inserting into edgesMissingNodesHash";
        edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                     QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
        missingNode = true;
    }

    if (missingNode)
    {
        return;
    }

    source = nodeHash[edge_source];
    target = nodeHash[edge_target];
    qDebug() << "source " << edge_source
             << " num " << source
             << " - target " << edge_target << " num " << target
             << " edgeDirType " << edgeDirType;
}

/**
 * @brief Signals for a new edge to be created/added
 *
 * Called at the end of edge element
 *
 * @param xml
 */
void Parser::endGraphMLElementEdge(QXmlStreamReader &xml)
{
    Q_UNUSED(xml);
    if (missingNode)
    {
        qDebug() << "missingNode true "
                 << " postponing edge creation signal";
        return;
    }
    qDebug() << "signaling to create new edge"
             << source << "->" << target << " edgeDirType value " << edgeDirType;
    if (m_parseSink)
    {
        m_parseSink->createEdge(source, target, edgeWeight, edgeColor, edgeDirType,
                                arrows, bezier, edgeLabel, false, edgeCustomAttributes);
    }

    totalLinks++;
    bool_edge = false;
}

/**
 * @brief Reads data for edges and nodes
 *
 * called at a data element (usually nested inside a node or an edge element)
 *
 * @param xml
 */
void Parser::readGraphMLElementData(QXmlStreamReader &xml)
{

    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    key_id = xmlStreamAttr.value("key").toString();
    key_name = keyName.value(key_id);
    key_value = xml.text().toString();

    qDebug() << "parding data for key_id: "
             << key_id << "key_value " << key_value;

    if (key_value.trimmed() == "")
    {
        qDebug() << "empty key_value: "
                 << key_value
                 << "reading more xml.text()...";

        xml.readNext();

        key_value = xml.text().toString();

        qDebug() << "now key_value: " << key_value;

        if (key_value.trimmed() != "")
        {
            // if there's simple text after the StartElement,
            qDebug() << "key_id " << key_id
                     << " value is simple text " << key_value;
        }
        else
        { // no text, probably more tags. Return...
            qDebug() << "key_id " << key_id
                     << " for " << keyFor.value(key_id)
                     << ". More elements nested here. Returning";
            return;
        }
    }
    if (keyFor.value(key_id) == "node")
    {

        if (key_name == "color")
        {
            qDebug() << "Data found. Node color: "
                     << key_value << " for this node";
            nodeColor = key_value;
        }
        else if (key_name == "label")
        {
            qDebug() << "Data found. Node label: "
                        ""
                     << key_value << " for this node";
            nodeLabel = key_value;
        }
        else if (key_name == "x_coordinate")
        {
            qDebug() << "Data found. Node x: "
                     << key_value << " for this node";
            conv_OK = false;
            randX = key_value.toFloat(&conv_OK);
            if (!conv_OK)
                randX = 0;
            else
                randX = randX * gwWidth;
            qDebug() << "Using: " << randX;
        }
        else if (key_name == "y_coordinate")
        {
            qDebug() << "Data found. Node y: "
                     << key_value << " for this node";
            conv_OK = false;
            randY = key_value.toFloat(&conv_OK);
            if (!conv_OK)
                randY = 0;
            else
                randY = randY * gwHeight;
            qDebug() << "Using: " << randY;
        }
        else if (key_name == "size")
        {
            qDebug() << "Data found. Node size: "
                     << key_value << " for this node";
            conv_OK = false;
            nodeSize = key_value.toInt(&conv_OK);
            if (!conv_OK)
                nodeSize = initNodeSize;
            qDebug() << "Using: " << nodeSize;
        }
        else if (key_name == "label.size")
        {
            qDebug() << "Data found. Node label size: "
                     << key_value << " for this node";
            conv_OK = false;
            nodeLabelSize = key_value.toInt(&conv_OK);
            if (!conv_OK)
                nodeLabelSize = initNodeLabelSize;
            qDebug() << "Using: " << nodeSize;
        }
        else if (key_name == "label.color")
        {
            qDebug() << "Data found. Node label Color: "
                     << key_value << " for this node";
            nodeLabelColor = key_value;
        }
        else if (key_name == "shape")
        {
            qDebug() << "Data found. Node shape: "
                     << key_value << " for this node";
            nodeShape = key_value;
        }
        else if (key_name == "custom-icon")
        {
            qDebug() << "Data found. Node custom-icon path: "
                     << key_value << " for this node";
            nodeIconPath = key_value;
            nodeIconPath = fileDirPath + ("/") + nodeIconPath;
            qDebug() << "full node custom-icon path: "
                     << nodeIconPath;
        }
        else
        {
            qDebug() << "Data found for custom attribute: "
                     << key_name << " of this node. Data: " << key_value;
            nodeCustomAttributes.insert(key_name, key_value);
        }
    }
    else if (keyFor.value(key_id) == "edge")
    {
        if (key_name == "color")
        {
            qDebug() << "Data found. Edge color: "
                     << key_value << " for this edge";
            edgeColor = key_value;
            if (missingNode)
            {
                edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                             QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
            }
        }
        else if ((key_name == "value" || key_name == "weight"))
        {
            conv_OK = false;
            edgeWeight = key_value.toDouble(&conv_OK);
            if (!conv_OK)
                edgeWeight = 1.0;
            if (missingNode)
            {
                edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                             QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
            }
            qDebug() << "Data found. Edge value: "
                     << key_value << " Using " << edgeWeight << " for this edge";
        }
        else if (key_name == "size of arrow")
        {
            conv_OK = false;
            qreal temp = key_value.toFloat(&conv_OK);
            if (!conv_OK)
                arrowSize = 1;
            else
                arrowSize = temp;
            qDebug() << "Data found. Edge arrow size: "
                     << key_value << " Using  " << arrowSize << " for this edge";
        }
        else if (key_name == "label")
        {
            edgeLabel = key_value;
            if (missingNode)
            {
                edgesMissingNodesHash.insert(edge_source + "===>" + edge_target,
                                             QString::number(edgeWeight) + "|" + edgeColor + "|" + QString::number(edgeDirType));
            }
            qDebug() << "Data found. Edge label: "
                     << edgeLabel << " for this edge";
        }
        else
        {
            qDebug() << "Data found for custom edge attribute:"
                     << key_name << "value:" << key_value;
            edgeCustomAttributes.insert(key_name, key_value);
        }
    }
}

/**
 * @brief Reads node graphics data and properties: label, color, shape, size, coordinates, etc.
 * @param xml
 */
void Parser::readGraphMLElementNodeGraphics(QXmlStreamReader &xml)
{
    qDebug() << "reading node graphics/properties, element name"
             << xml.name().toString();
    qreal tempX = -1, tempY = -1, temp = -1;
    QString color;
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();

    if (xml.name().toString() == "Geometry")
    {
        if (xmlStreamAttr.hasAttribute("x"))
        {
            conv_OK = false;
            tempX = xml.attributes().value("x").toString().toFloat(&conv_OK);
            if (conv_OK)
                randX = tempX;
        }
        if (xmlStreamAttr.hasAttribute("y"))
        {
            conv_OK = false;
            tempY = xml.attributes().value("y").toString().toFloat(&conv_OK);
            if (conv_OK)
                randY = tempY;
        }
        qDebug() << "Node Coordinates: "
                 << tempX << " " << tempY << " Using coordinates" << randX << " " << randY;
        if (xmlStreamAttr.hasAttribute("width"))
        {
            conv_OK = false;
            temp = xmlStreamAttr.value("width").toString().toFloat(&conv_OK);
            if (conv_OK)
                nodeSize = temp;
            qDebug() << "Node Size: "
                     << temp << " Using nodesize" << nodeSize;
        }
        if (xmlStreamAttr.hasAttribute("shape"))
        {
            nodeShape = xmlStreamAttr.value("shape").toString();
            qDebug() << "Node Shape: "
                     << nodeShape;
        }
    }
    else if (xml.name().toString() == "Fill")
    {
        if (xmlStreamAttr.hasAttribute("color"))
        {
            nodeColor = xmlStreamAttr.value("color").toString();
            qDebug() << "Node color: "
                     << nodeColor;
        }
    }
    else if (xml.name().toString() == "BorderStyle")
    {
    }
    else if (xml.name().toString() == "NodeLabel")
    {
        key_value = xml.readElementText(); // see if there's simple text after the StartElement
        if (!xml.hasError())
        {
            qDebug() << "Node Label "
                     << key_value;
            nodeLabel = key_value;
        }
        else
        {
            qDebug() << "Cannot read Node Label. There must be more elements nested here, continuing";
        }
    }
    else if (xml.name().toString() == "Shape")
    {
        if (xmlStreamAttr.hasAttribute("type"))
        {
            nodeShape = xmlStreamAttr.value("type").toString();
            qDebug() << "Node shape: "
                     << nodeShape;
        }
    }
}

/**
 * @brief Reads edge graphics data and properties: path, linestyle,width, arrows, etc
 * @param xml
 */
void Parser::readGraphMLElementEdgeGraphics(QXmlStreamReader &xml)
{
    qDebug() << "reading edge graphics/props, element name"
             << xml.name().toString();

    qreal tempX = -1, tempY = -1, temp = -1;
    QString color, tempString;
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();

    if (xml.name().toString() == "Path")
    {
        if (xmlStreamAttr.hasAttribute("sx"))
        {
            conv_OK = false;
            tempX = xmlStreamAttr.value("sx").toString().toFloat(&conv_OK);
            if (conv_OK)
                bez_p1_x = tempX;
            else
                bez_p1_x = 0;
        }
        if (xmlStreamAttr.hasAttribute("sy"))
        {
            conv_OK = false;
            tempY = xmlStreamAttr.value("sy").toString().toFloat(&conv_OK);
            if (conv_OK)
                bez_p1_y = tempY;
            else
                bez_p1_y = 0;
        }
        if (xmlStreamAttr.hasAttribute("tx"))
        {
            conv_OK = false;
            tempX = xmlStreamAttr.value("tx").toString().toFloat(&conv_OK);
            if (conv_OK)
                bez_p2_x = tempX;
            else
                bez_p2_x = 0;
        }
        if (xmlStreamAttr.hasAttribute("ty"))
        {
            conv_OK = false;
            tempY = xmlStreamAttr.value("ty").toString().toFloat(&conv_OK);
            if (conv_OK)
                bez_p2_y = tempY;
            else
                bez_p2_y = 0;
        }
        qDebug() << "Edge Path control points: "
                 << bez_p1_x << " " << bez_p1_y << " " << bez_p2_x << " " << bez_p2_y;
    }
    else if (xml.name().toString() == "LineStyle")
    {
        if (xmlStreamAttr.hasAttribute("color"))
        {
            edgeColor = xmlStreamAttr.value("color").toString();
            qDebug() << "Edge color: "
                     << edgeColor;
        }
        if (xmlStreamAttr.hasAttribute("type"))
        {
            edgeType = xmlStreamAttr.value("type").toString();
            qDebug() << "Edge type: "
                     << edgeType;
        }
        if (xmlStreamAttr.hasAttribute("width"))
        {
            temp = xmlStreamAttr.value("width").toString().toFloat(&conv_OK);
            if (conv_OK)
                edgeWeight = temp;
            else
                edgeWeight = 1.0;
            qDebug() << "Edge width: "
                     << edgeWeight;
        }
    }
    else if (xml.name().toString() == "Arrows")
    {
        if (xmlStreamAttr.hasAttribute("source"))
        {
            tempString = xmlStreamAttr.value("source").toString();
            qDebug() << "Edge source arrow type: "
                     << tempString;
        }
        if (xmlStreamAttr.hasAttribute("target"))
        {
            tempString = xmlStreamAttr.value("target").toString();
            qDebug() << "Edge target arrow type: "
                     << tempString;
        }
    }
    else if (xml.name().toString() == "EdgeLabel")
    {
        key_value = xml.readElementText(); // see if there's simple text after the StartElement
        if (!xml.hasError())
        {
            qDebug() << "Edge Label "
                     << key_value;
            // probably there's more than simple text after StartElement
            edgeLabel = key_value;
        }
        else
        {
            qDebug() << "Can't read Edge Label. More elements nested ? Continuing with blank edge label....";
            edgeLabel = "";
        }
    }
}

/**
 * @brief Trivial call for unknown elements
 * @param xml
 */
void Parser::readGraphMLElementUnknown(QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement());
    qDebug() << "unknown element found:" << xml.name().toString();
}

/**
 * @brief Creates any missing node edges
 */
void Parser::createMissingNodeEdges()
{
    qDebug() << "Creating missing node edges... ";
    int count = 0;
    if ((count = edgesMissingNodesHash.size()) > 0)
    {

        bool ok;
        edgeWeight = initEdgeWeight;
        edgeColor = initEdgeColor;
        edgeDirType = EdgeType::Directed;
        qDebug() << "edges to create " << count;
        QHash<QString, QString>::const_iterator it =
            edgesMissingNodesHash.constBegin();
        while (it != edgesMissingNodesHash.constEnd())
        {
            qDebug() << "creating missing edge "
                     << it.key() << " data " << it.value();
            edgeMissingNodesList = (it.key()).split("===>");
            if (!((edgeMissingNodesList[0]).isEmpty()) && !((edgeMissingNodesList[1]).isEmpty()))
            {
                source = nodeHash.value(edgeMissingNodesList[0], -666);
                target = nodeHash.value(edgeMissingNodesList[1], -666);
                if (source == -666 || target == -666)
                {
                    // emit something that this node has not been declared
                    continue;
                }
                edgeMissingNodesListData = (it.value()).split("|");
                if (!edgeMissingNodesListData[0].isEmpty())
                {
                    edgeWeight = edgeMissingNodesListData[0].toInt(&ok, 10);
                }
                if (!edgeMissingNodesListData[1].isEmpty())
                {
                    edgeColor = edgeMissingNodesListData[1];
                }
                if (!edgeMissingNodesListData[2].isEmpty())
                {
                    if ((edgeMissingNodesListData[2]).contains("2"))
                        edgeDirType = EdgeType::Undirected;
                }
                qDebug() << "signaling to create new edge:"
                         << source << "->" << target << " edgeDirType value " << edgeDirType;
                if (m_parseSink)
                {
                    m_parseSink->createEdge(source, target, edgeWeight, edgeColor, edgeDirType, arrows, bezier, edgeLabel);
                }
            }
            ++it;
        }
    }
    else
    {
        qDebug() << "nothing to do";
    }
}
