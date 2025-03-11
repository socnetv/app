/**
 * @file dialogdatasetselect.cpp
 * @brief Implements the DialogDatasetSelect class for selecting datasets in SocNetV.
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

#include "dialogdatasetselect.h"

#include <QDebug>
#include <QPushButton>


DialogDataSetSelect::DialogDataSetSelect (QWidget *parent) :
    QDialog (parent),
    ui(new Ui::DialogDataSetSelect)
{
    ui->setupUi(this);

    (ui->buttonBox)->button (QDialogButtonBox::Ok)->setDefault(true);

    datasets_list
            << "Krackhardt: High-tech managers (multirelational), 24 actors"
            << "Padgett: Florentine Families (marital and business relations), 16 actors"
            << "Zachary: Karate Club (binary & valued ties), 34 actors"
            << "Bernard: Killworth Fraternity (multirelational), 58 actors"
            << "Thurman: In the office: Networks and Coalitions, 15 actors"
            << "Stokman-Ziegler: Corporate Interlocks in Netherlands, 16 actors"
            << "Stokman-Ziegler: Corporate Interlocks in West Germany, 15 actors"
            << "Galaskiewicz: CEOs and clubs (affiliation data)"
            << "Freeman's EIES networks (multirelational, 32 actors)"
            << "Freeman: EIES network, at time-1, 48 actors"
            << "Freeman: EIES network, at time-2, 48 actors"
            << "Freeman: EIES network, number of messages, 48 actors"
            << "Freeman: The 34 possible graphs with N=5 (as multirelational), 5 actors"
            << "Mexican Power Network in the 1940s (list format)"
            << "Knoke: Bureaucracies Information & Money Exchange Network, 10 actors, 2 relationships"
            << "Stephenson and Zelen (1989): Network of 40 AIDS patients (sex relationship)"
            << "Stephenson and Zelen (1989): Information Centrality test dataset, 5 actors"
            << "Dunbar and Dunbar (1975): Network of Gelada baboon colony (H22a), 12 actors"
            << "Wasserman and Faust: star, circle and line graphs of 7 actors (multirelational)"
            << "Wasserman and Faust: Countries Trade (basic manufactured goods), 24 actors"
            << "Borgatti (1992): Campnet dataset, 18 actors"
            << "Petersen graph: A non-planar, undirected graph with 10 vertices and 15 edges"
            << "Herschel graph: The smallest nonhamiltonian polyhedral graph. 11 nodes, 18 edges";

    datasets_filenames
            << "Krackhardt_High-tech_managers.paj"
            << "Padgett_Florentine_Families.paj"
            << "Zachary_Karate_Club.dl"
            << "Bernard_Killworth_Fraternity.dl"
            << "Thurman_Office_Networks_Coalitions.dl"
            << "Stokman_Ziegler_Corporate_Interlocks_Netherlands.dl"
            << "Stokman_Ziegler_Corporate_Interlocks_West_Germany.dl"
            << "Galaskiewicz_CEOs_and_clubs_affiliation_network_data.2sm"
            << "Freeman_EIES_networks_32actors.dl"
            << "Freeman_EIES_network_48actors_Acquaintanceship_at_time-1.dl"
            << "Freeman_EIES_network_48actors_Acquaintanceship_at_time-2.dl"
            << "Freeman_EIES_network_48actors_Messages.dl"
            << "Freeman_34_possible_graphs_with_N_5_multirelational.paj"
            << "Mexican_Power_Network_1940s.lst"
            << "Knoke_Bureaucracies_Network.pajek"
            << "Stephenson&Zelen_40_AIDS_patients_sex_contact.paj"
            << "Stephenson&Zelen_5actors_6edges_IC_test_dataset.paj"
            << "Stephenson&Zelen_Dunbar&Dunbar_Gelada_baboon_colony_H22a_IC.paj"
            << "Wasserman_Faust_7actors_star_circle_line_graphs.paj"
            << "Wasserman_Faust_Countries_Trade_Data_Basic_Manufactured_Goods.pajek"
            << "Campnet.paj"
            << "Petersen_Graph.paj"
            << "Herschel_Graph.paj";

    (ui->selectBox)->insertItems( 1, datasets_list );
}



void DialogDataSetSelect::getUserChoices(){
    qDebug()<< "DialogDataSetSelect: gathering Data!...";
    int index = (ui->selectBox)->currentIndex();
    QString dataset_name = datasets_filenames[index];
    qDebug()<< "DialogDataSetSelect: user selected: " << dataset_name;
	emit userChoices( dataset_name );
			
}


void DialogDataSetSelect::on_buttonBox_accepted()
{
    this->getUserChoices();
    this->accept();
}

void DialogDataSetSelect::on_buttonBox_rejected()
{
    this->reject();
}

DialogDataSetSelect::~DialogDataSetSelect(){
     datasets_list.clear(); datasets_filenames.clear();
}
