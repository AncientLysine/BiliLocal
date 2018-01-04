/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Eval.cpp
*   Time:        2013/05/10
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

#include "Common.h"
#include "Eval.h"
#include <exception>

namespace{
	template<class T>
	class SStack
	{
	public:
		inline T &top()
		{
			if (isEmpty()){
				throw std::runtime_error("token mismatch");
			}
			return stk.top();
		}

		inline T pop()
		{
			if (isEmpty()){
				throw std::runtime_error("token mismatch");
			}
			return stk.pop();
		}

		inline void push(const T &i)
		{
			stk.push(i);
		}

		inline bool isEmpty()
		{
			return stk.isEmpty();
		}

	private:
		QStack<T> stk;
	};
}

double Utils::evaluate(QString exp)
{
	auto priority = [](QChar o){
		switch (o.unicode())
		{
		case '(':
			return 1;
		case '+':
		case '-':
			return 2;
		case '*':
		case '/':
			return 3;
		case '+' + 128:
		case '-' + 128:
			return 4;
		case ':':
		case ':' + 128:
			return 5;
		default:
			return 0;
		}
	};
	exp.remove(' ');
	QString pst;
	SStack<QChar> opt;
	int i = 0;
	opt.push('#');
	while (i < exp.length()){
		if (exp[i].isDigit() || exp[i] == '.'){
			pst.append(exp[i]);
		}
		else{
			auto tra = [&](){
				pst.append(' ');
				while (priority(exp[i]) <= priority(opt.top())){
					pst.append(opt.pop());
				}
				opt.push(exp[i]);
			};
			int colon = 0;
			switch (exp[i].unicode()){
			case '(':
				opt.push(exp[i]);
				break;
			case ')':
				while (opt.top() != '('){
					pst.append(opt.pop());
				}
				opt.pop();
				break;
			case '+':
			case '-':
			{
				if ((i == 0 || (!exp[i - 1].isDigit() && exp[i - 1] != ')')) && (i + 1) < exp.length() && (exp[i + 1].isDigit() || exp[i + 1] == '(')){
					exp[i].unicode() += 128;
				}
				tra();
				break;
			}
			case ':':
				switch (colon++){
				case 2:
					exp[i].unicode() += 128;
				case 1:
				case 0:
					break;
				default:
					throw std::runtime_error("colon overflow");
				}
				tra();
				break;
			case '*':
			case '/':
				tra();
				break;
			default:
				throw std::runtime_error("token unrecognized");
			}
		}
		++i;
	}
	while (!opt.isEmpty()){
		pst.append(opt.pop());
	}
	SStack<double> num;
	i = 0;
	while (pst[i] != '#'){
		if (pst[i].isDigit() || pst[i] == '.'){
			double n = 0;
			while (pst[i].isDigit()){
				n = n * 10 + pst[i++].toLatin1() - '0';
			}
			if (pst[i] == '.'){
				++i;
				double d = 1;
				while (pst[i].isDigit()){
					n += (d /= 10)*(pst[i++].toLatin1() - '0');
				}
			}
			num.push(n);
		}
		else{
			switch (pst[i].unicode()){
			case '+' + 128:
				num.push(+num.pop());
				break;
			case '-' + 128:
				num.push(-num.pop());
				break;
			case '+':
			{
				double r = num.pop(), l = num.pop();
				num.push(l + r);
				break;
			}
			case '-':
			{
				double r = num.pop(), l = num.pop();
				num.push(l - r);
				break;
			}
			case '*':
			{
				double r = num.pop(), l = num.pop();
				num.push(l*r);
				break;
			}
			case '/':
			{
				double r = num.pop(), l = num.pop();
				num.push(l / r);
				break;
			}
			case ':':
			{
				double r = num.pop(), l = num.pop();
				num.push(l * 60 + r);
				break;
			}
			case ':' + 128:
			{
				double r = num.pop(), l = num.pop();
				num.push(l * 24 + r);
				break;
			}
			}
			i++;
		}
	}
	return num.top();
}
