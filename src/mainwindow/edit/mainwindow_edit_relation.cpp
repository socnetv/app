/**
 * @file mainwindow_edit_relation.cpp
 * @brief Implements MainWindow relation management (add/rename/clear/switch) and the directed-graph toggle.
 * @author Dimitris B. Kalamaras (http://dimitris.apeiro.gr)
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

#include "mainwindow.h"
#include "graph.h"
#include "graphicswidget.h"
#include "graphicsnode.h"
#include "graphicsedge.h"
#include "widgets/graphtablewidget.h"
#include "forms/dialognodeedit.h"
#include "forms/dialogedgeedit.h"
#include "forms/dialogbulkedit.h"
#include "forms/dialognodefind.h"
#include "forms/dialogedgedichotomization.h"
#include "forms/dialogfilteredgesbyweight.h"
#include "forms/dialogfilternodesbycentrality.h"
#include "forms/dialogfilterbyattribute.h"
#include "forms/dialogquerybuilder.h"

#include <QtWidgets>

/**
 * @brief Clears the relations combo.
 */
void MainWindow::slotEditRelationsClear()
{
    qCDebug(lcMainWindow) << "Clearing relations combo...";
    editRelationChangeCombo->clear();
}

/**
 * @brief Prompts the user to enter the name of a new relation
 *
 * On success, emits signal to Graph to change to the new relation.
 */
void MainWindow::slotEditRelationAddPrompt()
{

    bool ok;
    QString newRelationName;
    int relationsCounter = activeGraph->relations();

    qCDebug(lcMainWindow) << "Prompting the user for the new relation name to be added to the relations combo...";

    //
    // Prompt the user for the new relation name
    //

    // Check if this is the first time, in order to show a more comprehensive message
    if (relationsCounter == 1 && activeNodes() == 0)
    {
        newRelationName = QInputDialog::getText(
            this,
            tr("Add new relation"),
            tr("Enter a name for a new relation between the actors.\n"
               "A relation is a collection of ties of a "
               "specific kind between the network actors.\n"
               "For instance, enter \"friendship\" if the "
               "edges of this relation refer to the set of \n"
               "friendships between pairs of actors."),
            QLineEdit::Normal, QString(), &ok);
    }
    else
    {
        newRelationName = QInputDialog::getText(
            this, tr("Add new relation"),
            tr("Enter a name for the new relation (or press Cancel):"),
            QLineEdit::Normal, QString(), &ok);
    }

    //
    // Check which button was pressed
    //
    if (ok)
    {

        // user pressed OK

        // Check if new relation name
        if (!newRelationName.isEmpty())
        {

            // a relation name entered

            // Check if it is already used by another relation.
            if (editRelationChangeCombo->findText(newRelationName) > -1)
            {
                slotHelpMessageToUser(USER_MSG_CRITICAL,
                                      tr("Error. Relation name is used!"),
                                      tr("The relation name is already used."),
                                      tr("Please use another relation name that is not already used."));
                return;
            }

            // Emit signal to Graph to add the relation and change to it
            bool changeRelation = true;
            emit signalRelationAddAndChange(newRelationName, changeRelation);
        }
        else
        {
            // no name entered
            slotHelpMessageToUser(USER_MSG_CRITICAL,
                                  tr("Error. No relation name entered!"),
                                  tr("You did not type a name for this new relation"));
            return;
        }
    }
    else
    {
        // user pressed Cancel
        statusMessage(QString(tr("New relation cancelled.")));
        return;
    }

    statusMessage(QString(tr("New relation named %1, added."))
                      .arg(newRelationName));
}

/**
 * @brief Adds a new relation to the relations combo
 *
 * Called from Graph when the network file parser or another Graph method
 * demands a new relation to be added to the UI combo.
 *
 * @param newRelationName
 */
void MainWindow::slotEditRelationAdd(const QString &newRelationName)
{

    qCDebug(lcMainWindow) << "Adding new relation to relations combo:"
             << newRelationName;

    if (!newRelationName.isNull())
    {

        editRelationChangeCombo->addItem(newRelationName);

        // Enable prev/next widgets, if they are disabled.
        if (!editRelationPreviousAct->isEnabled() && editRelationChangeCombo->count() > 1)
        {
            editRelationPreviousAct->setEnabled(true);
            editRelationNextAct->setEnabled(true);
        }

        statusMessage(QString(tr("Added a new relation named: %1."))
                          .arg(newRelationName));
    }
}

/**
 * @brief Changes the editRelations combo box index to relIndex
 *
 * If relIndex==RAND_MAX the index is set to the last relation index
 *
 * @param relIndex
 */
void MainWindow::slotEditRelationChange(const int &relIndex)
{
    if (relIndex == RAND_MAX)
    {
        qCDebug(lcMainWindow) << "relIndex==RANDMAX. Changing relation combo to last relation...";
        editRelationChangeCombo->setCurrentIndex(
            (editRelationChangeCombo->count() - 1));
    }
    else
    {
        qCDebug(lcMainWindow) << "Changing relation combo to index" << relIndex;
        editRelationChangeCombo->setCurrentIndex(relIndex);
    }
}

/**
 * @brief Prompts the user to enter a new name for the current relation
 */
void MainWindow::slotEditRelationRename()
{

    bool ok = false;

    qCDebug(lcMainWindow) << "Request to rename current relation:"
             << editRelationChangeCombo->currentText()
             << "Prompting for new name...";

    //
    // Get new name from user
    //
    QString newName = QInputDialog::getText(
        this,
        tr("Rename current relation"),
        tr("Enter a new name for this relation."),
        QLineEdit::Normal, QString(), &ok);

    //
    // Check entered name
    //
    if (newName.isEmpty() || !ok)
    {
        slotHelpMessageToUser(USER_MSG_CRITICAL,
                              tr("Not a valid name."),
                              tr("Error"),
                              tr("You did not enter a valid name for this relation."));
        return;
    }

    //
    // Change name in combo - this will trigger the signal to activeGraph
    //
    editRelationChangeCombo->setCurrentText(newName);
}

/**
 * @brief Updates the edge-mode combo and arrows action to reflect the directed
 *        state of the newly selected relation, without calling setDirected/setUndirected.
 *
 * Connected to Graph::signalGraphDirectedChanged, which is emitted by relationSet()
 * whenever the user switches to a different relation.
 *
 * @param directed true if the new relation is directed
 */
void MainWindow::slotEditGraphDirectedChanged(const bool &directed)
{
    qCDebug(lcMainWindow) << "MainWindow::slotEditGraphDirectedChanged - directed:" << directed;
    // Block the combo's signal so we don't re-enter slotEditEdgeMode and
    // accidentally call setDirected/setUndirected (which mutate edges).
    toolBoxEditEdgeModeSelect->blockSignals(true);
    toolBoxEditEdgeModeSelect->setCurrentIndex(directed ? 0 : 1);
    toolBoxEditEdgeModeSelect->blockSignals(false);
    optionsEdgeArrowsAct->setChecked(directed);
}
