#include "randscalefreedialog.h"

#include <QDebug>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>
#include <QGraphicsColorizeEffect>

RandScaleFreeDialog::RandScaleFreeDialog(QWidget *parent) :
    QDialog(parent)
{
    qDebug() << "::RandScaleFreeDialog() " ;

    ui.setupUi(this);

    nodes = 0;
    initialNodes = 0;
    mode = "";
    diag = false;

    connect ( ui.buttonBox, &QDialogButtonBox::accepted,
              this, &RandScaleFreeDialog::gatherData );

    ui.buttonBox -> button (QDialogButtonBox::Ok) -> setDefault(true);

    (ui.nodesSpinBox )->setFocus();

    ui.initialNodesSpinBox-> setEnabled(true);
    ui.undirectedRadioButton->setChecked(false);
    ui.directedRadioButton->setEnabled(true);
    ui.directedRadioButton->setChecked(true);
    ui.diagCheckBox->setText("No, set zero");
    ui.diagCheckBox ->setChecked(false);
    ui.diagCheckBox -> setEnabled(false);

    connect ( ui.undirectedRadioButton,&QRadioButton::clicked,
              this, &RandScaleFreeDialog::setModeUndirected );
    connect ( ui.directedRadioButton,&QRadioButton::clicked,
              this, &RandScaleFreeDialog::setModeDirected );

    connect ( ui.diagCheckBox,&QCheckBox::clicked,
              this, &RandScaleFreeDialog::setDiag);

}


void RandScaleFreeDialog::setModeDirected (){
    ui.directedRadioButton->setChecked(true) ;
    ui.undirectedRadioButton->setChecked(false) ;

}

void RandScaleFreeDialog::setModeUndirected (){
    ui.directedRadioButton->setChecked(false) ;
    ui.undirectedRadioButton->setChecked(true) ;
}

void RandScaleFreeDialog::setDiag (){
    if (ui.diagCheckBox -> isChecked())
        ui.diagCheckBox->setText("Yes, allow");
    else
        ui.diagCheckBox->setText("No, set zero");
}

void RandScaleFreeDialog::checkErrors() {
    qDebug()<< " RandSmallWorldDialog::checkErrors()" ;

    //     if ( !ui.gnpRadioButton->isChecked() &&  !ui.gnmRadioButton->isChecked())
    //     {
    //         QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect;
    //         effect->setColor(QColor("red"));
    //         ui.gnpRadioButton->setGraphicsEffect(effect);
    //         (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(false);
    //     }
    //     else {
    //         ui.gnpRadioButton->setGraphicsEffect(0);
    //         ui.gnmRadioButton->setGraphicsEffect(0);
    //         (ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setEnabled(true);
    //     }
    //gatherData();
}

void RandScaleFreeDialog::gatherData() {
    qDebug() << "RandScaleFreeDialog::gatherData() " ;
    nodes = ui.nodesSpinBox->value();
    power = ui.powerSpinBox->value();
    initialNodes = ui.initialNodesSpinBox->value();
    edgesPerStep = ui.edgesPerStepSpinBox ->value();
    zeroAppeal = ui.zeroAppealSpinBox->value();
    mode = (ui.directedRadioButton->isChecked() ? "digraph" : "graph" );
 //   diag = (ui.diagCheckBox -> isChecked() ? true : false);

    qDebug() << "nodes " << nodes ;
    qDebug() << "initialNodes " << initialNodes;
    qDebug() << "mode " << mode;
    qDebug() << "diag " << diag;
    emit userChoices(nodes, power, initialNodes, edgesPerStep,zeroAppeal, mode);

}


