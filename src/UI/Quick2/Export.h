#pragma once

#include "../../Config.h"
#include "../../Local.h"
#include "../../Access/Load.h"
#include "../../Access/Post.h"
#include "../../Access/Seek.h"
#include "../../Access/Sign.h"
#include "../../Model/Danmaku.h"
#include "../../Model/Running.h"
#include "../../Model/List.h"
#include "../../Model/Shield.h"
#include "../../Player/APlayer.h"
#include "../../Render/ARender.h"
#include <QtCore>

namespace UI
{
	class Export :public QObject
	{
		Q_OBJECT
	public:
		explicit Export(QObject *parent = nullptr)
			: QObject(parent)
		{
		}

#define lReg(ModuleType, Name) \
		Q_PROPERTY(ModuleType * Name READ get##ModuleType) \
		ModuleType *get##ModuleType() const { return lApp->findObject<ModuleType>(); }
		lReg(Config, Config);
		lReg(Shield, Shield);
		lReg(ARender, Render);
		lReg(APlayer, Player);
		lReg(Danmaku, Danmaku);
		lReg(Running, Running);
		lReg(List, List);
		lReg(Load, Load);
		lReg(Post, Post);
		lReg(Seek, Seek);
		lReg(Sign, Sign);
#undef lReg
	};
}