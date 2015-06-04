#ifndef RANDSCALEFREEDIALOG_H
#define RANDSCALEFREEDIALOG_H

#include <QDialog>

#include "ui_randscalefreedialog.h"

class RandScaleFreeDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RandScaleFreeDialog(QWidget *parent = 0);

public slots:
    void checkErrors();
    void gatherData();
    void setModeDirected();
    void setModeUndirected();
    void setDiag();

signals:
    void userChoices( const int nodes,
                      const int initialNodes,
                      const QString mode,
                      const bool diag);
private:
    QString mode;
    int nodes, initialNodes;
    bool diag;
    Ui::RandScaleFreeDialog ui;

};

#endif // RANDSCALEFREEDIALOG_H
