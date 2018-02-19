/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.4
 Written in Qt
 
                         parser.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
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

#include "parser.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QTextStream>
#include <QString>
#include <QRegExp>
#include <QtDebug>		//used for qDebug messages
#include <QPointF>
#include <QMessageBox>
#include <QTextCodec>
#include <QRegularExpression>
#include <list>  // used as list<int> listDummiesPajek
#include <queue>		//for priority queue

#include "graph.h"	//needed for setParent

using namespace std;

Parser::Parser()
{
    qDebug() << "Parser::Parser() - running on thread "  << this->thread() ;


}



Parser::~Parser () {
    qDebug()<< "**** Parser::~Parser() destructor " << this->thread()
                <<" clearing hashes... ";
    nodeHash.clear();
    keyFor.clear();
    keyName.clear();
    keyType.clear();
    keyDefaultValue.clear();
    edgesMissingNodesHash.clear();
    edgeMissingNodesList.clear();
    edgeMissingNodesListData.clear();
    firstModeMultiMap.clear();
    secondModeMultiMap.clear();
    if (xml!=0) {
        qDebug()<< "**** Parser::~Parser() clearing xml reader object " ;
        xml->clear();
        delete xml;
        xml=0;
    }


}

/**
 * @brief Loads the network calling one of the load* methods
 */
void Parser::load(const QString fn,
                  const QString m_codec,
                  const int iNS, const QString iNC, const QString iNSh,
                  const QString iNNC, const int iNNS,
                  const QString iNLC, const int iNLS ,
                  const QString iEC,
                  const int width, const int height,
                  const int fFormat,
                  const int sm_mode,
                  const QString delim)  {


    qDebug()<< "**** Parser::load() - On a new thread " << this->thread();

    initNodeSize=iNS;
    initNodeColor=iNC;
    initNodeShape=iNSh;
    initNodeNumberColor=iNNC;
    initNodeNumberSize=iNNS;
    initNodeLabelColor=iNLC;
    initNodeLabelSize=iNLS;

    initEdgeColor=iEC;

    edgeDirType=EDGE_DIRECTED;
    arrows=true;
    bezier=false;
    fileName=fn;
    userSelectedCodecName = m_codec;
    networkName=(fileName.split ("/")).last();
    gwWidth=width;
    gwHeight=height;
    randX=0;
    randY=0;
    fileFormat= fFormat;
    two_sm_mode = sm_mode;
    if (!delim.isNull() && !delim.isEmpty())
        delimiter = delim;
    else delimiter=" ";

    xml=0;
    errorMessage=QString::null;

    qDebug()<< "**** Parser::load() - networkName "<< networkName
            << " fileFormat "<< fileFormat
              << "delim" << delim << "delimiter"<<delimiter;

    errorMessage=QString::null;

    switch (fileFormat){
    case FILE_GRAPHML:
        qDebug()<< "Parser::load() - calling loadGraphML()";
        if (loadGraphML()){
            qDebug()<< "Parser::load() - that was GRAPHML-formatted file";
        }
        break;
    case FILE_PAJEK:
        qDebug()<< "Parser::load() - calling loadPajek()";
        if ( loadPajek() ) {
            qDebug()<< "Parser::load() - that was PAJEK formatted file";
        }
        break;
    case FILE_ADJACENCY:
        qDebug()<< "Parser::load() - calling loadAdjacency()";
        if (loadAdjacency() ) {
            qDebug()<< "Parser::load() - that was ADJACENCY-formatted file";
        }
        break;
    case FILE_GRAPHVIZ:
        qDebug()<< "Parser::load() - calling loadDot()";
        if (loadDot() ) {
           qDebug()<< "Parser::load() - that was GRAPHVIZ-formatted file";
        }
        break;
    case FILE_UCINET:
        qDebug()<< "Parser::load() - calling loadDL()";
        if (loadDL() ){
            qDebug()<< "Parser::load() - that was UCINET-formatted file";
        }
        break;

    case FILE_GML:
        qDebug()<< "Parser::load() - calling loadGML()";
        if (loadGML() ){
            qDebug()<< "Parser::load() - that was GML-formatted file";
        }
        break;

    case FILE_EDGELIST_WEIGHTED:
        qDebug()<< "Parser::load() - calling loadEdgeListWeighted()";
        if (loadEdgeListWeighed(delimiter) ){
            qDebug()<< "Parser::load() - that was weighted EDGELIST-formatted file";
                    }
        break;

    case FILE_EDGELIST_SIMPLE:
        qDebug()<< "Parser::load() - calling loadEdgeListSimple()";
        if (loadEdgeListSimple(delimiter) ){
            qDebug()<< "Parser::load() - that was simple EDGELIST-formatted file";
        }
        break;

    case FILE_TWOMODE:
        qDebug()<< "Parser::load() - calling loadTwoModeSociomatrix()";
        if (loadTwoModeSociomatrix() ){
            qDebug()<< "Parser::load() - that was weigted TWOMODE-formatted file";
        }
        break;

    default:	//GraphML
        qDebug()<< "Parser::load() - default case - calling loadGraphML()";
        if (loadGraphML() ){
            qDebug()<< "Parser::load() - that was GRAPHML-formatted file";
        }
        break;
    }

    if (errorMessage!=QString::null) {
        loadFileError(errorMessage);
        return;
    }


    qDebug()<< "**** Parser::load() - on thread " << this->thread()
               << "Reached end. "
                  "Emitting finished() calling loadFileError() if any"
               << " fileFormat now "<< fileFormat ;

    emit finished ("Parser::load() - reach end");


}


/**
 * @brief Parser::loadFileError
 * @param errorMessage
 * Called when some Parser method cannot read or parse correctly a file.
 * It informs the Graph and then MW with an error message
 */
void Parser::loadFileError(const QString &errorMessage) {
    qDebug()<<"Parser::loadFileError() - errorMessage:"
           <<errorMessage;
    emit networkFileLoaded(FILE_UNRECOGNIZED,
                           QString::null,
                           QString::null,
                           0,
                           0,
                           false,
                           errorMessage
                           );
}

/**
 * @brief Parser::createRandomNodes
 * @param fixedNum
 * @param label
 * @param newNodes
 * Creates either a new node numbered fixedNum
 * or newNodes nodes numbered from 1 to to newNodes
 */
void Parser::createRandomNodes(const int &fixedNum,
                               const QString &label,
                               const int &newNodes){
    qDebug() << "Parser::createRandomNodes()";
    if (newNodes != 1 ) {
        for (int i=0; i<newNodes; i++) {
            qDebug() << "Parser::createRandomNodes() - Multiple nodes. "
                        "Creating node: "<< i+1;
            emit createNodeAtPosRandom(false);
        }
    }
    else {
        qDebug() << "Parser::createRandomNodes() - Single node. "
                    "Creating node: "<< fixedNum
               << " with label: " << label;
        emit createNodeAtPosRandomWithLabel( fixedNum, label, false );

    }
}

/**
    Tries to load a file as DL-formatted network (UCINET)
    If not it returns -1
*/
bool Parser::loadDL(){

    qDebug() << "Parser::loadDL() - Reading UCINET formatted file ";
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) {
        errorMessage = tr("Cannot open UCINET file ");
        return false;
    }
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str, label, nm_str, relation, prevLineStr=QString::null;

    int source=1, target=1, nm=0,lineCounter=0, mark=0, mark2=0, nodeSum=0;
    int relationCounter=0;

    bool labels_flag=false, data_flag=false, intOK=false, floatOK=false;
    bool relation_flag=false;
    bool fullmatrixFormat=false;
    bool edgelist1Format=false;
    QStringList lineElement, labelsList;
    relationsList.clear();
    totalLinks=0;
    arrows=true;
    bezier=false;
    edgeWeight=0;

    while ( !ts.atEnd() )   {
        str= ts.readLine();
        str=str.simplified();
        lineCounter++;
        qDebug() << "Parser::loadDL() - lineCount " << lineCounter
                 << "str.simplified: \n" << str;

        if ( isComment(str) )
            continue;

        if ( (lineCounter == 1) &&
             (!str.startsWith("DL",Qt::CaseInsensitive)  ) ) {
            qDebug() << "Parser::loadDL() - Not a DL file. Aborting!";
            errorMessage = tr("File does not start with DL in line 1");
            file.close();
            return false;
        }

        if (str.startsWith( "N=", Qt::CaseInsensitive)
                ||  str.startsWith( "N =", Qt::CaseInsensitive) )  {
            mark=str.indexOf("=");
            qDebug() << "Parser::loadDL() - Network size N declared here. "
                        "Check if NM exists";
            if ( (mark2=str.indexOf("NM", Qt::CaseInsensitive)) != -1 ) {
                nm_str = str.right(str.size() - mark2 );
                qDebug() << "Parser::loadDL() - NM exists at " << mark2
                         << " contains " << nm_str;
                nm_str = nm_str.simplified();
                nm_str.remove(0,2);
                nm_str = nm_str.simplified();
                if (nm_str.startsWith("="))
                    nm_str.remove(0,1);
                nm = nm_str.toInt(&intOK,10);
                qDebug() << "Parser::loadDL() - NM str: " << nm_str
                         << " and toInt:"<< nm ;
                if (!intOK) {
                    qDebug() << "Parser::loadDL() - NM conversion error..." ;
                    //emit something here...
                    errorMessage = tr("Cannot convert NM value to integer");
                    return false;
                }
                str.truncate(mark2);
                str=str.trimmed();
                qDebug() << "Parser::loadDL() -rest str becomes: " << str;
            }
            str=str.right(str.size()-mark-1);
            qDebug() << "Parser::loadDL() - N is declared to be : "
                     << str.toLatin1() ;
            totalNodes=str.toInt(&intOK,10);
            if (!intOK) {
                qDebug() << "Parser::loadDL() - N conversion error..." ;
                errorMessage = tr("Cannot convert N value to integer");
                return false;
            }
            qDebug() << "Parser::loadDL() - Finally N=" << totalNodes << "NM=" <<nm;
            continue;
        }

        if (str.startsWith( "FORMAT =", Qt::CaseInsensitive)
                || str.startsWith( "FORMAT=", Qt::CaseInsensitive))  {
            mark=str.indexOf("=");
            str=str.right(str.size()-mark-1);
            str=str.trimmed();
            qDebug() << "Parser::loadDL() - FORMAT : " <<  str.toLatin1() ;
            if (str.contains("FULLMATRIX",Qt::CaseInsensitive)) {
                fullmatrixFormat=true;
                qDebug() << "Parser::loadDL() - FORMAT fullmatrix detected" ;
            }
            else if (str.contains("edgelist",Qt::CaseInsensitive) ){
                edgelist1Format=true;
                qDebug() << "Parser::loadDL() - FORMAT edgelist detected" ;
            }
            continue;
        }
        else if (str.startsWith( "labels", Qt::CaseInsensitive)
                 || str.startsWith( "row labels", Qt::CaseInsensitive)) {
            labels_flag=true; data_flag=false;relation_flag=false;
            qDebug() << "Parser::loadDL() - START LABELS RECOGNITION "
                         "AND NODE CREATION";
            continue;
        }
        else if (str.startsWith( "COLUMN LABELS", Qt::CaseInsensitive)) {
            labels_flag=true; data_flag=false;relation_flag=false;
            qDebug() << "Parser::loadDL() - START COLUMN LABELS RECOGNITION "
                        "AND NODE CREATION";
            continue;
        }
        else if ( str.startsWith( "data:", Qt::CaseInsensitive)
                  || str.startsWith( "data :", Qt::CaseInsensitive) ) {
            data_flag=true; labels_flag=false;relation_flag=false;
            qDebug() << "Parser::loadDL() - START DATA RECOGNITION "
                        "AND EDGE CREATION";
            continue;
        }
        else if (str.startsWith( "LEVEL LABELS", Qt::CaseInsensitive) ) {
            relation_flag=true; data_flag=false; labels_flag=false;
            qDebug() << "Parser::loadDL() - START RELATIONS RECOGNITION";
            continue;
        }
        else if (str.isEmpty()){
            qDebug() << "Parser::loadDL() - EMPTY STRING - CONTINUE";
            continue;
        }


        if (labels_flag) {  //read a label and create a node in a random position
            label=str;
            if ( labelsList.contains(label) ) {
                qDebug() << "Parser::loadDL() - label exists. CONTINUE";
                continue;
            }
            else{
                qDebug() << "Parser::loadDL() - Adding label " << label
                         << " to labelList";
                labelsList << label;
            }
            nodeSum++;
            createRandomNodes(nodeSum, label,1);

        }
        if ( relation_flag){
            relation=str;
            if ( relationsList.contains(relation) ) {
                qDebug() << "Parser::loadDL() -relation exists. CONTINUE";
                continue;
            }
            else{
                qDebug() << "Parser::loadDL() - adding relation "<< relation
                         << " to relationsList and emitting addRelation ";
                relationsList << relation;
                emit addRelation( relation );
            }
        }
        if ( data_flag && fullmatrixFormat){
            qDebug() << "Parser::loadDL() - reading edges in fullmatrix format";
            // check if we haven't created any nodes...
            if ( nodeSum < totalNodes ){
                qDebug() << "Parser::loadDL() -nodes have not been created yet. "
                         << " calling createRandomNodes()" ;
                createRandomNodes(1, QString::null, totalNodes);
                nodeSum = totalNodes;
            }
            //SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS)
            if (!prevLineStr.isEmpty()) {
                str=(prevLineStr.append(" ")).append(str) ;
                qDebug() << "Parser::loadDL() -prevLineStr not empty - "
                            "prepending it to str - new str: \n" << str;
                str=str.simplified();
            }
            qDebug() << "Parser::loadDL() - splitting str to elements ";
            lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);
            qDebug() << "Parser::loadDL() - line elements " << lineElement.count();
            if (lineElement.count() < totalNodes ) {
                qDebug() << "Parser::loadDL() -This line has "
                         << lineElement.count()
                         << " elements, expected "
                         << totalNodes << " - appending next line";
                prevLineStr=str;
                continue;
            }
            prevLineStr.clear();
            target=1;
            if (source==1 && relationCounter>0){
                qDebug() << "Parser::loadDL() - we are at source 1. "
                            "Checking relationList";
                relation = relationsList[ relationCounter ];
                qDebug() << "Parser::loadDL() - "
                            "WE ARE THE FIRST DATASET/MATRIX"
                         << " source node counter is " << source
                         << " and relation to " << relation<< ": "
                         << relationCounter;
                emit relationSet (relationCounter);
            }
            else if (source>totalNodes) {
                source=1;
                relationCounter++;
                relation = relationsList[ relationCounter ];
                qDebug() << "Parser::loadDL() - "
                            "LOOKS LIKE WE ENTERED A NEW DATASET/MATRIX "
                         << " init source node counter to " << source
                         << " and relation to " << relation << ": "
                         << relationCounter;
                emit relationSet (relationCounter);
            }
            else {
                qDebug() << "Parser::loadDL() - source node counter is " << source;
            }

            for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
                //qDebug()<< (*it1).toLatin1() ;
                if ( (*it1)!="0"){
                    edgeWeight=(*it1).toFloat(&floatOK);
                    qDebug() << "Parser::loadDL() - relation "
                             << relationCounter
                             << " found edge from "
                             << source << " to " << target
                             << " weight " << edgeWeight
                             << " emitting edgeCreate() to parent" ;

                    emit edgeCreate( source, target, edgeWeight, initEdgeColor,
                                     EDGE_DIRECTED, arrows, bezier);
                    totalLinks++;
                    qDebug() << "Parser::loadDL() - TotalLinks= " << totalLinks;

                }
                target++;
            }
            source++;


        }
        if (data_flag && edgelist1Format) { //read edges in edgelist1 format
            // check if we haven't created any nodes...
            if ( nodeSum < totalNodes ){
                qDebug() << "Parser::loadDL() - nodes have not been created yet. "
                         << " calling createRandomNodes()" ;
                createRandomNodes(1, QString::null, totalNodes);
                nodeSum = totalNodes;
            }
            lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);

            if ( lineElement.count() != 3 ) {
                qDebug() << "Parser::loadDL() - Not an edgelist1 UCINET "
                            "formatted file. Aborting!!";
                file.close();
                //emit something...
                errorMessage = tr("UCINET file declared as edgelist but I found "
                                  "a line which did not have 3 elements (source, target, weight)");
                return false;
            }

            source =  (lineElement[0]).toInt(&intOK);
            target =  (lineElement[1]).toInt(&intOK);
            qDebug() << "Parser::loadDL() - source node "
                     << source  << " target node " << target;

            edgeWeight=(lineElement[2]).toDouble(&intOK);
            if (intOK) {
                qDebug() << "Parser::loadDL() -list file declares edge weight: "
                         << edgeWeight;
            }
            else {
                edgeWeight=1.0;
                qDebug () << "	list file NOT declaring edge weight. Setting default: " << edgeWeight;
            }

            qDebug() << "Parser::loadDL() - Creating link "
                    << source << " -> "<< target << " weight= "<< edgeWeight
                    <<  " TotalLinks=  " << totalLinks+1;
            emit edgeCreate(source, target, edgeWeight, initEdgeColor, EDGE_DIRECTED,
                            arrows, bezier);
            totalLinks++;
        }
    }
    //sanity check
    if (nodeSum != totalNodes) {
        qDebug()<< "Error: aborting";
        //emit something
        errorMessage = tr("UCINET declared N actors initially, "
                          "but I found a different number of actors");
        return false;
    }

    if (relationsList.count() == 0) {
        emit addRelation("unnamed");
    }


    //The network has been loaded. Tell MW the statistics and network type
    emit relationSet (0);
    qDebug() << "Parser::loadDL() - FINISHED - "
                "emitting networkFileLoaded() and clearing arrays";
    emit networkFileLoaded(FILE_UCINET, fileName, networkName,
                           totalNodes, totalLinks, EDGE_DIRECTED);

    lineElement.clear(); labelsList.clear(); relationsList.clear();
    return true;

}





/**
    Tries to load the file as Pajek-formatted network. If not it returns -1
*/
bool Parser::loadPajek(){

    qDebug ("\n\nParser: loadPajek");
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly ))  {
        errorMessage = tr("Cannot open Pajek file");
        return false;
    }
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str, label, temp;
    nodeColor="";
    edgeColor="";
    nodeShape="";
    initEdgeLabel = QString::null;
    QStringList lineElement;
    bool ok=false, intOk=false, check1=false, check2=false;
    bool has_arcs=false;
    bool nodes_flag=false, edges_flag=false, arcs_flag=false, arcslist_flag=false, matrix_flag=false;
    fileContainsNodeColors=false;
    fileContainsNodeCoords=false;
    fileContainsLinkColors=false;
    fileContainsLinkLabels=false;
    bool zero_flag=false;
    int   i=0, j=0, miss=0, source= -1, target=-1, nodeNum, colorIndex=-1;
    int coordIndex=-1, labelIndex=-1;
    unsigned long int lineCounter=0;
    int pos=-1, relationCounter=0;
    float weight=1;
    QString relation;
    list<int> listDummiesPajek;
    totalLinks=0;
    totalNodes=0;
    j=0;  //counts how many real nodes exist in the file
    miss=0; //counts missing nodeNumbers.
    //if j + miss < nodeNum, it creates (nodeNum-miss) dummy nodes which are deleted in the end.
    relationsList.clear();


    while ( !ts.atEnd() )   {
        str= ts.readLine();
        str = str.simplified();

        if ( isComment(str)  )
            continue;

        lineCounter++;
        qDebug()<< "*** Parser:loadPajek(): "
                << str;
        if (lineCounter==1) {
            if ( str.startsWith("graph",Qt::CaseInsensitive)
                 || str.startsWith("digraph",Qt::CaseInsensitive)
                 || str.startsWith("DL",Qt::CaseInsensitive)
                 || str.startsWith("list",Qt::CaseInsensitive)
                 || str.startsWith("graphml",Qt::CaseInsensitive)
                 || str.startsWith("<?xml",Qt::CaseInsensitive)
                 || str.startsWith("LEDA.GRAPH",Qt::CaseInsensitive)
                 || ( !str.startsWith("*network",Qt::CaseInsensitive)
                    && !str.startsWith("*vertices",Qt::CaseInsensitive) )
                 ) {
                qDebug()<< "*** Parser:loadPajek(): Not a Pajek-formatted file. Aborting!!";
                file.close();
                errorMessage = tr("Not a Pajek-formatted file. "
                                  "First not-comment line does not start with "
                                  "Network or Vertices");
                return false;
            }
        }

        if (!edges_flag && !arcs_flag && !nodes_flag && !arcslist_flag && !matrix_flag) {
            //qDebug("Parser-loadPajek(): reading headlines");
            if ( (lineCounter == 1) &&
                 (!str.contains("network",Qt::CaseInsensitive)
                  && !str.contains("vertices",Qt::CaseInsensitive)
                  )
                )
            {
                qDebug("*** Parser-loadPajek(): Not a Pajek file. Aborting!");
                errorMessage = tr("Not a Pajek-formatted file. "
                                  "First not-comment line does not start with "
                                  "Network or Vertices");
                file.close();
                return false;
            }
            else if (str.startsWith( "*network",Qt::CaseInsensitive) )  { //NETWORK NAME
                networkName = (str.right(str.size() - 8 )).simplified() ;
                if (!networkName.isEmpty() ) {
                    qDebug()<<"Parser::loadPajek(): networkName: "
                           <<networkName;
                }
                else {
                    qDebug()<<"Parser::loadPajek(): set networkName to unnamed.";
                    networkName = "unnamed";
                }
                continue;
            }
            if (str.contains( "vertices", Qt::CaseInsensitive) )  {
                lineElement=str.split(QRegExp("\\s+"));
                if (!lineElement[1].isEmpty()) 	totalNodes=lineElement[1].toInt(&intOk,10);
                qDebug ("Parser-loadPajek(): Vertices %i.",totalNodes);
                continue;
            }
            qDebug("Parser-loadPajek(): headlines end here");
        }
        /**SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS) IN SEVERAL ELEMENTS*/
        lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        if ( str.contains( "*edges", Qt::CaseInsensitive) ) {
            edges_flag=true; arcs_flag=false; arcslist_flag=false; matrix_flag=false;
            continue;
        }
        else if ( str.contains( "*arcs", Qt::CaseInsensitive) ) {
            arcs_flag=true; edges_flag=false; arcslist_flag=false;
            matrix_flag=false;
            //check if row has label for arcs data,
            // and use it as relation name
            if ( (pos = str.indexOf(":")) != -1 ) {
                relation = str.right(str.size() - pos -1) ;
                relation = relation.simplified();
                qDebug() << "Parser::loadPajek() - adding relation "<< relation
                         << " to relationsList and emitting addRelation ";
                relationsList << relation;
                emit addRelation( relation );
                if (relationCounter > 0) {
                    qDebug () << "Parser::loadPajek() relationCounter "
                              << relationCounter
                              << "emitting relationSet";
                    emit relationSet(relationCounter);
                    i=0; // reset the source node index
                }
                relationCounter++;
            }

            continue;
        }
        else if ( str.contains( "*arcslist", Qt::CaseInsensitive) ) {
            arcs_flag=false; edges_flag=false; arcslist_flag=true; matrix_flag=false;
            continue;
        }
        else if ( str.contains( "*matrix", Qt::CaseInsensitive) ) {
            qDebug() << str ;
            arcs_flag=false; edges_flag=false; arcslist_flag=false;
            matrix_flag=true;
            //check if row has label for matrix data,
            // and use it as relation name
            if ( (pos = str.indexOf(":")) != -1 ) {
                relation = str.right(str.size() - pos -1) ;
                relation = relation.simplified();
                qDebug() << "Parser::loadPajek() - adding relation "<< relation
                         << " to relationsList and emitting addRelation ";
                relationsList << relation;
                emit addRelation( relation );
                if (relationCounter > 0) {
                    qDebug () << "Parser::loadPajek() relationCounter "
                              << relationCounter
                              << "emitting relationSet";
                    emit relationSet(relationCounter);
                    i=0; // reset the source node index
                }
                relationCounter++;
            }
            continue;
        }

        /** READING NODES */
        if (!edges_flag && !arcs_flag && !arcslist_flag && !matrix_flag) {
            //qDebug("=== Reading nodes ===");
            nodes_flag=true;
            nodeNum=lineElement[0].toInt(&intOk, 10);
            //qDebug()<<"node number: "<<nodeNum;
            if (nodeNum==0) {
                qDebug ("Node is zero numbered! Raising zero-start-flag - increasing nodenum");
                zero_flag=true;
            }
            if (zero_flag){
                nodeNum++;
            }
            if (lineElement.size() < 2 ){
                label=lineElement[0];
                randX=rand()%gwWidth;
                randY=rand()%gwHeight;
                nodeColor=initNodeColor;
                nodeShape=initNodeShape;
            }
            else {	/** NODELABEL */
                label=lineElement[1];
                //qDebug()<< "node label: " << lineElement[1].toLatin1();
                str.remove (0, str.lastIndexOf(label)+label.size() );
                //qDebug()<<"cropped str: "<< str.toLatin1();
                if (label.contains('"', Qt::CaseInsensitive) )
                    label=label.remove('\"');
                //qDebug()<<"node label now: " << label.toLatin1();

                /** NODESHAPE: There are five possible . */
                if (str.contains("Ellipse", Qt::CaseInsensitive) ) nodeShape="ellipse";
                else if (str.contains("circle", Qt::CaseInsensitive) ) nodeShape="circle";
                else if (str.contains("box", Qt::CaseInsensitive) ) nodeShape="box";
                else if (str.contains("star", Qt::CaseInsensitive) ) nodeShape="star";
                else if (str.contains("triangle", Qt::CaseInsensitive) ) nodeShape="triangle";
                else nodeShape="diamond";
                /** NODECOLORS */
                //if there is an "ic" tag, a specific NodeColor for this node follows...
                if (str.contains("ic",Qt::CaseInsensitive)) {
                    for (int c=0; c< lineElement.count(); c++) {
                        if (lineElement[c] == "ic") {
                            //the colourname is at c+1 position.
                            nodeColor=lineElement[c+1];
                            fileContainsNodeColors=true;
                            break;
                        }
                    }
                    //qDebug()<<"nodeColor:" << nodeColor;
                    if (nodeColor.contains (".") )  nodeColor=initNodeColor;
                    if (nodeColor.startsWith("RGB")) nodeColor.replace(0,3,"#");
                    qDebug () << " \n\n PAJEK color " << nodeColor;
                }
                else { //there is no nodeColor. Use the default
                    //qDebug("No nodeColor");
                    fileContainsNodeColors=false;
                    nodeColor=initNodeColor;

                }
                /**READ NODE COORDINATES */
                if ( str.contains(".",Qt::CaseInsensitive) ) {
                    for (int c=0; c< lineElement.count(); c++)   {
                        temp=lineElement.at(c);
                        //		qDebug()<< temp.toLatin1();
                        if ((coordIndex=temp.indexOf(".", Qt::CaseInsensitive)) != -1 ){
                            if (lineElement.at(c-1) == "ic" ) continue;  //pajek declares colors with numbers!
                            if ( !temp[coordIndex-1].isDigit()) continue;  //needs 0.XX
                            if (c+1 == lineElement.count() ) {//first coord zero, i.e: 0  0.455
                                //qDebug ()<<"coords: " <<lineElement.at(c-1).toLatin1() << " " <<temp.toLatin1() ;
                                randX=lineElement.at(c-1).toDouble(&check1);
                                randY=temp.toDouble(&check2);
                            }
                            else {
                                //qDebug ()<<"coords: " << temp.toLatin1() << " " <<lineElement[c+1].toLatin1();
                                randX=temp.toDouble(&check1);
                                randY=lineElement[c+1].toDouble(&check2);
                            }

                            if (check1 && check2)    {
                                randX=randX * gwWidth;
                                randY=randY * gwHeight;
                                fileContainsNodeCoords=true;
                            }
                            if (randX <= 0.0 || randY <= 0.0 ) {
                                randX=rand()%gwWidth;
                                randY=rand()%gwHeight;
                            }
                            break;
                        }
                    }
                    //qDebug()<<"Coords: "<<randX << randY<< gwHeight;
                }
                else {
                    fileContainsNodeCoords=false;
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                    //qDebug()<<"No coords. Using random "<<randX << randY;
                }
            }
            /**START NODE CREATION */
            qDebug ()<<"Creating node numbered "<< nodeNum << " Real nodes count (j)= "<< j+1;
            j++;  //Controls the real number of nodes.
            //If the file misses some nodenumbers then we create dummies and delete them afterwards!
            if ( j + miss < nodeNum)  {
                qDebug ()<<"MW There are "<< j << " nodes but this node has number "<< nodeNum
                        <<"Creating node at "<< randX<<","<< randY;
                for (int num=j; num< nodeNum; num++) {
                    //qDebug()<< "Parser-loadPajek(): Creating dummy node number num = "<< num;
                    //qDebug()<<"Creating node at "<< randX<<","<< randY;

                    emit createNode(
                                num, initNodeSize, nodeColor,
                                initNodeNumberColor, initNodeNumberSize,
                                label, lineElement[3], initNodeLabelSize,
                            QPointF(randX, randY),
                            nodeShape, false
                            );
                    listDummiesPajek.push_back(num);
                    miss++;
                }
            }
            else if ( j > nodeNum ) {
                qDebug ("Error: This Pajek net declares this node with nodeNumber smaller than previous nodes. Aborting");
                errorMessage = tr("Pajek-formatted file declares a node with "
                                  "nodeNumber smaller than previous nodes.");
                return false;
            }
            qDebug ()<<"emitting createNode()";
            emit createNode(
                        nodeNum,initNodeSize, nodeColor,
                        initNodeNumberColor, initNodeNumberSize,
                        label, initNodeLabelColor, initNodeLabelSize,
                        QPointF(randX, randY),
                        nodeShape, false
                        );
            initNodeColor=nodeColor;
        }
        // NODES CREATED. CREATE EDGES/ARCS NOW.
        // first check that all nodes are already created
        else {
            if (j && j!=totalNodes)  {  //if there were more or less nodes than the file declared
                qDebug()<<"*** WARNING ***: The Pajek file declares " << totalNodes <<"  nodes, but I found " <<  j << " nodes...." ;
                totalNodes=j;
            }
            else if (j==0) {  //if there were no nodes at all, we need to create them now.
                qDebug()<< "The Pajek file declares "<< totalNodes<< " but I didnt found any nodes. I will create them....";
                for (int num=j+1; num<= totalNodes; num++) {
                    qDebug() << "Parser-loadPajek(): Creating node number i = "<< num;
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                    emit createNode(
                                num,initNodeSize, initNodeColor,
                                initNodeNumberColor, initNodeNumberSize,
                                QString::number(i), initNodeLabelColor,initNodeLabelSize,
                                QPointF(randX, randY),
                                initNodeShape, false
                                );
                }
                j=totalNodes;
            }
            if (edges_flag && !arcs_flag)   {  /**EDGES */
                qDebug("Parser-loadPajek(): ==== Reading edges ====");
                qDebug()<<lineElement;
                source =  lineElement[0].toInt(&ok, 10);
                target = lineElement[1].toInt(&ok,10);
                if (source == 0 || target == 0 ) {
                    errorMessage = tr("Pajek-formatted file declares edge "
                                            "with a zero source or target nodeNumber. "
                                            "Each node should have a nodeNumber > 0.");
                    return false;  //  i --> (i-1)   internally
                }
                else if (source < 0 && target >0  ) {  //weights come first...
                    edgeWeight  = lineElement[0].toFloat(&ok);
                    source=  lineElement[1].toInt(&ok, 10);
                    if (lineElement.count()>2) {
                        target = lineElement[2].toInt(&ok,10);
                    }
                    else {
                        target = lineElement[1].toInt(&ok,10);  //self link
                    }
                }
                else if (lineElement.count()>2)
                    edgeWeight =lineElement[2].toFloat(&ok);
                else
                    edgeWeight =1.0;

                //qDebug()<<"Parser-loadPajek(): weight "<< weight;

                if (lineElement.contains("c", Qt::CaseSensitive ) ) {
                    //qDebug("Parser-loadPajek(): file with link colours");
                    fileContainsLinkColors=true;
                    colorIndex=lineElement.indexOf( QRegExp("[c]"), 0 ) + 1;
                    if (colorIndex >= lineElement.count()) edgeColor=initEdgeColor;
                    else 	edgeColor=lineElement [ colorIndex ];
                    if (edgeColor.contains (".") )  edgeColor=initEdgeColor;
                    //qDebug()<< " current color "<< edgeColor;
                }
                else  {
                    //qDebug("Parser-loadPajek(): file with no link colours");
                    edgeColor=initEdgeColor;
                }

                if (lineElement.contains("l", Qt::CaseSensitive ) ) {
                    qDebug("Parser-loadPajek(): file with link labels");
                    fileContainsLinkLabels=true;
                    labelIndex=lineElement.indexOf( QRegExp("[l]"), 0 ) + 1;
                    if (labelIndex >= lineElement.count()) edgeLabel=initEdgeLabel;
                    else 	edgeLabel=lineElement [ labelIndex ];
                    if (edgeLabel.contains (".") )  edgeLabel=initEdgeLabel;
                    qDebug()<< " edge label "<< edgeLabel;
                }
                else  {
                    //qDebug("Parser-loadPajek(): file with no link labels");
                    edgeLabel=initEdgeLabel;
                }

                arrows=false;
                bezier=false;
                qDebug()<< "Parser-loadPajek(): EDGES: Create edge between " << source << " - "<< target;
                emit edgeCreate(source, target, edgeWeight, edgeColor,
                                EDGE_UNDIRECTED, arrows, bezier, edgeLabel);
                totalLinks=totalLinks+2;

            } //end if EDGES
            else if (!edges_flag && arcs_flag)   {  /** ARCS */
                //qDebug("Parser-loadPajek(): === Reading arcs ===");
                source=  lineElement[0].toInt(&ok, 10);
                target = lineElement[1].toInt(&ok,10);
                if (source == 0 || target == 0 ) {
                    errorMessage = tr("Pajek-formatted file declares arc "
                                            "with a zero source or target nodeNumber. "
                                            "Each node should have a nodeNumber > 0.");
                    return false;   //  i --> (i-1)   internally
                }
                else if (source < 0 && target >0 ) {  //weights come first...
                    edgeWeight  = lineElement[0].toFloat(&ok);
                    source=  lineElement[1].toInt(&ok, 10);
                    if (lineElement.count()>2) {
                        target = lineElement[2].toInt(&ok,10);
                    }
                    else {
                        target = lineElement[1].toInt(&ok,10);  //self link
                    }
                }
                else if (lineElement.count()>2)
                    edgeWeight  =lineElement[2].toFloat(&ok);
                else
                    edgeWeight =1.0;

                if (lineElement.contains("c", Qt::CaseSensitive ) ) {
                    //qDebug("Parser-loadPajek(): file with link colours");
                    edgeColor=lineElement.at ( lineElement.indexOf( QRegExp("[c]"), 0 ) + 1 );
                    fileContainsLinkColors=true;
                }
                else  {
                    //qDebug("Parser-loadPajek(): file with no link colours");
                    edgeColor=initEdgeColor;
                }

                if (lineElement.contains("l", Qt::CaseSensitive ) ) {
                    qDebug("Parser-loadPajek(): file with link labels");
                    fileContainsLinkLabels=true;
                    labelIndex=lineElement.indexOf( QRegExp("[l]"), 0 ) + 1;
                    if (labelIndex >= lineElement.count()) edgeLabel=initEdgeLabel;
                    else 	edgeLabel=lineElement.at ( labelIndex );
                    //if (edgeLabel.contains (".") )  edgeLabel=initEdgeLabel;
                    qDebug()<< " edge label "<< edgeLabel;
                }
                else  {
                    //qDebug("Parser-loadPajek(): file with no link labels");
                    edgeLabel=initEdgeLabel;
                }
                arrows=true;
                bezier=false;
                has_arcs=true;
                qDebug()<<"Parser-loadPajek(): ARCS: Creating arc from node "<< source << " to node "<< target << " with weight "<< weight;
                emit edgeCreate(source, target, edgeWeight , edgeColor,
                                EDGE_DIRECTED, arrows, bezier, edgeLabel);
                totalLinks++;
            } //else if ARCS
            else if (arcslist_flag)   {  /** ARCSlist */
                //qDebug("Parser-loadPajek(): === Reading arcs list===");
                if (lineElement[0].startsWith("-") ) lineElement[0].remove(0,1);
                source= lineElement[0].toInt(&ok, 10);
                fileContainsLinkColors=false;
                edgeColor=initEdgeColor;
                has_arcs=true;
                arrows=true;
                bezier=false;
                for (int index = 1; index < lineElement.size(); index++) {
                    target = lineElement.at(index).toInt(&ok,10);
                    qDebug()<<"Parser-loadPajek(): ARCS LIST: Creating ARC source "<< source << " target "<< target << " with weight "<< weight;
                    emit edgeCreate(source, target, edgeWeight, edgeColor,
                                    EDGE_DIRECTED, arrows, bezier);
                    totalLinks++;
                }
            } //else if ARCSLIST
            else if (matrix_flag)   {  /** matrix */
                //qDebug("Parser-loadPajek(): === Reading matrix of edges===");
                i++;
                source= i;
                fileContainsLinkColors=false;
                edgeColor=initEdgeColor;
                has_arcs=true;
                arrows=true;
                bezier=false;
                for (target = 0; target < lineElement.size(); target ++) {
                    if ( lineElement.at(target) != "0" ) {
                        edgeWeight  = lineElement.at(target).toFloat(&ok);
                        qDebug()<<"Parser-loadPajek():  MATRIX: Creating arc source "
                               << source << " target "<< target +1
                               << " with weight "<< weight;
                        emit edgeCreate(source, target+1, edgeWeight, edgeColor,
                                        EDGE_DIRECTED, arrows, bezier);
                        totalLinks++;
                    }
                }
            } //else if matrix
        } //end if BOTH ARCS AND EDGES
    } //end WHILE
    file.close();
    if (j==0) {
        errorMessage = tr("Could not find node declarations in this Pajek-formatted file.");
        return false;
    }

    qDebug("Parser-loadPajek(): Removing all dummy nodes, if any");
    if (listDummiesPajek.size() > 0 ) {
        qDebug("Trying to delete the dummies now");
        for ( list<int>::iterator it=listDummiesPajek.begin(); it!=listDummiesPajek.end(); it++ ) {
            emit removeDummyNode(*it);
        }
    }

    if (relationsList.count() == 0) {
        emit addRelation(networkName);
    }

    qDebug("Parser-loadPajek(): Clearing DumiesList from Pajek");
    listDummiesPajek.clear();
    relationsList.clear();

    emit relationSet (0);

    //The network has been loaded. Tell MW the statistics and network type
    emit networkFileLoaded(FILE_PAJEK, fileName, networkName,
                           totalNodes, totalLinks,
                           ( (has_arcs) ? EDGE_DIRECTED: EDGE_UNDIRECTED));


    return true;

}






/**
    Tries to load the file as adjacency sociomatrix-formatted. If not it returns -1
*/
bool Parser::loadAdjacency(){
    qDebug("\n\nParser: loadAdjacency()");
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) {
        return false;
    }
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str;
    QStringList lineElement;
    int i=0, j=0, newCount=0, lastCount=0;
    bool intOK=false;
    relationsList.clear();
    totalNodes=0;
    edgeWeight=1.0;
    totalLinks=0;
    i=1;
    while ( i < 11 &&  !ts.atEnd() )   {
        str= ts.readLine() ;
        str=str.simplified();

        if ( isComment(str) )
            continue;

        if ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("graph",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL",Qt::CaseInsensitive)
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
             ) {
            qDebug()<< "*** Parser:loadAdjacency(): Not an Adjacency-formatted file. Aborting!!";
            file.close();
            errorMessage = tr("Not an Adjacency-formatted file. "
                                    "A non-comment line includes prohibited strings (i.e GraphML), "
                                    "not only numbers and delimiters as expected.");
            return false;
        }
        if ( str.contains (","))
            newCount = (str.split(",")).count();
        else
            newCount = (str.split(" ")).count();
        qDebug() << str;
        qDebug() << "newCount "<<newCount << " i " << i;
        if  ( (newCount != lastCount && i>1 ) || (newCount < i) ) {
            // line element count differ, therefore this can't be an adjacency matrix
            qDebug()<< "*** Parser:loadAdjacency(): Not an Adjacency-formatted file. Aborting!!";
            file.close();
            errorMessage = tr("Error reading Adjacency-formatted file. "
                              "Matrix row %1 has different number of elements from previous row.").arg(i);
            return false;
        }

        lastCount=newCount;

        i++;
    }

    ts.reset();
    ts.seek(0);
    i=0;


    while ( !ts.atEnd() )   {
        str= ts.readLine() ;
        str=str.simplified();  // transforms "/t", "  ", etc to plain " ".

        if ( isComment(str) )
            continue;

        if ( str.contains (","))
            lineElement=str.split(",");
        else
            lineElement=str.split(" ");
        if (i == 0 ) {
            totalNodes=lineElement.count();
            qDebug("Parser-loadAdjacency(): There are %i nodes in this file", totalNodes);
            for (j=0; j<totalNodes; j++) {

                randX=rand()%gwWidth;
                randY=rand()%gwHeight;
                qDebug()<<"Parser-loadAdjacency(): Calling createNode() for node "<< j+1
                       <<" No coords found. Using random "<<randX <<", " << randY;

                emit createNode( j+1,initNodeSize,  initNodeColor,
                                 initNodeNumberColor, initNodeNumberSize,
                                 QString::number(j+1), initNodeLabelColor, initNodeLabelSize,
                                 QPointF(randX, randY),
                                 initNodeShape, false
                                 );
            }
        }
        qDebug("Parser-loadAdjacency(): Finished creation of nodes");
        if ( totalNodes != (int) lineElement.count() )  {
            errorMessage = tr("Error reading Adjacency-formatted file. "
                                    "Matrix row %1 has different number of elements from previous row.").arg(i);
            return false;
        }
        j=0;
        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            if ( (*it1)!="0"){
                edgeWeight =(*it1).toFloat(&intOK);
                arrows=true;
                bezier=false;
                emit edgeCreate(i+1, j+1, edgeWeight, initEdgeColor, EDGE_DIRECTED, arrows, bezier);
                totalLinks++;
                qDebug() << "Parser-loadAdjacency(): Link from i=" << i+1 << " to j=" <<  j+1
                         << "weight=" << edgeWeight << ". TotalLinks= " << totalLinks;
            }

            j++;
        }
        i++;
    }
    file.close();

    if (relationsList.count() == 0 ) {
        emit addRelation( "unnamed" );
    }

    qDebug() << "Parser: SM network has been loaded. Tell MW the statistics and network type";
    emit networkFileLoaded(FILE_ADJACENCY, fileName, networkName,
                           totalNodes, totalLinks, EDGE_DIRECTED);

    return true;

}




/**
    Tries to load the file as two-mode sociomatrix. If not it returns -1
*/
bool Parser::loadTwoModeSociomatrix(){
    qDebug("\n\nParser: loadTwoModeSociomatrix()");
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) {
        errorMessage = tr("Cannot open two-mode sociomatrix file. ") ;
        return false;
    }
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str;
    QStringList lineElement;
    int i=0, j=0,  newCount=0, lastCount=0;
    totalNodes=0;
    edgeWeight=1.0;
    relationsList.clear();

    while (  !ts.atEnd() )  {
        i++;
        str= ts.readLine();
        str=str.simplified();
        if ( isComment(str) )
            continue;
        if ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("graph",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL",Qt::CaseInsensitive)
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
             ) {
            qDebug()<< "*** Parser:loadTwoModeSociomatrix(): Not a two mode sociomatrix-formatted file. Aborting!!";
            file.close();
            errorMessage = tr("Not a two-mode sociomatrix formatted file. "
                              "A non-comment line includes prohibited strings (i.e GraphML), "
                              "not only numbers and delimiters as expected.");
            return false;
        }
        if ( str.contains (",")){
            lineElement=str.split(",");
            newCount = lineElement.count();
        }
        else {
            lineElement=str.split(" ");
            newCount = lineElement.count();
        }
        qDebug() << str;
        qDebug() << "newCount "<<newCount << " nodes. We are at i = " << i;
        if  ( (newCount != lastCount && i>1 )  ) { // line element count differ
            qDebug()<< "*** Parser:loadTwoModeSociomatrix(): Not a Sociomatrix-formatted file. Aborting!!";
            file.close();
            errorMessage = tr("Row %1 has fewer or more elements than previous line.").arg(i);
            return false;
        }
        lastCount=newCount;
        randX=rand()%gwWidth;
        randY=rand()%gwHeight;
        qDebug()<< "Parser-loadTwoModeSociomatrix(): Calling createNode() for node "
                << i << " at random x,y: "<<randX << randY;
        emit createNode( i,initNodeSize, initNodeColor,
                         initNodeNumberColor, initNodeNumberSize,
                         QString::number(i), initNodeLabelColor, initNodeLabelSize,
                         QPointF(randX, randY),
                         initNodeShape, false
                         );
        j=1;
        qDebug()<< "Parser-loadTwoModeSociomatrix(): reading actor affiliations...";
        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            if ( (*it1)!="0"){
                qDebug() << "Parser-loadTwoModeSociomatrix(): there is an 1 from "<< i << " to "<<  j;
                firstModeMultiMap.insert(i, j);
                secondModeMultiMap.insert(j, i);
                for (int k = 1; k < i ; ++k) {
                    qDebug() << "Checking earlier discovered actor k = " << k;
                    if ( firstModeMultiMap.contains(k, j) ) {
                        arrows=true;
                        bezier=false;
                        edgeWeight = 1;
                        qDebug() << " Actor " << i << " on the same event as actor " << k << ". Creating edge ";
                        emit edgeCreate(i, k, edgeWeight, initEdgeColor,
                                        EDGE_UNDIRECTED, arrows, bezier);
                        totalLinks++;
                    }
                }

            }
            j++;
        }
    }
    file.close();


    if (relationsList.count() == 0) {
        emit addRelation("unnamed");
    }

    qDebug() << "Parser: Two-mode SM network has been loaded. Tell MW the statistics and network type";
    emit networkFileLoaded(FILE_TWOMODE, fileName,networkName,
                           totalNodes, totalLinks, EDGE_UNDIRECTED);

    return true;

}




/**
    Tries to load a file as GraphML (not GML) formatted network.
    If not it returns -1
*/
bool Parser::loadGraphML(){

    qDebug("\n\nParser: loadGraphML()");
    totalNodes=0;
    totalLinks=0;
    nodeHash.clear();
    relationsList.clear();

    bool_key=false; bool_node=false; bool_edge=false;
    key_id = "";
    key_name = "";
    key_type = "";
    key_value = "";
    initEdgeWeight = 1;
    edgeWeight=1;
    edgeColor="black";
    arrows=true;
    edgeDirType=EDGE_DIRECTED;
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) {
        return false;
    }

    //QXmlStreamReader *xml = new QXmlStreamReader();
    QXmlStreamReader xml;

    qDebug() << " Parser::loadGraphML(): read file to a byte array";
    QByteArray encodedData = file.readAll();
    QByteArray userSelectedCodec =userSelectedCodecName.toLatin1();
    xml.addData(encodedData);

    qDebug() << " Parser::loadGraphML(): test if XML document encoding == userCodec";

    xml.readNext();
    if (xml.isStartDocument()) {
        qDebug()<< " Parser::loadGraphML(): Testing XML document " << " version "
                << xml.documentVersion()
                << " encoding " << xml.documentEncoding()
                << " userSelectedCodecName.toUtf8() "
                << userSelectedCodecName.toUtf8();
         if ( xml.documentEncoding().toString() != userSelectedCodecName) {
                qDebug() << " Parser::loadGraphML(): Conflicting encodings. "
                         << " Re-reading data with userCodec";
                xml.clear();
                QTextStream in(&encodedData);
                in.setAutoDetectUnicode(false);
                QTextCodec *codec = QTextCodec::codecForName( userSelectedCodec );
                in.setCodec(codec);
                QString decodedData = in.readAll();
                xml.addData(decodedData);
         }
         else {
             qDebug() << " Parser::loadGraphML(): Testing XML: OK";
             xml.clear();
             xml.addData(encodedData);
         }
    }


    while (!xml.atEnd()) {
        xml.readNext();
        qDebug()<< " Parser::loadGraphML(): xml.token "<< xml.tokenString();
        if (xml.isStartDocument()) {
            qDebug()<< " Parser::loadGraphML(): xml startDocument" << " version "
                    << xml.documentVersion()
                    << " encoding " << xml.documentEncoding();
        }

        if (xml.isStartElement()) {
            qDebug()<< " Parser::loadGraphML(): element name "<< xml.name().toString();

            if (xml.name() == "graphml") {
                qDebug()<< " Parser::loadGraphML(): GraphML start. NamespaceUri is "
                        << xml.namespaceUri().toString()
                        << "Calling readGraphML()";
                if (! readGraphML(xml) ) {
                    return false;
                }
            }
            else {	//not a GraphML doc, return false.
                xml.raiseError(
                            QObject::tr(" loadGraphML(): not a GraphML file."));
                qDebug()<< "*** loadGraphML(): Error in startElement "
                        << " The file is not an GraphML version 1.0 file ";
                file.close();
                errorMessage = tr("XML at startElement but element name not graphml.");
                return false;
            }
        }
        else if  ( xml.tokenString() == "Invalid" ){
            xml.raiseError(
                        QObject::tr(" loadGraphML(): invalid GraphML or encoding."));
            qDebug()<< "*** loadGraphML(): Cannot find  startElement"
                    << " The file is not valid GraphML or has invalid encoding";
            file.close();
            errorMessage = tr("XML tokenString at line %1 invalid.").arg(xml.lineNumber());
            return false;
        }
    }

    emit relationSet (0);
    //The network has been loaded. Tell MW the statistics and network type
    emit networkFileLoaded(FILE_GRAPHML, fileName, networkName,
                           totalNodes, totalLinks,
                           edgeDirType);
    //clear our mess - remove every hash element...
    keyFor.clear();
    keyName.clear();
    keyType.clear();
    keyDefaultValue.clear();
    nodeHash.clear();
    return true;
}


/*
 * Called from loadGraphML
 * This method checks the xml token name and calls the appropriate function.
 */
bool Parser::readGraphML(QXmlStreamReader &xml){
    qDebug()<< " Parser::readGraphML() " ;
    bool_node=false;
    bool_edge=false;
    bool_key=false;
    //Q_ASSERT(xml.isStartElement() && xml.name() == "graph");

    while (!xml.atEnd()) { //start reading until QXmlStreamReader end().
        xml.readNext();	//read next token

        qDebug()<< "Parser::readGraphML() - line:" << xml.lineNumber();
        if (xml.hasError()) {
            qDebug()<< "Parser::readGraphML() - xml.hasError():" << xml.errorString();
            errorMessage =
                        tr("XML has error at line %1, token name %2:\n%3")
                        .arg(xml.lineNumber())
                        .arg(xml.name().toString())
                        .arg(xml.errorString());
            return false;
        }

        if (xml.isStartElement()) {	//new token (graph, node, or edge) here
            qDebug()<< "Parser::readGraphML() - start of element: "
                    << xml.name().toString() ;
            if (xml.name() == "graph")	//graph definition token
                readGraphMLElementGraph(xml);

            else if (xml.name() == "key")	{//key definition token
                QXmlStreamAttributes xmlStreamAttr = xml.attributes();
                readGraphMLElementKey(  xmlStreamAttr );
            }
            else if (xml.name() == "default") //default key value token
                readGraphMLElementDefaultValue(xml);

            else if (xml.name() == "node")	//graph definition token
                readGraphMLElementNode(xml);

            else if (xml.name() == "data")	//data definition token
                readGraphMLElementData(xml);

            else if ( xml.name() == "ShapeNode") {
                bool_node =  true;
            }
            else if ( ( xml.name() == "Geometry"
                        || xml.name() == "Fill"
                        || xml.name() == "BorderStyle"
                        || xml.name() == "NodeLabel"
                        || xml.name() == "Shape"
                        ) && 	bool_node
                      ) {
                readGraphMLElementNodeGraphics(xml);
            }

            else if (xml.name() == "edge")	{//edge definition token
                QXmlStreamAttributes xmlStreamAttr = xml.attributes();
                readGraphMLElementEdge( xmlStreamAttr  );
            }

            else if ( xml.name() == "BezierEdge") {
                bool_edge =  true;
            }

            else if (	 (
                             xml.name() == "Path"
                             || xml.name() == "LineStyle"
                             || xml.name() == "Arrows"
                             || xml.name() == "EdgeLabel"
                             )
                         && 	bool_edge
                         ) {
                readGraphMLElementEdgeGraphics(xml);
            }

            else
                readGraphMLElementUnknown(xml);
        }

        if (xml.isEndElement()) {		//token ends here
            qDebug()<< "Parser::readGraphML() -  element ends here: "
                    << xml.name().toString() ;
            if (xml.name() == "node")	//node definition end
                endGraphMLElementNode(xml);
            else if (xml.name() == "edge")	//edge definition end
                endGraphMLElementEdge(xml);
        }
    }
    // call createMissingNodeEdges() to create any edges with missing nodes
    createMissingNodeEdges();

    return true;
}


// this method reads a graph definition 
// called at Graph element
void Parser::readGraphMLElementGraph(QXmlStreamReader &xml){
    qDebug()<< "Parser::readGraphMLElementGraph";
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    QString defaultDirection = xmlStreamAttr.value("edgedefault").toString();
    qDebug()<< "Parser::readGraphMLElementGraph() - edgedefault "
            << defaultDirection;
    if (defaultDirection=="undirected"){
        qDebug()<< "Parser::readGraphMLElementGraph() - this is an undirected graph ";
        edgeDirType=EDGE_UNDIRECTED;
        arrows=false;
    }
    else {
        qDebug()<< "Parser::readGraphMLElementGraph() - this is a directed graph ";
        edgeDirType=EDGE_DIRECTED;
        arrows=true;
    }
    networkName = xmlStreamAttr.value("id").toString();
    relationsList << networkName;
    qDebug()<< "Parser::readGraphMLElementGraph() - emit addRelation()" <<networkName;
    emit addRelation( networkName);
    int relationCounter = relationsList.count() - 1; //zero indexed
    if (relationCounter > 0) {
        totalNodes=0;
        qDebug () << "Parser::readGraphMLElementGraph() - relations now "
                  << relationCounter
                  << "emitting relationSet to change to the new relation"
                  << "and setting totalNodes to " <<totalNodes;
        emit relationSet(relationCounter);
    }
    qDebug()<< "Parser::readGraphMLElementGraph() - graph id  "  << networkName; //store graph id to return it afterwards
}



// this method is needed because the QXmlStreamReader::hasAttribute
// has been implemented in Qt 4.5. Therefore we need this ugly hack to 
// be able to compile SocNetV in all previous Qt4 version. :(
//FIXME: This will be obsolete soon
bool Parser::xmlStreamHasAttribute( QXmlStreamAttributes &xmlStreamAttr, QString str) const
{
    int size = xmlStreamAttr.size();
    for (int  i = 0 ; i < size ; i++) {
        qDebug() << "Parser::xmlStreamHasAttribute() - "
                 << xmlStreamAttr.at(i).name().toString() ;
        if ( xmlStreamAttr.at(i).name() == str)
            return true;
    }
    return false;
}



// this method reads a key definition 
// called at key element
void Parser::readGraphMLElementKey ( QXmlStreamAttributes &xmlStreamAttr )
{
    qDebug()<< "Parser::readGraphMLElementKey()";
    key_id = xmlStreamAttr.value("id").toString();
    qDebug()<< "Parser::readGraphMLElementKey() - key id "<< key_id;
    key_what = xmlStreamAttr.value("for").toString();
    keyFor [key_id] = key_what;
    qDebug()<< "Parser::readGraphMLElementKey() - key for "<< key_what;

    // if (xmlStreamAttr.hasAttribute("attr.name") ) {  // to be enabled in later versions..
    if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("attr.name") ) ) {
        key_name =xmlStreamAttr.value("attr.name").toString();
        keyName [key_id] = key_name;
        qDebug()<< "Parser::readGraphMLElementKey() - key attr.name "
                << key_name;
    }
    //if (xmlStreamAttr.hasAttribute("attr.type") ) {
    if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("attr.type") ) ) {
        key_type=xmlStreamAttr.value("attr.type").toString();
        keyType [key_id] = key_type;
        qDebug()<< "Parser::readGraphMLElementKey() - key attr.type "
                << key_type;
    }
    //else if (xmlStreamAttr.hasAttribute("yfiles.type") ) {
    else if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("yfiles.type") ) ) {
        key_type=xmlStreamAttr.value("yfiles.type").toString();
        keyType [key_id] = key_type;
        qDebug()<< "Parser::readGraphMLElementKey() - key yfiles.type "
                << key_type;
    }

}


// this method reads default key values 
// called at a default element (usually nested inside key element)
void Parser::readGraphMLElementDefaultValue(QXmlStreamReader &xml) {
    qDebug()<< "Parser::readGraphMLElementDefaultValue()";

    key_value=xml.readElementText();
    keyDefaultValue [key_id] = key_value;	//key_id is already stored
    qDebug()<< "Parser::readGraphMLElementDefaultValue() - key default value is "
            << key_value;
    if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementDefaultValue() - key default value "
                << key_value << " is for node size";
        conv_OK=false;
        initNodeSize= key_value.toInt(&conv_OK);
        if (!conv_OK) initNodeSize = 8;
    }
    if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementDefaultValue() - key default value "
                << key_value << " is for nodes shape";
        initNodeShape= key_value;
    }
    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementDefaultValue() - key default value "
                << key_value << " is for nodes color";
        initNodeColor= key_value;
    }
    if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementDefaultValue() - key default value "
                << key_value << " is for node labels color";
        initNodeLabelColor= key_value;
    }
    if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementDefaultValue() - key default value "
                << key_value << " is for node labels size";
        conv_OK=false;
        initNodeLabelSize= key_value.toInt(&conv_OK);
        if (!conv_OK) initNodeLabelSize = 8;
    }
    if (keyName.value(key_id) == "weight" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "Parser::readGraphMLElementDefaultValue() - key default value "
                << key_value << " is for edges weight";
        conv_OK=false;
        initEdgeWeight= key_value.toFloat(&conv_OK);
        if (!conv_OK)
            initEdgeWeight = 1;
    }
    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "Parser::readGraphMLElementDefaultValue() - key default value "
                << key_value << " is for edges color";
        initEdgeColor= key_value;
    }

}



// this method reads basic node attributes and sets the nodeNumber.
// called at the start of a node element
void Parser::readGraphMLElementNode(QXmlStreamReader &xml){
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    node_id = (xmlStreamAttr.value("id")).toString();
    totalNodes++;
    qDebug()<<"Parser::readGraphMLElementNode() - node id "<<  node_id
           << " index " << totalNodes << " added to nodeHash ";

    nodeHash[node_id]=totalNodes;

    //copy default node attribute values.
    //Some might change when reading element data, some will stay the same...
    nodeColor = initNodeColor;
    nodeShape = initNodeShape;
    nodeSize = initNodeSize;
    nodeNumberSize=initNodeNumberSize;
    nodeNumberColor=initNodeNumberColor;
    nodeLabel = node_id;
    nodeLabelSize=initNodeLabelSize;
    nodeLabelColor=initNodeLabelColor;
    bool_node = true;
    randX=rand()%gwWidth;
    randY=rand()%gwHeight;

}


// this method emits the node creation signal.
// called at the end of a node element   

void Parser::endGraphMLElementNode(QXmlStreamReader &xml){
    Q_UNUSED(xml);
    //@todo this check means we cannot have different nodes between relations.
    if (relationsList.count() > 1 ) {
        qDebug()<<"Parser::endGraphMLElementNode() - multirelational data"
                  "skipping node creation. Node should have been created in earlier relation";
            bool_node = false;
        return;
    }
    qDebug()<<"Parser::endGraphMLElementNode() - signal to create node "
           << " nodenumber "<< totalNodes  << " id " << node_id
           << " label " << nodeLabel << " coords " <<randX << ", " <<randY;
    emit createNode(
                totalNodes, nodeSize, nodeColor,
                nodeNumberColor, nodeNumberSize,
                nodeLabel, nodeLabelColor, nodeLabelSize,
                QPointF(randX,randY),
                nodeShape, false
                );
    bool_node = false;

}


// this method reads basic edge creation properties.
// called at the start of an edge element
void Parser::readGraphMLElementEdge(QXmlStreamAttributes &xmlStreamAttr){

    edge_source = xmlStreamAttr.value("source").toString();
    edge_target = xmlStreamAttr.value("target").toString();
    edge_directed = xmlStreamAttr.value("directed").toString();
    qDebug()<< "Parser::readGraphMLElementEdge() - id: "
            <<	xmlStreamAttr.value("id").toString()
                << "edge_source " << edge_source
                << "edge_target " << edge_target
                << "directed " << edge_directed;

    missingNode=false;
    edgeWeight=initEdgeWeight;
    edgeColor=initEdgeColor;
    edgeLabel = "";
    bool_edge= true;

    if ( edge_directed=="false" || (edge_directed.contains("false"),Qt::CaseInsensitive) ) {
        edgeDirType=EDGE_UNDIRECTED;
        qDebug()<< "Parser::readGraphMLElementEdge() - UNDIRECTED";
    }
    else {
        edgeDirType=EDGE_DIRECTED;
        qDebug()<< "Parser::readGraphMLElementEdge() - DIRECTED";
    }
    if (!nodeHash.contains(edge_source)) {
        qDebug() << "Parser::readGraphMLElementEdge() - source node id "
                 << edge_source
                 << "for edge from " << edge_source << " to " << edge_target
                 << " DOES NOT EXIST!"
                 << "Inserting into edgesMissingNodesHash";
        edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                     QString::number(edgeWeight)+"|"+edgeColor
                                     +"|"+QString::number(edgeDirType));
        missingNode=true;
    }
    if (!nodeHash.contains(edge_target)) {
        qDebug() << "Parser::readGraphMLElementEdge() - target node id "
                 << edge_target
                 << "for edge from " << edge_source << " to " << edge_target
                 << " DOES NOT EXIST!"
                 << "Inserting into edgesMissingNodesHash";
        edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                     QString::number(edgeWeight)+"|"+edgeColor
                                     +"|"+QString::number(edgeDirType));
        missingNode=true;
    }

    if (missingNode) {
        return;
    }

    source = nodeHash [edge_source];
    target = nodeHash [edge_target];
    qDebug()<< "Parser::readGraphMLElementEdge() - source "<< edge_source
            << " num "<< source
            <<" - target "<< edge_target << " num "<< target
              << " edgeDirType " << edgeDirType;


}


// this method emits the edge creation signal.
// called at the end of edge element   
void Parser::endGraphMLElementEdge(QXmlStreamReader &xml){
    Q_UNUSED(xml);
    if (missingNode) {
        qDebug()<<"Parser::endGraphMLElementEdge() - missingNode true "
               << " postponing edge creation signal";
        return;
    }
    qDebug()<<"Parser::endGraphMLElementEdge() - signal edgeCreate "
           << source << " -> " << target << " edgeDirType value " << edgeDirType;
    emit edgeCreate(source, target, edgeWeight, edgeColor, edgeDirType,
                    arrows, bezier, edgeLabel);
    totalLinks++;
    bool_edge= false;
}


/*
 * this method reads data for edges and nodes
 * called at a data element (usually nested inside a node an edge element)
 */
void Parser::readGraphMLElementData (QXmlStreamReader &xml){
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    key_id = xmlStreamAttr.value("key").toString();
    key_value=xml.text().toString();
    qDebug()<< "Parser::readGraphMLElementData() - key_id: "
            <<  key_id <<  " key_value "<< key_value;
    if (key_value.trimmed() == "")
    {
        qDebug()<< "Parser::readGraphMLElementData() - text: " << key_value;
        xml.readNext();
        key_value=xml.text().toString();
        qDebug()<< "Parser::readGraphMLElementData() - text: " << key_value;
        if (  key_value.trimmed() != "" ) { //if there's simple text after the StartElement,
            qDebug()<< "Parser::readGraphMLElementData() - key_id " << key_id
                    << " value is simple text " <<key_value ;
        }
        else {  //no text, probably more tags. Return...
            qDebug()<< "Parser::readGraphMLElementData() - key_id " << key_id
                    << " for " <<keyFor.value(key_id)
                    << ". More elements nested here, continuing";
            return;
        }

    }

    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementData() -Data found. Node color: "
                << key_value << " for this node";
        nodeColor= key_value;
    }
    else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "node" ){
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Node label: "
                   ""<< key_value << " for this node";
        nodeLabel = key_value;
    }
    else if (keyName.value(key_id) == "x_coordinate" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Node x: "
                << key_value << " for this node";
        conv_OK=false;
        randX= key_value.toFloat( &conv_OK ) ;
        if (!conv_OK)
            randX = 0;
        else
            randX=randX * gwWidth;
        qDebug()<< "Parser::readGraphMLElementData() - Using: "<< randX;
    }
    else if (keyName.value(key_id) == "y_coordinate" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Node y: "
                << key_value << " for this node";
        conv_OK=false;
        randY= key_value.toFloat( &conv_OK );
        if (!conv_OK)
            randY = 0;
        else
            randY=randY * gwHeight;
        qDebug()<< "Parser::readGraphMLElementData() - Using: "<< randY;
    }
    else if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Node size: "
                << key_value << " for this node";
        conv_OK=false;
        nodeSize= key_value.toInt ( &conv_OK );
        if (!conv_OK)
            nodeSize = initNodeSize;
        qDebug()<< "Parser::readGraphMLElementData() - Using: "<< nodeSize;
    }
    else if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Node label size: "
                << key_value << " for this node";
        conv_OK=false;
        nodeLabelSize= key_value.toInt ( &conv_OK );
        if (!conv_OK)
            nodeLabelSize = initNodeLabelSize;
        qDebug()<< "Parser::readGraphMLElementData() - Using: "<< nodeSize;
    }
    else if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node" ){
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Node label Color: "
                << key_value << " for this node";
        nodeLabelColor = key_value;
    }
    else if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Node shape: "
                << key_value << " for this node";
        nodeShape= key_value;
    }
    else if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Edge color: "
                << key_value << " for this edge";
        edgeColor= key_value;
        if (missingNode){
            edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                         QString::number(edgeWeight)+"|"+edgeColor
                                         +"|"+QString::number(edgeDirType));
        }
    }
    else if ( ( keyName.value(key_id) == "value" ||  keyName.value(key_id) == "weight" ) && keyFor.value(key_id) == "edge" ) {
        conv_OK=false;
        edgeWeight= key_value.toFloat( &conv_OK );
        if (!conv_OK)
            edgeWeight = 1.0;
        if (missingNode){
            edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                         QString::number(edgeWeight)+"|"+edgeColor
                                         +"|"+QString::number(edgeDirType));
        }
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Edge value: "
                << key_value << " Using "<< edgeWeight << " for this edge";
    }
    else if ( keyName.value(key_id) == "size of arrow"  && keyFor.value(key_id) == "edge" ) {
        conv_OK=false;
        float temp = key_value.toFloat( &conv_OK );
        if (!conv_OK) arrowSize = 1;
        else  arrowSize = temp;
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Edge arrow size: "
                << key_value << " Using  "<< arrowSize << " for this edge";
    }
    else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "edge" ){
        edgeLabel = key_value;
        if (missingNode){
            edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                         QString::number(edgeWeight)+"|"+edgeColor
                                         +"|"+QString::number(edgeDirType));
        }
        qDebug()<< "Parser::readGraphMLElementData() - Data found. Edge label: "
                << edgeLabel << " for this edge";
    }



}



/**
 * 	Reads node graphics data and properties: label, color, shape, size, coordinates, etc.
 */
void Parser::readGraphMLElementNodeGraphics(QXmlStreamReader &xml) {
    qDebug()<< "Parser::readGraphMLElementNodeGraphics() - element name "
            << xml.name().toString();
    float tempX =-1, tempY=-1, temp=-1;
    QString color;
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();

    if ( xml.name() == "Geometry" ) {

        if ( xmlStreamHasAttribute ( xmlStreamAttr, "x") ) {
            conv_OK=false;
            tempX = xml.attributes().value("x").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                randX = tempX;
        }
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "y") ) {
            conv_OK=false;
            tempY = xml.attributes().value("y").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                randY = tempY;
        }
        qDebug()<< "Parser::readGraphMLElementNodeGraphics() - Node Coordinates: "
                << tempX << " " << tempY << " Using coordinates" << randX<< " "<<randY;
        if (xmlStreamHasAttribute ( xmlStreamAttr, "width") ) {
            conv_OK=false;
            temp = xmlStreamAttr.value("width").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                nodeSize = temp;
            qDebug()<< "Parser::readGraphMLElementNodeGraphics() - Node Size: "
                    << temp<< " Using nodesize" << nodeSize;
        }
        if (xmlStreamHasAttribute ( xmlStreamAttr, "shape") ) {
            nodeShape = xmlStreamAttr.value("shape").toString();
            qDebug()<< "Parser::readGraphMLElementNodeGraphics() - Node Shape: "
                    << nodeShape;
        }

    }
    else if (xml.name() == "Fill" ){
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "color") ) {
            nodeColor= xmlStreamAttr.value("color").toString();
            qDebug()<< "Parser::readGraphMLElementNodeGraphics() - Node color: "
                    << nodeColor;
        }

    }
    else if ( xml.name() == "BorderStyle" ) {


    }
    else if (xml.name() == "NodeLabel" ) {
        key_value=xml.readElementText();  //see if there's simple text after the StartElement
        if (!xml.hasError()) {
            qDebug()<< "Parser::readGraphMLElementNodeGraphics() - Node Label "
                    << key_value;
            nodeLabel = key_value;
        }
        else {
            qDebug()<< "Parser::readGraphMLElementNodeGraphics() - "
                       "Cannot read Node Label. There must be more elements nested here, continuing";
        }
    }
    else if (xml.name() == "Shape" ) {
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "type") ) {
            nodeShape= xmlStreamAttr.value("type").toString();
            qDebug()<< "Parser::readGraphMLElementNodeGraphics() - Node shape: "
                    << nodeShape;
        }

    }


}

void Parser::readGraphMLElementEdgeGraphics(QXmlStreamReader &xml) {
    qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - element name "
            << xml.name().toString();

    float tempX =-1, tempY=-1, temp=-1;
    QString color, tempString;
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();

    if ( xml.name() == "Path" ) {
        //if ( xmlStreamAttr.hasAttribute("sx") ) {
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "sx") ) {
            conv_OK=false;
            tempX = xmlStreamAttr.value("sx").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                bez_p1_x = tempX;
            else bez_p1_x = 0 ;
        }
        //if ( xmlStreamAttr.hasAttribute("sy") ) {
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "sy") ) {
            conv_OK=false;
            tempY = xmlStreamAttr.value("sy").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                bez_p1_y = tempY;
            else bez_p1_y = 0 ;
        }
        //if ( xmlStreamAttr.hasAttribute("tx") ) {
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "tx") ) {
            conv_OK=false;
            tempX = xmlStreamAttr.value("tx").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                bez_p2_x = tempX;
            else bez_p2_x = 0 ;
        }
        //if ( xmlStreamAttr.hasAttribute("ty") ) {
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "ty") ) {
            conv_OK=false;
            tempY = xmlStreamAttr.value("ty").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                bez_p2_y = tempY;
            else bez_p2_y = 0 ;
        }
        qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - Edge Path control points: "
                << bez_p1_x << " " << bez_p1_y << " " << bez_p2_x << " " << bez_p2_y;
    }
    else if (xml.name() == "LineStyle" ){
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "color") ) {
            edgeColor= xmlStreamAttr.value("color").toString();
            qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - Edge color: "
                    << edgeColor;
        }
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "type") ) {
            edgeType= xmlStreamAttr.value("type").toString();
            qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - Edge type: "
                    << edgeType;
        }
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "width") ) {
            temp = xmlStreamAttr.value("width").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                edgeWeight = temp;
            else
                edgeWeight=1.0;
            qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - Edge width: "
                    << edgeWeight;
        }

    }
    else if ( xml.name() == "Arrows" ) {
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "source") ) {
            tempString = xmlStreamAttr.value("source").toString();
            qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - Edge source arrow type: "
                    << tempString;
        }
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "target") ) {
            tempString = xmlStreamAttr.value("target").toString();
            qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - Edge target arrow type: "
                    << tempString;
        }



    }
    else if (xml.name() == "EdgeLabel" ) {
        key_value=xml.readElementText();  //see if there's simple text after the StartElement
        if (!xml.hasError()) {
            qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - Edge Label "
                    << key_value;
            //probably there's more than simple text after StartElement
            edgeLabel = key_value;
        }
        else {
            qDebug()<< "Parser::readGraphMLElementEdgeGraphics() - "
                       "Can't read Edge Label. More elements nested ? Continuing with blank edge label....";
            edgeLabel = "" ;
        }
    }



}


void Parser::readGraphMLElementUnknown(QXmlStreamReader &xml) {
    qDebug()<< "Parser::readGraphMLElementUnknown()";
    Q_ASSERT(xml.isStartElement());
    qDebug()<< "   "<< xml.name().toString() ;
}


void Parser::createMissingNodeEdges(){
    qDebug()<<"Parser::createMissingNodeEdges() ";
    int count=0;
    if ( (count = edgesMissingNodesHash.size()) > 0 ) {

        bool ok;
        edgeWeight = initEdgeWeight;
        edgeColor = initEdgeColor;
        edgeDirType=EDGE_DIRECTED;
        qDebug()<<"Parser::createMissingNodeEdges() - edges to create " << count;
        QHash<QString, QString>::const_iterator it =
                edgesMissingNodesHash.constBegin();
        while (it != edgesMissingNodesHash.constEnd()) {
            qDebug()<<"Parser::createMissingNodeEdges() - creating missing edge "
                   << it.key() << " data " << it.value() ;
            edgeMissingNodesList = (it.key()).split("===>");
            if (! ((edgeMissingNodesList[0]).isEmpty() )
                    && !((edgeMissingNodesList[1]).isEmpty()) ) {
                source = nodeHash.value(edgeMissingNodesList[0], -666);
                target = nodeHash.value(edgeMissingNodesList[1], -666);
                if (source == -666 || target == -666 ) {
                    //emit something that this node has not been declared
                    continue;
                }
                edgeMissingNodesListData = (it.value()).split("|");
                if (!edgeMissingNodesListData[0].isEmpty() ){
                    edgeWeight = edgeMissingNodesListData[0].toInt(&ok, 10);
                }
                if (!edgeMissingNodesListData[1].isEmpty() ){
                    edgeColor = edgeMissingNodesListData[1];
                }
                if (!edgeMissingNodesListData[2].isEmpty() ){
                    if ( (edgeMissingNodesListData[2]).contains("2") )
                        edgeDirType=EDGE_UNDIRECTED;

                }
                qDebug()<<"Parser::createMissingNodeEdges() - signal edgeCreate "
                       << source << " -> " << target << " edgeDirType value " << edgeDirType;

                emit edgeCreate(source, target, edgeWeight, edgeColor, edgeDirType, arrows, bezier, edgeLabel);

            }
            ++it;
        }
    }
    else {
        qDebug()<<"Parser::createMissingNodeEdges() - nothing to do";
    }
}


/**
    Tries to load a file as GML formatted network. If not it returns -1
*/
bool Parser::loadGML(){
    qDebug()<< "Parser::loadGML()";

    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) {
        return false;
    }
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());

    QRegularExpression onlyDigitsExp("^\\d+$");
    QStringList tempList;
    QString str;
    int fileLine=0;
    bool floatOK= false;
    bool isPlanar = false, graphKey=false, graphicsKey=false,
            edgeKey=false, nodeKey=false, graphicsCenterKey=false;
    Q_UNUSED(isPlanar);

    relationsList.clear();

    node_id= QString::null;
    arrows=true;
    bezier=false;
    edgeDirType=EDGE_UNDIRECTED;
    totalNodes=0;
    while (!ts.atEnd() )   {
        floatOK= false;
        fileContainsNodeCoords = false;
        nodeShape = initNodeShape;
        nodeColor = true;

        fileLine++;
        str= ts.readLine() ;

        str=str.simplified();

        qDebug()<< "Parser::loadGML() - line"  << fileLine <<":"
                << str;

        if ( isComment(str) )
            continue;

        if ( fileLine == 1 &&
             ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL",Qt::CaseInsensitive)
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
             )
               ) {
            qDebug()<< "*** Parser::loadGML(): Not a GML-formatted file. Aborting!!";
            errorMessage = tr("Not an GML-formatted file. "
                              "Non-comment line %1 includes prohibited strings (i.e GraphML)")
                    .arg(fileLine);
            file.close();
            return false;
        }

        if  ( str.startsWith("comment",Qt::CaseInsensitive) ) {
                qDebug()<< "Parser::loadGML() - This is a comment. Continue.";
                continue;
        }
        if  ( str.startsWith("creator",Qt::CaseInsensitive) ) {
                qDebug()<< "Parser::loadGML() - This is a creator description. Continue.";
                continue;
        }
        else if  ( str.startsWith("graph",Qt::CaseInsensitive) ) {
            //describe a graph
            qDebug()<< "Parser::loadGML() - graph description list start";
            graphKey = true;
        }
        else if ( str.startsWith("directed",Qt::CaseInsensitive) ) {
            //graph attribute declarations
            if (graphKey) {
                if ( str.contains("1")) {
                    qDebug()<< "Parser::loadGML() - graph directed 1. A directed graph.";
                    edgeDirType=EDGE_DIRECTED;
                }
                else {
                    qDebug()<< "Parser::loadGML() - graph directed 0. An undirected graph.";
                }
            }
        }
        else if ( str.startsWith("isPlanar",Qt::CaseInsensitive) ) {
            //key declarations
            if (graphKey) {
                if ( str.contains("1")) {
                    qDebug()<< "Parser::loadGML() - graph isPlanar 1. Planar graph.";
                    isPlanar = true;
                }
                else {
                    isPlanar = false;
                }
            }
        }

        else if ( str.startsWith("node",Qt::CaseInsensitive) ) {
            //node declarations
            qDebug()<< "Parser::loadGML() - node description list starts";
            nodeKey = true;
        }
        else if ( str.startsWith("id",Qt::CaseInsensitive) ) {
            //describes identification number for an object
            if ( nodeKey ) {
                totalNodes++;
                node_id = str.split(" ",QString::SkipEmptyParts).last();
                if (!node_id.contains(onlyDigitsExp)) {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Node id tag at line %1 has non-arithmetic value.")
                            .arg(fileLine);
                    return false;
                }
                qDebug()<< "Parser::loadGML() - id description "
                           << "This node" << totalNodes
                              <<"id"<< node_id;
            }
        }

        else if ( str.startsWith("label ",Qt::CaseInsensitive) ) {
            //describes label
            if ( nodeKey ) {
                nodeLabel = str.split(" ",QString::SkipEmptyParts).last().remove("\"");
                qDebug()<< "Parser::loadGML() - node label definition"
                           << "node" << totalNodes
                              <<"id"<< node_id
                                << "label" << nodeLabel;

                //FIXME REMOVE ANY "
            }
            else if ( edgeKey ) {
                edgeLabel = str.split(" ",QString::SkipEmptyParts).last();
                qDebug()<< "Parser::loadGML() - edge label definition"
                           << "edge" << totalLinks
                                << "label" << edgeLabel;
            }
        }


        else if ( str.startsWith("edge ",Qt::CaseInsensitive) ) {
            //edge declarations
            qDebug()<< "Parser::loadGML() - edge description list start";
            edgeKey = true;
            totalLinks++;
        }
        else if ( str.startsWith("source ",Qt::CaseInsensitive) ) {
            if (edgeKey) {
                edge_source = str.split(" ",QString::SkipEmptyParts).last();
                //if edge_source
                if (!edge_source.contains(onlyDigitsExp)) {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge source tag at line %1 has non-arithmetic value.")
                            .arg(fileLine);
                    return false;
                }
                source = edge_source.toInt(0);
                qDebug()<< "Parser::loadGML() - edge source definition"
                           << "edge source" << edge_source;
            }
        }
        else if ( str.startsWith("target ",Qt::CaseInsensitive) ) {
            if (edgeKey) {
                edge_target = str.split(" ",QString::SkipEmptyParts).last();
                if (!edge_source.contains(onlyDigitsExp)) {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge target tag at line %1 has non-arithmetic value.")
                            .arg(fileLine);
                    return false;
                }

                target = edge_target.toInt(0);
                qDebug()<< "Parser::loadGML() - edge target definition"
                           << "edge target" << edge_target;
            }
        }
        else if ( str.startsWith("graphics",Qt::CaseInsensitive) ) {
            //Describes graphics which are used to draw a particular object.
            if (nodeKey)  {
                qDebug()<< "Parser::loadGML() - node graphics description list start";
            }
            else if (edgeKey) {
                qDebug()<< "Parser::loadGML() - edge graphics description list start";
            }
            graphicsKey = true;
        }
        else if ( str.startsWith("center",Qt::CaseInsensitive) ) {
            if (graphicsKey && nodeKey)  {
                qDebug()<< "Parser::loadGML() - node graphics center start";
                if ( str.contains("[", Qt::CaseInsensitive) ) {
                    if ( str.contains("]", Qt::CaseInsensitive) &&
                         str.contains("x", Qt::CaseInsensitive) &&
                         str.contains("y", Qt::CaseInsensitive)) {
                        str.remove("center");
                        str.remove("[");
                        str.remove("]");
                        str = str.simplified();
                        tempList = str.split(" ",QString::SkipEmptyParts);
                        randX = (tempList.at(1)).toFloat(&floatOK);
                        if (!floatOK) {
                            errorMessage = tr("Not a proper GML-formatted file. "
                                              "Node center tag at line %1 cannot be converted to float.")
                                    .arg(fileLine);
                            return false;
                        }
                        randY = tempList.at(3).toFloat(&floatOK);
                        if (!floatOK) {
                            errorMessage = tr("Not a proper GML-formatted file. "
                                              "Node center tag at line %1 cannot be converted to float.")
                                    .arg(fileLine);
                            return false;
                        }
                        qDebug()<< "Parser::loadGML() - node graphics center"
                                << "x" << randX
                                << "y" << randY;
                        fileContainsNodeCoords = true;
                    }
                    else {
                        graphicsCenterKey = true;
                    }
                }
            }
        }
        else if ( str.startsWith("center",Qt::CaseInsensitive) &&
                  nodeKey && graphicsKey && graphicsCenterKey ) {
            //this is the case where the bracker [ is below the center tag
        }
        else if ( str.startsWith("type",Qt::CaseInsensitive) ) {
            if (graphicsKey && nodeKey)  {
                qDebug()<< "Parser::loadGML() - node graphics type start";
                nodeShape = str.split(" ",QString::SkipEmptyParts).last();
                if (nodeShape.isNull() || nodeShape.isEmpty() ) {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Node type tag at line %1 has no value.")
                            .arg(fileLine);
                    return false;
                }
                nodeShape.remove("\"");
            }
        }
        else if ( str.startsWith("fill",Qt::CaseInsensitive) ) {
            if (graphicsKey && nodeKey)  {
                qDebug()<< "Parser::loadGML() - node graphics fill start";
                nodeColor = str.split(" ",QString::SkipEmptyParts).last();
                if (nodeColor.isNull() || nodeColor.isEmpty() ) {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Node fill tag at line %1 has no value.")
                            .arg(fileLine);
                    return false;
                }
            }
        }
        else if ( str.startsWith("]",Qt::CaseInsensitive) ) {
            if (nodeKey && graphicsKey && graphicsCenterKey ) {
                qDebug()<< "Parser::loadGML() - node graphics center ends";
                graphicsCenterKey = false;
            }
            else if (graphicsKey) {
                qDebug()<< "Parser::loadGML() - graphics list ends";
                graphicsKey = false;
            }
            else if (nodeKey && !graphicsKey) {
                qDebug()<< "Parser::loadGML() - node description list ends";
                nodeKey = false;
                if (!fileContainsNodeCoords) {
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                }
                qDebug()<<" *** Creating node "<< node_id
                       << " at "<< randX <<","<< randY
                       <<" label "<<nodeLabel;
                emit createNode(
                            node_id.toInt(0), initNodeSize, nodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            nodeLabel , initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX,randY),
                            nodeShape, false
                            );

            }
            else if (edgeKey && !graphicsKey) {
                qDebug()<< "Parser::loadGML() - edge description list ends";
                edgeKey = false;
                edgeWeight = 1;
                edgeColor = "black";
                if (edgeLabel==QString::null) {
                    edgeLabel = edge_source + "->" + edge_target;
                }
                emit edgeCreate(source,target, edgeWeight, edgeColor,
                                edgeDirType, arrows, bezier, edgeLabel);
            }

            else if (graphKey) {
                qDebug()<< "Parser::loadGML() - graph description list ends";
                graphKey = false;
            }
        }

    }

    if (relationsList.count() == 0 ) {
        emit addRelation( "unnamed" );
    }
    //The network has been loaded. Tell MW the statistics and network type
    emit networkFileLoaded(FILE_GML, fileName, networkName, totalNodes, totalLinks, edgeDirType);
    qDebug() << "Parser-loadGML()";
    return true;
}


/**
    Tries to load the file as Dot (Graphviz) formatted network. If not it returns -1
*/
bool Parser::loadDot(){

    qDebug("\n\nParser: loadDotNetwork");
    int fileLine=0, aNum=-1;
    int start=0, end=0, next=0;
    QString label, node, nodeLabel, fontName, fontColor, edgeShape, edgeColor, edgeLabel, networkLabel;
    QString str, temp, prop, value ;
    QStringList lineElement;
    nodeColor="red";
    edgeColor="black";
    nodeShape="";
    edgeWeight=1.0;
    float nodeValue=1.0;
    bool netProperties = false;
    QStringList labels;
    QList<QString> nodeSequence;   //holds edges
    QList<QString> nodesDiscovered; //holds nodes;

    relationsList.clear();

    edgeDirType=EDGE_DIRECTED;
    arrows=true;
    bezier=false;
    source=0, target=0;

    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());

    totalNodes=0;

    while (!ts.atEnd() )   {
        str= ts.readLine() ;
        str=str.simplified();
        str=str.trimmed();

        if ( isComment (str) )
            continue;

        fileLine++;

        qDebug ()<<"Reading fileLine "<< fileLine;
        qDebug ()<< str;
        if ( fileLine == 1 ) {
            if ( str.contains("vertices",Qt::CaseInsensitive) 	//Pajek
                 || str.contains("network",Qt::CaseInsensitive)	//Pajek?
                 || str.contains("[",Qt::CaseInsensitive)    	// GML
                 || str.contains("DL",Qt::CaseInsensitive) 		//DL format
                 || str.contains("list",Qt::CaseInsensitive)		//list
                 || str.startsWith("<graphml",Qt::CaseInsensitive)  // GraphML
                 || str.startsWith("<?xml",Qt::CaseInsensitive)
                 ) {
                qDebug() << "*** Parser:loadDot(): Not an GraphViz -formatted file. Aborting";
                file.close();
                errorMessage = tr("Not a GraphViz-formatted file. "
                                  "First non-comment line includes prohibited strings (i.e GraphML).");
                return false;
            }

            if ( str.contains("digraph", Qt::CaseInsensitive) ) {
                lineElement=str.split(" ");
                edgeDirType=EDGE_DIRECTED;
                if (lineElement[1]!="{" ) networkName=lineElement[1];
                qDebug() << "This is a DOT DIGRAPH named " << networkName;
                continue;
            }
            else if ( str.contains("graph", Qt::CaseInsensitive) ) {
                lineElement=str.split(" ");
                edgeDirType=EDGE_UNDIRECTED;
                if (lineElement[1] !="{" ) networkName=lineElement[1];
                qDebug() << "This is a DOT GRAPH named " << networkName;
                continue;
            }
            else {
                qDebug()<<" *** Parser:loadDot(): Not a GraphViz file. "
                          "Abort: dot format can only start with \" (di)graph netname {\"";
                errorMessage = tr("Not properly GraphViz-formatted file. "
                                  "First non-comment line should start with \" (di)graph netname {\"");
                return false;
            }
        }

        if (  str.contains("graph [" ,Qt::CaseInsensitive) ) {
                netProperties = true;
                Q_UNUSED(netProperties);
        }

        if (
             str.startsWith("label",Qt::CaseInsensitive)
             || str.startsWith("mincross",Qt::CaseInsensitive)
             || str.startsWith("ratio",Qt::CaseInsensitive)
                || str.startsWith("name",Qt::CaseInsensitive)
                || str.startsWith("type",Qt::CaseInsensitive)
                || str.startsWith("loops",Qt::CaseInsensitive)
             ) { 	 //Default network properties
            next=str.indexOf('=', 1);
            qDebug("Found next = at %i. Start is at %i", next, 1);
            prop=str.mid(0, next).simplified();
            qDebug()<<"Prop: "<<prop.toLatin1() ;
            value=str.right(str.count()-next-1).simplified();
            qDebug() << "Value "<< value;
            if ( prop == "label" || prop == "name"){
                networkLabel= value;
            }
            else if ( prop == "ratio" ){

            }
            else if ( prop == "mincross" ){

            }

        }
        else if ( netProperties && str.contains ("]",Qt::CaseInsensitive) )
        {
            netProperties = false;
        }
        else if ( str.startsWith("node",Qt::CaseInsensitive) )
        { 	 //Default node properties
            netProperties = false;
            qDebug() << "* Node properties found...";
            start=str.indexOf('[');
            end=str.indexOf(']');
            temp=str.right(str.size()-end-1);
            str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
            qDebug()<<"Properties start at "<< start<< " and end at "<< end;
            qDebug()<<str.toLatin1();
            str=str.simplified();
            qDebug()<<str.toLatin1();
            start=0;
            end=str.count();
            readDotProperties(str, nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor );
            qDebug ("* Finished NODE PROPERTIES");
        }
        else if ( str.startsWith("edge",Qt::CaseInsensitive) ) { //Default edge properties
            netProperties = false;
            qDebug("* Edge properties found...");
            start=str.indexOf('[');
            end=str.indexOf(']');
            str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
            qDebug()<<"Properties start at "<< start <<" and end at "<< end;
            qDebug()<<str.toLatin1();
            str=str.simplified();
            qDebug()<<str.toLatin1();
            start=0;
            end=str.count();
            qDebug ("* Finished EDGE PROPERTIES!");
        }
        //ti ginetai an exeis mesa sxolia ? p.x. sto telos tis grammis //
        else if ( !str.startsWith('[', Qt::CaseInsensitive)
                  && !str.contains("--",Qt::CaseInsensitive)
                  && !str.contains("->",Qt::CaseInsensitive)
                  && str.contains ("=",Qt::CaseInsensitive)
                  && !netProperties
                  )
        {
            qDebug()<< "* A node definition must be here: \n" << str;
            end=str.indexOf('[');
            if (end!=-1) {
                temp=str.right(str.size()-end-1); //keep the properties
                temp=temp.remove(']');
                temp=temp.remove(';');
                node=str.left(end-1);
                node=node.remove('\"');
                qDebug()<<"node named "<<node.toLatin1();
                qDebug()<<"node properties "<<temp.toLatin1();
                nodeLabel=node;  //Will change only if label exists in readDotProperties
                readDotProperties(temp, nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor );
                if (nodeLabel=="") nodeLabel=node;
                totalNodes++;
                randX=rand()%gwWidth;
                randY=rand()%gwHeight;
                qDebug()<<" *** Creating node "<< totalNodes
                       << " at "<< randX <<","<< randY
                       <<" label "<<node.toLatin1()
                      << " colored "<< initNodeColor
                      << "initNodeSize " << initNodeSize
                      << "initNodeNumberColor " <<initNodeNumberColor
                      << "initNodeNumberSize " << initNodeNumberSize
                      << "initNodeLabelColor " << initNodeLabelColor
                      << "nodeShape" <<  initNodeShape;
                emit createNode(
                            totalNodes, initNodeSize, initNodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            nodeLabel , initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX,randY),
                            initNodeShape, false
                            );
                // Note that we push the numbered nodelabel whereas we create
                // the node with its file specified node label.
                nodesDiscovered.push_back( node  );
                qDebug()<<" * Total totalNodes " << totalNodes
                       << " nodesDiscovered  "<< nodesDiscovered.size() ;
                target=totalNodes;

            }
            else {
                    qDebug ("* ERROR!");
                    errorMessage = tr("Not properly GraphViz-formatted file. "
                                      "Node definition without opening [");
                    return false;

            }
        }
        else if ( !str.contains('[', Qt::CaseInsensitive)
                  && !str.contains("node", Qt::CaseInsensitive)
                  && !str.contains(']', Qt::CaseInsensitive)
                  && !str.contains("--",Qt::CaseInsensitive)
                  && !str.contains("->",Qt::CaseInsensitive)
                  && !str.contains ("=",Qt::CaseInsensitive)
                  && !netProperties
                  )
        {
            qDebug()<< "* A node definition without properties must be here ..." << str;
            end=str.indexOf(';');
            if (end!=-1) {
                node=str.left(str.size()-end); //keep the properties
                qDebug()<<"node named "<<node.toLatin1();
                node=node.remove(']').remove(';').remove('\"');
                qDebug()<<"node named "<<node.toLatin1();
                nodeLabel=node;
                totalNodes++;
                randX=rand()%gwWidth;
                randY=rand()%gwHeight;
                qDebug()<<" *** Creating node "<< totalNodes
                       << " at "<< randX <<","<< randY
                       <<" label "<<node.toLatin1()
                      << " colored "<< initNodeColor
                      << "initNodeSize " << initNodeSize
                      << "initNodeNumberColor " <<initNodeNumberColor
                      << "initNodeNumberSize " << initNodeNumberSize
                      << "initNodeLabelColor " << initNodeLabelColor
                      << "nodeShape" <<  initNodeShape;
                emit createNode(
                            totalNodes, initNodeSize, initNodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            nodeLabel , initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX,randY),
                            initNodeShape, false
                            );
                nodesDiscovered.push_back( node  );			// Note that we push the numbered nodelabel whereas we create the node with its file specified node label.
                qDebug()<<" * Total nodes" << totalNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
                target=totalNodes;

            }
            else
                end=str.indexOf(';');
            qDebug ("* Finished node!");
        }

        else if (str.contains('-',Qt::CaseInsensitive))
        {
            netProperties = false;
            qDebug("* Edge definition found ...");
            end=str.indexOf('[');
            if (end!=-1) {
                temp=str.right(str.size()-end-1); //keep the properties
                qDebug("* Edge definition found - reading properties...");
                temp=temp.remove(']');
                temp=temp.remove(';');
                qDebug()<<"edge properties "<<temp.toLatin1();
                readDotProperties(temp, edgeWeight, edgeLabel, edgeShape, edgeColor, fontName, fontColor );
            }
            else{
                qDebug("* Edge definition found - no properties...");
                edgeLabel="";
                edgeColor=initEdgeColor;
                edgeWeight=initEdgeWeight;
                end=str.indexOf(';');
            }

            //FIXME Cannot parse nodes named with '-' character
            str=str.mid(0, end).remove('\"');  //keep only edges

            qDebug()<<"edge "<<str.toLatin1();

            if (!str.contains("->",Qt::CaseInsensitive) ){  //non directed = symmetric links
                if ( str.contains("--",Qt::CaseInsensitive) )
                    nodeSequence=str.split("--");
                else
                    nodeSequence=str.split("-");
                edgeDirType=EDGE_UNDIRECTED;
            }
            else { 											//is directed
                nodeSequence=str.split("->");
                edgeDirType=EDGE_DIRECTED;
            }
            //Create all nodes defined in nodeSequence
            for ( QList<QString>::iterator it=nodeSequence.begin(); it!=nodeSequence.end(); it++ )  {
                node=(*it).simplified();
                qDebug () << " nodeSequence node "<< node;
                if ( (aNum=nodesDiscovered.indexOf( node ) ) == -1) {
                    //node not discovered before
                    totalNodes++;
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                    qDebug()<<" *** Creating node "<< totalNodes
                           << " at "<< randX <<","<< randY
                           <<" label "<<node.toLatin1()
                          << " colored "<< nodeColor
                          << "initNodeSize " << initNodeSize
                          << "initNodeNumberColor " <<initNodeNumberColor
                          << "initNodeNumberSize " << initNodeNumberSize
                          << "initNodeLabelColor " << initNodeLabelColor
                          << "nodeShape" <<  initNodeShape;
                    emit createNode(
                                totalNodes, initNodeSize, nodeColor,
                                initNodeNumberColor, initNodeNumberSize,
                                node , initNodeLabelColor, initNodeLabelSize,
                                QPointF(randX,randY),
                                initNodeShape, false
                                );
                    nodesDiscovered.push_back( node  );
                    qDebug()<<" * Total totalNodes "
                           << totalNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
                    target=totalNodes;
                    if (it!=nodeSequence.begin()) {
                        qDebug()<<"-- Drawing Link between node "
                               << source<< " and node " <<target;
                        emit edgeCreate(source,target, edgeWeight, edgeColor,
                                        edgeDirType, arrows, bezier);
                    }
                }
                else {
                    //node discovered before
                    target=aNum+1;
                    qDebug("# Node already exists. Vector num: %i ",target);
                    if (it!=nodeSequence.begin()) {
                        qDebug()<<"-- Drawing Link between node "
                               <<source<<" and node " << target;
                        emit edgeCreate(source,target, edgeWeight , edgeColor,
                                        edgeDirType, arrows, bezier);
                    }
                }
                source=target;
            }
            nodeSequence.clear();
            qDebug("Finished reading fileLine %i ",fileLine);
        }
        else if ( str.contains ("[",Qt::CaseInsensitive)
             && str.contains("=")
                  && !netProperties )
        { 	 //Default node properties - no node keyword

            qDebug("* Node properties found but with no Node keyword in the beginning!");
            start=str.indexOf('[');
            end=str.indexOf(']');
            temp=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
            qDebug()<<"Properties start at "<< start<< " and end at " << end;
            temp=temp.simplified();
            qDebug()<<temp.toLatin1();
            readDotProperties(temp, nodeValue, label, nodeShape, nodeColor, fontName, fontColor );
            qDebug ("Finished the properties!");

            if (start > 2 ) {//there is a node definition here
                node=str.left(start).remove('\"').simplified();
                qDebug()<<"node label: "<<node.toLatin1()<<"." ;
                if (!nodesDiscovered.contains(node)) {
                    qDebug("not discovered node");
                    totalNodes++;
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                    qDebug()<<"***  Creating node at "<<  randX << " "<< randY<< " label "<<node.toLatin1() << " colored "<< nodeColor;
                    emit createNode(
                                totalNodes, initNodeSize, nodeColor,
                                initNodeNumberColor, initNodeNumberSize,
                                label, initNodeLabelColor, initNodeLabelSize,
                                QPointF(randX,randY),
                                nodeShape, false
                                );
                    aNum=totalNodes;
                    nodesDiscovered.push_back( node);
                    qDebug()<<" Total totalNodes: "<<  totalNodes<< " nodesDiscovered = "<< nodesDiscovered.size();
                }
                else {
                    qDebug("discovered node - skipping it!");
                }
            }
        }

        else {
            qDebug() <<  "  Redudant data: "<< str.toLatin1();
        }
    }
    file.close();

    if (relationsList.count() == 0) {
        emit addRelation( (!networkName.isEmpty()) ? networkName :"unnamed");
    }

    //The network has been loaded. Tell MW the statistics and network type
    emit networkFileLoaded(FILE_GRAPHVIZ, fileName, networkName,
                           totalNodes, totalLinks, edgeDirType);
    return true;
}





void Parser::readDotProperties(QString str, float &nValue, QString &label,
                           QString &shape, QString &color, QString &fontName,
                           QString &fontColor ){
    int next=0;
    QString prop, value;

    //FIXME Implement a qstringlist here splitted from str in ,
    bool ok=false;
    do  {		//stops when it passes the index of ']'
        next=str.indexOf('=', 1);
        qDebug("Found next = at %i. Start is at %i", next, 1);
        prop=str.mid(0, next).simplified();
        qDebug()<<"Prop: "<<prop.toLatin1() ;
        str=str.right(str.count()-next-1).simplified();
        if (str.startsWith(","))
            str=str.right(str.count()-1).simplified();

        qDebug()<<"whatsleft: "<<str.toLatin1() ;


        if ( str.indexOf('\"') == 0) {
            qDebug("found text, parsing...");
            next=str.indexOf('\"', 1);
            value=str.left(next).simplified().remove('\"');

            if (prop=="label") {
                label=value.trimmed();
                qDebug()<<"Found label "<<value.toLatin1() <<". Assigned label "<<label.toLatin1();
            }
            else if (prop=="fontname"){
                qDebug()<<"Found fontname"<<value.toLatin1();
                fontName=value.trimmed();
            }
            str=str.right(str.count()-next-1).simplified();
            qDebug()<<"whatsleft: "<<str.toLatin1() <<".";
        }
        else {
            if (str.isEmpty()) break;
            if ( str.contains(',') ){
                next=str.indexOf(',');
                value=str.mid(0, next).simplified();
            }

            else if ( str.contains(' ') ){
                next=str.indexOf(' ');
                value=str.mid(0, next).simplified();
            }
            else		{
                value=str;
            }

            qDebug()<<"Prop Value: "<<value.toLatin1() ;
            if (prop=="value") {
                qDebug()<<"Found value "<<value.toLatin1();
                nValue=value.trimmed().toFloat(&ok);
                if ( ok )
                    qDebug()<<"Assigned value "<<nValue;
                else
                    qDebug()<<"Error in value conversion ";
            }
            else if (prop=="color") {
                color=value.trimmed();
                qDebug()<<"Found color "<<value.toLatin1()  <<". Assigned color "<<color.toLatin1()<<".";
            }
            else if (prop=="fillcolor") {
                qDebug()<<"Found color "<<value.toLatin1();
                color=value.trimmed();
                qDebug()<<"Assigned node color "<<color.toLatin1()<<".";
            }

            else if (prop=="fontcolor") {
                qDebug()<<"Found fontcolor "<<value.toLatin1();
                fontColor=value.trimmed();
            }
            else if (prop=="shape") {
                shape=value.trimmed();
                qDebug()<<"Found node shape "<<shape.toLatin1();
            }
            qDebug()<<"count"<< str.count()<<  " next "<< next;
            str=str.right(str.count()-next-1).simplified();
            qDebug()<<"whatsleft: "<<str.toLatin1()<<".";
            if ( (next=str.indexOf('=', 1))==-1) break;
        }
    } while (!str.isEmpty());

}



/**
 * Debugging only - Used in loadEdgeListWeighed
 */
template<typename T> void print_queue(T& q) {
    qDebug() << "print_queue() ";
    while(!q.empty()) {
        qDebug() << q.top().key << " value: " << q.top().value << " ";
        q.pop();
    }
    qDebug() << endl;
}




/**
 * @brief A method to load a weighted edge list formatted file.
 * @param delimiter
 * @return
 * This method can read and parse edgelist formated files
 * where edge source and target are either named with numbers or with labels
 * That is the following formats can be parsed:
# edgelist with node numbers
1 2 1
1 3 2
1 6 2
1 8 2
...

# edgelist with node labels
actor1 actor2 1
actor2 actor4 2
actor1 actor3 1
actorX actorY 3
name othername 1
othername somename 2
....
 */
bool Parser::loadEdgeListWeighed(const QString &delimiter){
    qDebug() << "Parser::loadEdgeListWeighed() - column delimiter" << delimiter ;

    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly ))
        return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());

    QMap<QString, int> nodeMap;
    // use a minimum priority queue to order Actors<QString key, int value> by their value
    // so that we can create the discovered nodes by either their increasing nodeNumber
    // (if nodesWithLabels == true) or by their actual number in the file (if nodesWithLabels == false).
    priority_queue<Actor, vector<Actor>, CompareActors> nodeQ;
    QHash<QString, float> edgeList;
    QString str, edgeKey,edgeKeyDelimiter="====>" ;
    QStringList lineElement, edgeElement;
    // one or more digits
    QRegularExpression onlyDigitsExp("^\\d+$");

    bool nodesWithLabels = false;
    bool floatOK = false;
    int fileLine = 1;
    totalNodes=0;
    edgeWeight=1.0;
    edgeDirType=EDGE_DIRECTED;
    arrows=true;
    bezier=false;

    relationsList.clear();

    qDebug()<< "*** Parser::loadEdgeListWeighed() - Initial file parsing "
               "to test integrity and edge naming scheme";
    while ( !ts.atEnd() )   {
        str= ts.readLine() ;

        qDebug()<< "Parser::loadEdgeListWeighed() - str" << str;

        str=str.simplified();
        qDebug()<< "Parser::loadEdgeListWeighed() - simplified str" << str;
        if ( isComment(str) )
            continue;

        if ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("graph",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL",Qt::CaseInsensitive)
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
             ) {
            qDebug()<< "Parser::loadEdgeListWeighed() - "
                       "Not a Weighted list-formatted file. Aborting!!";
            file.close();
            errorMessage = tr("Not an EdgeList-formatted file. "
                              "A non-comment line includes prohibited strings (i.e GraphML)");
            return false;
        }

        lineElement=str.split(delimiter);

        if ( lineElement.count()  != 3 ) {
            qDebug()<< "*** Parser::loadEdgeListWeighed() - "
                       "Not a Weighted list-formatted file. Aborting!!";
            file.close();
            errorMessage = tr("Not a properly EdgeList-formatted file. "
                              "Row %1 has not 3 elements as expected (i.e. source, target, weight)")
                    .arg(fileLine);
            return false;
        }

        edge_source = lineElement[0];
        edge_target = lineElement[1];
        edge_weight = lineElement[2];
        qDebug()<< "Parser::loadEdgeListWeighed() - Dissecting line - "
                   "source:"
                << edge_source
                << "target:"
                << edge_target
                << "weight:"
                << edge_weight;

        if (!edge_source.contains(onlyDigitsExp)) {
            qDebug()<< "Parser::loadEdgeListWeighed() - node named by non-digit only string. "
                       "nodesWithLabels = true";
            nodesWithLabels = true;
        }

        if (!edge_target.contains(onlyDigitsExp)) {
            qDebug()<< "Parser::loadEdgeListWeighed() - node named by non-digit only string. "
                       "nodesWithLabels = true";
            nodesWithLabels = true;
        }
        fileLine ++;
    }

    ts.seek(0);

    qDebug()<< "*** Parser::loadEdgeListWeighed() - Initial file parsing finished. "
               "This is really a weighted edge list. Proceed to main parsing";

    while ( !ts.atEnd() )   {
        str= ts.readLine() ;

        qDebug()<< "Parser::loadEdgeListWeighed() - str" << str;

        str=str.simplified();
        qDebug()<< "Parser::loadEdgeListWeighed() - simplified str" << str;

        if ( isComment(str) )
            continue;

        lineElement=str.split(delimiter);

        edge_source = lineElement[0];
        edge_target = lineElement[1];
        edge_weight = lineElement[2];
        qDebug()<< "Parser::loadEdgeListWeighed() - Dissecting line - "
                   "source:"
                << edge_source
                << "target:"
                << edge_target
                << "weight:"
                << edge_weight;

        if ( ! nodeMap.contains(edge_source) ) {
            totalNodes++;
            Actor sourceActor;
            sourceActor.key = edge_source;
            if (nodesWithLabels) {
                sourceActor.value = totalNodes;
                // order by an increasing totalNodes index
                nodeQ.push( sourceActor );
                nodeMap.insert(edge_source, totalNodes);
            }
            else {
                sourceActor.value = edge_source.toInt();
                // order by the actual actor number in the file
                nodeQ.push( sourceActor );
                nodeMap.insert(edge_source, edge_source.toInt() );
            }
            qDebug()<< "Parser::loadEdgeListWeighed() - source, new node named"
                    << edge_source
                    << "totalNodes" << totalNodes
                    << "nodeMap.count"
                    << nodeMap.count();

        }
        else {
            qDebug()<< "Parser::loadEdgeListWeighed() - source already found, continue";
        }
        if ( ! nodeMap.contains(edge_target) ) {
            totalNodes++;
            Actor targetActor;
            targetActor.key = edge_target;
            if (nodesWithLabels) {
                targetActor.value = totalNodes ;
                // order by an increasing totalNodes index
                nodeQ.push( targetActor );
                nodeMap.insert(edge_target, totalNodes);
            }
            else {
                targetActor.value = edge_target.toInt();
                // order by the actual actor number in the file
                nodeQ.push( targetActor );
                nodeMap.insert(edge_target, edge_target.toInt() );
            }
            qDebug()<< "Parser::loadEdgeListWeighed() - target, new node named"
                    << edge_target
                    << "totalNodes" << totalNodes
                    << "nodeMap.count"
                    << nodeMap.count();

        }
        else {
            qDebug()<< "Parser::loadEdgeListWeighed() - target already found, continue";
        }

        edgeWeight = edge_weight.toFloat(&floatOK);
        if (floatOK) {
            qDebug()<< "Parser::loadEdgeListWeighed() - read edge weight"
                    << edgeWeight;
        }
        else {
            edgeWeight = 1.0;
            qDebug()<< "Parser::loadEdgeListWeighed() - error reading edge weight."
                       "Using default edgeWeight"
                    << edgeWeight;
        }
        edgeKey = edge_source + edgeKeyDelimiter + edge_target;
        if ( ! edgeList.contains( edgeKey ) ) {
            qDebug()<< "Parser::loadEdgeListWeighed() - inserting edgeKey"
                    << edgeKey
                    << "in edgeList with weight" << edgeWeight;
            edgeList.insert( edgeKey, edgeWeight );
            totalLinks++;
        }


    } //end ts.stream while here
    file.close();

    qDebug() << "Parser::loadEdgeListWeighed() - finished reading file, "
                "start creating nodes and edges";

    //print_queue(nodeQ);

    // create nodes one by one
    while (!nodeQ.empty()) {

        Actor node = nodeQ.top();
        nodeQ.pop();
        randX=rand()%gwWidth;
        randY=rand()%gwHeight;

        if (nodesWithLabels) {
            qDebug() << "Parser::loadEdgeListWeighed() - creating node named"
                     << node.key << "numbered" << node.value
                     << "at position" << QPointF(randX, randY);
            emit createNode( node.value,
                             initNodeSize,
                             initNodeColor,
                             initNodeNumberColor,
                             initNodeNumberSize,
                             node.key,
                             initNodeLabelColor, initNodeLabelSize,
                             QPointF(randX, randY),
                             initNodeShape,
                             false
                             );
        }
        else {

            qDebug() << "Parser::loadEdgeListWeighed() - creating node named"
                     << node.key << "numbered" << node.key.toInt()
                     << "at position" << QPointF(randX, randY);
            emit createNode( node.key.toInt(),
                             initNodeSize,
                             initNodeColor,
                             initNodeNumberColor,
                             initNodeNumberSize,
                             node.key,
                             initNodeLabelColor, initNodeLabelSize,
                             QPointF(randX, randY),
                             initNodeShape,
                             false
                             );

        }

    }

    //create edges one by one
    QHash<QString, float>::const_iterator edge = edgeList.constBegin();
    while (edge!= edgeList.constEnd()) {

        qDebug() << "Parser::loadEdgeListWeighed() - creating edge named"
                 << edge.key() << " weight " << edge.value();

        edgeElement=edge.key().split(edgeKeyDelimiter);
        if (nodesWithLabels) {
            source = nodeMap.value( edgeElement[0] ) ;
            target = nodeMap.value( edgeElement[1] ) ;
        }
        else {
            source = edgeElement[0].toInt() ;
            target = edgeElement[1].toInt() ;
        }
        edgeWeight = edge.value();
        emit edgeCreate(source,
                        target,
                        edgeWeight,
                        initEdgeColor,
                        edgeDirType,
                        arrows,
                        bezier);

        ++edge;
    }



    if (relationsList.count() == 0) {
        emit addRelation("unnamed");
    }

    //The network has been loaded. Tell MW the statistics and network type
    emit networkFileLoaded(FILE_EDGELIST_WEIGHTED, fileName, networkName,
                           totalNodes, totalLinks, edgeDirType);
    qDebug() << "Parser::loadEdgeListWeighed() - END. Returning.";

    return true;

}




bool Parser::loadEdgeListSimple(const QString &delimiter){
    qDebug() << "Parser::loadEdgeListSimple() - column delimiter" << delimiter ;
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly ))
        return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());  // set the userselectedCodec
    QString str, edgeKey,edgeKeyDelimiter="====>" ;
    QStringList lineElement,edgeElement;
    int columnCount=0;
    int fileLine = 0;
    bool nodesWithLabels= false;
    //@TODO Always use nodesWithLabels= true

    QMap<QString, int> nodeMap;
    // use a minimum priority queue to order Actors<QString key, int value> by their value
    // so that we can create the discovered nodes by either their increasing nodeNumber
    // (if nodesWithLabels == true) or by their actual number in the file (if nodesWithLabels == false).
    priority_queue<Actor, vector<Actor>, CompareActors> nodeQ;
    QHash<QString, float> edgeList;

    QRegularExpression onlyDigitsExp("^\\d+$");

    totalNodes = 0;
    initEdgeWeight=1.0;

    edgeDirType=EDGE_DIRECTED;
    arrows=true;
    bezier=false;

    relationsList.clear();

    qDebug()<< "*** Parser::loadEdgeListSimple() - Initial file parsing "
               "to test integrity and edge naming scheme";

    while ( !ts.atEnd() )   {
        fileLine++;
        str= ts.readLine() ;
        qDebug()<< "Parser::loadEdgeListSimple() - line "  << fileLine
                << endl << str;
        str=str.simplified();

        if ( isComment(str) )
            continue;

        if ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("graph",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL",Qt::CaseInsensitive)
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
             ) {
            qDebug()<< "*** Parser:loadEdgeListSimple(): Not an EdgeList-formatted file. Aborting!!";
            errorMessage = tr("Not an EdgeList-formatted file. "
                              "Non-comment line %1 includes prohibited strings (i.e GraphML)")
                    .arg(fileLine);
            file.close();
            return false;
        }

        lineElement=str.split(delimiter);

        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            edge_source = (*it1);
            if (!edge_source.contains(onlyDigitsExp)) {
                qDebug()<< "Parser::loadEdgeListSimple() - node named by non-digit only string. "
                           "nodesWithLabels = true";
                nodesWithLabels = true;
            }
            if ( edge_source == "0" ) {
                nodesWithLabels = true;
            }
        }

    }


    ts.seek(0);
    fileLine = 0;

    qDebug () << "Parser::loadEdgeListSimple() - Reset and read lines. nodesWithLabels"
                 << nodesWithLabels;

    while ( !ts.atEnd() )   {
        fileLine++;
        str= ts.readLine() ;

        str=str.simplified();

        qDebug()<< "Parser::loadEdgeListSimple() - line" << fileLine
                << endl << str;

        if ( isComment(str) )
            continue;

        lineElement=str.split(delimiter);

        columnCount = 0;
        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            columnCount ++;
            if (columnCount == 1) {  // source node
                edge_source = (*it1);
                qDebug()<< "Parser::loadEdgeListSimple() - Dissecting line - "
                           "source node:"
                        << edge_source;

                if ( ! nodeMap.contains(edge_source) ) {


                    totalNodes++;
                    Actor sourceActor;
                    sourceActor.key = edge_source;
                    if (nodesWithLabels) {
                        sourceActor.value = totalNodes;
                        // order by an increasing totalNodes index
                        nodeQ.push( sourceActor );
                        nodeMap.insert(edge_source, totalNodes);
                    }
                    else {
                        sourceActor.value = edge_source.toInt();
                        // order by the actual actor number in the file
                        nodeQ.push( sourceActor );
                        nodeMap.insert(edge_source, edge_source.toInt() );
                    }
                    qDebug()<< "Parser::loadEdgeListSimple() - source, new node named"
                            << edge_source
                            << "totalNodes" << totalNodes
                            << "nodeMap.count"
                            << nodeMap.count();

                }
                else {
                    qDebug()<< "Parser::loadEdgeListWeighed() - source already found, continue";
                }
            }
            else { // target nodes
                edge_target= (*it1);
                qDebug()<< "Parser::loadEdgeListSimple() - Dissecting line - "
                           "target node:"
                        << edge_target;

                if ( ! nodeMap.contains(edge_target) ) {

                    totalNodes++;
                    Actor targetActor;
                    targetActor.key = edge_target;
                    if (nodesWithLabels) {
                        targetActor.value = totalNodes ;
                        // order by an increasing totalNodes index
                        nodeQ.push( targetActor );
                        nodeMap.insert(edge_target, totalNodes);
                    }
                    else {
                        targetActor.value = edge_target.toInt();
                        // order by the actual actor number in the file
                        nodeQ.push( targetActor );
                        nodeMap.insert(edge_target, edge_target.toInt() );
                    }
                    qDebug()<< "Parser::loadEdgeListSimple() - target, new node named"
                            << edge_target
                            << "totalNodes" << totalNodes
                            << "nodeMap.count"
                            << nodeMap.count();

                }
                else {
                    qDebug()<< "Parser::loadEdgeListSimple() - target already found, continue";
                }

            }

            if (columnCount > 1)  {
                edgeKey = edge_source + edgeKeyDelimiter + edge_target;
                if ( ! edgeList.contains( edgeKey ) ) {
                    qDebug()<< "Parser::loadEdgeListSimple() - inserting edgeKey"
                            << edgeKey
                            << "in edgeList with initial weight" << initEdgeWeight;
                    edgeList.insert( edgeKey, initEdgeWeight );
                    totalLinks++;
                }
                else { // if edge already discovered, then increase its weight by 1
                    edgeWeight = edgeList.value(edgeKey);
                    edgeWeight = edgeWeight + 1;
                    qDebug()<< "Parser::loadEdgeListSimple() - edgeKey"
                            << edgeKey
                            << "found before, adding in edgeList with increased weight"
                            << edgeWeight;

                    edgeList.insert( edgeKey, edgeWeight );
                }
            }

        }  // end for QStringList::Iterator

    } //end ts.stream while here
    file.close();


    // create nodes one by one
    while (!nodeQ.empty()) {

        Actor node = nodeQ.top();
         nodeQ.pop();
         randX=rand()%gwWidth;
         randY=rand()%gwHeight;

         if (nodesWithLabels) {
             qDebug() << "Parser::loadEdgeListSimple() - creating node named"
                      << node.key << "numbered" << node.value
                      << "at position" << QPointF(randX, randY);
             emit createNode( node.value,
                              initNodeSize,
                              initNodeColor,
                              initNodeNumberColor,
                              initNodeNumberSize,
                              node.key,
                              initNodeLabelColor, initNodeLabelSize,
                              QPointF(randX, randY),
                              initNodeShape,
                              false
                              );
         }
         else {

             qDebug() << "Parser::loadEdgeListSimple() - creating node named"
                      << node.key << "numbered" << node.key.toInt()
                      << "at position" << QPointF(randX, randY);
             emit createNode( node.key.toInt(),
                              initNodeSize,
                              initNodeColor,
                              initNodeNumberColor,
                              initNodeNumberSize,
                              node.key,
                              initNodeLabelColor, initNodeLabelSize,
                              QPointF(randX, randY),
                              initNodeShape,
                              false
                              );

         }

     }


    //create edges one by one
    QHash<QString, float>::const_iterator edge = edgeList.constBegin();
     while (edge!= edgeList.constEnd()) {

         qDebug() << "Parser::loadEdgeListWeighed() - creating edge named"
                  << edge.key() << " weight " << edge.value();

         edgeElement=edge.key().split(edgeKeyDelimiter);
         if (nodesWithLabels) {
             source = nodeMap.value( edgeElement[0] ) ;
             target = nodeMap.value( edgeElement[1] ) ;
         }
         else {
             source = edgeElement[0].toInt() ;
             target = edgeElement[1].toInt() ;
         }
         edgeWeight = edge.value();
         emit edgeCreate(source,
                         target,
                         edgeWeight,
                         initEdgeColor,
                         edgeDirType,
                         arrows,
                         bezier);

         ++edge;
     }


     if (relationsList.count() == 0) {
         emit addRelation("unnamed");
     }

    //The network has been loaded. Tell MW the statistics and network type
    emit networkFileLoaded(FILE_EDGELIST_SIMPLE, fileName, networkName, totalNodes, totalLinks, edgeDirType);
    qDebug() << "Parser-loadEdgeListSimple() ending and returning...";
    return true;

}





//Returns true if QString str is a comment inside the network file.
bool Parser::isComment(QString str){
    if ( str.startsWith("#", Qt::CaseInsensitive)
         || str.startsWith("/*", Qt::CaseInsensitive)
         || str.startsWith("%", Qt::CaseInsensitive)
         || str.startsWith("/*", Qt::CaseInsensitive)
         || str.startsWith("//", Qt::CaseInsensitive)
         || str.isEmpty()
         ) {
        qDebug () << "Parser::isComment() - Comment or an empty line was found. "
                     "Skipping...";
        return true;
    }
    return false;

}
