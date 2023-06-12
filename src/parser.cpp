/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.1
 Written in Qt
 
                         parser.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2023 by Dimitris B. Kalamaras
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

#include "parser.h"

#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QTextStream>
#include <QString>
#include <QtDebug>		//used for qDebug messages
#include <QPointF>
#include <QTextCodec>
#include <QRegularExpression>

#include <list>  // used as list<int> listDummiesPajek
#include <queue>		//for priority queue

#include "graph.h"	//needed for setParent

using namespace std;


Parser::Parser()
{
    qDebug() << "Parser constructor, on thread:"  << this->thread() ;
}



Parser::~Parser () {
    qDebug()<< "**** Parser destructor on thread:" << this->thread()
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
        qDebug()<< "**** clearing xml reader object " ;
        xml->clear();
        delete xml;
        xml=0;
    }


}



/**
 * @brief Loads the network calling one of the load* methods
 *
 * @param fileName
 * @param codecName
 * @param defNodeSize
 * @param defNodeColor
 * @param defNodeShape
 * @param defNodeNumberColor
 * @param defNodeNumberSize
 * @param defNodeLabelColor
 * @param defNodeLabelSize
 * @param defEdgeColor
 * @param width
 * @param height
 * @param format
 * @param sm_mode
 * @param delim
 */
void Parser::load(const QString &fileName,
                  const QString &codecName,
                  const int &defNodeSize,
                  const QString &defNodeColor,
                  const QString &defNodeShape,
                  const QString &defNodeNumberColor,
                  const int &defNodeNumberSize,
                  const QString &defNodeLabelColor,
                  const int &defNodeLabelSize ,
                  const QString &defEdgeColor,
                  const int &canvasWidth,
                  const int &canvasHeight,
                  const int &format,
                  const int &sm_mode,
                  const QString &delim)  {


    qDebug()<< "Parser loading file:" << fileName
            << "codecName" << codecName
            << "- Running On thread " << this->thread();

    initNodeSize=defNodeSize;
    initNodeColor=defNodeColor;
    initNodeShape=defNodeShape;
    initNodeNumberColor=defNodeNumberColor;
    initNodeNumberSize=defNodeNumberSize;
    initNodeLabelColor=defNodeLabelColor;
    initNodeLabelSize=defNodeLabelSize;

    initEdgeColor=defEdgeColor;

    edgeDirType=EdgeType::Directed;
    arrows=true;
    bezier=false;
    m_textCodecName = codecName;
    networkName=(fileName.split ("/")).last();
    gwWidth=canvasWidth;
    gwHeight=canvasHeight;
    randX=0;
    randY=0;
    fileFormat=format;
    two_sm_mode=sm_mode;
    fileLoaded=false;

    if (!delim.isNull() && !delim.isEmpty()) {
        delimiter = delim;
    }
    else {
        delimiter=" ";
    }

    xml=0;

    qDebug()<< "Initial networkName:"<< networkName
            << "requested fileFormat: "<< fileFormat
              << "delim:" << delim << "delimiter" << delimiter;

    errorMessage=QString();

    // Start a timer.
    QElapsedTimer computationTimer;
    computationTimer.start();

    // Try to open the file
    qDebug()<< "Opening file...";
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) {
        qint64 elapsedTime = computationTimer.elapsed();
        qDebug()<< "Cannot open file" << fileName;
        errorMessage = tr("Cannot open file: %1").arg(fileName);
        emit signalFileLoaded(FileType::UNRECOGNIZED,
                               QString(),
                               QString(),
                               0,
                               0,
                               false,
                               elapsedTime,
                               errorMessage
                               );
        return;
    }


    // Get the canonical path of the file to load (only the path)
    fileDirPath= QFileInfo(fileName).canonicalPath();

    // Read the file into a byte array
    qDebug() << "Reading the whole file into a byte array...";
    QByteArray rawData = file.readAll();

    // Close the file
    file.close();

    switch (fileFormat){
    case FileType::GRAPHML:
        if (parseAsGraphML(rawData)){
            fileLoaded = true;
        }
        break;
    case FileType::PAJEK:
        if (parseAsPajek(rawData) ) {
            fileLoaded = true;
        }
        break;
    case FileType::ADJACENCY:
        if (parseAsAdjacency(rawData) ) {
            fileLoaded = true;
        }
        break;
    case FileType::GRAPHVIZ:
        if (parseAsDot(rawData) ) {
           fileLoaded = true;
        }
        break;
    case FileType::UCINET:
        if (parseAsDL(rawData) ){
            fileLoaded = true;
        }
        break;
    case FileType::GML:
        if (parseAsGML(rawData) ){
            fileLoaded = true;
        }
        break;
    case FileType::EDGELIST_WEIGHTED:
        if (parseAsEdgeListWeighted(rawData, delimiter) ){
            fileLoaded = true;
        }
        break;
    case FileType::EDGELIST_SIMPLE:
        if (parseAsEdgeListSimple(rawData, delimiter) ){
            fileLoaded = true;
        }
        break;
    case FileType::TWOMODE:
        if (parseAsTwoModeSociomatrix(rawData) ){
            fileLoaded = true;
        }
        break;
    default:	//GraphML
        if (parseAsGraphML(rawData)){
            fileLoaded = true;
        }
        break;
    }

    // Store computation time
    qint64 elapsedTime = computationTimer.elapsed();

    if (fileLoaded){
        emit signalFileLoaded(fileFormat,
                               fileName,
                               networkName,
                               totalNodes,
                               totalLinks,
                               edgeDirType,
                               elapsedTime);
    }
    else if (errorMessage!=QString()) {
        emit signalFileLoaded(FileType::UNRECOGNIZED,
                               QString(),
                               QString(),
                               0,
                               0,
                               false,
                               elapsedTime,
                               errorMessage
                               );
        return;
    }


    qDebug()<< "**** Parser finished. Emitting finished() signal. ";

    emit finished ("Parser::load() - reach end");

}


/**
 * @brief Signals to create either a single new node (numbered fixedNum) or multiple new nodes (numbered from 1 to to newNodes)
 * @param fixedNum
 * @param label
 * @param newNodes
 */
void Parser::createRandomNodes(const int &fixedNum,
                               const QString &label,
                               const int &newNodes){
    if (newNodes != 1 ) {
        for (int i=0; i<newNodes; i++) {
            qDebug() << "Signaling to create multiple nodes. Now signaling for node:" << i+1;
            emit signalCreateNodeAtPosRandom(false);
        }
    }
    else {
        qDebug() << "Signaling to create a single node:"<< fixedNum << "with label:" << label;
        emit signalCreateNodeAtPosRandomWithLabel( fixedNum, label, false );

    }
}



/**
 * @brief Parses the data as DL-formatted (UCINET)
 *
 * @param rawData
 * @return bool
 */
bool Parser::parseAsDL(const QByteArray &rawData){

    qDebug() << "Parsing data as DL formatted (UCINET)...";

    QTextCodec *codec = QTextCodec::codecForName( m_textCodecName.toLatin1() );
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QString str=QString();
    QString relation=QString();
    QString prevLineStr=QString();
    QString label=QString();
    QString value=QString();
    QString dlFormat=QString();
    QString edgeStr;

    unsigned long int fileLineNumber=0;
    unsigned long int actualLineNumber=0;
    int source=1;
    int target=1;
    int NM=0;
    int NR=0;
    int NC=0;
    int nodeSum=0;
    int relationCounter=0;

    bool rowLabels_flag=false;
    bool colLabels_flag=false;
    bool data_flag=false;
    bool relation_flag=false;
    bool nodesCreated_flag=false;
    bool twoMode_flag=false;

    bool fullmatrixFormat=false;
    bool edgelist1Format=false;

    bool intOK=false;
    bool conversionOK=false;

    QStringList lineElement;
    QStringList tempList;
    QStringList rowLabels;
    QStringList colLabels;

    relationsList.clear();

    totalLinks=0;
    arrows=true;
    bezier=false;
    edgeWeight=0;
    edgeDirType=EdgeType::Directed;

    while ( !ts.atEnd() )   {

        fileLineNumber++;

        str= ts.readLine();
        str=str.simplified();

        if ( isComment(str) )
            continue;

        actualLineNumber++;

        qDebug() << "actualLineNumber " << actualLineNumber
                 << "str.simplified: \n" << str;

        if ( actualLineNumber == 1) {
            if (!str.startsWith("DL",Qt::CaseInsensitive)  )  {
                qDebug() << "Not a DL file. Aborting!";
                errorMessage = tr("Invalid UCINET-formatted file. The file does not start with DL in first non-comment line %1").arg(fileLineNumber);
                return false;
            }
        } // end if actualLineNumber == 1

        //
        // This is a DL file.
        // Check if the line contains DL and comma
        // or we are still in search for N,NM, and FORMAT keywords
        //

        if (  str.startsWith("DL",Qt::CaseInsensitive) ) {

            if ( str.contains(",") ) {
                qDebug() << "DL starting line contains a comma" ;
                // If it is a DL file and contains a comma in the first line,
                // then the line might declare some keywords (N, NM, FORMAT)
                // this happens in R's sna output files
                lineElement = str.split(",", Qt::SkipEmptyParts);
                readDLKeywords(lineElement, totalNodes, NM, NR, NC, fullmatrixFormat, edgelist1Format);

            } //  end if str.contains(",")

            // if the line contains DL, does not contain any comma
            // but contains at least one "=" then we have keywords space separated.
            else if (str.contains("=")){
                qDebug() << "DL starting line contains a = but not a comma" ;
                // this is space separated
                lineElement = str.split(" ", Qt::SkipEmptyParts);
                readDLKeywords(lineElement, totalNodes, NM, NR, NC, fullmatrixFormat, edgelist1Format);

            } // end else if contains =

        } // end if startsWith("DL")


        //
        // Check if keywords are given in other lines, which do not start with DL
        //
        if ( ! str.contains("DL",Qt::CaseInsensitive) &&
             (  str.contains("n =",Qt::CaseInsensitive) ||
                str.contains("n=",Qt::CaseInsensitive)  ||
                str.contains("nm=",Qt::CaseInsensitive)  ||
                str.contains("nm =",Qt::CaseInsensitive)  ||
                str.contains("nr=",Qt::CaseInsensitive)  ||
                str.contains("nr =",Qt::CaseInsensitive)  ||
                str.contains("nc=",Qt::CaseInsensitive)  ||
                str.contains("nc =",Qt::CaseInsensitive)  ||
                str.contains("format =",Qt::CaseInsensitive)  ||
                str.contains("format=",Qt::CaseInsensitive) ) )
        {

            // check if this line contains precisely one "="
            if ( str.count("=",Qt::CaseInsensitive) == 1 ) {
                 qDebug() << "Line contains just one = " ;
                // then one of the above keywords is declared here
                tempList = str.split("=", Qt::SkipEmptyParts);

                label = tempList[0].simplified();
                value= tempList[1].simplified();

                if (  label == "n" || label  == "N" ) {
                    qDebug() << "N is declared to be : "
                             << value ;
                    totalNodes=value.toInt(&intOK,10);
                    if (!intOK) {
                        qDebug() << "N conversion error..." ;
                        //emit something here...
                        errorMessage = tr("Problem interpreting UCINET-formatted file. Cannot convert N value to integer at line %1.").arg(fileLineNumber);
                        return false;
                    }
                }
                else if (  label == "nm" || label  == "NM" ) {
                    qDebug() << "NM is declared to be : "
                             << value ;
                    NM = value.toInt(&intOK,10);
                    if (!intOK) {
                        qDebug() << "NM conversion error..." ;
                        //emit something here...
                        errorMessage = tr("Problem interpreting UCINET-formatted file. Cannot convert NM value to integer at line %1").arg(fileLineNumber);
                        return false;
                    }
                }
                else if (  label == "nr" || label  == "NR" ) {
                    qDebug() << "NR is declared to be : "
                             << value ;
                    NR = value.toInt(&intOK,10);
                    if (!intOK) {
                        qDebug() << "NR conversion error..." ;
                        //emit something here...
                        errorMessage = tr("Problem interpreting UCINET-formatted file. Cannot convert NR value to integer at line %1").arg(fileLineNumber);
                        return false;
                    }
                }
                else if (  label == "nc" || label  == "NC" ) {
                    qDebug() << "NC is declared to be : "
                             << value ;
                    NC = value.toInt(&intOK,10);
                    if (!intOK) {
                        qDebug() << "NC conversion error..." ;
                        //emit something here...
                        errorMessage = tr("Problem interpreting UCINET-formatted file. Cannot convert NC value to integer at line %1").arg(fileLineNumber);
                        return false;
                    }
                }
                else if (  label == "format" || label  == "FORMAT" ) {
                    qDebug() << "FORMAT is declared to be : "
                             << value ;
                    if (value.contains("FULLMATRIX",Qt::CaseInsensitive)) {
                        fullmatrixFormat=true;
                        qDebug() << "FORMAT fullmatrix detected" ;
                    }
                    else if (value.contains("edgelist",Qt::CaseInsensitive) ){
                        edgelist1Format=true;
                        qDebug() << "FORMAT edgelist detected" ;
                    }
                }
            } // end if count 1 "=" in line (network properties)

            // check if this line contains more than one "="
            else if  ( str.count("=",Qt::CaseInsensitive) > 1 ) {
                qDebug() << "Line contains multiple = " ;
                 if (str.contains(",")) {
                    // this is comma separated
                    lineElement = str.split(",", Qt::SkipEmptyParts);
                   readDLKeywords(lineElement, totalNodes, NM, NR, NC, fullmatrixFormat, edgelist1Format);
                 } // end else if contains comma

                 // check if line contains space i.e. "NR=18 NC=14"
                 else if (str.contains(" ")) {
                     // this is space separated
                     lineElement = str.split(" ", Qt::SkipEmptyParts);
                     readDLKeywords(lineElement, totalNodes, NM, NR, NC, fullmatrixFormat, edgelist1Format);
                  } // end else if contains space

            } // end if str.count("=") > 1 in line (network properties)

        } // end if str contains keywords




        else if (str.startsWith( "labels", Qt::CaseInsensitive)
                 || str.startsWith( "row labels", Qt::CaseInsensitive)) {
            rowLabels_flag=true; colLabels_flag=false; data_flag=false;relation_flag=false;
            qDebug() << "START LABELS RECOGNITION "
                         "AND NODE CREATION";
            continue;
        }
        else if (str.startsWith( "COLUMN LABELS", Qt::CaseInsensitive)) {
            colLabels_flag=true; rowLabels_flag=false; data_flag=false;relation_flag=false;
            qDebug() << "START COLUMN LABELS RECOGNITION "
                        "AND NODE CREATION";
            continue;
        }
        else if ( str.startsWith( "data:", Qt::CaseInsensitive)
                  || str.startsWith( "data :", Qt::CaseInsensitive) ) {
            data_flag=true; rowLabels_flag=false;colLabels_flag=false; relation_flag=false;
            qDebug() << "START DATA RECOGNITION "
                        "AND EDGE CREATION";
            continue;
        }
        else if (str.startsWith( "LEVEL LABELS", Qt::CaseInsensitive) ) {
            relation_flag=true; data_flag=false; rowLabels_flag=false; colLabels_flag=false;
            qDebug() << "START RELATIONS RECOGNITION";
            continue;
        }
        else if ( str.startsWith( "matrix labels:", Qt::CaseInsensitive)
                  || str.startsWith( "matrix labels :", Qt::CaseInsensitive) ) {
            data_flag=false; rowLabels_flag=false;colLabels_flag=false; relation_flag=false;
            qDebug() << "matrix labels not supported";
            continue;
        }

        else if (str.isEmpty()){
            qDebug() << "EMPTY STRING - CONTINUE";
            continue;
        }


        if (rowLabels_flag) {
            // try to read row labels

            label=str;

            if ( rowLabels.contains(label) ) {
                qDebug() << "label exists. CONTINUE";
                continue;
            }
            else{
                qDebug() << "Adding label " << label
                         << " to rowLabels";
                rowLabels << label;
            }
        }

        else if (colLabels_flag) {
            // try to read col labels

            label=str;

            if ( colLabels.contains(label) ) {
                qDebug() << "col label exists. CONTINUE";
                continue;
            }
            else{
                qDebug() << "Adding col label " << label
                         << " to colLabels";
                colLabels << label;
            }

        }
        else if ( relation_flag ){
            relation=str;
            if ( relationsList.contains(relation) ) {
                qDebug() << "relation exists. CONTINUE";
                continue;
            }
            else{
                qDebug() << "adding new relation"<< relation
                         << "to relationsList and signaling to create new relation";
                relationsList << relation;
                emit signalAddNewRelation( relation );
            }
        }

        else if ( data_flag ) {

            // check if we haven't created any nodes...
            if (!nodesCreated_flag) {

                // check if there were NR and NC declared (then this is two-mode)
                qDebug() << "check if NR != 0 (two mode net).";
                if (NR != 0 && NC != 0) {
                    twoMode_flag=true;
                    qDebug() << "this is a two-mode net.";
                    //emit something
//                    errorMessage = tr("UCINET declared NR=") + QString::number(NR)
//                            + tr(" and NC=") + QString::number(NC)
//                            + tr(" aka a two-mode net which is not yet supported.");
//                    return false;
                }

                // check if we have found row labels
                if ( rowLabels.size() == 0 ) {
                    // no labels found
                    qDebug() << "Nodes have not been created yet."
                             << "No node labels found."
                             << "Calling createRandomNodes(N) for all" ;
                    createRandomNodes(1, QString(), totalNodes);
                    nodeSum = totalNodes;

                }
                else if ( rowLabels.size() == 1 ) {
                    // only one label line was found
                    // probably contains a comma to separate labels
                    // split it
                    qDebug() << "Nodes have not been created yet."
                             << "One row for labels found."
                             << "Splitting at a comma and calling createRandomNodes(1) for each label" ;
                    tempList = rowLabels[0].split(",", Qt::SkipEmptyParts);
                    for (QStringList::Iterator it1 = tempList.begin(); it1!=tempList.end(); ++it1)   {
                        label = (*it1);
                        nodeSum++;
                        createRandomNodes(nodeSum, label,1);

                    }
                }
                else {
                    // multiple label lines were found

                    qDebug() << "Nodes have not been created yet."
                             << "Multiple label lines were found."
                             << "Calling createRandomNodes(1) for each label" ;
                    for (QStringList::Iterator it1 = rowLabels.begin(); it1!=rowLabels.end(); ++it1)   {
                        label = (*it1);
                        nodeSum++;
                        createRandomNodes(nodeSum, label,1);
                    }
                }

                if (twoMode_flag) {

                    // check if we have found col labels
                    if ( colLabels.size() == 0 ) {
                        // no  col labels found
                        qDebug() << "Nodes have not been created yet."
                                 << "No node labels found."
                                 << "Calling createRandomNodes(NC) for all columns" ;
                        createRandomNodes(totalNodes, QString(), NC);

                    }
                    else if ( colLabels.size() == 1 ) {
                        // only one col label line was found
                        // probably contains a comma to separate labels
                        // split it
                        qDebug() << "Nodes have not been created yet."
                                 << "One line for col label found."
                                 << "Splitting at a comma and calling createRandomNodes(1) for each label" ;
                        tempList = colLabels[0].split(",", Qt::SkipEmptyParts);
                        for (QStringList::Iterator it1 = tempList.begin(); it1!=tempList.end(); ++it1)   {
                            label = (*it1);
                            nodeSum++;
                            createRandomNodes(nodeSum, label,1);

                        }
                    }
                    else {
                        // multiple  col label lines were found
                        qDebug() << "Nodes have not been created yet."
                                 << "Multiple col label lines were found."
                                 << "Calling createRandomNodes(1) for each label" ;
                        for (QStringList::Iterator it1 = colLabels.begin(); it1!=colLabels.end(); ++it1)   {
                            label = (*it1);
                            nodeSum++;
                            createRandomNodes(nodeSum, label,1);
                        }
                    }

                }
                nodesCreated_flag = true;

            } // endif nodesCreated

            if ( fullmatrixFormat ) {

                if (!twoMode_flag) {

                    qDebug() << "reading edges in fullmatrix format";

                    //SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS)
                    if (!prevLineStr.isEmpty()) {
                        str=(prevLineStr.append(" ")).append(str) ;
                        qDebug() << "prevLineStr not empty - "
                                    "prepending it to str - new str: \n" << str;
                        str=str.simplified();
                    }
                    qDebug() << "splitting str to elements ";
                    lineElement=str.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    qDebug() << "line elements " << lineElement.size();
                    if (lineElement.size() < totalNodes ) {
                        qDebug() << "This line has "
                                 << lineElement.size()
                                 << " elements, expected "
                                 << totalNodes << " - appending next line";
                        prevLineStr=str;
                        continue;
                    }
                    prevLineStr.clear();
                    target=1;
                    if (source==1 && relationCounter>0){
                        qDebug() << "we are at source 1. "
                                    "Checking relationList";
                        relation = relationsList[ relationCounter ];
                        qDebug() << "WE ARE THE FIRST DATASET/MATRIX"
                                 << "source node counter is" << source
                                 << "and relation to:" << relation<< "index:"
                                 << relationCounter << "signaling to change to that relation...";
                        emit signalSetRelation (relationCounter);
                    }
                    else if (source>totalNodes) {
                        source=1;
                        relationCounter++;
                        relation = relationsList[ relationCounter ];
                        qDebug() << "LOOKS LIKE WE ENTERED A NEW DATASET/MATRIX "
                                 << " init source node counter to" << source
                                 << " and relation to" << relation << ": "
                                 << relationCounter << "signaling to change to that relation...";
                        emit signalSetRelation (relationCounter);
                    }
                    else {
                        qDebug() << "source node counter is " << source;
                    }

                    for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {

                        edgeStr = (*it1);
                        edgeWeight=(*it1).toDouble(&conversionOK);
                        if ( !conversionOK )  {
                            errorMessage = tr("Problem interpreting UCINET fullmatrix-formatted file. "
                                              "In edge (%1->%2), the weight (%3) could not be converted to number, at line %4.")
                                               .arg(source)
                                               .arg(target)
                                               .arg(edgeWeight)
                                               .arg(fileLineNumber);
                            return false;
                        }

                        if ( edgeWeight ){

                            qDebug() << "relation"
                                     << relationCounter
                                     << "found edge from "
                                     << source << " to " << target
                                     << "weight " << edgeWeight
                                     << "signaling to create new edge..." ;

                            emit signalCreateEdge( source, target, edgeWeight, initEdgeColor,
                                             EdgeType::Directed, arrows, bezier);
                            totalLinks++;
                            qDebug() << "TotalLinks= " << totalLinks;

                        }
                        target++;
                    } // end for

                    source++;

                }
                else {
                    // two-mode
                    target=NR+1;
                    qDebug() << "this is a two-mode fullmatrix file. "
                                "Splitting str to elements:";
                    lineElement=str.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    qDebug()<< "lineElement:" << lineElement;
                    if (lineElement.size() != NC) {
                        qDebug() << "Not a two-mode fullmatrix UCINET "
                                    "formatted file. Aborting!!";
                        //emit something...
                        errorMessage = tr("Problem interpreting UCINET two-mode fullmatrix-formatted file. The file declared %1 columns initially, "
                                          "but I found a different number %2 of matrix columns, at line %3.")
                                          .arg(QString::number(NC))
                                          .arg(QString::number(lineElement.size()))
                                          .arg(fileLineNumber);
                        return false;

                    }
                    for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {

                        edgeStr = (*it1);
                        edgeWeight=(*it1).toDouble(&conversionOK);
                        if ( !conversionOK )  {
                            errorMessage = tr("Problem interpreting UCINET two-mode file. "
                                              "In edge (%1->%2), the weight (%3) cannot be converted to number, at line %4.")
                                               .arg(source)
                                               .arg(target)
                                               .arg(edgeWeight)
                                               .arg(fileLineNumber);
                            return false;
                        }

                        if ( edgeWeight ){

                            qDebug() << "relation "
                                     << relationCounter
                                     << "found edge from "
                                     << source << " to " << target
                                     << "weight " << edgeWeight
                                     << "signaling to create new edge" ;

                            emit signalCreateEdge( source, target, edgeWeight, initEdgeColor, EdgeType::Directed, arrows, bezier);

                            totalLinks++;
                            qDebug() << "TotalLinks= " << totalLinks;

                        }
                        target++;
                    } // end for

                    source++;

                }


            } // END FULLMATRIX FORMAT READING

            if (edgelist1Format) {
                // read edges in edgelist1 format

                lineElement=str.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                qDebug() << "edgelist str line:"<< str;
                qDebug() << "edgelist data element:"<< lineElement;
                if ( lineElement.size() != 3 ) {
                    qDebug() << "Not an edgelist1 UCINET "
                                "formatted file. Aborting!!";
                    //emit something...
                    errorMessage = tr("Problem interpreting UCINET-formatted file. "
                                      "The file was declared as edgelist but I found "
                                      "a line which did not have 3 elements (source, target, weight), at line %1")
                                       .arg(fileLineNumber);
                    return false;
                }

                source =  (lineElement[0]).toInt(&intOK);
                target =  (lineElement[1]).toInt(&intOK);

                qDebug() << "source node "
                         << source  << " target node " << target;

                edgeWeight=(lineElement[2]).toDouble(&conversionOK);

                if (conversionOK) {
                    qDebug() << "list file declares edge weight: "
                             << edgeWeight;
                }
                else {
                    edgeWeight=1.0;
                    qDebug () << "	list file NOT declaring edge weight. Setting default: " << edgeWeight;
                }

                qDebug() << "Signaling to create new edge"
                         << source << "->"<< target << " weight= "<< edgeWeight
                         <<  " TotalLinks=  " << totalLinks+1;
                emit signalCreateEdge(source, target, edgeWeight, initEdgeColor, EdgeType::Directed,
                                arrows, bezier);
                totalLinks++;
            } // END edgelist1 format reading.

        } // end if data_flag

    } // end while there are more lines

    //sanity check
    if (!twoMode_flag && nodeSum != totalNodes) {
        qDebug()<< "Error: aborting";
        //emit something
        errorMessage = tr("Problem interpreting UCINET-formatted file. The file declared %1 actors initially, "
                          "but I found a different number %2 of node labels, at line %3.")
                            .arg(QString::number(totalNodes))
                            .arg(QString::number(nodeSum))
                            .arg(fileLineNumber);
        return false;
    }

    if (relationsList.size() == 0) {
        emit signalAddNewRelation("unnamed");
    }


    //The network has been loaded. Change to the first relation
    emit signalSetRelation (0);

    // Clear temp arrays
    lineElement.clear();
    tempList.clear();
    rowLabels.clear();
    colLabels.clear();
    relationsList.clear();

    qDebug() << "Finished OK. Returning.";
    return true;

}



bool Parser::readDLKeywords(QStringList &strList,
                            int &N,
                            int &NM,
                            int &NR,
                            int &NC,
                            bool &fullmatrixFormat,
                            bool &edgelist1Format){
    QStringList tempList;
    QString tempStr=QString();
    QString label=QString();
    QString value=QString();
    bool intOK=false;

    for (QStringList::Iterator it1 = strList.begin(); it1!=strList.end(); ++it1)   {
        tempStr = (*it1);
        qDebug() << "element:" << tempStr.toLatin1() ;

        if ( tempStr.startsWith("DL", Qt::CaseInsensitive )){
            // remove DL
            tempStr.remove("DL",Qt::CaseInsensitive);
            tempStr=tempStr.simplified();
            qDebug() << "element contained DL. Removed it:"
                     << tempStr;
        }

        // check if this element contains a "="
        if ( tempStr.size() > 0  ) {

            if (tempStr.contains("=",Qt::CaseInsensitive)) {
                qDebug() << "splitting element at = sign";

                tempList = tempStr.split("=", Qt::SkipEmptyParts);

                label = tempList[0].simplified();
                value= tempList[1].simplified();

                if (  label == "n" || label  == "N" ) {
                    qDebug() << "N is declared to be : "
                             << value ;
                    N=value.toInt(&intOK,10);
                    if (!intOK) {
                        qDebug() << "N conversion error..." ;
                        //emit something here...
                        errorMessage = tr("Error while reading UCINET-formatted file. Cannot convert N value to integer. ");
                        return false;
                    }
                }
                else if (  label == "nm" || label  == "NM" ) {
                    qDebug() << "NM is declared to be : "
                             << value ;
                    NM = value.toInt(&intOK,10);
                    if (!intOK) {
                        qDebug() << "NM conversion error..." ;
                        //emit something here...
                        errorMessage = tr("Problem interpreting UCINET file. Cannot convert NM value to integer. ");
                        return false;
                    }
                }
                else if (  label == "nr" || label  == "NR" ) {
                    qDebug() << "NR is declared to be : "
                             << value ;
                    NR = value.toInt(&intOK,10);
                    if (!intOK) {
                        qDebug() << "NR conversion error..." ;
                        //emit something here...
                        errorMessage = tr("Error while reading UCINET-formatted file. Cannot convert NR value to integer.");
                        return false;
                    }
                }
                else if (  label == "nc" || label  == "NC" ) {
                    qDebug() << "NC is declared to be : "
                             << value ;
                    NC = value.toInt(&intOK,10);
                    if (!intOK) {
                        qDebug() << "NC conversion error..." ;
                        //emit something here...
                        errorMessage = tr("Error while reading UCINET-formatted file. Cannot convert NC value to integer. ");
                        return false;
                    }
                }
                else if (  label == "format" || label  == "FORMAT" ) {
                    qDebug() << "FORMAT is declared to be : "
                             << value ;
                    if (value.contains("FULLMATRIX",Qt::CaseInsensitive)) {
                        fullmatrixFormat=true;
                        qDebug() << "FORMAT fullmatrix detected" ;
                    }
                    else if (value.contains("edgelist",Qt::CaseInsensitive) ){
                        edgelist1Format=true;
                        qDebug() << "FORMAT edgelist detected" ;
                    }
                } // end format
            } // end if contains =
            else {
                return false;
            }


        } // end if > 0




    } // end for lineElement
    return true;
}




/**
 * @brief Parses the data as Pajek-formatted
 *
 * @param rawData
 * @return
 */
bool Parser::parseAsPajek(const QByteArray &rawData){

    qDebug() << "Parsing data as pajek formatted...";

    QTextCodec *codec = QTextCodec::codecForName( m_textCodecName.toLatin1() );
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);


    QString str, label, temp;
    nodeColor="";
    edgeColor="";
    nodeShape="";
    initEdgeLabel = QString();
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
    unsigned long int fileLineNumber=0;
    unsigned long int actualLineNumber=0;
    int pos=-1, lastRelationIndex=0;
    qreal weight=1;
    QString relation;
    list<int> listDummiesPajek;
    totalLinks=0;
    totalNodes=0;
    j=0;  //counts how many real nodes exist in the file
    miss=0; //counts missing nodeNumbers.
    //if j + miss < nodeNum, it creates (nodeNum-miss) dummy nodes which are deleted in the end.
    relationsList.clear();

    while ( !ts.atEnd() )   {

        fileLineNumber++;

        str= ts.readLine();
        str = str.simplified();

        if ( isComment(str)  )
            continue;

        actualLineNumber++;

        qDebug()<< "*** str:" << str;

        if (actualLineNumber==1) {
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
                qDebug()<< "*** Not a Pajek-formatted file. Aborting!!";
                errorMessage = tr("Not a Pajek-formatted file. "
                                  "First not-comment line %1 (at file line %2) does not start with "
                                  "Network or Vertices").arg(actualLineNumber).arg(fileLineNumber);
                return false;
            }
        }

        if (!edges_flag && !arcs_flag && !nodes_flag && !arcslist_flag && !matrix_flag) {
            //qDebug("reading headlines");
            if ( (actualLineNumber == 1) &&
                 (!str.contains("network",Qt::CaseInsensitive)
                  && !str.contains("vertices",Qt::CaseInsensitive)
                  )
                )
            {
                qDebug("*** Not a Pajek file. Aborting!");
                errorMessage = tr("Not a Pajek-formatted file. "
                                  "First not-comment line does not start with "
                                  "Network or Vertices");
                return false;
            }
            else if (str.startsWith( "*network",Qt::CaseInsensitive) )  { //NETWORK NAME
                networkName = (str.right(str.size() - 8 )).simplified() ;
                if (!networkName.isEmpty() ) {
                    qDebug()<<"networkName: "
                           <<networkName;
                }
                else {
                    qDebug()<<"set networkName to unnamed.";
                    networkName = "unnamed";
                }
                continue;
            }
            if (str.contains( "vertices", Qt::CaseInsensitive) )  {
                lineElement=str.split(QRegularExpression("\\s+"));
                if (!lineElement[1].isEmpty()) 	totalNodes=lineElement[1].toInt(&intOk,10);
                qDebug ("Vertices %i.",totalNodes);
                continue;
            }
            qDebug("headlines end here");
        }
        /**SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS) IN SEVERAL ELEMENTS*/
        lineElement=str.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

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
                relationsList << relation;
                qDebug() << "added new relation"<< relation
                         << "to relationsList - signaling to add new relation";
                emit signalAddNewRelation( relation );
                lastRelationIndex = relationsList.size() - 1;
                if ( lastRelationIndex > 0) {
                    qDebug () << "last relation index:"
                              << lastRelationIndex
                              << "signaling to change to the last relation...";
                    emit signalSetRelation(lastRelationIndex);
                    i=0; // reset the source node index
                }

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
                relationsList << relation;
                qDebug() << "added new relation"<< relation
                         << "to relationsList - signaling to add new relation";
                emit signalAddNewRelation( relation );
                lastRelationIndex = relationsList.size()-1;
                if ( lastRelationIndex > 0) {
                    qDebug () << "last relation index:"
                              << lastRelationIndex
                              << "signaling to change to the last relation...";
                    emit signalSetRelation(lastRelationIndex);
                    i=0; // reset the source node index
                }
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
                    for (int c=0; c< lineElement.size(); c++) {
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
                    for (int c=0; c< lineElement.size(); c++)   {
                        temp=lineElement.at(c);
                        //		qDebug()<< temp.toLatin1();
                        if ((coordIndex=temp.indexOf(".", Qt::CaseInsensitive)) != -1 ){
                            if (lineElement.at(c-1) == "ic" ) continue;  //pajek declares colors with numbers!
                            if ( !temp[coordIndex-1].isDigit()) continue;  //needs 0.XX
                            if (c+1 == lineElement.size() ) {//first coord zero, i.e: 0  0.455
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
            // START NODE CREATION
            qDebug ()<<"Creating node numbered "<< nodeNum << " Real nodes count (j)= "<< j+1;
            j++;  //Controls the real number of nodes.
            //If the file misses some nodenumbers then we create dummies and delete them afterwards!
            if ( j + miss < nodeNum)  {
                qDebug ()<<"There are "<< j << " nodes but this node has number"<< nodeNum;
                for (int num=j; num< nodeNum; num++) {
                    qDebug()<<"Signaling to create new dummy node"<< num
                           << "at"<< QPointF(randX,randY);
                    emit signalCreateNode( num,
                                     initNodeSize,
                                     nodeColor,
                                     initNodeNumberColor,
                                     initNodeNumberSize,
                                     label,
                                     lineElement[3],
                                    initNodeLabelSize,
                                    QPointF(randX, randY),
                                    nodeShape,
                                    QString()
                                    );

                    listDummiesPajek.push_back(num);
                    miss++;
                }
            }
            else if ( j > nodeNum ) {
                qDebug ("Error: This Pajek net declares this node with nodeNumber smaller than previous nodes. Aborting");
                errorMessage = tr("Invalid Pajek-formatted file. It declares a node with "
                                  "nodeNumber smaller than previous nodes.");
                return false;
            }
            qDebug()<<"Signaling to create new node"<< nodeNum
                   << "at"<< QPointF(randX,randY);
            emit signalCreateNode(
                        nodeNum,initNodeSize, nodeColor,
                        initNodeNumberColor, initNodeNumberSize,
                        label, initNodeLabelColor, initNodeLabelSize,
                        QPointF(randX, randY),
                        nodeShape, QString()
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
                qDebug()<< "The Pajek file declares "<< totalNodes<< " but I didn't found any nodes. I will create them....";
                for (int num=j+1; num<= totalNodes; num++) {
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                    qDebug()<<"Signaling to create new node"<< num
                           << "at random pos:"<< QPointF(randX,randY);
                    emit signalCreateNode(
                                num,
                                initNodeSize,
                                initNodeColor,
                                initNodeNumberColor,
                                initNodeNumberSize,
                                QString::number(i),
                                initNodeLabelColor,
                                initNodeLabelSize,
                                QPointF(randX, randY),
                                initNodeShape,
                                QString(),
                                false
                                );
                }
                j=totalNodes;
            }
            if (edges_flag && !arcs_flag)   {  /**EDGES */

                qDebug("==== Reading edges ====");
                qDebug()<<lineElement;

                source =  lineElement[0].toInt(&ok, 10);
                target = lineElement[1].toInt(&ok,10);

                if (source == 0 || target == 0 ) {
                    errorMessage = tr("Invalid Pajek-formatted file. The file declares an edge "
                                            "with a zero source or target nodeNumber. "
                                            "However, each node should have a nodeNumber > 0.");
                    return false;
                }
                else if (source < 0 && target >0  ) {  //weights come first...

                    edgeWeight  = lineElement[0].toDouble(&ok);

                    source=  lineElement[1].toInt(&ok, 10);

                    if (lineElement.size()>2) {
                        target = lineElement[2].toInt(&ok,10);
                    }
                    else {
                        target = lineElement[1].toInt(&ok,10);  //self link
                    }

                }
                else if (lineElement.size()>2) {
                    edgeWeight =lineElement[2].toDouble(&ok);
                }
                else {
                    edgeWeight =1.0;
                }

                //qDebug()<<"weight "<< weight;

                if (lineElement.contains("c", Qt::CaseSensitive ) ) {
                    //qDebug("file with link colours");
                    fileContainsLinkColors=true;
                    colorIndex=lineElement.indexOf( QRegularExpression("[c]"), 0 ) + 1;
                    if (colorIndex >= lineElement.size()) edgeColor=initEdgeColor;
                    else 	edgeColor=lineElement [ colorIndex ];
                    if (edgeColor.contains (".") )  edgeColor=initEdgeColor;
                    //qDebug()<< " current color "<< edgeColor;
                }
                else  {
                    //qDebug("file with no link colours");
                    edgeColor=initEdgeColor;
                }

                if (lineElement.contains("l", Qt::CaseSensitive ) ) {
                    qDebug("file with link labels");
                    fileContainsLinkLabels=true;
                    labelIndex=lineElement.indexOf( QRegularExpression("[l]"), 0 ) + 1;
                    if (labelIndex >= lineElement.size()) edgeLabel=initEdgeLabel;
                    else 	edgeLabel=lineElement [ labelIndex ];
                    if (edgeLabel.contains (".") )  edgeLabel=initEdgeLabel;
                    qDebug()<< " edge label "<< edgeLabel;
                }
                else  {
                    //qDebug("file with no link labels");
                    edgeLabel=initEdgeLabel;
                }

                arrows=false;
                bezier=false;
                qDebug()<< "EDGES: signaling to create new edge:" << source << " - "<< target;
                emit signalCreateEdge(source, target, edgeWeight, edgeColor,
                                EdgeType::Undirected, arrows, bezier, edgeLabel);
                totalLinks=totalLinks+2;

            } //end if EDGES
            else if (!edges_flag && arcs_flag)   {  /** ARCS */

                //qDebug("=== Reading arcs ===");
                source = lineElement[0].toInt(&ok, 10);
                target = lineElement[1].toInt(&ok,10);

                if (source == 0 || target == 0 ) {
                    errorMessage = tr("Invalid Pajek-formatted file. The file declares arc "
                                            "with a zero source or target nodeNumber. "
                                            "However, each node should have a nodeNumber > 0.");
                    return false;   //  i -->(i-1)   internally
                }
                else if (source < 0 && target >0 ) {  //weights come first...

                    edgeWeight  = lineElement[0].toDouble(&ok);
                    source=  lineElement[1].toInt(&ok, 10);

                    if (lineElement.size()>2) {
                        target = lineElement[2].toInt(&ok,10);
                    }
                    else {
                        target = lineElement[1].toInt(&ok,10);  //self link
                    }

                }
                else if (lineElement.size()>2) {
                    edgeWeight  =lineElement[2].toDouble(&ok);
                }
                else {
                    edgeWeight =1.0;
                }

                if (lineElement.contains("c", Qt::CaseSensitive ) ) {
                    //qDebug("file with link colours");
                    edgeColor=lineElement.at ( lineElement.indexOf( QRegularExpression("[c]"), 0 ) + 1 );
                    fileContainsLinkColors=true;
                }
                else  {
                    //qDebug("file with no link colours");
                    edgeColor=initEdgeColor;
                }

                if (lineElement.contains("l", Qt::CaseSensitive ) ) {
                    qDebug("file with link labels");
                    fileContainsLinkLabels=true;
                    labelIndex=lineElement.indexOf( QRegularExpression("[l]"), 0 ) + 1;
                    if (labelIndex >= lineElement.size()) edgeLabel=initEdgeLabel;
                    else 	edgeLabel=lineElement.at ( labelIndex );
                    //if (edgeLabel.contains (".") )  edgeLabel=initEdgeLabel;
                    qDebug()<< " edge label "<< edgeLabel;
                }
                else  {
                    //qDebug("file with no link labels");
                    edgeLabel=initEdgeLabel;
                }
                arrows=true;
                bezier=false;
                has_arcs=true;
                qDebug()<<"ARCS: signaling to create new arc:"<< source << "->"<< target << "with weight "<< weight;
                emit signalCreateEdge(source, target, edgeWeight , edgeColor,
                                EdgeType::Directed, arrows, bezier, edgeLabel);
                totalLinks++;
            } //else if ARCS
            else if (arcslist_flag)   {  /** ARCSlist */
                //qDebug("=== Reading arcs list===");
                if (lineElement[0].startsWith("-") ) lineElement[0].remove(0,1);
                source= lineElement[0].toInt(&ok, 10);
                fileContainsLinkColors=false;
                edgeColor=initEdgeColor;
                has_arcs=true;
                arrows=true;
                bezier=false;
                for (int index = 1; index < lineElement.size(); index++) {
                    target = lineElement.at(index).toInt(&ok,10);
                    qDebug()<<"ARCS LIST: signaling to create new arc:"<< source << "->"<< target << "with weight "<< weight;
                    emit signalCreateEdge(source, target, edgeWeight, edgeColor,
                                    EdgeType::Directed, arrows, bezier);
                    totalLinks++;
                }
            } //else if ARCSLIST
            else if (matrix_flag)   {  /** matrix */
                //qDebug("=== Reading matrix of edges===");
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
                        qDebug()<<" MATRIX: signaling to create new arc"
                               << source << "->"<< target +1
                               << "with weight"<< weight;
                        emit signalCreateEdge(source, target+1, edgeWeight, edgeColor,
                                        EdgeType::Directed, arrows, bezier);
                        totalLinks++;
                    }
                }
            } //else if matrix
        } //end if BOTH ARCS AND EDGES
    } //end WHILE

    if (j==0) {
        errorMessage = tr("Invalid Pajek-formatted file. Could not find node declarations in this file.");
        return false;
    }

    qDebug("Removing all dummy nodes, if any");
    if (listDummiesPajek.size() > 0 ) {
        qDebug("Trying to delete the dummies now");
        for ( list<int>::iterator it=listDummiesPajek.begin(); it!=listDummiesPajek.end(); it++ ) {
            emit removeDummyNode(*it);
        }
    }

    if (relationsList.size() == 0) {
        emit signalAddNewRelation(networkName);
    }

    qDebug() << "Clearing temporary dummies and relations list";
    listDummiesPajek.clear();
    relationsList.clear();

    qDebug() << "signaling to change to the first relation...";
    emit signalSetRelation (0);

    if (has_arcs) {
      edgeDirType = EdgeType::Directed;
    }
    else {
      edgeDirType = EdgeType::Undirected;
    }

    qDebug() << "Finished OK. Returning.";
    return true;

}





/**
 * @brief Parses the data as adjacency sociomatrix-formatted.
 *
 * @return bool
 */
bool Parser::parseAsAdjacency(const QByteArray &rawData){

    qDebug() << "Parsing data as adjacency formatted...";

    QTextCodec *codec = QTextCodec::codecForName( m_textCodecName.toLatin1() );
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QString str;
    QString edgeStr;
    QStringList currentRow;
    int fileLine=0;
    int actualLineNumber=0;
    int i=0, j=0, colCount=0, lastCount=0;
    bool conversionOK=false;

    relationsList.clear();
    totalNodes=0;
    edgeWeight=1.0;
    totalLinks=0;
    edgeDirType=EdgeType::Directed;

    // Initially, do a small 11-row read to understand what kind of file this is
    while ( actualLineNumber < 11 &&  !ts.atEnd() ) {

        fileLine ++ ;

        str= ts.readLine().simplified() ; // transforms "/t", "  ", etc to plain " ".

        if ( isComment(str) )
            continue;

        actualLineNumber ++;

        if (
             str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("graph",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL n",Qt::CaseInsensitive)
             || str == "DL"
             || str == "dl"
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)

             ) {
            qDebug()<< "*** Not an Adjacency-formatted file. Aborting!!";

            errorMessage = tr("Invalid adjacency-formatted file. "
                              "Non-comment line %1 includes keywords reserved by other file formats (i.e vertices, graphml, network, graph, digraph, DL, xml)")
                    .arg(fileLine);

            return false;
        }

        // Check the delimiter and split the row -- we support comma or space(s)
        if ( str.contains (",")) {
            colCount = (str.split(",")).size();
        }
        else {
            colCount = (str.split(" ")).size();
        }

        qDebug() << "non-comment row:" << actualLineNumber << ":"<< str << "colCount:"<<colCount ;

        if  ( (colCount != lastCount && actualLineNumber > 1 ) || (colCount < actualLineNumber) ) {
            // row column sizes differ, this can't be an adjacency matrix
            qDebug()<< "*** Not an Adjacency-formatted file. Aborting!!";

            errorMessage = tr("Invalid Adjacency-formatted file. "
                              "Matrix row %1 at line %2 has different number of elements from previous row.").arg(actualLineNumber).arg(fileLine);
            return false;
        }

        lastCount=colCount;

    }  // end while

    // RESET file and row counter
    ts.reset();
    ts.seek(0);

    actualLineNumber = 0;
    fileLine = 0;

    // Now do the full read
    while ( !ts.atEnd() ) {

        fileLine ++ ;

        str= ts.readLine().simplified();

        if ( isComment(str) ) {
            qDebug()<< tr("fileLine: %1 is comment...").arg(fileLine);
            if ( fileLine == 0 ) {
                // TOTHINK: Should we read node labels from the first line?
            }
            continue;
        }

        i = ++actualLineNumber;

        qDebug()<<"fileLine: " << fileLine << "i: " <<i;

        // Split the current row
        if ( str.contains (",")) {
            currentRow=str.split(",");
        }
        else {
            currentRow=str.split(" ");
        }

        if ( actualLineNumber == 1 ) {

            // Since a sociomatrix is NxN matrix,
            // the number of items in the first row
            // is the total nodes declared in this file.
            totalNodes=currentRow.size();

            qDebug()<< "Nodes to be created:"<< totalNodes;

            // We know how many nodes there are in this adjacency sociomatrix
            // thus we create them, one by one.

            for (j=1; j<=totalNodes; j++) {

                // compute random position for this node
                randX=rand()%gwWidth;
                randY=rand()%gwHeight;

                qDebug()<<"Signaling to create new node"<< j
                       << "at random pos"<< QPointF(randX,randY);

                emit signalCreateNode( j,
                                 initNodeSize,
                                 initNodeColor,
                                 initNodeNumberColor,
                                 initNodeNumberSize,
                                 QString::number(j),
                                 initNodeLabelColor,
                                 initNodeLabelSize,
                                 QPointF(randX, randY),
                                 initNodeShape,
                                 QString()
                                 );
            }
            qDebug() << "Finished creating nodes";
        }


        // Check if this actual line is over the expected total nodes
        if ( i > totalNodes ) {
            emit signalCreateNode( i,
                             initNodeSize,
                             initNodeColor,
                             initNodeNumberColor,
                             initNodeNumberSize,
                             QString::number(i),
                             initNodeLabelColor,
                             initNodeLabelSize,
                             QPointF(randX, randY),
                             initNodeShape,
                             QString()
                             );
        }

        // Check the number of items in this line
        if ( (int) currentRow.size() > totalNodes )  {
            errorMessage = tr("Invalid Adjacency-formatted file.  "
                              "Not a NxN matrix. Row %1 declares %2 edges. Expected: %3").arg(actualLineNumber).arg((int) currentRow.size()).arg(totalNodes);
            return false;
        }

        // Edge creation loop
        // Create the edges declared in current row.

        // Init the column counter
        j=0;

        qDebug()<< "Starting edge creation loop for matrix row:" << actualLineNumber;

        for (QStringList::Iterator it1 = currentRow.begin(); it1!=currentRow.end(); ++it1)  {

            j++;

            edgeStr = (*it1);
            edgeWeight =edgeStr.toDouble(&conversionOK);

            if ( !conversionOK )  {
                errorMessage = tr("Error reading Adjacency-formatted file. "
                                   "Element (%1,%2) cannot be converted to number. ").arg(i).arg(j);
                return false;
            }

            if ( edgeWeight ){

                arrows=true;
                bezier=false;

                qDebug() << "signaling to create new edge: " << i << "->" <<  j
                         << "weight" << edgeWeight << "TotalLinks: " << totalLinks+1;

                emit signalCreateEdge(i, j, edgeWeight, initEdgeColor, EdgeType::Directed, arrows, bezier);

                totalLinks++;

            }

        } // end edge creation loop


    }  // end full while


    if (relationsList.size() == 0 ) {
        emit signalAddNewRelation( "unnamed" );
    }

    qDebug() << "Finished OK. Returning.";
    return true;

}



/**
 * @brief Parses the data as two-mode sociomatrix formatted network.
 * @param rawData
 * @return
 */
bool Parser::parseAsTwoModeSociomatrix(const QByteArray &rawData){


    qDebug()<< "Parsing data as two-mode sociomatrix formatted...";

    QTextCodec *codec = QTextCodec::codecForName( m_textCodecName.toLatin1() );
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    QString str;
    QStringList lineElement;
    int fileLine=0, actualLineNumber=0;
    int i=0, j=0,  newCount=0, lastCount=0;
    totalNodes=0;
    edgeWeight=1.0;
    edgeDirType=EdgeType::Undirected;
    relationsList.clear();

    while (  !ts.atEnd() )  {
        i++;
        fileLine ++ ;
        str= ts.readLine().simplified();
        if ( isComment(str) )
            continue;
        actualLineNumber ++;
        if ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("graph",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL n",Qt::CaseInsensitive)
             || str == "DL"
             || str == "dl"
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
             ) {
            qDebug()<< "*** Not a two mode sociomatrix-formatted file. Aborting!!";

            errorMessage = tr("Invalid two-mode sociomatrix file. "
                             "Non-comment line %1 includes keywords reserved by other file formats (i.e vertices, graphml, network, graph, digraph, DL, xml)")
                             .arg(fileLine);
            return false;
        }
        if ( str.contains (",")){
            lineElement=str.split(",");
            newCount = lineElement.size();
        }
        else {
            lineElement=str.split(" ");
            newCount = lineElement.size();
        }
        qDebug() << str;
        qDebug() << "newCount "<<newCount << " nodes. We are at i = " << i;
        if  ( (newCount != lastCount && i>1 )  ) { // line element count differ
            qDebug()<< "*** Not a Sociomatrix-formatted file. Aborting!!";

            errorMessage = tr("Invalid two-mode sociomatrix file. "
                              "Row %1 has fewer or more elements than previous line.").arg(i);
            return false;
        }
        lastCount=newCount;
        randX=rand()%gwWidth;
        randY=rand()%gwHeight;
        qDebug()<< "Signaling to create new node"
                << i << "at random pos:"<<randX << "x" << randY;
        emit signalCreateNode( i,initNodeSize, initNodeColor,
                         initNodeNumberColor, initNodeNumberSize,
                         QString::number(i), initNodeLabelColor, initNodeLabelSize,
                         QPointF(randX, randY),
                         initNodeShape, QString()
                         );
        j=1;
        qDebug()<< "reading actor affiliations...";
        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            if ( (*it1)!="0"){
                qDebug() << "there is an 1 from "<< i << " to "<<  j;
                firstModeMultiMap.insert(i, j);
                secondModeMultiMap.insert(j, i);
                for (int k = 1; k < i ; ++k) {
                    qDebug() << "Checking earlier discovered actor k = " << k;
                    if ( firstModeMultiMap.contains(k, j) ) {
                        arrows=true;
                        bezier=false;
                        edgeWeight = 1;
                        qDebug() << "Actor" << i << " on the same event as actor " << k << ". signaling to create new edge";
                        emit signalCreateEdge(i, k, edgeWeight, initEdgeColor, EdgeType::Undirected, arrows, bezier);
                        totalLinks++;
                    }
                }

            }
            j++;
        }
    }


    if (relationsList.size() == 0) {
        emit signalAddNewRelation("unnamed");
    }

    qDebug() << "Finished OK. Returning.";
    return true;

}



/**
 * @brief Parses the data as GraphML (not GML) formatted network.
 *
 * @param rawData
 * @return bool
 */
bool Parser::parseAsGraphML(const QByteArray &rawData){

    qDebug() << "Parsing data as GraphML formatted...";

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
    edgeDirType=EdgeType::Directed;

    // Create a xml parser
    QXmlStreamReader xml;

    // Prepare the user selected codec, if needed
    QByteArray userSelectedCodec =m_textCodecName.toLatin1();

    // Add raw data into xml parser
    xml.addData(rawData);

    qDebug() << "Testing if XML document encoding is the same as the userSelectedCodec:" << userSelectedCodec;

    xml.readNext();
    if (xml.isStartDocument()) {
        qDebug()<< "XML document version"
                << xml.documentVersion()
                << "encoding" << xml.documentEncoding()
                << "userSelectedCodec"
                << m_textCodecName;
         if ( xml.documentEncoding().toString() != m_textCodecName) {
                qDebug() << "Conflicting encodings. "
                     << " Re-reading data with userSelectedCodec" << userSelectedCodec;
                xml.clear();

                QTextCodec *codec = QTextCodec::codecForName( userSelectedCodec );
                QString decodedData = codec->toUnicode(rawData);
                xml.addData(decodedData);

//                QTextStream in(&rawData);
//                in.setAutoDetectUnicode(false);
//                QString decodedData = in.readAll();
//                // QTextStream no longer supports setCodec
//                in.setEncoding()
//                QTextStream in(&rawData);

         }
         else {
             qDebug() << "Testing XML: OK";
             xml.clear();
             xml.addData(rawData);
         }
    }

    while (!xml.atEnd()) {
        xml.readNext();
        qDebug()<< "xml.token "<< xml.tokenString();
        if (xml.isStartDocument()) {
            qDebug()<< "xml startDocument" << " version "
                    << xml.documentVersion()
                    << " encoding " << xml.documentEncoding();
        }

        if (xml.isStartElement()) {
            qDebug()<< "element name "<< xml.name().toString();

            if (xml.name().toString() == "graphml") {
                qDebug()<< "GraphML start. NamespaceUri is "
                        << xml.namespaceUri().toString()
                        << "Calling readGraphML()";
                if (! readGraphML(xml) ) {
                    //return false;
                    break;
                }
            }
            else {	//not a GraphML doc, return false.
                xml.raiseError(
                            QObject::tr("not a GraphML file."));
                qDebug()<< "### Error in startElement "
                        << " The file is not an GraphML version 1.0 file ";
                errorMessage = tr("Invalid GraphML file. "
                                  "XML at startElement but element name not graphml.");
                break;
            }
        }
        else if  ( xml.tokenString() == "Invalid" ){
            xml.raiseError(
                        QObject::tr("invalid GraphML or encoding."));
            qDebug()<< "### Cannot find startElement"
                    << " The file is not valid GraphML or has invalid encoding";
            errorMessage = tr("Invalid GraphML file. "
                              "XML tokenString at line %1 invalid.").arg(xml.lineNumber());
            break;
        }
    } // end while

    //clear our mess - remove every hash element...
    keyFor.clear();
    keyName.clear();
    keyType.clear();
    keyDefaultValue.clear();
    nodeHash.clear();
    edgeMissingNodesList.clear();

    // if there was an error return false with error string
    if (xml.hasError()) {
        qDebug()<< "### xmls has error! "
                   "Returning false with errorString" << xml.errorString();
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
    emit signalSetRelation (0);

    qDebug() << "Finished OK. Returning.";
    return true;
}


/**
 * @brief Checks the xml token name and calls the appropriate function.
 *
 * @param xml
 * @return bool
 */
bool Parser::readGraphML(QXmlStreamReader &xml){
    qDebug()<< "Reading graphml token/element..." ;
    bool_node=false;
    bool_edge=false;
    bool_key=false;
    //Q_ASSERT(xml.isStartElement() && xml.name().toString() == "graph");

    while (!xml.atEnd()) { //start reading until QXmlStreamReader end().

        xml.readNext();	//read next token

        qDebug()<< "line:" << xml.lineNumber();

        if (xml.isStartElement()) {	//new token (graph, node, or edge) here
            qDebug()<< "isStartElement() : "
                    << xml.name().toString() ;
            if (xml.name().toString() == "graph")	//graph definition token
                readGraphMLElementGraph(xml);

            else if (xml.name().toString() == "key")	{//key definition token
                QXmlStreamAttributes xmlStreamAttr = xml.attributes();
                readGraphMLElementKey(  xmlStreamAttr );
            }
            else if (xml.name().toString() == "default") //default key value token
                readGraphMLElementDefaultValue(xml);

            else if (xml.name().toString() == "node")	//graph definition token
                readGraphMLElementNode(xml);

            else if (xml.name().toString() == "data")	//data definition token
                readGraphMLElementData(xml);

            else if ( xml.name().toString() == "ShapeNode") {
                bool_node =  true;
            }
            else if ( ( xml.name().toString() == "Geometry"
                        || xml.name().toString() == "Fill"
                        || xml.name().toString() == "BorderStyle"
                        || xml.name().toString() == "NodeLabel"
                        || xml.name().toString() == "Shape"
                        ) && 	bool_node
                      ) {
                readGraphMLElementNodeGraphics(xml);
            }

            else if (xml.name().toString() == "edge")	{//edge definition token
                QXmlStreamAttributes xmlStreamAttr = xml.attributes();
                readGraphMLElementEdge( xmlStreamAttr  );
            }

            else if ( xml.name().toString() == "BezierEdge") {
                bool_edge =  true;
            }

            else if ( ( xml.name().toString() == "Path"
                         || xml.name().toString() == "LineStyle"
                         || xml.name().toString() == "Arrows"
                         || xml.name().toString() == "EdgeLabel"
                       ) && bool_edge
                      ) {
                readGraphMLElementEdgeGraphics(xml);
            }

            else
                readGraphMLElementUnknown(xml);
        }

        if (xml.isEndElement()) {		//token ends here
            qDebug()<< " element ends here: "
                    << xml.name().toString() ;
            if (xml.name().toString() == "node")	//node definition end
                endGraphMLElementNode(xml);
            else if (xml.name().toString() == "edge")	//edge definition end
                endGraphMLElementEdge(xml);
        }

        if (xml.hasError()) {
            qDebug()<< "xml has error:" << xml.errorString();
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
void Parser::readGraphMLElementGraph(QXmlStreamReader &xml){
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    QString defaultDirection = xmlStreamAttr.value("edgedefault").toString();
    qDebug()<< "Parsing graph element - edgedefault "
            << defaultDirection;
    if (defaultDirection=="undirected"){
        qDebug()<< "this is an undirected graph ";
        edgeDirType=EdgeType::Undirected;
        arrows=false;
    }
    else {
        qDebug()<< "this is a directed graph ";
        edgeDirType=EdgeType::Directed;
        arrows=true;
    }
    //store graph id
    networkName = xmlStreamAttr.value("id").toString();
    // add it as relation
    relationsList << networkName;
    qDebug()<< "Signaling to add new relation:" <<networkName;
    emit signalAddNewRelation( networkName);
    int lastRelationIndex = relationsList.size() - 1;
    if (lastRelationIndex > 0) {
        totalNodes=0;
        qDebug () << "last relation index:"
                  << lastRelationIndex
                  << "signaling to change to the new relation";
        emit signalSetRelation(lastRelationIndex);
    }
    qDebug()<< "graph id:" << networkName;
}






/**
 * @brief Reads a key definition
 *
 * called at key element
 *
 * @param xmlStreamAttr
 */
void Parser::readGraphMLElementKey ( QXmlStreamAttributes &xmlStreamAttr )
{
    key_id = xmlStreamAttr.value("id").toString();
    qDebug()<< "Reading key element - key id"<< key_id;
    key_what = xmlStreamAttr.value("for").toString();
    keyFor [key_id] = key_what;
    qDebug()<< "key for "<< key_what;

    if (xmlStreamAttr.hasAttribute("attr.name") ) {  // to be enabled in later versions..
        key_name =xmlStreamAttr.value("attr.name").toString();
        keyName [key_id] = key_name;
        qDebug()<< "key attr.name" << key_name;
    }
    if (xmlStreamAttr.hasAttribute("attr.type") ) {
        key_type=xmlStreamAttr.value("attr.type").toString();
        keyType [key_id] = key_type;
        qDebug()<< "key attr.type" << key_type;
    }
    else if (xmlStreamAttr.hasAttribute("yfiles.type") ) {
        key_type=xmlStreamAttr.value("yfiles.type").toString();
        keyType [key_id] = key_type;
        qDebug()<< "key yfiles.type"<< key_type;
    }

}



/**
 * @brief Reads default key values
 *
 * Called at a default element (usually nested inside key element)
 *
 * @param xml
 */
void Parser::readGraphMLElementDefaultValue(QXmlStreamReader &xml) {

    key_value=xml.readElementText();
    keyDefaultValue [key_id] = key_value;	//key_id is already stored

    qDebug()<< "Reading default key values - key default value is"
            << key_value;

    if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "key default value" << key_value << "is for node size";
        conv_OK=false;
        initNodeSize= key_value.toInt(&conv_OK);
        if (!conv_OK) initNodeSize = 8;
    }
    if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "key default value" << key_value << "is for nodes shape";
        initNodeShape= key_value;
    }
    if (keyName.value(key_id) == "custom-icon" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "key default value" << key_value << "is for node custom-icon path";
        initNodeCustomIcon = key_value;
        initNodeCustomIcon = fileDirPath + "/"  + initNodeCustomIcon;
        qDebug()<< "initNodeCustomIcon full path:" << initNodeCustomIcon ;
        if (QFileInfo::exists(initNodeCustomIcon)){
            qDebug()<< "custom icon file exists!";
        }
        else {
            qDebug()<< "custom icon file does not exists!";
            xml.raiseError(
                        QObject::tr(" Default custom icon for nodes does not exist in the filesystem. \nThe declared icon file was: \n%1").arg(initNodeCustomIcon));
        }
    }
    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "key default value" << key_value << "is for nodes color";
        initNodeColor= key_value;
    }
    if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "key default value" << key_value << "is for node labels color";
        initNodeLabelColor= key_value;
    }
    if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "key default value" << key_value << "is for node labels size";
        conv_OK=false;
        initNodeLabelSize= key_value.toInt(&conv_OK);
        if (!conv_OK) initNodeLabelSize = 8;
    }
    if (keyName.value(key_id) == "weight" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "key default value" << key_value << "is for edges weight";
        conv_OK=false;
        initEdgeWeight= key_value.toDouble(&conv_OK);
        if (!conv_OK)
            initEdgeWeight = 1;
    }
    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "key default value" << key_value << "is for edges color";
        initEdgeColor= key_value;
    }

}



/**
 * @brief Reads basic node attributes and sets the nodeNumber.
 *
 * called at the start of a node element
 *
 * @param xml
 */
void Parser::readGraphMLElementNode(QXmlStreamReader &xml){
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    node_id = (xmlStreamAttr.value("id")).toString();
    totalNodes++;

//    qDebug()<< "reading node id"<<  node_id
//           << "index" << totalNodes
//           << "added to nodeHash"
//           << "gwWidth, gwHeight "<< gwWidth<< "," <<gwHeight;

    nodeHash[node_id]=totalNodes;

    //copy default node attribute values.
    //Some might change when reading element data, some will stay the same...
    nodeColor = initNodeColor;
    nodeShape = initNodeShape;
    nodeIconPath = initNodeCustomIcon;
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



/**
 * @brief Signals to create a new node
 *
 * called at the end of a node element
 *
 * @param xml
 */
void Parser::endGraphMLElementNode(QXmlStreamReader &xml){
    Q_UNUSED(xml);
    //@todo this check means we cannot have different nodes between relations.
    if (relationsList.size() > 1 ) {
        qDebug()<<"multirelational data"
                  "skipping node creation. Node should have been created in earlier relation";
        bool_node = false;
        return;
    }

    qDebug()<<"signaling to create a new node"
           << totalNodes  << "id " << node_id
           << " label " << nodeLabel << "at pos:" << QPointF(randX,randY);

    if ( nodeShape == "custom") {
        emit signalCreateNode( totalNodes,
                         nodeSize,
                         nodeColor,
                         nodeNumberColor,
                         nodeNumberSize,
                         nodeLabel,
                         nodeLabelColor,
                         nodeLabelSize,
                         QPointF(randX,randY),
                         nodeShape,
                         ( nodeIconPath.isEmpty() ? initNodeCustomIcon: nodeIconPath)
                         );
    }
    else {
        emit signalCreateNode( totalNodes,
                         nodeSize,
                         nodeColor,
                         nodeNumberColor,
                         nodeNumberSize,
                         nodeLabel,
                         nodeLabelColor,
                         nodeLabelSize,
                         QPointF(randX,randY),
                         nodeShape,
                         QString()
                         );
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
void Parser::readGraphMLElementEdge(QXmlStreamAttributes &xmlStreamAttr){

    edge_source = xmlStreamAttr.value("source").toString();
    edge_target = xmlStreamAttr.value("target").toString();
    edge_directed = xmlStreamAttr.value("directed").toString();

//    qDebug()<< "Parsing edge id: "
//            <<	xmlStreamAttr.value("id").toString()
//                << "edge_source " << edge_source
//                << "edge_target " << edge_target
//                << "directed " << edge_directed;

    missingNode=false;
    edgeWeight=initEdgeWeight;
    edgeColor=initEdgeColor;
    edgeLabel = "";
    bool_edge= true;

    if ( edge_directed=="false" || edge_directed.contains("false",Qt::CaseInsensitive) ) {
        edgeDirType=EdgeType::Undirected;
        qDebug()<< "Edge is UNDIRECTED";
    }
    else {
        edgeDirType=EdgeType::Directed;
        qDebug()<< "Edge is DIRECTED";
    }
    if (!nodeHash.contains(edge_source)) {
        qDebug() << "source node id "
                 << edge_source
                 << "for edge from " << edge_source << " to " << edge_target
                 << "DOES NOT EXIST!"
                 << "Inserting into edgesMissingNodesHash";
        edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                     QString::number(edgeWeight)+"|"+edgeColor
                                     +"|"+QString::number(edgeDirType));
        missingNode=true;
    }
    if (!nodeHash.contains(edge_target)) {
        qDebug() << "target node id "
                 << edge_target
                 << "for edge from " << edge_source << " to " << edge_target
                 << "DOES NOT EXIST!"
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
    qDebug()<< "source "<< edge_source
            << " num "<< source
            <<" - target "<< edge_target << " num "<< target
              << " edgeDirType " << edgeDirType;


}


/**
 * @brief Signals for a new edge to be created/added
 *
 * Called at the end of edge element
 *
 * @param xml
 */
void Parser::endGraphMLElementEdge(QXmlStreamReader &xml){
    Q_UNUSED(xml);
    if (missingNode) {
        qDebug()<<"missingNode true "
               << " postponing edge creation signal";
        return;
    }
    qDebug()<<"signaling to create new edge"
           << source << "->" << target << " edgeDirType value " << edgeDirType;
    emit signalCreateEdge(source, target, edgeWeight, edgeColor, edgeDirType,
                    arrows, bezier, edgeLabel);
    totalLinks++;
    bool_edge= false;
}


/**
 * @brief Reads data for edges and nodes
 *
 * called at a data element (usually nested inside a node or an edge element)
 *
 * @param xml
 */
void Parser::readGraphMLElementData (QXmlStreamReader &xml){

    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    key_id = xmlStreamAttr.value("key").toString();
    key_value=xml.text().toString();

    qDebug()<< "parding data for key_id: "
            <<  key_id <<  "key_value "<< key_value;

    if (key_value.trimmed() == "")
    {
        qDebug()<< "empty key_value: "
                << key_value
                << "reading more xml.text()...";

        xml.readNext();

        key_value=xml.text().toString();

        qDebug()<< "now key_value: " << key_value;

        if (  key_value.trimmed() != "" ) {
            //if there's simple text after the StartElement,
            qDebug()<< "key_id " << key_id
                    << " value is simple text " <<key_value ;
        }
        else {  //no text, probably more tags. Return...
            qDebug()<< "key_id " << key_id
                    << " for " <<keyFor.value(key_id)
                    << ". More elements nested here. Returning";
            return;
        }

    }

    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Data found. Node color: "
                << key_value << " for this node";
        nodeColor= key_value;
    }
    else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "node" ){
        qDebug()<< "Data found. Node label: "
                   ""<< key_value << " for this node";
        nodeLabel = key_value;
    }
    else if (keyName.value(key_id) == "x_coordinate" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Data found. Node x: "
                << key_value << " for this node";
        conv_OK=false;
        randX= key_value.toFloat( &conv_OK ) ;
        if (!conv_OK)
            randX = 0;
        else
            randX=randX * gwWidth;
        qDebug()<< "Using: "<< randX;
    }
    else if (keyName.value(key_id) == "y_coordinate" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Data found. Node y: "
                << key_value << " for this node";
        conv_OK=false;
        randY= key_value.toFloat( &conv_OK );
        if (!conv_OK)
            randY = 0;
        else
            randY=randY * gwHeight;
        qDebug()<< "Using: "<< randY;
    }
    else if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Data found. Node size: "
                << key_value << " for this node";
        conv_OK=false;
        nodeSize= key_value.toInt ( &conv_OK );
        if (!conv_OK)
            nodeSize = initNodeSize;
        qDebug()<< "Using: "<< nodeSize;
    }
    else if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Data found. Node label size: "
                << key_value << " for this node";
        conv_OK=false;
        nodeLabelSize= key_value.toInt ( &conv_OK );
        if (!conv_OK)
            nodeLabelSize = initNodeLabelSize;
        qDebug()<< "Using: "<< nodeSize;
    }
    else if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node" ){
        qDebug()<< "Data found. Node label Color: "
                << key_value << " for this node";
        nodeLabelColor = key_value;
    }
    else if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Data found. Node shape: "
                << key_value << " for this node";
        nodeShape= key_value;
    }
    else if (keyName.value(key_id) == "custom-icon" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "Data found. Node custom-icon path: "
                << key_value << " for this node";
        nodeIconPath = key_value;
        nodeIconPath = fileDirPath + ("/") + nodeIconPath;
        qDebug()<< "full node custom-icon path: "
                    << nodeIconPath  ;
    }
    else if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "Data found. Edge color: "
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
        edgeWeight= key_value.toDouble( &conv_OK );
        if (!conv_OK)
            edgeWeight = 1.0;
        if (missingNode){
            edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                         QString::number(edgeWeight)+"|"+edgeColor
                                         +"|"+QString::number(edgeDirType));
        }
        qDebug()<< "Data found. Edge value: "
                << key_value << " Using "<< edgeWeight << " for this edge";
    }
    else if ( keyName.value(key_id) == "size of arrow"  && keyFor.value(key_id) == "edge" ) {
        conv_OK=false;
        qreal temp = key_value.toFloat( &conv_OK );
        if (!conv_OK) arrowSize = 1;
        else  arrowSize = temp;
        qDebug()<< "Data found. Edge arrow size: "
                << key_value << " Using  "<< arrowSize << " for this edge";
    }
    else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "edge" ){
        edgeLabel = key_value;
        if (missingNode){
            edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                         QString::number(edgeWeight)+"|"+edgeColor
                                         +"|"+QString::number(edgeDirType));
        }
        qDebug()<< "Data found. Edge label: "
                << edgeLabel << " for this edge";
    }



}



/**
 * @brief Reads node graphics data and properties: label, color, shape, size, coordinates, etc.
 * @param xml
 */
void Parser::readGraphMLElementNodeGraphics(QXmlStreamReader &xml) {
    qDebug()<< "reading node graphics/properties, element name"
            << xml.name().toString();
    qreal tempX =-1, tempY=-1, temp=-1;
    QString color;
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();

    if ( xml.name().toString() == "Geometry" ) {
        if ( xmlStreamAttr.hasAttribute("x") ) {
            conv_OK=false;
            tempX = xml.attributes().value("x").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                randX = tempX;
        }
        if ( xmlStreamAttr.hasAttribute("y") ) {
            conv_OK=false;
            tempY = xml.attributes().value("y").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                randY = tempY;
        }
        qDebug()<< "Node Coordinates: "
                << tempX << " " << tempY << " Using coordinates" << randX<< " "<<randY;
        if ( xmlStreamAttr.hasAttribute("width") ) {
            conv_OK=false;
            temp = xmlStreamAttr.value("width").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                nodeSize = temp;
            qDebug()<< "Node Size: "
                    << temp<< " Using nodesize" << nodeSize;
        }
        if ( xmlStreamAttr.hasAttribute("shape") ) {
            nodeShape = xmlStreamAttr.value("shape").toString();
            qDebug()<< "Node Shape: "
                    << nodeShape;
        }

    }
    else if (xml.name().toString() == "Fill" ){
        if ( xmlStreamAttr.hasAttribute("color") ) {
            nodeColor= xmlStreamAttr.value("color").toString();
            qDebug()<< "Node color: "
                    << nodeColor;
        }

    }
    else if ( xml.name().toString() == "BorderStyle" ) {


    }
    else if (xml.name().toString() == "NodeLabel" ) {
        key_value=xml.readElementText();  //see if there's simple text after the StartElement
        if (!xml.hasError()) {
            qDebug()<< "Node Label "
                    << key_value;
            nodeLabel = key_value;
        }
        else {
            qDebug()<< "Cannot read Node Label. There must be more elements nested here, continuing";
        }
    }
    else if (xml.name().toString() == "Shape" ) {
        if ( xmlStreamAttr.hasAttribute("type") ) {
            nodeShape= xmlStreamAttr.value("type").toString();
            qDebug()<< "Node shape: "
                    << nodeShape;
        }

    }


}


/**
 * @brief Reads edge graphics data and properties: path, linestyle,width, arrows, etc
 * @param xml
 */
void Parser::readGraphMLElementEdgeGraphics(QXmlStreamReader &xml) {
    qDebug()<< "reading edge graphics/props, element name"
            << xml.name().toString();

    qreal tempX =-1, tempY=-1, temp=-1;
    QString color, tempString;
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();

    if ( xml.name().toString() == "Path" ) {
        if ( xmlStreamAttr.hasAttribute("sx") ) {
            conv_OK=false;
            tempX = xmlStreamAttr.value("sx").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                bez_p1_x = tempX;
            else bez_p1_x = 0 ;
        }
        if ( xmlStreamAttr.hasAttribute("sy") ) {
            conv_OK=false;
            tempY = xmlStreamAttr.value("sy").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                bez_p1_y = tempY;
            else bez_p1_y = 0 ;
        }
        if ( xmlStreamAttr.hasAttribute("tx") ) {
            conv_OK=false;
            tempX = xmlStreamAttr.value("tx").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                bez_p2_x = tempX;
            else bez_p2_x = 0 ;
        }
        if ( xmlStreamAttr.hasAttribute("ty") ) {
            conv_OK=false;
            tempY = xmlStreamAttr.value("ty").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                bez_p2_y = tempY;
            else bez_p2_y = 0 ;
        }
        qDebug()<< "Edge Path control points: "
                << bez_p1_x << " " << bez_p1_y << " " << bez_p2_x << " " << bez_p2_y;
    }
    else if (xml.name().toString() == "LineStyle" ){
        if ( xmlStreamAttr.hasAttribute("color") ) {
            edgeColor= xmlStreamAttr.value("color").toString();
            qDebug()<< "Edge color: "
                    << edgeColor;
        }
        if ( xmlStreamAttr.hasAttribute("type") ) {
            edgeType= xmlStreamAttr.value("type").toString();
            qDebug()<< "Edge type: "
                    << edgeType;
        }
        if ( xmlStreamAttr.hasAttribute("width") ) {
            temp = xmlStreamAttr.value("width").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                edgeWeight = temp;
            else
                edgeWeight=1.0;
            qDebug()<< "Edge width: "
                    << edgeWeight;
        }

    }
    else if ( xml.name().toString() == "Arrows" ) {
        if ( xmlStreamAttr.hasAttribute("source") ) {
            tempString = xmlStreamAttr.value("source").toString();
            qDebug()<< "Edge source arrow type: "
                    << tempString;
        }
        if ( xmlStreamAttr.hasAttribute("target") ) {
            tempString = xmlStreamAttr.value("target").toString();
            qDebug()<< "Edge target arrow type: "
                    << tempString;
        }



    }
    else if (xml.name().toString() == "EdgeLabel" ) {
        key_value=xml.readElementText();  //see if there's simple text after the StartElement
        if (!xml.hasError()) {
            qDebug()<< "Edge Label "
                    << key_value;
            //probably there's more than simple text after StartElement
            edgeLabel = key_value;
        }
        else {
            qDebug()<< "Can't read Edge Label. More elements nested ? Continuing with blank edge label....";
            edgeLabel = "" ;
        }
    }


}


/**
 * @brief Trivial call for unknown elements
 * @param xml
 */
void Parser::readGraphMLElementUnknown(QXmlStreamReader &xml) {
    Q_ASSERT(xml.isStartElement());
    qDebug()<< "unknown element found:"<< xml.name().toString() ;
}



/**
 * @brief Creates any missing node edges
 */
void Parser::createMissingNodeEdges(){
    qDebug()<<"Creating missing node edges... ";
    int count=0;
    if ( (count = edgesMissingNodesHash.size()) > 0 ) {

        bool ok;
        edgeWeight = initEdgeWeight;
        edgeColor = initEdgeColor;
        edgeDirType=EdgeType::Directed;
        qDebug()<<"edges to create " << count;
        QHash<QString, QString>::const_iterator it =
                edgesMissingNodesHash.constBegin();
        while (it != edgesMissingNodesHash.constEnd()) {
            qDebug()<<"creating missing edge "
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
                        edgeDirType=EdgeType::Undirected;

                }
                qDebug()<<"signaling to create new edge:"
                       << source << "->" << target << " edgeDirType value " << edgeDirType;

                emit signalCreateEdge(source, target, edgeWeight, edgeColor, edgeDirType, arrows, bezier, edgeLabel);

            }
            ++it;
        }
    }
    else {
        qDebug()<<"nothing to do";
    }
}




/**
 * @brief Parses the data as GML formatted network.
 *
 * @param rawData
 * @return bool
 */
bool Parser::parseAsGML(const QByteArray &rawData){

    qDebug() << "Parsing data as GML formatted...";

    QTextCodec *codec = QTextCodec::codecForName( m_textCodecName.toLatin1() );
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);


    QRegularExpression onlyDigitsExp("^\\d+$");
    QStringList tempList;
    QString str;
    int fileLine=0, actualLineNumber=0;
    bool floatOK= false;
    bool isPlanar = false, graphKey=false, graphicsKey=false,
            edgeKey=false, nodeKey=false, graphicsCenterKey=false;
    Q_UNUSED(isPlanar);

    relationsList.clear();

    node_id= QString();
    arrows=true;
    bezier=false;
    edgeDirType=EdgeType::Undirected;
    totalNodes=0;

    while (!ts.atEnd() )   {

        floatOK= false;
        fileContainsNodeCoords = false;
        nodeShape = initNodeShape;
        nodeColor = initNodeColor;

        fileLine++;

        str= ts.readLine().simplified();

        qDebug()<< "line"  << fileLine <<":"
                << str;

        if ( isComment(str) )
            continue;

        actualLineNumber ++;

        if ( actualLineNumber == 1 &&
             ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL n",Qt::CaseInsensitive)
             || str == "DL"
             || str == "dl"
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
             )
               ) {
            qDebug()<< "*** Not a GML-formatted file. Aborting!!";
            errorMessage = tr("Not an GML-formatted file. "
                              "Non-comment line %1 includes keywords reserved by other file formats  (i.e vertices, graphml, network, digraph, DL, xml)")
                              .arg(fileLine);

            return false;
        }

        if  ( str.startsWith("comment",Qt::CaseInsensitive) ) {
                qDebug()<< "This is a comment. Continue.";
                continue;
        }
        if  ( str.startsWith("creator",Qt::CaseInsensitive) ) {
                qDebug()<< "This is a creator description. Continue.";
                continue;
        }
        else if  ( str.startsWith("graph",Qt::CaseInsensitive) ) {
            //describe a graph
            qDebug()<< "graph description list start";
            graphKey = true;
        }
        else if ( str.startsWith("directed",Qt::CaseInsensitive) ) {
            //graph attribute declarations
            if (graphKey) {
                if ( str.contains("1")) {
                    qDebug()<< "graph directed 1. A directed graph.";
                    edgeDirType=EdgeType::Directed;
                }
                else {
                    qDebug()<< "graph directed 0. An undirected graph.";
                }
            }
        }
        else if ( str.startsWith("isPlanar",Qt::CaseInsensitive) ) {
            //key declarations
            if (graphKey) {
                if ( str.contains("1")) {
                    qDebug()<< "graph isPlanar 1. Planar graph.";
                    isPlanar = true;
                }
                else {
                    isPlanar = false;
                }
            }
        }

        else if ( str.startsWith("node",Qt::CaseInsensitive) ) {
            //node declarations
            qDebug()<< "node description list starts";
            nodeKey = true;
        }
        else if ( str.startsWith("id",Qt::CaseInsensitive) ) {
            //describes identification number for an object
            if ( nodeKey ) {
                totalNodes++;
                node_id = str.split(" ",Qt::SkipEmptyParts).last();
                if (!node_id.contains(onlyDigitsExp)) {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Node id tag at line %1 has non-arithmetic value.")
                            .arg(fileLine);
                    return false;
                }
                qDebug()<< "id description "
                           << "This node" << totalNodes
                              <<"id"<< node_id;
            }
        }

        else if ( str.startsWith("label ",Qt::CaseInsensitive) ) {
            //describes label
            if ( nodeKey ) {
                nodeLabel = str.split(" ",Qt::SkipEmptyParts).last().remove("\"");
                qDebug()<< "node label definition"
                           << "node" << totalNodes
                              <<"id"<< node_id
                                << "label" << nodeLabel;

                //FIXME REMOVE ANY "
            }
            else if ( edgeKey ) {
                edgeLabel = str.split(" ",Qt::SkipEmptyParts).last();
                qDebug()<< "edge label definition"
                           << "edge" << totalLinks
                                << "label" << edgeLabel;
            }
        }


        else if ( str.startsWith("edge ",Qt::CaseInsensitive) ) {
            //edge declarations
            qDebug()<< "edge description list start";
            edgeKey = true;
            totalLinks++;
        }
        else if ( str.startsWith("source ",Qt::CaseInsensitive) ) {
            if (edgeKey) {
                edge_source = str.split(" ",Qt::SkipEmptyParts).last();
                //if edge_source
                if (!edge_source.contains(onlyDigitsExp)) {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge source tag at line %1 has non-arithmetic value.")
                            .arg(fileLine);
                    return false;
                }
                source = edge_source.toInt(0);
                qDebug()<< "edge source definition"
                           << "edge source" << edge_source;
            }
        }
        else if ( str.startsWith("target ",Qt::CaseInsensitive) ) {
            if (edgeKey) {
                edge_target = str.split(" ",Qt::SkipEmptyParts).last();
                if (!edge_source.contains(onlyDigitsExp)) {
                    errorMessage = tr("Not a proper GML-formatted file. "
                                      "Edge target tag at line %1 has non-arithmetic value.")
                            .arg(fileLine);
                    return false;
                }

                target = edge_target.toInt(0);
                qDebug()<< "edge target definition"
                           << "edge target" << edge_target;
            }
        }
        else if ( str.startsWith("graphics",Qt::CaseInsensitive) ) {
            //Describes graphics which are used to draw a particular object.
            if (nodeKey)  {
                qDebug()<< "node graphics description list start";
            }
            else if (edgeKey) {
                qDebug()<< "edge graphics description list start";
            }
            graphicsKey = true;
        }
        else if ( str.startsWith("center",Qt::CaseInsensitive) ) {
            if (graphicsKey && nodeKey)  {
                qDebug()<< "node graphics center start";
                if ( str.contains("[", Qt::CaseInsensitive) ) {
                    if ( str.contains("]", Qt::CaseInsensitive) &&
                         str.contains("x", Qt::CaseInsensitive) &&
                         str.contains("y", Qt::CaseInsensitive)) {
                        str.remove("center");
                        str.remove("[");
                        str.remove("]");
                        str = str.simplified();
                        tempList = str.split(" ",Qt::SkipEmptyParts);
                        randX = (tempList.at(1)).toFloat(&floatOK);
                        if (!floatOK) {
                            errorMessage = tr("Not a proper GML-formatted file. "
                                              "Node center tag at line %1 cannot be converted to qreal.")
                                    .arg(fileLine);
                            return false;
                        }
                        randY = tempList.at(3).toFloat(&floatOK);
                        if (!floatOK) {
                            errorMessage = tr("Not a proper GML-formatted file. "
                                              "Node center tag at line %1 cannot be converted to qreal.")
                                    .arg(fileLine);
                            return false;
                        }
                        qDebug()<< "node graphics center"
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
                qDebug()<< "node graphics type start";
                nodeShape = str.split(" ",Qt::SkipEmptyParts).last();
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
                qDebug()<< "node graphics fill start";
                nodeColor = str.split(" ",Qt::SkipEmptyParts).last();
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
                qDebug()<< "node graphics center ends";
                graphicsCenterKey = false;
            }
            else if (graphicsKey) {
                qDebug()<< "graphics list ends";
                graphicsKey = false;
            }
            else if (nodeKey && !graphicsKey) {
                qDebug()<< "node description list ends";
                nodeKey = false;
                if (!fileContainsNodeCoords) {
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                }
                qDebug()<<" *** Signaling to create new node "<< node_id
                       << " at "<< randX <<","<< randY
                       <<" label "<<nodeLabel;
                emit signalCreateNode(
                            node_id.toInt(0), initNodeSize, nodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            nodeLabel , initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX,randY),
                            nodeShape, QString()
                            );

            }
            else if (edgeKey && !graphicsKey) {
                qDebug()<< "edge description list ends. signaling to create new edge.";
                edgeKey = false;
                edgeWeight = 1;
                edgeColor = "black";
                if (edgeLabel==QString()) {
                    edgeLabel = edge_source + "->" + edge_target;
                }
                emit signalCreateEdge(source,target, edgeWeight, edgeColor,
                                edgeDirType, arrows, bezier, edgeLabel);
            }

            else if (graphKey) {
                qDebug()<< "graph description list ends";
                graphKey = false;
            }
        }

    }

    if (relationsList.size() == 0 ) {
        emit signalAddNewRelation( "unnamed" );
    }

    qDebug() << "Finished OK. Returning.";
    return true;
}



/**
 * @brief Parses the data as dot (Graphviz) formatted network.
 *
 * @param rawData
 * @return
 */
bool Parser::parseAsDot(const QByteArray &rawData){

    qDebug()<< "Parsing data as dot (Graphviz) formatted...";

    int fileLine=0, actualLineNumber=0, aNum=-1;
    int start=0, end=0, next=0;
    QString label, node, nodeLabel, fontName, fontColor, edgeShape, edgeColor, edgeLabel, networkLabel;
    QString str, temp, prop, value ;
    QStringList lineElement;
    nodeColor="red";
    edgeColor="black";
    nodeShape="";
    edgeWeight=1.0;
    qreal nodeValue=1.0;
    bool netProperties = false;
    QStringList labels;
    QList<QString> nodeSequence;   //holds edges
    QList<QString> nodesDiscovered; //holds nodes;

    relationsList.clear();

    edgeDirType=EdgeType::Directed;
    arrows=true;
    bezier=false;
    source=0, target=0;

    QTextCodec *codec = QTextCodec::codecForName( m_textCodecName.toLatin1() );
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);

    totalNodes=0;

    while (!ts.atEnd() )   {

        fileLine++;

        qDebug ()<<"Reading fileLine "<< fileLine;

        str= ts.readLine().simplified().trimmed();

        qDebug ()<< str;

        if ( isComment (str) )
            continue;

        actualLineNumber ++;

        if ( actualLineNumber == 1 ) {
            if ( str.contains("vertices",Qt::CaseInsensitive) 	//Pajek
                 || str.contains("network",Qt::CaseInsensitive)	//Pajek?
                 || str.contains("[",Qt::CaseInsensitive)    	// GML
                 || str.contains("DL n",Qt::CaseInsensitive)    //DL format
                 || str == "DL"
                 || str == "dl"
                 || str.contains("list",Qt::CaseInsensitive)		//list
                 || str.startsWith("<graphml",Qt::CaseInsensitive)  // GraphML
                 || str.startsWith("<?xml",Qt::CaseInsensitive)
                 ) {
                qDebug() << "*** Not an GraphViz -formatted file. Aborting";

                errorMessage = tr("Not a GraphViz-formatted file. "
                                  "First non-comment line includes keywords reserved by other file formats  (i.e vertices, graphml, network, DL, xml).");
                return false;
            }

            if ( str.contains("digraph", Qt::CaseInsensitive) ) {
                lineElement=str.split(" ");
                edgeDirType=EdgeType::Directed;
                if (lineElement[1]!="{" ) networkName=lineElement[1];
                qDebug() << "This is a DOT DIGRAPH named " << networkName;
                continue;
            }
            else if ( str.contains("graph", Qt::CaseInsensitive) ) {
                lineElement=str.split(" ");
                edgeDirType=EdgeType::Undirected;
                if (lineElement[1] !="{" ) networkName=lineElement[1];
                qDebug() << "This is a DOT GRAPH named " << networkName;
                continue;
            }
            else {
                qDebug()<<" *** Not a GraphViz file. "
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
            value=str.right(str.size()-next-1).simplified();
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
            end=str.size();
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
            end=str.size();
            qDebug ("* Finished EDGE PROPERTIES!");
        }
        // TODO What about comments inside ? ie. in the end of the line //
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
                qDebug()<<" *** Signaling to create new node "<< totalNodes
                       << "at "<< randX <<","<< randY
                       <<" label "<<node.toLatin1()
                      << " colored "<< initNodeColor
                      << "initNodeSize " << initNodeSize
                      << "initNodeNumberColor " <<initNodeNumberColor
                      << "initNodeNumberSize " << initNodeNumberSize
                      << "initNodeLabelColor " << initNodeLabelColor
                      << "nodeShape" <<  initNodeShape;
                emit signalCreateNode(
                            totalNodes, initNodeSize, initNodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            nodeLabel , initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX,randY),
                            initNodeShape,QString()
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
                qDebug()<<" *** Signaling to create new node "<< totalNodes
                       << " at "<< randX <<","<< randY
                       <<" label "<<node.toLatin1()
                      << " colored "<< initNodeColor
                      << "initNodeSize " << initNodeSize
                      << "initNodeNumberColor " <<initNodeNumberColor
                      << "initNodeNumberSize " << initNodeNumberSize
                      << "initNodeLabelColor " << initNodeLabelColor
                      << "nodeShape" <<  initNodeShape;
                emit signalCreateNode(
                            totalNodes, initNodeSize, initNodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            nodeLabel , initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX,randY),
                            initNodeShape,QString()
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
                edgeDirType=EdgeType::Undirected;
            }
            else { 											//is directed
                nodeSequence=str.split("->");
                edgeDirType=EdgeType::Directed;
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
                    qDebug()<<" *** Signaling to create new node"<< totalNodes
                           << "at"<< QPointF(randX,randY)
                           <<"label"<<node.toLatin1()
                          << "colored "<< nodeColor
                          << "initNodeSize " << initNodeSize
                          << "initNodeNumberColor " <<initNodeNumberColor
                          << "initNodeNumberSize " << initNodeNumberSize
                          << "initNodeLabelColor " << initNodeLabelColor
                          << "nodeShape" <<  initNodeShape;
                    emit signalCreateNode(
                                totalNodes, initNodeSize, nodeColor,
                                initNodeNumberColor, initNodeNumberSize,
                                node , initNodeLabelColor, initNodeLabelSize,
                                QPointF(randX,randY),
                                initNodeShape, QString()
                                );
                    nodesDiscovered.push_back( node  );
                    qDebug()<<" * Total totalNodes "
                           << totalNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
                    target=totalNodes;
                    if (it!=nodeSequence.begin()) {
                        qDebug()<<"-- signaling to create new edge:"
                               << source<< "->" <<target;
                        emit signalCreateEdge(source,target, edgeWeight, edgeColor,
                                        edgeDirType, arrows, bezier);
                    }
                }
                else {
                    //node discovered before
                    target=aNum+1;
                    qDebug("# Node already exists. Vector num: %i ",target);
                    if (it!=nodeSequence.begin()) {
                        qDebug()<<"-- signaling to create new edge"
                               <<source<<"->" << target;
                        emit signalCreateEdge(source,target, edgeWeight , edgeColor,
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
                    qDebug()<<"*** Signaling to create new node at"
                            <<  randX << ","<< randY<< "label"<<node.toLatin1()
                             << " colored "<< nodeColor;
                    emit signalCreateNode(
                                totalNodes, initNodeSize, nodeColor,
                                initNodeNumberColor, initNodeNumberSize,
                                label, initNodeLabelColor, initNodeLabelSize,
                                QPointF(randX,randY),
                                nodeShape, QString()
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

    if (relationsList.size() == 0) {
        emit signalAddNewRelation( (!networkName.isEmpty()) ? networkName :"unnamed");
    }

    qDebug() << "Parser::parseAsDot() - Finished OK. Returning.";
    return true;
}




/**
 * @brief Reads the properties of a dot element
 * @param str
 * @param nValue
 * @param label
 * @param shape
 * @param color
 * @param fontName
 * @param fontColor
 */
void Parser::readDotProperties(QString str, qreal &nValue, QString &label,
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
        str=str.right(str.size()-next-1).simplified();
        if (str.startsWith(","))
            str=str.right(str.size()-1).simplified();

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
            str=str.right(str.size()-next-1).simplified();
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
            qDebug()<<"count"<< str.size()<<  " next "<< next;
            str=str.right(str.size()-next-1).simplified();
            qDebug()<<"whatsleft: "<<str.toLatin1()<<".";
            if ( (next=str.indexOf('=', 1))==-1) break;
        }
    } while (!str.isEmpty());

}



/**
 * Debugging only - Used while parsing weighted edge lists
 */
template<typename T> void print_queue(T& q) {
    qDebug() << "print_queue() ";
    while(!q.empty()) {
        qDebug() << q.top().key << " value: " << q.top().value << " ";
        q.pop();
    }
    qDebug() << "\n";
}




/**
 * @brief Parses the data as weighted edgelist formatted network
 *
 *  This method can read and parse edgelist formated files
 * where edge source and target are either named with numbers or with labels
 * That is the following formats can be parsed:
 * # edgelist with node numbers
 * 1 2 1
 * 1 3 2
 * 1 6 2
 * 1 8 2
 * ...
 *
 * # edgelist with node labels
 * actor1 actor2 1
 * actor2 actor4 2
 * actor1 actor3 1
 * actorX actorY 3
 * name othername 1
 * othername somename 2
 * ...

 * @param rawData
 * @param delimiter
 * @return
 */
bool Parser::parseAsEdgeListWeighted(const QByteArray &rawData, const QString &delimiter){

    qDebug() << "Parsing data as weighted edgelist formatted..." << "column delimiter" << delimiter ;

    QTextCodec *codec = QTextCodec::codecForName( m_textCodecName.toLatin1() );
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);


    QMap<QString, int> nodeMap;
    // use a minimum priority queue to order Actors<QString key, int value> by their value
    // so that we can create the discovered nodes by either their increasing nodeNumber
    // (if nodesWithLabels == true) or by their actual number in the file (if nodesWithLabels == false).
    priority_queue<Actor, vector<Actor>, CompareActors> nodeQ;
    QHash<QString, qreal> edgeList;
    QString str, edgeKey,edgeKeyDelimiter="====>" ;
    QStringList lineElement, edgeElement;
    // one or more digits
    QRegularExpression onlyDigitsExp("^\\d+$");

    bool nodesWithLabels = false;
    bool conversionOK = false;
    int fileLine = 0, actualLineNumber=0;
    totalNodes=0;
    edgeWeight=1.0;
    edgeDirType=EdgeType::Directed;
    arrows=true;
    bezier=false;

    relationsList.clear();

    qDebug()<< "***  Initial file parsing "
               "to test integrity and edge naming scheme";
    while ( !ts.atEnd() )   {

        fileLine ++;

        str= ts.readLine().simplified().trimmed();

        qDebug()<< " simplified str" << str;

        if ( isComment(str) )
            continue;

        actualLineNumber ++ ;

        if (
              actualLineNumber == 1 &&

             ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("graph",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL n",Qt::CaseInsensitive)    //DL format
             || str == "DL"
             || str == "dl"
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
              )
             ) {
            qDebug()<< "Not a Weighted list-formatted file. Aborting!!";

            errorMessage = tr("Not an EdgeList-formatted file. "
                              "A non-comment line includes keywords reserved by other file formats (i.e vertices, graphml, network, graph, digraph, DL, xml)");
            return false;
        }

        lineElement=str.split(delimiter);

        if ( lineElement.size()  != 3 ) {
            qDebug()<< "*** Not a Weighted list-formatted file. Aborting!!";

            errorMessage = tr("Not a properly EdgeList-formatted file. "
                              "Row %1 has not 3 elements as expected (i.e. source, target, weight)")
                    .arg(fileLine);
            return false;
        }

        edge_source = lineElement[0];
        edge_target = lineElement[1];
        edge_weight = lineElement[2];
        qDebug()<< " Dissecting line - "
                   "source:"
                << edge_source
                << "target:"
                << edge_target
                << "weight:"
                << edge_weight;

        if (!edge_source.contains(onlyDigitsExp)) {
            qDebug()<< " node named by non-digit only string. "
                       "nodesWithLabels = true";
            nodesWithLabels = true;
        }

        if (!edge_target.contains(onlyDigitsExp)) {
            qDebug()<< " node named by non-digit only string. "
                       "nodesWithLabels = true";
            nodesWithLabels = true;
        }

    }

    ts.seek(0);
    fileLine = 0;


    qDebug()<< "***  Initial file parsing finished. "
               "This is really a weighted edge list. Proceed to main parsing";

    while ( !ts.atEnd() )   {
        str= ts.readLine() ;

        qDebug()<< " str" << str;

        str=str.simplified();
        qDebug()<< " simplified str" << str;

        if ( isComment(str) )
            continue;

        lineElement=str.split(delimiter);

        edge_source = lineElement[0];
        edge_target = lineElement[1];
        edge_weight = lineElement[2];
        qDebug()<< " Dissecting line - "
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
            qDebug()<< " source, new node named"
                    << edge_source
                    << "totalNodes" << totalNodes
                    << "nodeMap.count"
                    << nodeMap.size();

        }
        else {
            qDebug()<< " source already found, continue";
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
            qDebug()<< " target, new node named"
                    << edge_target
                    << "totalNodes" << totalNodes
                    << "nodeMap.count"
                    << nodeMap.size();

        }
        else {
            qDebug()<< " target already found, continue";
        }

        edgeWeight = edge_weight.toDouble(&conversionOK);
        if (conversionOK) {
            qDebug()<< " read edge weight"
                    << edgeWeight;
        }
        else {
            edgeWeight = 1.0;
            qDebug()<< " error reading edge weight."
                       "Using default edgeWeight"
                    << edgeWeight;
        }
        edgeKey = edge_source + edgeKeyDelimiter + edge_target;
        if ( ! edgeList.contains( edgeKey ) ) {
            qDebug()<< " inserting edgeKey"
                    << edgeKey
                    << "in edgeList with weight" << edgeWeight;
            edgeList.insert( edgeKey, edgeWeight );
            totalLinks++;
        }


    } //end ts.stream while here


    qDebug() << " finished reading file, "
                "start creating nodes and edges";

    //print_queue(nodeQ);

    // create nodes one by one
    while (!nodeQ.empty()) {

        Actor node = nodeQ.top();
        nodeQ.pop();
        randX=rand()%gwWidth;
        randY=rand()%gwHeight;

        if (nodesWithLabels) {
            qDebug() << "signaling to create new node" << node.value
                     << "label" << node.key
                     << "at pos" << QPointF(randX, randY);
            emit signalCreateNode( node.value,
                             initNodeSize,
                             initNodeColor,
                             initNodeNumberColor,
                             initNodeNumberSize,
                             node.key,
                             initNodeLabelColor, initNodeLabelSize,
                             QPointF(randX, randY),
                             initNodeShape,QString()
                             );
        }
        else {

            qDebug() << "signaling to create new node" << node.key.toInt()
                     << "label" << node.key
                     << "at pos" << QPointF(randX, randY);
            emit signalCreateNode( node.key.toInt(),
                             initNodeSize,
                             initNodeColor,
                             initNodeNumberColor,
                             initNodeNumberSize,
                             node.key,
                             initNodeLabelColor, initNodeLabelSize,
                             QPointF(randX, randY),
                             initNodeShape,QString()
                             );

        }

    }

    //create edges one by one
    QHash<QString, qreal>::const_iterator edge = edgeList.constBegin();
    while (edge!= edgeList.constEnd()) {

        qDebug() << " creating edge named"
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
        emit signalCreateEdge(source,
                        target,
                        edgeWeight,
                        initEdgeColor,
                        edgeDirType,
                        arrows,
                        bezier);

        ++edge;
    }



    if (relationsList.size() == 0) {
        emit signalAddNewRelation("unnamed");
    }

    qDebug() << " END. Returning.";
    return true;

}



/**
 * @brief Parses the data as simple edgelist formatted
 *
 * @param rawData
 * @param delimiter
 * @return bool
 */
bool Parser::parseAsEdgeListSimple(const QByteArray &rawData, const QString &delimiter){

    qDebug() << "Parsing data as simple edgelist formatted..." << "column delimiter" << delimiter ;

    QTextCodec *codec = QTextCodec::codecForName( m_textCodecName.toLatin1() );
    QString decodedData = codec->toUnicode(rawData);
    QTextStream ts(&decodedData);


    QString str, edgeKey,edgeKeyDelimiter="====>" ;
    QStringList lineElement,edgeElement;
    int columnCount=0;
    int fileLine = 0, actualLineNumber=0;
    bool nodesWithLabels= false;
    //@TODO Always use nodesWithLabels= true

    QMap<QString, int> nodeMap;
    // use a minimum priority queue to order Actors<QString key, int value> by their value
    // so that we can create the discovered nodes by either their increasing nodeNumber
    // (if nodesWithLabels == true) or by their actual number in the file (if nodesWithLabels == false).
    priority_queue<Actor, vector<Actor>, CompareActors> nodeQ;
    QHash<QString, qreal> edgeList;

    QRegularExpression onlyDigitsExp("^\\d+$");

    totalNodes = 0;
    initEdgeWeight=1.0;

    edgeDirType=EdgeType::Directed;
    arrows=true;
    bezier=false;

    relationsList.clear();

    qDebug()<< "***  Initial file parsing "
               "to test integrity and edge naming scheme";

    while ( !ts.atEnd() )   {

        fileLine++;

        str= ts.readLine().simplified().trimmed();

        qDebug()<< " line "  << fileLine
                << "\n" << str;

        if ( isComment(str) ) {
            continue;
        }

        actualLineNumber ++;

        if ( actualLineNumber == 1 &&
             ( str.contains("vertices",Qt::CaseInsensitive)
             || str.contains("network",Qt::CaseInsensitive)
             || str.contains("graph",Qt::CaseInsensitive)
             || str.contains("digraph",Qt::CaseInsensitive)
             || str.contains("DL n",Qt::CaseInsensitive)
             || str == "DL"
             ||  str == "dl"
             || str.contains("list",Qt::CaseInsensitive)
             || str.contains("graphml",Qt::CaseInsensitive)
             || str.contains("xml",Qt::CaseInsensitive)
              )
             ) {
            qDebug()<< "*** Not an EdgeList-formatted file. Aborting!!";
            errorMessage = tr("Not an EdgeList-formatted file. "
                              "Non-comment line %1 includes keywords reserved by other file formats (i.e vertices, graphml, network, graph, digraph, DL, xml)")
                    .arg(fileLine);

            return false;
        }

        lineElement=str.split(delimiter);

        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            edge_source = (*it1);
            if (!edge_source.contains(onlyDigitsExp)) {
                qDebug()<< " node named by non-digit only string. "
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

    qDebug () << " Reset and read lines. nodesWithLabels"
                 << nodesWithLabels;

    while ( !ts.atEnd() )   {
        fileLine++;
        str= ts.readLine() ;

        str=str.simplified();

        qDebug()<< " line" << fileLine
                << "\n" << str;

        if ( isComment(str) )
            continue;

        lineElement=str.split(delimiter);

        columnCount = 0;
        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            columnCount ++;
            if (columnCount == 1) {  // source node
                edge_source = (*it1);
                qDebug()<< " Dissecting line - "
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
                    qDebug()<< " source, new node named"
                            << edge_source
                            << "totalNodes" << totalNodes
                            << "nodeMap.count"
                            << nodeMap.size();

                }
                else {
                    qDebug()<< " source already found, continue";
                }
            }
            else { // target nodes
                edge_target= (*it1);
                qDebug()<< " Dissecting line - "
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
                    qDebug()<< " target, new node named"
                            << edge_target
                            << "totalNodes" << totalNodes
                            << "nodeMap.count"
                            << nodeMap.size();

                }
                else {
                    qDebug()<< " target already found, continue";
                }

            }

            if (columnCount > 1)  {
                edgeKey = edge_source + edgeKeyDelimiter + edge_target;
                if ( ! edgeList.contains( edgeKey ) ) {
                    qDebug()<< " inserting edgeKey"
                            << edgeKey
                            << "in edgeList with initial weight" << initEdgeWeight;
                    edgeList.insert( edgeKey, initEdgeWeight );
                    totalLinks++;
                }
                else { // if edge already discovered, then increase its weight by 1
                    edgeWeight = edgeList.value(edgeKey);
                    edgeWeight = edgeWeight + 1;
                    qDebug()<< " edgeKey"
                            << edgeKey
                            << "found before, adding in edgeList with increased weight"
                            << edgeWeight;

                    edgeList.insert( edgeKey, edgeWeight );
                }
            }

        }  // end for QStringList::Iterator

    } //end ts.stream while here

    // create nodes one by one
    while (!nodeQ.empty()) {

        Actor node = nodeQ.top();
         nodeQ.pop();
         randX=rand()%gwWidth;
         randY=rand()%gwHeight;

         if (nodesWithLabels) {
             qDebug() << "signaling to create new node" << node.value
                      << "label" << node.key
                      << "at pos" << QPointF(randX, randY);
             emit signalCreateNode( node.value,
                              initNodeSize,
                              initNodeColor,
                              initNodeNumberColor,
                              initNodeNumberSize,
                              node.key,
                              initNodeLabelColor, initNodeLabelSize,
                              QPointF(randX, randY),
                              initNodeShape,QString()
                              );
         }
         else {

             qDebug() << "signaling to create new node"
                      << node.key.toInt()
                      << "label" << node.key
                      << "at pos" << QPointF(randX, randY);
             emit signalCreateNode( node.key.toInt(),
                              initNodeSize,
                              initNodeColor,
                              initNodeNumberColor,
                              initNodeNumberSize,
                              node.key,
                              initNodeLabelColor, initNodeLabelSize,
                              QPointF(randX, randY),
                              initNodeShape,QString()
                              );

         }

     }


    //create edges one by one
    QHash<QString, qreal>::const_iterator edge = edgeList.constBegin();
     while (edge!= edgeList.constEnd()) {

         qDebug() << " creating edge named"
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
         emit signalCreateEdge(source,
                         target,
                         edgeWeight,
                         initEdgeColor,
                         edgeDirType,
                         arrows,
                         bezier);

         ++edge;
     }


     if (relationsList.size() == 0) {
         emit signalAddNewRelation("unnamed");
     }

    qDebug() << " Finished OK. Returning.";
    return true;

}




//
/**
 * @brief Helper. Checks if the string parameter is a comment (starts with a known char, i.e #).
 *
 * @param str
 * @return  bool
 */
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
