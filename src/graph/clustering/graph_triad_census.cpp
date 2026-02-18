/**
 * @file graph_triad_census.cpp
 * @brief Implements triad census analysis for the Graph class, including MAN classification and triad type labeling.
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
 * @brief Graph::graphTriadCensus
 *  Conducts a triad census and updates QList::triadTypeFreqs,
 * 		which is the list carrying all triad type frequencies
 *  Complexity:O(n!)
 * @return
 */
bool Graph::graphTriadCensus()
{
    int mut = 0, asy = 0, nul = 0;
    int temp_mut = 0, temp_asy = 0, temp_nul = 0, counter_021 = 0;
    int ver1, ver2, ver3;
    int N = vertices();
    int progressCounter = 0;

    VList::const_iterator v1;
    VList::const_iterator v2;
    VList::const_iterator v3;

    qDebug() << "Graph::graphTriadCensus()";
    /*
     * QList::triadTypeFreqs stores triad type frequencies with the following order:
     * 0	1	2	3		4	5	6	7	8		9	10	11	12		13	14	15
     * 003 012 102	021D 021U 021C 111D	111U 030T 030C 201 	120D 120U 120C 210 300
     */

    for (int i = 0; i <= 15; ++i)
    {
        triadTypeFreqs.append(0);
        qDebug() << " initializing triadTypeFreqs[" << i << "] = " << triadTypeFreqs[i];
    }

    QString pMsg = tr("Computing Triad Census. \nPlease wait...");
    emit statusMessage(pMsg);
    emit signalProgressBoxCreate(N, pMsg);

    for (v1 = m_graph.cbegin(); v1 != m_graph.cend(); v1++)
    {

        emit signalProgressBoxUpdate(++progressCounter);

        for (v2 = (v1 + 1); v2 != m_graph.cend(); v2++)
        {

            ver1 = (*v1)->number();
            ver2 = (*v2)->number();

            temp_mut = 0, temp_asy = 0, temp_nul = 0;

            if ((*v1)->hasEdgeTo(ver2))
            {
                if ((*v2)->hasEdgeTo(ver1))
                    temp_mut++;
                else
                    temp_asy++;
            }
            else if ((*v2)->hasEdgeTo(ver1))
                temp_asy++;
            else
                temp_nul++;

            for (v3 = (v2 + 1); v3 != m_graph.cend(); v3++)
            {

                mut = temp_mut;
                asy = temp_asy;
                nul = temp_nul;

                ver3 = (*v3)->number();

                if ((*v1)->hasEdgeTo(ver3))
                {
                    if ((*v3)->hasEdgeTo(ver1))
                        mut++;
                    else
                        asy++;
                }
                else if ((*v3)->hasEdgeTo(ver1))
                    asy++;
                else
                    nul++;

                if ((*v2)->hasEdgeTo(ver3))
                {
                    if ((*v3)->hasEdgeTo(ver2))
                        mut++;
                    else
                        asy++;
                }
                else if ((*v3)->hasEdgeTo(ver2))
                    asy++;
                else
                    nul++;

                qDebug() << "triad of (" << ver1 << "," << ver2 << "," << ver3
                         << ") = (" << mut << "," << asy << "," << nul << ")";
                triadType_examine_MAN_label(mut, asy, nul, (*v1), (*v2), (*v3));

                if (mut == 3 && asy == 0 && nul == 0)
                {
                    counter_021++;
                }
            } // end 3rd for

        } // end 2rd for

    } // end 1rd for
    qDebug() << " ****** 003 COUNTER: " << counter_021;

    calculatedTriad = true;

    emit signalProgressBoxKill();

    return true;
}

/**
    Examines the triad type (in Mutual-Asymmetric-Null label format)
    and increases by one the proper frequency element
    inside QList::triadTypeFreqs
*/
void Graph::triadType_examine_MAN_label(int mut, int asy, int nul,
                                        GraphVertex *vert1,
                                        GraphVertex *vert2,
                                        GraphVertex *vert3)
{
    VList m_triad;
    bool isDown = false, isUp = false, isCycle = false, isTrans = false;
    bool isOutLinked = false, isInLinked = false;

    qDebug() << "Graph::triadType_examine_MAN_label() "
             << " adding (" << vert1->number() << "," << vert2->number()
             << "," << vert3->number() << ") to m_triad ";

    m_triad << vert1 << vert2 << vert3;

    switch (mut)
    {
    case 0:
        switch (asy)
        {
        case 0: //"003";
            triadTypeFreqs[0]++;
            break;
        case 1: //"012";
            triadTypeFreqs[1]++;
            break;
        case 2:
            // "021?" - find out!
            //	qDebug() << "triad vertices: ( "<< vert1->number() << ", "<< vert2->number()<< ", "<< vert3->number()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";
            foreach (GraphVertex *source, m_triad)
            {
                // qDebug() << "  vertex " << source->number() ;
                isOutLinked = false;
                isInLinked = false;

                foreach (GraphVertex *target, m_triad)
                {
                    if (source->number() == target->number())
                        continue;

                    if (source->hasEdgeTo(target->number()))
                    {
                        if (isOutLinked)
                        {
                            triadTypeFreqs[3]++; //"021D"
                            break;
                        }
                        else if (isInLinked)
                        {
                            triadTypeFreqs[5]++; //"021C"
                            break;
                        }
                        else
                        {
                            isOutLinked = true;
                        }
                    }
                    else if (target->hasEdgeTo(source->number()))
                    {
                        //	qDebug() << "    vertex " << source->number()  << " is IN linked from " <<target->number();
                        if (isInLinked)
                        {
                            triadTypeFreqs[4]++; //"021U"
                            break;
                        }
                        else if (isOutLinked)
                        {
                            triadTypeFreqs[5]++; //"021C"
                            break;
                        }
                        else
                        {
                            isInLinked = true;
                        }
                    }
                }
            }
            break;
        case 3:
            qDebug() << "triad vertices: ( " << vert1->number() << ", " << vert2->number() << ", " << vert3->number() << " ) = (" << mut << "," << asy << "," << nul << ")";
            isTrans = false;
            foreach (GraphVertex *source, m_triad)
            {
                qDebug() << "  vertex " << source->number();

                isOutLinked = false;

                foreach (GraphVertex *target, m_triad)
                {
                    if (source->number() == target->number())
                        continue;

                    if (source->hasEdgeTo(target->number()))
                    {

                        if (isOutLinked)
                        {
                            triadTypeFreqs[8]++; //"030T"
                            isTrans = true;
                            break;
                        }
                        else
                        {
                            isOutLinked = true;
                        }
                    }
                }
            }
            if (!isTrans)
            { //"030C"
                triadTypeFreqs[9]++;
            }
            break;
        }
        break;

    case 1:
        switch (asy)
        {
        case 0: //"102";
            triadTypeFreqs[2]++;
            break;
        case 1:
            isUp = false;
            // qDebug() << "triad vertices: ( "<< vert1->number() << ", "<< vert2->number()<< ", "<< vert3->number()<< " ) = ("	<<mut<<","<< asy<<","<<nul<<")";
            foreach (GraphVertex *source, m_triad)
            {
                //	qDebug() << "  vertex " << source->number() ;

                isInLinked = false;

                foreach (GraphVertex *target, m_triad)
                {
                    if (source->number() == target->number())
                        continue;

                    if (target->hasEdgeTo(source->number()))
                    {

                        if (isInLinked)
                        {
                            triadTypeFreqs[6]++; //"030T"
                            isUp = true;
                            break;
                        }
                        else
                        {
                            isInLinked = true;
                        }
                    }
                }
            }
            if (!isUp)
            { //"111U"
                triadTypeFreqs[7]++;
            }
            break;
        case 2:
            isDown = false;
            isUp = false;
            isCycle = true;
            qDebug() << "triad vertices: ( " << vert1->number() << ", "
                     << vert2->number() << ", " << vert3->number() << " ) = ("
                     << mut << "," << asy << "," << nul << ")";

            foreach (GraphVertex *source, m_triad)
            {
                // qDebug() << "  vertex " << source->number() ;
                isOutLinked = false;
                isInLinked = false;

                foreach (GraphVertex *target, m_triad)
                {
                    if (source->number() == target->number())
                        continue;

                    if (source->hasEdgeTo(target->number()))
                    {
                        if (target->hasEdgeTo(source->number()))
                        {
                            isInLinked = true;
                            isOutLinked = true;
                            continue;
                        }
                        else if (isOutLinked && !isInLinked)
                        {
                            triadTypeFreqs[11]++; //"120D"
                            isDown = true;
                            isCycle = false;
                            break;
                        }
                        else
                        {
                            isOutLinked = true;
                        }
                    }
                    else if (target->hasEdgeTo(source->number()))
                    {
                        //	qDebug() << "    vertex " << source->number()  << " is IN linked from " <<target->number();
                        if (source->hasEdgeTo(target->number()))
                        {
                            isOutLinked = true;
                            isInLinked = true;
                            continue;
                        }
                        else if (isInLinked && !isOutLinked)
                        {
                            triadTypeFreqs[12]++; //"120U"
                            isUp = true;
                            isCycle = false;
                            break;
                        }
                        else
                        {
                            isInLinked = true;
                        }
                    }
                }
                if (isUp || isDown)
                    break;
            }
            if (isCycle)
            { //"120C"
                triadTypeFreqs[13]++;
            }
            break;
        case 3:
            // nothing here!
            break;
        }

        break;
    case 2:
        switch (asy)
        {
        case 0: // "201"
            triadTypeFreqs[10]++;
            break;
        case 1: // "210"
            triadTypeFreqs[14]++;
            break;
        }
        break;
    case 3: // "300"
        if (asy == 0 && nul == 0)
            triadTypeFreqs[15]++;
        break;
    }
}


