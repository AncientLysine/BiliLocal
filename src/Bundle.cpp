#include "Common.h"
#include "Bundle.h"
#include "Utils.h"
#include <QtCore>

namespace
{
	void push(Utils::Path path)
	{
		QStack<QFileInfo> wait;
		QString type;
		switch (path)
		{
		case Utils::Locale:
			type = "Locale";
			break;
		case Utils::Plugin:
			type = "Plugin";
			break;
		case Utils::Script:
			type = "Script";
			break;
		default:
			break;
		}
		wait.push(QFileInfo(":/Bundle/" + type));
		QDir base = wait.top().filePath();
		QDir real = Utils::localPath(path);
		QDir root = QFileInfo(real.path()).dir();
		while (!wait.isEmpty()) {
			const QFileInfo &info = wait.pop();
			if (info.isDir()) {
				QDir node = info.filePath();
				for (const QFileInfo &iter : node.entryInfoList()) {
					if (iter != info) {
						wait.push(iter);
					}
				}
			}
			else if (info.isFile()) {
				QString srcPath = info.absoluteFilePath();
				QString relPath = base.relativeFilePath(srcPath);
				QString dstPath = real.absoluteFilePath(relPath);
				QFileInfo dest(dstPath);
				if (dest.exists()) {
					continue;
				}
				QString dirPath = root.relativeFilePath(dest.absolutePath());
				if (!root.mkpath(dirPath) || !QFile::copy(srcPath, dstPath)) {
					qDebug() << "Bundle: push failed," << dstPath;
				}
			}
		}
	}
}

void Bundle::push()
{
	::push(Utils::Locale);
	::push(Utils::Plugin);
	::push(Utils::Script);
}
