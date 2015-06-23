/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.9
 Written in Qt
 
                         parser.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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
#include <list>
#include "graph.h"	//needed for setParent

Parser::Parser( const QString fn,
                   const QString m_codec,
                   const int iNS, const QString iNC, const QString iNSh,
                   const QString iNNC, const int iNNS,
                   const QString iNLC, const int iNLS ,
                   const QString iEC,
                   const int width, const int height,
                   const int fFormat,
                   const int sm_mode
                   )
{
    qDebug() << "Parser::load() fn: " << fn
                << "running on thread "  << this->thread() ;
    initNodeSize=iNS;
    initNodeColor=iNC;
    initNodeShape=iNSh;
    initNodeNumberColor=iNNC;
    initNodeNumberSize=iNNS;
    initNodeLabelColor=iNLC;
    initNodeLabelSize=iNLS;

    initEdgeColor=iEC;

    undirected=0; arrows=false; bezier=false;
    fileName=fn;
    userSelectedCodecName = m_codec;
    networkName=(fileName.split ("/")).last();
    gwWidth=width;
    gwHeight=height;
    randX=0;
    randY=0;
    fileFormat= fFormat;
    two_sm_mode = sm_mode;
    xml=0;

}



Parser::~Parser () {
    qDebug()<< "**** Parser::~Parser() destructor " << this->thread()
                <<" clearing hashes... ";
    nodeNumber.clear();
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

/** starts the new thread calling the load* methods
*/
bool Parser::run()  {
    qDebug()<< "**** Parser:: run(). on a new thread " << this->thread()
            << " networkName "<< networkName
            << " fileFormat "<< fileFormat ;

    switch (fileFormat){
    case 1:	//GraphML
        if (loadGraphML()){
            qDebug("* Parser: that was  a GraphML network");
        }
        else fileFormat=-1;
        break;
    case 2: //Pajek
        if ( loadPajek() ) {
            qDebug("* Parser: that was a Pajek network");
        }
        else fileFormat=-1;
        break;
    case 3: //Adjacency
        if (loadAdjacency() ) {
            qDebug("* Parser: that was an adjacency-matrix network");
        }
        else fileFormat=-1;
        break;
    case 4: //Dot
        if (loadDot() ) {
            qDebug("* Parser: that was a GraphViz (dot) network");
        }
        else fileFormat=-1;
        break;
    case 5:	//GML
        if (loadGML() ){
            qDebug("* Parser: that was a GML (gml) network");
        }
        else fileFormat=-1;
        break;
    case 6: //DL
        if (loadDL() ){
            qDebug("Parser: this is a DL formatted (.dl) network");
        }
        else fileFormat=-1;
        break;

    case 7:	// Weighted List
        if (loadWeighedList() ){
            qDebug("Parser: this is a weighted list formatted (.list) network");
        }
        else fileFormat=-1;
        break;

    case 8:	// List
        if (loadSimpleList() ){
            qDebug("Parser: this is a simple list formatted (.list) network");
        }
        else fileFormat=-1;
        break;

    case 9:	// twomode sociomatrix, affiliation network matrix
        if (loadTwoModeSociomatrix() ){
            qDebug("Parser: OK, this is a two-mode sociomatrix (.tsm) network");
        }
        else fileFormat=-1;
        break;

    default:	//GraphML
        if (loadGraphML() ){
            qDebug("Parser: this is a GraphML network");
        }
        else fileFormat=-1;
        break;
    }

    qDebug()<< "**** Parser::run() - we return back to Graph and MW! "
                        << " fileFormat now "<< fileFormat ;

    emit finished ("Parser::run() - reach end");
    return (fileFormat==-1) ? false: true;
}



void Parser::createRandomNodes(int nodeNum=1,QString label=NULL, int totalNodes=1){
    if (totalNodes != 1 ) {
        int i=0;
        for (i=0; i<totalNodes; i++) {
            randX=rand()%gwWidth;
            randY=rand()%gwHeight;
            nodeLabel = QString::number(i+1);
            qDebug()<<"Creating node: "<< i+1
                   << " with label: " << nodeLabel
                   << " at "<< randX<<","<< randY;
            emit createNode(
                        i+1, initNodeSize,initNodeColor,
                        initNodeNumberColor, initNodeNumberSize,
                        nodeLabel, initNodeLabelColor, initNodeLabelSize,
                        QPointF(randX, randY),
                        initNodeShape, false
                        );
        }
    }
    else {
        randX=rand()%gwWidth;
        randY=rand()%gwHeight;
        if (label.isEmpty()) {
            nodeLabel= QString::number(nodeNum+1);
        }
        else
            nodeLabel=label;
        qDebug()<<"Creating node: "<< nodeNum
               << " with label: " << nodeLabel
               << " at "<< randX<<","<< randY;
        emit createNode(
                    nodeNum, initNodeSize,initNodeColor,
                    initNodeNumberColor, initNodeNumberSize,
                    nodeLabel, initNodeLabelColor, initNodeLabelSize,
                    QPointF(randX, randY),
                    initNodeShape, false
                    );

    }
}

/**
    Tries to load a file as DL-formatted network (UCINET)
    If not it returns -1
*/
bool Parser::loadDL(){

    qDebug () << "\n\nParser: loadDL (UCINET)";
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str, label, nm_str, relation, prevLineStr;

    int source=1, target=1, nm=0,lineCounter=0, mark=0, mark2=0, nodeSum=0;
    int relationCounter=0;
    edgeWeight=0;
    bool labels_flag=false, data_flag=false, intOK=false, floatOK=false;
    bool relation_flag=false;
    bool fullmatrixFormat=false;
    bool edgelist1Format=false;
    QStringList lineElement, labelsList, relationsList;
    totalLinks=0;

    while ( !ts.atEnd() )   {
        str= ts.readLine();
        str=str.simplified();
        lineCounter++;
        qDebug() << "Parser: loadDL - lineCounter: " << lineCounter
                 << "\n  str: " << str
                 << "\n  removing whitespaces from str "<< str;

        if ( isComment(str) )
            continue;

        if ( (lineCounter == 1) &&
             (!str.startsWith("DL",Qt::CaseInsensitive)  ) ) {
            qDebug("*** Parser-loadDL(): not a DL file. Aborting!");
            file.close();
            return false;
        }

        if (str.startsWith( "N=", Qt::CaseInsensitive)
                ||  str.startsWith( "N =", Qt::CaseInsensitive) )  {
            mark=str.indexOf("=");
            qDebug() << "Network size is declared here - check if NM exists";
            if ( (mark2=str.indexOf("NM", Qt::CaseInsensitive)) != -1 ) {
                nm_str = str.right(str.size() - mark2 );
                qDebug() << " NM exists at " << mark2
                         << " contains " << nm_str;
                nm_str = nm_str.simplified();
                nm_str.remove(0,2);
                nm_str = nm_str.simplified();
                if (nm_str.startsWith("="))
                    nm_str.remove(0,1);
                nm = nm_str.toInt(&intOK,10);
                qDebug() << " NM str: " << nm_str
                         << " and toInt:"<<nm ;
                if (!intOK) {
                    qDebug() <<
                                "Parser: loadDL(): NM conversion error..." ;
                    //emit something here...
                    return false;
                }
                str.truncate(mark2);
                str=str.trimmed();
                qDebug() << " rest str becomes: " << str;
            }
            str=str.right(str.size()-mark-1);
            qDebug()<< "N is declared to be : " << str.toLatin1() ;
            aNodes=str.toInt(&intOK,10);
            if (!intOK) {
                qDebug() <<
                            "Parser: loadDL(): N conversion error..." ;
                return false;
            }
            qDebug() << " FINALLY N="<<aNodes << "NM="<<nm;
            continue;
        }

        if (str.startsWith( "FORMAT =", Qt::CaseInsensitive)
                || str.startsWith( "FORMAT=", Qt::CaseInsensitive))  {
            mark=str.indexOf("=");
            str=str.right(str.size()-mark-1);
            str=str.trimmed();
            qDebug()<<  "FORMAT = : " <<  str.toLatin1() ;
            if (str.contains("FULLMATRIX",Qt::CaseInsensitive)) {
                fullmatrixFormat=true;
            }
            else if (str.contains("edgelist",Qt::CaseInsensitive) ){
                edgelist1Format=true;
            }
            continue;
        }
        else if (str.startsWith( "labels", Qt::CaseInsensitive)
                 || str.startsWith( "row labels", Qt::CaseInsensitive)) {
            labels_flag=true; data_flag=false;relation_flag=false;
            qDebug() << " START LABELS RECOGNITION AND NODE CREATION";
            continue;
        }
        else if (str.startsWith( "COLUMN LABELS", Qt::CaseInsensitive)) {
            labels_flag=true; data_flag=false;relation_flag=false;
            qDebug() << " START COLUMN LABELS RECOGNITION AND NODE CREATION";
            continue;
        }
        else if ( str.startsWith( "data:", Qt::CaseInsensitive)
                  || str.startsWith( "data :", Qt::CaseInsensitive) ) {
            data_flag=true; labels_flag=false;relation_flag=false;
            qDebug() << " START DATA RECOGNITION AND EDGE CREATION";
            continue;
        }
        else if (str.startsWith( "LEVEL LABELS", Qt::CaseInsensitive) ) {
            relation_flag=true; data_flag=false; labels_flag=false;
            qDebug() << " START RELATIONS RECOGNITION";
            continue;
        }
        else if (str.isEmpty()){
            qDebug() << " EMPTY STRING - CONTINUE";
            continue;
        }


        if (labels_flag) {  //read a label and create a node in a random position
            label=str;
            if ( labelsList.contains(label) ) {
                qDebug() << " label exists. CONTINUE";
                continue;
            }
            else{
                qDebug() << "adding label " << label << " to labelList";
                labelsList << label;
            }
            nodeSum++;
            createRandomNodes(nodeSum, label,1);

        }
        if ( relation_flag){
            relation=str;
            if ( relationsList.contains(relation) ) {
                qDebug() << " relation exists. CONTINUE";
                continue;
            }
            else{
                qDebug() << "adding relation "<< relation
                         << " to relationsList and emitting addRelation ";
                relationsList << relation;
                emit addRelation( relation );
            }
        }
        if ( data_flag && fullmatrixFormat){		//read edges in matrix format
            // check if we haven't created any nodes...
            if ( nodeSum < aNodes ){
                qDebug() << " nodes have not been created yet. "
                         << " calling createRandomNodes()" ;
                createRandomNodes(1, QString::null, aNodes);
                nodeSum = aNodes;
            }
            //SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS)
            if (!prevLineStr.isEmpty()) {
                str=(prevLineStr.append(" ")).append(str) ;
                qDebug() << " prevLineStr not empty - prepending it to str - "
                         << " new str: \n" << str;
                str=str.simplified();
            }
            lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);

            if (lineElement.count() < aNodes ) {
                qDebug() << "This line has fewer than " << aNodes << " edges";
                prevLineStr=str;
                continue;
            }
            prevLineStr.clear();
            target=1;
            if (source==1){
                relation = relationsList[ relationCounter ];
                qDebug() << " WE ARE THE FIRST DATASET/MATRIX"
                         << " source node counter is " << source
                         << " and relation to " << relation<< ": "
                         << relationCounter;
                emit changeRelation (relationCounter);
            }
            else if (source>aNodes) {
                source=1;
                relationCounter++;
                relation = relationsList[ relationCounter ];
                qDebug() << " LOOKS LIKE WE ENTERED A NEW DATASET/MATRIX "
                         << " init source node counter to " << source
                         << " and relation to " << relation << ": "
                         << relationCounter;
                emit changeRelation (relationCounter);
            }
            else {
                qDebug() << "source node counter is " << source;
            }

            for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
                //qDebug()<< (*it1).toLatin1() ;
                if ( (*it1)!="0"){
                    edgeWeight=(*it1).toFloat(&floatOK);
                    qDebug()<<  "Parser-loadDL(): relation "
                             << relationCounter
                             << " found edge from "
                             << source << " to " << target
                             << " weight " << edgeWeight
                             << " emitting createEdge() to parent" ;

                    undirected=0;
                    arrows=true;
                    bezier=false;
                    emit createEdge( source, target, edgeWeight, initEdgeColor, undirected, arrows, bezier);
                    totalLinks++;
                    qDebug() << "TotalLinks= " << totalLinks;

                }
                target++;
            }
            source++;


        }
        if (data_flag && edgelist1Format) { //read edges in edgelist1 format
            // check if we haven't created any nodes...
            if ( nodeSum < aNodes ){
                qDebug() << " nodes have not been created yet. "
                         << " calling createRandomNodes()" ;
                createRandomNodes(1, QString::null, aNodes);
                nodeSum = aNodes;
            }
            lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);

            if ( lineElement.count() != 3 ) {
                qDebug()<< "*** Parser:loadDL(): Not an edgelist1 UCINET formatted file. Aborting!!";
                file.close();
                //emit something...
                return false;
            }

            source =  (lineElement[0]).toInt(&intOK);
            target =  (lineElement[1]).toInt(&intOK);
            qDebug() << "	source node " << source  << " target node " << target;

            edgeWeight=(lineElement[2]).toDouble(&intOK);
            if (intOK) {
                qDebug () << "	list file declares edge weight: " << edgeWeight;
            }
            else {
                edgeWeight=1.0;
                qDebug () << "	list file NOT declaring edge weight. Setting default: " << edgeWeight;
            }

            qDebug()<< "	Parser::loadDL() - Creating link "
                    << source << " -> "<< target << " weight= "<< edgeWeight
                    <<  " TotalLinks=  " << totalLinks+1;
            emit createEdge(source, target, edgeWeight, initEdgeColor, undirected, arrows, bezier);
            totalLinks++;
        }
    }
    //sanity check
    if (nodeSum != aNodes) {
        qDebug()<< "Error: aborting";
        //emit something
        return false;
    }
    //The network has been loaded. Tell MW the statistics and network type
    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: List
    emit changeRelation (0);
    emit fileType(5, networkName, aNodes, totalLinks, undirected);
    qDebug() << "Parser-loadDL() clearing";
    lineElement.clear(); labelsList.clear(); relationsList.clear();
    return true;

}

/**
    Tries to load the file as Pajek-formatted network. If not it returns -1
*/
bool Parser::loadPajek(){

    qDebug ("\n\nParser: loadPajek");
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str, label, temp;
    nodeColor="";
    edgeColor="";
    nodeShape="";
    QStringList lineElement;
    bool ok=false, intOk=false, check1=false, check2=false;
    bool nodes_flag=false, edges_flag=false, arcs_flag=false, arcslist_flag=false, matrix_flag=false;
    fileContainsNodeColors=false;
    fileContainsNodeCoords=false;
    fileContainsLinkColors=false;
    bool zero_flag=false;
    int   i=0, j=0, miss=0, source= -1, target=-1, nodeNum, colorIndex=-1, coordIndex=-1;
    unsigned long int lineCounter=0;
    int pos=-1, relationCounter=0;
    float weight=1;
    QString relation;
    list<int> listDummiesPajek;
    totalLinks=0;
    aNodes=0;
    j=0;  //counts how many real nodes exist in the file
    miss=0; //counts missing nodeNumbers.
    //if j + miss < nodeNum, it creates (nodeNum-miss) dummy nodes which are deleted in the end.
    QList <int> toBeDeleted;  //FIXME ?
    QStringList relationsList;


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
                if (!lineElement[1].isEmpty()) 	aNodes=lineElement[1].toInt(&intOk,10);
                qDebug ("Parser-loadPajek(): Vertices %i.",aNodes);
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
            arcs_flag=true; edges_flag=false; arcslist_flag=false; matrix_flag=false;
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
            //check if pajek file has label for matrix data,
            // and use it as relation name
            if ( (pos = str.indexOf(":")) == 8 ) {
                relation = str.right(str.size() - pos -1) ;
                relation = relation.simplified();
                qDebug() << "Parser::loadPajek() - adding relation "<< relation
                         << " to relationsList and emitting addRelation ";
                relationsList << relation;
                emit addRelation( relation );
                if (relationCounter > 0) {
                    qDebug () << "Parser::loadPajek() relationCounter "
                              << relationCounter
                              << "emitting changeRelation";
                    emit changeRelation(relationCounter);
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

                /** NODESHAPE: There are four possible . */
                if (str.contains("Ellipse", Qt::CaseInsensitive) ) nodeShape="ellipse";
                else if (str.contains("circle", Qt::CaseInsensitive) ) nodeShape="circle";
                else if (str.contains("box", Qt::CaseInsensitive) ) nodeShape="box";
                else if (str.contains("triangle", Qt::CaseInsensitive) ) nodeShape="triangle";
                else nodeShape="diamond";
                /** NODECOLORS */
                //if there is an "ic" tag, a specific NodeColor for this node follows...
                if (str.contains("ic",Qt::CaseInsensitive)) {
                    for (register int c=0; c< lineElement.count(); c++) {
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
                    for (register int c=0; c< lineElement.count(); c++)   {
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
                return false;
            }

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
            if (j && j!=aNodes)  {  //if there were more or less nodes than the file declared
                qDebug()<<"*** WARNING ***: The Pajek file declares " << aNodes <<"  nodes, but I found " <<  j << " nodes...." ;
                aNodes=j;
            }
            else if (j==0) {  //if there were no nodes at all, we need to create them now.
                qDebug()<< "The Pajek file declares "<< aNodes<< " but I didnt found any nodes. I will create them....";
                for (int num=j+1; num<= aNodes; num++) {
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
                j=aNodes;
            }
            if (edges_flag && !arcs_flag)   {  /**EDGES */
                //qDebug("Parser-loadPajek(): ==== Reading edges ====");
                qDebug()<<lineElement;
                source =  lineElement[0].toInt(&ok, 10);
                target = lineElement[1].toInt(&ok,10);
                if (source == 0 || target == 0 ) return false;  //  i --> (i-1)   internally
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
                    colorIndex=lineElement.indexOf( QRegExp("[c]"), 0 )  +1;
                    if (colorIndex >= lineElement.count()) edgeColor=initEdgeColor;
                    else 	edgeColor=lineElement [ colorIndex ];
                    if (edgeColor.contains (".") )  edgeColor=initEdgeColor;
                    //qDebug()<< " current color "<< edgeColor;
                }
                else  {
                    //qDebug("Parser-loadPajek(): file with no link colours");
                    edgeColor=initEdgeColor;
                }
                undirected=2;
                arrows=true;
                bezier=false;
                qDebug()<< "Parser-loadPajek(): EDGES: Create edge between " << source << " - "<< target;
                emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
                totalLinks=totalLinks+2;

            } //end if EDGES
            else if (!edges_flag && arcs_flag)   {  /** ARCS */
                //qDebug("Parser-loadPajek(): === Reading arcs ===");
                source=  lineElement[0].toInt(&ok, 10);
                target = lineElement[1].toInt(&ok,10);
                if (source == 0 || target == 0 ) return false;   //  i --> (i-1)   internally
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
                undirected=0;
                arrows=true;
                bezier=false;
                qDebug()<<"Parser-loadPajek(): ARCS: Creating arc from node "<< source << " to node "<< target << " with weight "<< weight;
                emit createEdge(source, target, edgeWeight , edgeColor, undirected, arrows, bezier);
                totalLinks++;
            } //else if ARCS
            else if (arcslist_flag)   {  /** ARCSlist */
                //qDebug("Parser-loadPajek(): === Reading arcs list===");
                if (lineElement[0].startsWith("-") ) lineElement[0].remove(0,1);
                source= lineElement[0].toInt(&ok, 10);
                fileContainsLinkColors=false;
                edgeColor=initEdgeColor;
                undirected=0;
                arrows=true;
                bezier=false;
                for (register int index = 1; index < lineElement.size(); index++) {
                    target = lineElement.at(index).toInt(&ok,10);
                    qDebug()<<"Parser-loadPajek(): ARCS LIST: Creating ARC source "<< source << " target "<< target << " with weight "<< weight;
                    emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
                    totalLinks++;
                }
            } //else if ARCSLIST
            else if (matrix_flag)   {  /** matrix */
                //qDebug("Parser-loadPajek(): === Reading matrix of edges===");
                i++;
                source= i;
                fileContainsLinkColors=false;
                edgeColor=initEdgeColor;
                undirected=0;
                arrows=true;
                bezier=false;
                for (target = 0; target < lineElement.size(); target ++) {
                    if ( lineElement.at(target) != "0" ) {
                        edgeWeight  = lineElement.at(target).toFloat(&ok);
                        qDebug()<<"Parser-loadPajek():  MATRIX: Creating arc source "
                               << source << " target "<< target +1
                               << " with weight "<< weight;
                        emit createEdge(source, target+1, edgeWeight, edgeColor,
                                        undirected, arrows, bezier);
                        totalLinks++;
                    }
                }
            } //else if matrix
        } //end if BOTH ARCS AND EDGES
    } //end WHILE
    file.close();
    if (j==0) return false;
    //The network has been loaded. Tell MW the statistics and network type
    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: List
    emit fileType(2, networkName, aNodes, totalLinks, undirected);

    qDebug("Parser-loadPajek(): Removing all dummy aNodes, if any");
    if (listDummiesPajek.size() > 0 ) {
        qDebug("Trying to delete the dummies now");
        for ( list<int>::iterator it=listDummiesPajek.begin(); it!=listDummiesPajek.end(); it++ ) {
            emit removeDummyNode(*it);
        }
    }
    qDebug("Parser-loadPajek(): Clearing DumiesList from Pajek");
    listDummiesPajek.clear();
    relationsList.clear();
    emit changeRelation (0);

    return true;

}






/**
    Tries to load the file as adjacency sociomatrix-formatted. If not it returns -1
*/
bool Parser::loadAdjacency(){
    qDebug("\n\nParser: loadAdjacency()");
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str;
    QStringList lineElement;
    int i=0, j=0,  aNodes=0, newCount=0, lastCount=0;
    edgeWeight=1.0;
    bool intOK=false;
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
            aNodes=lineElement.count();
            qDebug("Parser-loadAdjacency(): There are %i nodes in this file", aNodes);
            for (j=0; j<aNodes; j++) {

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
        if ( aNodes != (int) lineElement.count() ) return false;
        j=0;
        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            if ( (*it1)!="0"){
                edgeWeight =(*it1).toFloat(&intOK);
                undirected=0;
                arrows=true;
                bezier=false;
                emit createEdge(i+1, j+1, edgeWeight, initEdgeColor, undirected, arrows, bezier);
                totalLinks++;
                qDebug() << "Parser-loadAdjacency(): Link from i=" << i+1 << " to j=" <<  j+1
                         << "weight=" << edgeWeight << ". TotalLinks= " << totalLinks;
            }

            j++;
        }
        i++;
    }
    file.close();

    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: List
    qDebug() << "Parser: SM network has been loaded. Tell MW the statistics and network type";
    emit fileType(3, networkName, aNodes, totalLinks, undirected);

    return true;

}




/**
    Tries to load the file as two-mode sociomatrix. If not it returns -1
*/
bool Parser::loadTwoModeSociomatrix(){
    qDebug("\n\nParser: loadTwoModeSociomatrix()");
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly ))
        return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str;
    QStringList lineElement;
    int i=0, j=0,  aNodes=0, newCount=0, lastCount=0;
    edgeWeight=1.0;

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
                QList<int> values;
                for (int k = 1; k < i ; ++k) {
                    qDebug() << "Checking earlier discovered actor k = " << k;
                    if ( firstModeMultiMap.contains(k, j) ) {
                        undirected=2;
                        arrows=true;
                        bezier=false;
                        edgeWeight = 1;
                        qDebug() << " Actor " << i << " on the same event as actor " << k << ". Creating edge ";
                        emit createEdge(i, k, edgeWeight, initEdgeColor, undirected, arrows, bezier);
                        totalLinks++;
                    }
                }

            }
            j++;
        }
    }
    file.close();
    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: List, 8 List, 9, TwoModeSociomatrix
    qDebug() << "Parser: Two-mode SM network has been loaded. Tell MW the statistics and network type";
    emit fileType(9, networkName, aNodes, totalLinks, undirected);

    return true;

}




/**
    Tries to load a file as GraphML (not GML) formatted network.
    If not it returns -1
*/
bool Parser::loadGraphML(){

    qDebug("\n\nParser: loadGraphML()");
    aNodes=0;
    totalLinks=0;
    nodeNumber.clear();
    bool_key=false; bool_node=false; bool_edge=false;
    key_id = "";
    key_name = "";
    key_type = "";
    key_value = "";
    initEdgeWeight = 1;
    edgeWeight=1;
    edgeColor="black";
    arrows=true;
    undirected=0;
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly ))
        return false;

    QXmlStreamReader *xml = new QXmlStreamReader();

    qDebug() << " loadGraphML(): read file to a byte array";
    QByteArray encodedData = file.readAll();
    QByteArray userSelectedCodec =userSelectedCodecName.toLatin1();
    xml->addData(encodedData);

    qDebug() << " loadGraphML(): test if XML document encoding == userCodec";

    xml->readNext();
    if (xml->isStartDocument()) {
        qDebug()<< " loadGraphML(): Testing XML document " << " version "
                << xml->documentVersion()
                << " encoding " << xml->documentEncoding()
                << " userSelectedCodecName.toUtf8() "
                << userSelectedCodecName.toUtf8();
         if ( xml->documentEncoding().toString() != userSelectedCodecName) {
                qDebug() << " loadGraphML(): Conflicting encodings. "
                         << " Re-reading data with userCodec";
                xml->clear();
                QTextStream in(&encodedData);
                in.setAutoDetectUnicode(false);
                QTextCodec *codec = QTextCodec::codecForName( userSelectedCodec );
                in.setCodec(codec);
                QString decodedData = in.readAll();
                xml->addData(decodedData);
         }
         else {
             qDebug() << " loadGraphML(): Testing XML: OK";
             xml->clear();
             xml->addData(encodedData);
         }
    }


    while (!xml->atEnd()) {
        xml->readNext();
        qDebug()<< " loadGraphML(): xml->token "<< xml->tokenString();
        if (xml->isStartDocument()) {
            qDebug()<< " loadGraphML(): xml startDocument" << " version "
                    << xml->documentVersion()
                    << " encoding " << xml->documentEncoding();
        }

        if (xml->isStartElement()) {
            qDebug()<< " loadGraphML(): element name "<< xml->name().toString();

            if (xml->name() == "graphml") {
                qDebug()<< " loadGraphML(): OK. NamespaceUri is "
                        << xml->namespaceUri().toString();
                readGraphML(*xml);
            }
            else {	//not a GraphML doc, return false.
                xml->raiseError(
                            QObject::tr(" loadGraphML(): not a GraphML file."));
                qDebug()<< "*** loadGraphML(): Error in startElement "
                        << " The file is not an GraphML version 1.0 file ";
                file.close();
                return false;
            }
        }
        else if  ( xml->tokenString() == "Invalid" ){
            xml->raiseError(
                        QObject::tr(" loadGraphML(): invalid GraphML or encoding."));
            qDebug()<< "*** loadGraphML(): Cannot find  startElement"
                    << " The file is not valid GraphML or has invalid encoding";
            file.close();
            return false;
        }
    }

    //The network has been loaded. Tell MW the statistics and network type
    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: List
    emit fileType(1, networkName, aNodes, totalLinks, undirected);
    //clear our mess - remove every hash element...
    keyFor.clear();
    keyName.clear();
    keyType.clear();
    keyDefaultValue.clear();
    nodeNumber.clear();
    return true;
}


/*
 * Called from loadGraphML
 * This method checks the xml token name and calls the appropriate function.
 */
void Parser::readGraphML(QXmlStreamReader &xml){
    qDebug()<< " Parser: readGraphML()";
    bool_node=false;
    bool_edge=false;
    bool_key=false;
    //Q_ASSERT(xml.isStartElement() && xml.name() == "graph");

    while (!xml.atEnd()) { //start reading until QXmlStreamReader end().
        xml.readNext();	//read next token

        if (xml.isStartElement()) {	//new token (graph, node, or edge) here
            qDebug()<< "\n  readGraphML(): start of element: "<< xml.name().toString() ;
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
            qDebug()<< "  readGraphML():  element ends here: "<< xml.name().toString() ;
            if (xml.name() == "node")	//node definition end
                endGraphMLElementNode(xml);
            else if (xml.name() == "edge")	//edge definition end
                endGraphMLElementEdge(xml);
        }
    }
    // call createEdgesMissingNodes() to create any edges with missing nodes
    createEdgesMissingNodes();


}


// this method reads a graph definition 
// called at Graph element
void Parser::readGraphMLElementGraph(QXmlStreamReader &xml){
    qDebug()<< "   Parser: readGraphMLElementGraph()";
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    QString defaultDirection = xmlStreamAttr.value("edgedefault").toString();
    qDebug()<< "    edgedefault "<< defaultDirection;
    if (defaultDirection=="undirected"){
        undirected = 2;
        arrows=false;
    }
    else {
        undirected = 0;
        arrows=true;
    }
    networkName = xmlStreamAttr.value("id").toString();
    qDebug()<< "    graph id  "  << networkName; //store graph id to return it afterwards
}



// this method is needed because the QXmlStreamReader::hasAttribute
// has been implemented in Qt 4.5. Therefore we need this ugly hack to 
// be able to compile SocNetV in all previous Qt4 version. :(
//FIXME: This will be obsolete soon
bool Parser::xmlStreamHasAttribute( QXmlStreamAttributes &xmlStreamAttr, QString str) const
{
    int size = xmlStreamAttr.size();
    for (register int  i = 0 ; i < size ; i++) {
        qDebug() << "		xmlStreamHasAttribute(): "
                 << xmlStreamAttr.at(i).name().toString() << endl;
        if ( xmlStreamAttr.at(i).name() == str)
            return true;
    }
    return false;
}



// this method reads a key definition 
// called at key element
void Parser::readGraphMLElementKey ( QXmlStreamAttributes &xmlStreamAttr )
{
    qDebug()<< "   Parser: readGraphMLElementKey()";
    key_id = xmlStreamAttr.value("id").toString();
    qDebug()<< "    key id "<< key_id;
    key_what = xmlStreamAttr.value("for").toString();
    keyFor [key_id] = key_what;
    qDebug()<< "    key for "<< key_what;

    // if (xmlStreamAttr.hasAttribute("attr.name") ) {  // to be enabled in later versions..
    if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("attr.name") ) ) {
        key_name =xmlStreamAttr.value("attr.name").toString();
        keyName [key_id] = key_name;
        qDebug()<< "    key attr.name "<< key_name;
    }
    //if (xmlStreamAttr.hasAttribute("attr.type") ) {
    if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("attr.type") ) ) {
        key_type=xmlStreamAttr.value("attr.type").toString();
        keyType [key_id] = key_type;
        qDebug()<< "    key attr.type "<< key_type;
    }
    //else if (xmlStreamAttr.hasAttribute("yfiles.type") ) {
    else if ( xmlStreamHasAttribute( xmlStreamAttr , QString ("yfiles.type") ) ) {
        key_type=xmlStreamAttr.value("yfiles.type").toString();
        keyType [key_id] = key_type;
        qDebug()<< "    key yfiles.type "<< key_type;
    }

}


// this method reads default key values 
// called at a default element (usually nested inside key element)
void Parser::readGraphMLElementDefaultValue(QXmlStreamReader &xml) {
    qDebug()<< "   Parser: readGraphMLElementDefaultValue()";

    key_value=xml.readElementText();
    keyDefaultValue [key_id] = key_value;	//key_id is already stored
    qDebug()<< "    key default value is "<< key_value;
    if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "    this key default value "<< key_value << " is for node size";
        conv_OK=false;
        initNodeSize= key_value.toInt(&conv_OK);
        if (!conv_OK) initNodeSize = 8;
    }
    if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "    this key default value "<< key_value << " is for nodes shape";
        initNodeShape= key_value;
    }
    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "    this key default value "<< key_value << " is for nodes color";
        initNodeColor= key_value;
    }
    if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "    this key default value "<< key_value << " is for node labels color";
        initNodeLabelColor= key_value;
    }
    if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "    this key default value "<< key_value << " is for node labels size";
        conv_OK=false;
        initNodeLabelSize= key_value.toInt(&conv_OK);
        if (!conv_OK) initNodeLabelSize = 8;
    }
    if (keyName.value(key_id) == "weight" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "    this key default value "<< key_value << " is for edges weight";
        conv_OK=false;
        initEdgeWeight= key_value.toFloat(&conv_OK);
        if (!conv_OK)
            initEdgeWeight = 1;
    }
    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "    this key default value "<< key_value << " is for edges color";
        initEdgeColor= key_value;
    }

}



// this method reads basic node attributes and sets the nodeNumber.
// called at the start of a node element
void Parser::readGraphMLElementNode(QXmlStreamReader &xml){
    QXmlStreamAttributes xmlStreamAttr = xml.attributes();
    node_id = (xmlStreamAttr.value("id")).toString();
    aNodes++;
    qDebug()<<"   Parser: readGraphMLElementNode() node id "<<  node_id
           << " index " << aNodes << " added to nodeNumber map";

    nodeNumber[node_id]=aNodes;

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

    qDebug()<<"   Parser: endGraphMLElementNode() *** signal to create node "
           << " nodenumber "<< aNodes  << " id " << node_id
           << " label " << nodeLabel << " coords " <<randX << ", " <<randY;
    emit createNode(
                aNodes, nodeSize, nodeColor,
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
    qDebug()<< "   Parser: readGraphMLElementEdge() id: " <<	xmlStreamAttr.value("id").toString();
    edge_source = xmlStreamAttr.value("source").toString();
    edge_target = xmlStreamAttr.value("target").toString();

    missingNode=false;
    edgeWeight=initEdgeWeight;
    edgeColor=initEdgeColor;
    bool_edge= true;

    if ( ((xmlStreamAttr.value("directed")).toString()).contains("false"),Qt::CaseInsensitive ) {
        undirected = 2;
    }
    if (!nodeNumber.contains(edge_source)) {
        qDebug() << "\n\n\nParser::readGraphMLElementEdge() source node id "
                 << edge_source
                 << "for edge from " << edge_source << " to " << edge_target
                 << " DOES NOT EXIST!";
        edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                     QString::number(edgeWeight)+"|"+edgeColor
                                     +"|"+QString::number(undirected));
        missingNode=true;
    }
    if (!nodeNumber.contains(edge_target)) {
        qDebug() << "\n\n\nParser::readGraphMLElementEdge() target node id "
                 << edge_target
                 << "for edge from " << edge_source << " to " << edge_target
                 << " DOES NOT EXIST!";
        edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                     QString::number(edgeWeight)+"|"+edgeColor
                                     +"|"+QString::number(undirected));
        missingNode=true;
    }

    if (missingNode) {
        return;
    }

    source = nodeNumber [edge_source];
    target = nodeNumber [edge_target];
    qDebug()<< "    edge source "<< edge_source << " num "<< source;
    qDebug()<< "    edge target "<< edge_target << " num "<< target;


}


// this method emits the edge creation signal.
// called at the end of edge element   
void Parser::endGraphMLElementEdge(QXmlStreamReader &xml){
    Q_UNUSED(xml);
    if (missingNode) {
        qDebug()<<"   Parser: endGraphMLElementEdge() *** missingNode true "
               << " postponing edge creation signal";
        return;
    }
    qDebug()<<"   Parser: endGraphMLElementEdge() *** signal createEdge "
           << source << " -> " << target << " undirected value " << undirected;
    //FIXME need to return edge label as well!
    emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
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
    qDebug()<< "   Parser: readGraphMLElementData(): key_id: " <<  key_id <<  " key_value "<< key_value;
    if (key_value.trimmed() == "")
    {
        qDebug()<< "   Parser: readGraphMLElementData(): text: " << key_value;
        xml.readNext();
        key_value=xml.text().toString();
        qDebug()<< "   Parser: readGraphMLElementData(): text: " << key_value;
        if (  key_value.trimmed() != "" ) { //if there's simple text after the StartElement,
            qDebug()<< "   Parser: readGraphMLElementData(): key_id " << key_id
                    << " value is simple text " <<key_value ;
        }
        else {  //no text, probably more tags. Return...
            qDebug()<< "   Parser: readGraphMLElementData(): key_id " << key_id
                    << " for " <<keyFor.value(key_id) << ". More elements nested here, continuing";
            return;
        }

    }

    if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "     Data found. Node color: "<< key_value << " for this node";
        nodeColor= key_value;
    }
    else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "node" ){
        qDebug()<< "     Data found. Node label: "<< key_value << " for this node";
        nodeLabel = key_value;
    }
    else if (keyName.value(key_id) == "x_coordinate" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "     Data found. Node x: "<< key_value << " for this node";
        conv_OK=false;
        randX= key_value.toFloat( &conv_OK ) ;
        if (!conv_OK)
            randX = 0;
        else
            randX=randX * gwWidth;
        qDebug()<< "     Using: "<< randX;
    }
    else if (keyName.value(key_id) == "y_coordinate" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "     Data found. Node y: "<< key_value << " for this node";
        conv_OK=false;
        randY= key_value.toFloat( &conv_OK );
        if (!conv_OK)
            randY = 0;
        else
            randY=randY * gwHeight;
        qDebug()<< "     Using: "<< randY;
    }
    else if (keyName.value(key_id) == "size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "     Data found. Node size: "<< key_value << " for this node";
        conv_OK=false;
        nodeSize= key_value.toInt ( &conv_OK );
        if (!conv_OK)
            nodeSize = initNodeSize;
        qDebug()<< "     Using: "<< nodeSize;
    }
    else if (keyName.value(key_id) == "label.size" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "     Data found. Node label size: "<< key_value << " for this node";
        conv_OK=false;
        nodeLabelSize= key_value.toInt ( &conv_OK );
        if (!conv_OK)
            nodeLabelSize = initNodeLabelSize;
        qDebug()<< "     Using: "<< nodeSize;
    }
    else if (keyName.value(key_id) == "label.color" && keyFor.value(key_id) == "node" ){
        qDebug()<< "     Data found. Node label Color: "<< key_value << " for this node";
        nodeLabelColor = key_value;
    }
    else if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
        qDebug()<< "     Data found. Node shape: "<< key_value << " for this node";
        nodeShape= key_value;
    }
    else if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
        qDebug()<< "     Data found. Edge color: "<< key_value << " for this edge";
        edgeColor= key_value;
        if (missingNode){
            edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                         QString::number(edgeWeight)+"|"+edgeColor
                                         +"|"+QString::number(undirected));
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
                                         +"|"+QString::number(undirected));
        }
        qDebug()<< "     Data found. Edge value: "<< key_value << " Using "<< edgeWeight << " for this edge";
    }
    else if ( keyName.value(key_id) == "size of arrow"  && keyFor.value(key_id) == "edge" ) {
        conv_OK=false;
        float temp = key_value.toFloat( &conv_OK );
        if (!conv_OK) arrowSize = 1;
        else  arrowSize = temp;
        qDebug()<< "     Data found. Edge arrow size: "<< key_value << " Using  "<< arrowSize << " for this edge";
    }
    else if (keyName.value(key_id) == "label" && keyFor.value(key_id) == "edge" ){
        edgeLabel = key_value;
        if (missingNode){
            edgesMissingNodesHash.insert(edge_source+"===>"+edge_target,
                                         QString::number(edgeWeight)+"|"+edgeColor
                                         +"|"+QString::number(undirected));
        }
        qDebug()<< "     Data found. Edge label: "<< edgeLabel << " for this edge";
    }



}



/**
 * 	Reads node graphics data and properties: label, color, shape, size, coordinates, etc.
 */
void Parser::readGraphMLElementNodeGraphics(QXmlStreamReader &xml) {
    qDebug()<< "       Parser: readGraphMLElementNodeGraphics(): element name "<< xml.name().toString();
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
        qDebug()<< "        Node Coordinates: " << tempX << " " << tempY << " Using coordinates" << randX<< " "<<randY;
        if (xmlStreamHasAttribute ( xmlStreamAttr, "width") ) {
            conv_OK=false;
            temp = xmlStreamAttr.value("width").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                nodeSize = temp;
            qDebug()<< "        Node Size: " << temp<< " Using nodesize" << nodeSize;
        }
        if (xmlStreamHasAttribute ( xmlStreamAttr, "shape") ) {
            nodeShape = xmlStreamAttr.value("shape").toString();
            qDebug()<< "        Node Shape: " << nodeShape;
        }

    }
    else if (xml.name() == "Fill" ){
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "color") ) {
            nodeColor= xmlStreamAttr.value("color").toString();
            qDebug()<< "        Node color: " << nodeColor;
        }

    }
    else if ( xml.name() == "BorderStyle" ) {


    }
    else if (xml.name() == "NodeLabel" ) {
        key_value=xml.readElementText();  //see if there's simple text after the StartElement
        if (!xml.hasError()) {
            qDebug()<< "         Node Label "<< key_value;
            nodeLabel = key_value;
        }
        else {
            qDebug()<< "         Can't read Node Label. There must be more elements nested here, continuing";
        }
    }
    else if (xml.name() == "Shape" ) {
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "type") ) {
            nodeShape= xmlStreamAttr.value("type").toString();
            qDebug()<< "        Node shape: " << nodeShape;
        }

    }


}

void Parser::readGraphMLElementEdgeGraphics(QXmlStreamReader &xml) {
    qDebug()<< "       Parser: readGraphMLElementEdgeGraphics() element name "<< xml.name().toString();

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
        qDebug()<< "        Edge Path control points: " << bez_p1_x << " " << bez_p1_y << " " << bez_p2_x << " " << bez_p2_y;
    }
    else if (xml.name() == "LineStyle" ){
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "color") ) {
            edgeColor= xmlStreamAttr.value("color").toString();
            qDebug()<< "        Edge color: " << edgeColor;
        }
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "type") ) {
            edgeType= xmlStreamAttr.value("type").toString();
            qDebug()<< "        Edge type: " << edgeType;
        }
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "width") ) {
            temp = xmlStreamAttr.value("width").toString().toFloat (&conv_OK) ;
            if (conv_OK)
                edgeWeight = temp;
            else
                edgeWeight=1.0;
            qDebug()<< "        Edge width: " << edgeWeight;
        }

    }
    else if ( xml.name() == "Arrows" ) {
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "source") ) {
            tempString = xmlStreamAttr.value("source").toString();
            qDebug()<< "        Edge source arrow type: " << tempString;
        }
        if ( xmlStreamHasAttribute ( xmlStreamAttr, "target") ) {
            tempString = xmlStreamAttr.value("target").toString();
            qDebug()<< "        Edge target arrow type: " << tempString;
        }



    }
    else if (xml.name() == "EdgeLabel" ) {
        key_value=xml.readElementText();  //see if there's simple text after the StartElement
        if (!xml.hasError()) {
            qDebug()<< "         Edge Label "<< key_value;
            //probably there's more than simple text after StartElement
            edgeLabel = key_value;
        }
        else {
            qDebug()<< "         Can't read Edge Label. More elements nested ? Continuing with blank edge label....";
            edgeLabel = "" ;
        }
    }



}


void Parser::readGraphMLElementUnknown(QXmlStreamReader &xml) {
    qDebug()<< "Parser: readGraphMLElementUnknown()";
    Q_ASSERT(xml.isStartElement());
    qDebug()<< "   "<< xml.name().toString() ;
}


void Parser::createEdgesMissingNodes(){
    int count=0;
    bool ok;
    edgeWeight = initEdgeWeight;
    edgeColor = initEdgeColor;
    undirected = 0;
    if ( (count = edgesMissingNodesHash.size()) > 0 ) {
        qDebug()<<"Parser::createEdgesMissingNodes() - edges to create " << count;
        QHash<QString, QString>::const_iterator it =
                edgesMissingNodesHash.constBegin();
        while (it != edgesMissingNodesHash.constEnd()) {
            qDebug() << "creating missing edge " << it.key() << " data " << it.value() ;
            edgeMissingNodesList = (it.key()).split("===>");
            if (! ((edgeMissingNodesList[0]).isEmpty() )
                    && !((edgeMissingNodesList[1]).isEmpty()) ) {
                source = nodeNumber.value(edgeMissingNodesList[0], -666);
                target = nodeNumber.value(edgeMissingNodesList[1], -666);
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
                        undirected=2;

                }
                qDebug()<<"   Parser: createEdgesMissingNodesHash() *** signal createEdge "
                       << source << " -> " << target << " undirected value " << undirected;
                //FIXME need to return edge label as well!
                emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);

            }
            ++it;
        }
    }
}


/**
    Tries to load a file as GML formatted network. If not it returns -1
*/
bool Parser::loadGML(){

    qDebug("\n\nParser: loadGML()");
    QFile file ( fileName );
    QString str, temp;
    int fileLine=0, start=0, end=0;
    Q_UNUSED(start);
    Q_UNUSED(end);

    if ( ! file.open(QIODevice::ReadOnly )) return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    while (!ts.atEnd() )   {
        str= ts.readLine() ;
        fileLine++;
        qDebug ()<<"Reading fileLine "<< fileLine;
        if ( fileLine == 1 ) {
            qDebug ()<<"Reading fileLine = "<< fileLine;
            if ( !str.startsWith("graph", Qt::CaseInsensitive) ) {
                qDebug() << "*** Parser:loadGML(): Not an GML-formatted file. Aborting";
                file.close();
                return false;
            }

        }
        if ( str.startsWith("directed",Qt::CaseInsensitive) ) { 	 //key declarations
        }
        else if ( str.startsWith("id",Qt::CaseInsensitive) ) {
        }
        else if ( str.startsWith("label",Qt::CaseInsensitive) ) {
        }
        else if ( str.startsWith("node",Qt::CaseInsensitive) ) { 	 //node declarations
        }
        else if ( str.startsWith("edge",Qt::CaseInsensitive) ) { 	 //edge declarations
        }
    }
    //The network has been loaded. Tell MW the statistics and network type
    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: List
    emit fileType(6, networkName, aNodes, totalLinks, undirected);
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

    undirected=0; arrows=true; bezier=false;
    source=0, target=0;
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly )) return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    aNodes=0;
    while (!ts.atEnd() )   {
        str= ts.readLine() ;
        str=str.simplified();
        str=str.trimmed();

        if ( isComment (str) )
            continue;

        fileLine++;

        qDebug ()<<"Reading fileLine "<< fileLine;

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
                return false;
            }

            if ( str.contains("digraph", Qt::CaseInsensitive) ) {
                lineElement=str.split(" ");
                if (lineElement[1]!="{" ) networkName=lineElement[1];
                qDebug() << "This is a DOT DIGRAPH named " << networkName;
                continue;
            }
            else if ( str.contains("graph", Qt::CaseInsensitive) ) {
                lineElement=str.split(" ");
                if (lineElement[1] !="{" ) networkName=lineElement[1];
                qDebug() << "This is a DOT GRAPH named " << networkName;
                continue;
            }
            else {
                qDebug()<<" *** Parser:loadDot(): Not a GraphViz file. Abort: dot format can only start with \" (di)graph netname {\"";
                return false;
            }
        }

        if (  str.contains("graph [" ,Qt::CaseInsensitive) ) {
                netProperties == true;
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
            dotProperties(str, nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor );
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
            qDebug()<< "* A node definition must be here ..." << str;
            end=str.indexOf('[');
            if (end!=-1) {
                temp=str.right(str.size()-end-1); //keep the properties
                temp=temp.remove(']');
                temp=temp.remove(';');
                node=str.left(end-1);
                node=node.remove('\"');
                qDebug()<<"node named "<<node.toLatin1();
                qDebug()<<"node properties "<<temp.toLatin1();
                nodeLabel=node;  //Will change only if label exists in dotProperties
                dotProperties(temp, nodeValue, nodeLabel, initNodeShape, initNodeColor, fontName, fontColor );
                if (nodeLabel=="") nodeLabel=node;
                aNodes++;
                randX=rand()%gwWidth;
                randY=rand()%gwHeight;
                qDebug()<<" *** Creating node "<< aNodes
                       << " at "<< randX <<","<< randY
                       <<" label "<<node.toLatin1()
                      << " colored "<< initNodeColor
                      << "initNodeSize " << initNodeSize
                      << "initNodeNumberColor " <<initNodeNumberColor
                      << "initNodeNumberSize " << initNodeNumberSize
                      << "initNodeLabelColor " << initNodeLabelColor
                      << "nodeShape" <<  initNodeShape;
                emit createNode(
                            aNodes, initNodeSize, initNodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            nodeLabel , initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX,randY),
                            initNodeShape, false
                            );
                nodesDiscovered.push_back( node  );			// Note that we push the numbered nodelabel whereas we create the node with its file specified node label.
                qDebug()<<" * Total aNodes " << aNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
                target=aNodes;

            }
            else
                end=str.indexOf(';');
            qDebug ("* Finished node!");
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
            qDebug()<< "* A node definition must be here ..." << str;
            end=str.indexOf(';');
            if (end!=-1) {
                temp=str.right(str.size()-end-1); //keep the properties
                temp=temp.remove(']');
                temp=temp.remove(';');
                node=str.left(end-1);
                node=node.remove('\"');
                qDebug()<<"node named "<<node.toLatin1();
                qDebug()<<"node properties "<<temp.toLatin1();
                nodeLabel=node;  //Will change only if label exists in dotProperties
                nodeLabel=node;
                aNodes++;
                randX=rand()%gwWidth;
                randY=rand()%gwHeight;
                qDebug()<<" *** Creating node "<< aNodes
                       << " at "<< randX <<","<< randY
                       <<" label "<<node.toLatin1()
                      << " colored "<< initNodeColor
                      << "initNodeSize " << initNodeSize
                      << "initNodeNumberColor " <<initNodeNumberColor
                      << "initNodeNumberSize " << initNodeNumberSize
                      << "initNodeLabelColor " << initNodeLabelColor
                      << "nodeShape" <<  initNodeShape;
                emit createNode(
                            aNodes, initNodeSize, initNodeColor,
                            initNodeNumberColor, initNodeNumberSize,
                            nodeLabel , initNodeLabelColor, initNodeLabelSize,
                            QPointF(randX,randY),
                            initNodeShape, false
                            );
                nodesDiscovered.push_back( node  );			// Note that we push the numbered nodelabel whereas we create the node with its file specified node label.
                qDebug()<<" * Total aNodes " << aNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
                target=aNodes;

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
                temp=temp.remove(']');
                temp=temp.remove(';');
                qDebug()<<"edge properties "<<temp.toLatin1();
                dotProperties(temp, edgeWeight, edgeLabel, edgeShape, edgeColor, fontName, fontColor );
            }
            else
                end=str.indexOf(';');

            //FIXME Cannot parse nodes named with '-' character
            str=str.mid(0, end).remove('\"');  //keep only edges

            qDebug()<<"edges "<<str.toLatin1();

            if (!str.contains("->",Qt::CaseInsensitive) ){  //non directed = symmetric links
                if ( str.contains("--",Qt::CaseInsensitive) )
                    nodeSequence=str.split("--");
                else
                    nodeSequence=str.split("-");
            }
            else { 											//is directed
                nodeSequence=str.split("->");
            }
            //Create all nodes defined in nodeSequence
            for ( QList<QString>::iterator it=nodeSequence.begin(); it!=nodeSequence.end(); it++ )  {
                node=(*it).simplified();
                qDebug () << " nodeSequence node "<< node;
                if ( (aNum=nodesDiscovered.indexOf( node ) ) == -1) {
                    aNodes++;
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                    qDebug()<<" *** Creating node "<< aNodes
                           << " at "<< randX <<","<< randY
                           <<" label "<<node.toLatin1()
                          << " colored "<< nodeColor
                          << "initNodeSize " << initNodeSize
                          << "initNodeNumberColor " <<initNodeNumberColor
                          << "initNodeNumberSize " << initNodeNumberSize
                          << "initNodeLabelColor " << initNodeLabelColor
                          << "nodeShape" <<  initNodeShape;
                    emit createNode(
                                aNodes, initNodeSize, nodeColor,
                                initNodeNumberColor, initNodeNumberSize,
                                node , initNodeLabelColor, initNodeLabelSize,
                                QPointF(randX,randY),
                                initNodeShape, false
                                );
                    nodesDiscovered.push_back( node  );
                    qDebug()<<" * Total aNodes " << aNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
                    target=aNodes;
                    if (it!=nodeSequence.begin()) {
                        qDebug()<<"-- Drawing Link between node "<< source<< " and node " <<target;
                        emit createEdge(source,target, edgeWeight, edgeColor, undirected, arrows, bezier);
                    }
                }
                else {
                    target=aNum+1;
                    qDebug("# Node already exists. Vector num: %i ",target);
                    if (it!=nodeSequence.begin()) {
                        qDebug()<<"-- Drawing Link between node "<<source<<" and node " << target;
                        emit createEdge(source,target, edgeWeight , edgeColor, undirected, arrows, bezier);
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
            dotProperties(temp, nodeValue, label, nodeShape, nodeColor, fontName, fontColor );
            qDebug ("Finished the properties!");

            if (start > 2 ) {//there is a node definition here
                node=str.left(start).remove('\"').simplified();
                qDebug()<<"node label: "<<node.toLatin1()<<"." ;
                if (!nodesDiscovered.contains(node)) {
                    qDebug("not discovered node");
                    aNodes++;
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                    qDebug()<<"***  Creating node at "<<  randX << " "<< randY<< " label "<<node.toLatin1() << " colored "<< nodeColor;
                    emit createNode(
                                aNodes, initNodeSize, nodeColor,
                                initNodeNumberColor, initNodeNumberSize,
                                label, initNodeLabelColor, initNodeLabelSize,
                                QPointF(randX,randY),
                                nodeShape, false
                                );
                    aNum=aNodes;
                    nodesDiscovered.push_back( node);
                    qDebug()<<" Total aNodes: "<<  aNodes<< " nodesDiscovered = "<< nodesDiscovered.size();
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
    //The network has been loaded. Tell MW the statistics and network type
    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: List
    emit fileType(4, networkName, aNodes, totalLinks, undirected);
    return true;
}





void Parser::dotProperties(QString str, float &nValue, QString &label, QString &shape, QString &color, QString &fontName, QString &fontColor ){
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




bool Parser::loadWeighedList(){
    qDebug() << "Parser: loadWeighedList()";
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly ))
        return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());
    QString str;
    QStringList lineElement;
    int  j=0,  source=0, target=0, newCount=0,  maxNodeCreated=0;
    bool intOK=false;

    edgeWeight=1.0;
    undirected=0;
    arrows=true;
    bezier=false;

    while ( !ts.atEnd() )   {
        str= ts.readLine() ;
        qDebug()<< " str " << str;
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
            qDebug()<< "*** Parser:loadWeighedList(): Not a Weighted list-formatted file. Aborting!!";
            file.close();
            return false;
        }

        lineElement=str.split(" ");
        newCount = lineElement.count();

        if ( newCount != 3 ) {
            qDebug()<< "*** Parser:loadWeighedList(): Not a Weighted list-formatted file. Aborting!!";
            file.close();
            return false;
        }

        source =  (lineElement[0]).toInt(&intOK);
        target =  (lineElement[1]).toInt(&intOK);
        qDebug() << "	source node " << source  << " target node " << target;

        edgeWeight=(lineElement[2]).toDouble(&intOK);
        if (intOK) {
            qDebug () << "	list file declares edge weight: " << edgeWeight;
        }
        else {
            edgeWeight=1.0;
            qDebug () << "	list file NOT declaring edge weight. Setting default: " << edgeWeight;
        }
        if (maxNodeCreated < source ) {
            for ( j = maxNodeCreated ; j != source ; j++ ) {
                randX=rand()%gwWidth;
                randY=rand()%gwHeight;
                qDebug()<< "	source node " << source
                        << "is less than maxNodeCreated - creating node "<< j+1
                        << "using random coords "<<randX << " "<< randY;
                emit createNode( j+1, initNodeSize,  initNodeColor,
                                 initNodeNumberColor, initNodeNumberSize,
                                 QString::number(j+1), initNodeLabelColor, initNodeLabelSize,
                                 QPointF(randX, randY),
                                 initNodeShape, false
                                 );
            }
            maxNodeCreated = source ;
        }
        if (maxNodeCreated < target ) {
            for ( j = maxNodeCreated ; j != target; j++ ) {
                randX=rand()%gwWidth;
                randY=rand()%gwHeight;
                qDebug()<< "	target node " << target
                        << "is less than maxNodeCreated - creating node "<< j+1
                        <<" using random coords "<<randX << " "<< randY;
                emit createNode( j+1, initNodeSize,  initNodeColor,
                                 initNodeNumberColor, initNodeNumberSize,
                                 QString::number(j+1), initNodeLabelColor, initNodeLabelSize,
                                 QPointF(randX, randY),
                                 initNodeShape, false
                                 );
            }
            maxNodeCreated = target ;
        }
        qDebug()<< "	Parser-loadWeighedList(): Creating link now... "
                << " link from i= " << source << " to j= "<< target << " weight= "<< edgeWeight <<  " TotalLinks=  " << totalLinks+1;
        emit createEdge(source, target, edgeWeight, initEdgeColor, undirected, arrows, bezier);
        totalLinks++;
    } //end ts.stream while here
    file.close();

    //The network has been loaded. Tell MW the statistics and network type
    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: PlainList, 8: WeightedList
    emit fileType(7, networkName, aNodes, totalLinks, undirected);
    qDebug() << "Parser-loadWeighedList() ending and returning...";
    return true;

}




bool Parser::loadSimpleList(){
    qDebug() << "Parser: loadSimpleList()";
    QFile file ( fileName );
    if ( ! file.open(QIODevice::ReadOnly ))
        return false;
    QTextStream ts( &file );
    ts.setCodec(userSelectedCodecName.toUtf8());  // set the userselectedCodec
    QString str;
    QStringList lineElement;
    int i=0, j=0, num=0, source=0, target=0, maxNodeCreated=0;
    bool intOK=false;
    bool nodeNumberingZero = false;
    initEdgeWeight=1.0;

    undirected=false;
    arrows=true;
    bezier=false;

    qDebug () << "Parser::loadSimpleList() - check node numbering...";
    while ( !ts.atEnd() )   {
        str= ts.readLine() ;
        qDebug()<< "Parser::loadSimpleList() - line: " << endl << str;
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
            qDebug()<< "*** Parser:loadSimpleList(): Not an Adjacency-formatted file. Aborting!!";
            file.close();
            return false;
        }

        lineElement=str.split(" ");

        if (lineElement.contains("0")) {
            nodeNumberingZero = true;
            break;
        }

    }

    if (nodeNumberingZero) {
        qDebug()<< "Parser::loadSimpleList() - node numbers start from zero ";
    }
    else
        qDebug()<< "Parser::loadSimpleList() - node numbers start from one ";

    ts.seek(0);

    qDebug () << "Parser::loadSimpleList() - reset and read lines...";

    while ( !ts.atEnd() )   {
        str= ts.readLine() ;

        str=str.simplified();

        qDebug()<< "Parser::loadSimpleList() - line: " << endl << str;

        if ( isComment(str) )
            continue;

        lineElement=str.split(" ");

        i=0;
        for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
            num = (*it1).toInt(&intOK);
            if ( !intOK  ) {
                qDebug()<< "Error! Error occured during a string conversion to integer...";
                file.close();
                return false;
            }
            if (nodeNumberingZero)
               num =  num + 1;

            if (i==0){
                source = num;
                qDebug() << "	source node: " << source;
            }
            else {
                target = num;
                qDebug() << "	target node: " << target;
            }

            if (maxNodeCreated < num ) {
                for ( j = maxNodeCreated ; j != num ; j++ ) {
                    randX=rand()%gwWidth;
                    randY=rand()%gwHeight;
                    qDebug()<<"		random coords "<<randX << " "<< randY;
                    emit createNode( j+1, initNodeSize,  initNodeColor,
                                     initNodeNumberColor, initNodeNumberSize,
                                     QString::number(j+1), initNodeLabelColor, initNodeLabelSize,
                                     QPointF(randX, randY),
                                     initNodeShape, false
                                     );
                }
                maxNodeCreated = num ;
            }
            if ( i != 0) {
                qDebug("	there is a link here");
                emit createEdge(source, target, initEdgeWeight, initEdgeColor, undirected, arrows, bezier);
                totalLinks++;
                qDebug() << "link from Node i= "<< source  << "  to j = " <<  target << " weight = "<< initEdgeWeight<< ". TotalLinks =  " << totalLinks;
            }
            i++;
        }

    } //end ts.stream while here
    file.close();
    //The network has been loaded. Tell MW the statistics and network type
    // 0: no format, 1: GraphML, 2:Pajek, 3:Adjacency, 4: Dot, 5:DL, 6:GML, 7: Weighted List, 8: simple list
    emit fileType(8, networkName, aNodes, totalLinks, undirected);
    qDebug() << "Parser-loadSimpleList() ending and returning...";
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
        qDebug () << " Parser: a comment or an empty line was found. Skipping...";
        return true;
    }
    return false;

}
