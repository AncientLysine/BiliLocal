/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Type.h
*   Time:        2015/04/26
*   Author:      Lysine
*
*   Lysine is a student majoring in Software Engineering
*   from the School of Software, SUN YAT-SEN UNIVERSITY.
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
=========================================================================*/

#pragma once

#include <QtCore>
#include <QtWidgets>
#include "../../Define/Comment.h"

namespace UI
{
	class Type :public QWidget
	{
		Q_OBJECT
	public:
		explicit Type(QWidget *parent);
		void setVisible(bool);

	private:
		QLineEdit * commentL;
		QComboBox * commentS;
		QComboBox * commentM;
		QAction * commentA;
		QPushButton * commentC;
		QPushButton * commentB;

		Comment getComment();

	public slots:
		QColor getColor();
		void setColor(QColor);
	};
}
