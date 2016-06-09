/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Sign.cpp
*   Time:        2015/11/14
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
#include "Sign.h"
#include "AccessPrivate.h"
#include "../Utils.h"

extern "C"
{
#include <openssl/pem.h>
#include <openssl/rsa.h>
}

class SignPrivate : public AccessPrivate<Sign, Sign::Proc, Sign::Task>
{
public:
	explicit SignPrivate(Sign *sign):
		AccessPrivate<Sign, Sign::Proc, Sign::Task>(sign)
	{
	}

	virtual void onShift() override
	{
		Q_Q(Sign);
		Sign::Task &task = queue.head();
		emit q->stateChanged(task.state);
		remain.insert(task.data.isEmpty() ? manager.get(task.request) : manager.post(task.request, task.data));
	}
};

Sign::Sign(QObject *parent)
	: QObject(parent), d_ptr(new SignPrivate(this))
{
	Q_D(Sign);
	setObjectName("Sign");

	auto biCaptcha = [this](){
		Task &task = *getHead();
		QString url("https://passport.%1/captcha");
		url = url.arg(Utils::customUrl(Utils::Bilibili));
		task.request.setUrl(url);
		forward(Code);
	};

	auto biProcess = [this, biCaptcha](QNetworkReply *reply){
		Task &task = *getHead();
		switch (task.state){
		case None:
		{
			QString url("http://member.%1/main.html");
			url = url.arg(Utils::customUrl(Utils::Bilibili));
			task.request.setUrl(url);
			forward(Test);
			break;
		}
		case Test:
		{
			QRegularExpressionMatch m = QRegularExpression("(?<=\\>).*(?=\\<\\/h3\\>)").match(reply->readAll());
			if (m.hasMatch()){
				task.username = m.captured();
				task.logged = true;
				emit stateChanged(task.state = Wait);
			}
			else{
				biCaptcha();
			}
			break;
		}
		case Code:
		{
			task.data = reply->readAll();
			emit stateChanged(task.state = Wait);
			task.data.clear();
			break;
		}
		case Wait:
		{
			QString url = "https://passport.%1/login?act=%2";
			url = url.arg(Utils::customUrl(Utils::Bilibili));
			if (task.logged){
				task.request.setUrl(url.arg("exit"));
				task.state = Data;
			}
			else{
				task.request.setUrl(url.arg("getkey"));
				task.state = Salt;
			}
			forward();
			break;
		}
		case Salt:
		{
			//RSA Encrypt
			try{
				QJsonObject key = QJsonDocument::fromJson(reply->readAll()).object();
				QByteArray pub = key["key"].toString().toUtf8();
				BIO *bio = BIO_new_mem_buf(pub.data(), pub.length());
				if (!bio){
					throw "failed to generate BIO";
				}
				RSA *rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
				if (!rsa){
					throw "failed to generate RSA_PUBKEY";
				}
				task.password.prepend(key["hash"].toString());
				QByteArray dat = task.password.toUtf8();
				QByteArray buf;
				buf.resize(1024);
				int len = RSA_public_encrypt(dat.length(), (const unsigned char *)dat.data(), (unsigned char*)buf.data(), rsa, RSA_PKCS1_PADDING);
				if (len == -1){
					throw "failed to encrypt using RSA";
				}
				buf.resize(len);
				task.password = buf.toBase64();
			}
			catch (...)
			{
				task.error = tr("An error occurred in RSA encryption.");
				biCaptcha();
			}

			QString url("https://passport.%1/login/dologin");
			url = url.arg(Utils::customUrl(Utils::Bilibili));
			task.request.setUrl(url);
			task.request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
			QUrlQuery params;
			params.addQueryItem("act", "login");
			params.addQueryItem("userid", task.username);
			params.addQueryItem("pwd", task.password);
			params.addQueryItem("vdcode", task.captcha.toUpper());
			params.addQueryItem("keeptime", "604800");
			task.data = params.query(QUrl::FullyEncoded).toUtf8();
			forward(Data);
			task.data.clear();
			break;
		}
		case Data:
		{
			if (task.logged){
				task.logged = false;
			}
			else{
				QByteArray data = reply->readAll();
				QString page = QTextCodec::codecForHtml(data, QTextCodec::codecForName("UTF-8"))->toUnicode(data);
				int sta = page.indexOf("document.write(\"");
				if (sta < 0){
					task.logged = true;
				}
				else{
					sta += 16;
					task.error = page.mid(sta, page.indexOf("\"", sta) - sta);
				}
			}
			if (task.logged){
				emit stateChanged(task.state = Wait);
			}
			else{
				biCaptcha();
			}
		}
		}
	};
	d->pool.append({ "Bilibili", 0, biProcess });

	connect(this, &Sign::stateChanged, [this](int code){
		switch (code){
		case None:
		case Test:
		case Code:
		case Wait:
		case Salt:
		case Data:
			break;
		default:
		{
			Q_D(Sign);
			if (!d->tryNext()){
				emit errorOccured(code);
			}
			break;
		}
		}
	});
}

Sign::~Sign()
{
	delete d_ptr;
}

void Sign::addProc(const Sign::Proc *proc)
{
	Q_D(Sign);
	d->addProc(proc);
}

const Sign::Proc *Sign::getProc(QString name)
{
	Q_D(Sign);
	return d->getProc(name);
}

QStringList Sign::modules()
{
	Q_D(Sign);
	QStringList list;
	for (const Proc &iter : d->pool){
		list << iter.name;
	}
	return list;
}

void Sign::forward()
{
	Q_D(Sign);
	d->forward();
}

void Sign::forward(int state)
{
	Q_D(Sign);
	d->queue.head().state = state;
	d->forward();
}

void Sign::dequeue()
{
	Q_D(Sign);
	d->dequeue();
}

bool Sign::enqueue(QString site)
{
	Task task;
	task.code = site;
	task.processer = getProc(site);
	if (task.processer == nullptr){
		return false;
	}

	Q_D(Sign);
	if (d->queue.size()){
		d->dequeue();
	}

	return d->enqueue(task);
}

Sign::Task *Sign::getHead()
{
	Q_D(Sign);
	return d->getHead();
}
