/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 1.7
 Written in Qt

                         previewform.h  -  description
                             -------------------
    copyright            : (C) 2005-2015 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
    website:             : http://dimitris.apeiro.gr
    project site         : http://socnetv.sourceforge.net
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


#ifndef PREVIEWFORM_H
#define PREVIEWFORM_H

#include <QDialog>
#include <QList>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QTextCodec;
class QTextEdit;

class PreviewForm : public QDialog
{
    Q_OBJECT
public:
    explicit PreviewForm(QWidget *parent = 0);
    void setCodecList(const QList<QTextCodec *> &list);
    void setEncodedData(const QByteArray &data, const QString, const int );
    QString decodedString() const { return decodedStr; }
signals:
    void userCodec(const QString, const QString, const int);
private slots:
    void updateTextEdit();
    void accept();

private:
    QByteArray encodedData;
    QString decodedStr, fileName;
    int format;
    QComboBox *encodingComboBox;
    QLabel *encodingLabel;
    QTextEdit *textEdit;
    QDialogButtonBox *buttonBox;

};

#endif // PREVIEWFORM_H
