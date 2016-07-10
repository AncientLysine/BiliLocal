#pragma once

#include <QtCore>
#include "../Utils.h"
#include <functional>

namespace Parse
{
	class ResultDelegate
	{
	public:
		typedef QVector<Comment> Result;
		typedef std::function<void()> Finish;

		class Record
		{
		public:
			virtual ~Record() = default;
			virtual void onFinish(Finish) = 0;
			virtual Result get() = 0;
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

		void onFinish(Finish cb)
		{
			if (data) {
				data->onFinish(cb);
			}
			else {
				cb();
			}
		}

		void clear()
		{
			data.clear();
		}

		operator Result()
		{
			return data == nullptr ? Result() : data->get();
		}
	};

	ResultDelegate parseComment(const QByteArray &data, Utils::Site site);
}
