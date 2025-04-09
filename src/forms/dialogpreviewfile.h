/**
 * @file dialogpreviewfile.h
 * @brief Declares the DialogPreviewFile class used for displaying a preview of network files before importing them into SocNetV.
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



#ifndef DIALOGPREVIEWFILE_H
#define DIALOGPREVIEWFILE_H

#include <QDialog>
#include <QList>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QTextCodec;
class QTextEdit;

class DialogPreviewFile : public QDialog
{
    Q_OBJECT
public:
    explicit DialogPreviewFile(QWidget *parent = Q_NULLPTR);
    void setCodecList(const QList<QTextCodec *> &list);
    void setEncodedData(const QByteArray &data, const QString &fileName, const int &fileFormat);
    QString decodedString() const { return decodedStr; }
signals:
    void loadNetworkFileWithCodec(const QString &fileName, const QString &codecName, const int &fileFormat);
private slots:
    void updateTextEdit();
    void accept();
private:
    QByteArray encodedData;
    QString decodedStr;
    QString m_fileName;
    int m_fileFormat;
    QComboBox *encodingComboBox;
    QLabel *encodingLabel;
    QTextEdit *textEdit;
    QDialogButtonBox *buttonBox;

};

#endif
