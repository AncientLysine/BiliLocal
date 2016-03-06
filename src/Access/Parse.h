#pragma once

#include <QtCore>
#include "../Utils.h"

namespace Parse
{
	class ResultDelegate
	{
	public:
		class Record
		{
		public:
			virtual ~Record() = default;
			virtual QVector<Comment> get() = 0;
		};

		QSharedPointer<Record> data;

		ResultDelegate() :
			data(nullptr)
		{
		}

		explicit ResultDelegate(Record *data) :
			data(data)
		{
		}

		operator QVector<Comment>()
		{
			return data == nullptr ? QVector<Comment>() : data->get();
		}
	};

	ResultDelegate parseComment(const QByteArray &data, Utils::Site site);
}
