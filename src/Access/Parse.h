#pragma once

#include <QByteArray>
#include "../Define/Comment.h"
#include "../Utility/Async.h"
#include "../Utility/Text.h"

namespace Parse
{
	FutureResult<QVector<Comment>> parseComment(const QByteArray &data, Utils::Site site);
}
