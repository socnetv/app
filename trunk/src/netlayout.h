/***************************************************************************
 SocNetV: Social Networks Visualiser
 version: 0.46
 Written in Qt 4.4
 
                         netlayout  -  description
                             -------------------
    copyright            : (C) 2005-2008 by Dimitris Kalamaras
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

#ifndef NETLAYOUT_H
#define NETLAYOUT_H


using namespace std;

#include <vector>
#include <QtGlobal>	//provides qDebug() function
#include <math.h>		//provides sqrt() function
#include "matrix.h"

class NetLayout
{
public:
	NetLayout() {}
	
	void springEmbedder (vector<vector<double> > &p, vector<vector<double> >&pp, int iter, Matrix&, int, int);
	void FR (vector<vector<double> > &p, vector<vector<double> >&pp, int iter, Matrix&, int, int);
	
// 	~NetLayout();

private:



};



#endif
