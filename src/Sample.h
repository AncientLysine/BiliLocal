#pragma once

#include <QtCore>

class Sample
{
public:
#ifdef GRAPHIC_DEBUG
	explicit Sample(QString name);
	~Sample();
	void start();
	void close();

	static void print(bool clear = true);

private:
	bool open;
	QString name;
	quint64 time;

	struct Node
	{
		quint64 time;

		Node *parent;
		QMap<QString, Node *> children;
	};

	struct Data
	{
		QElapsedTimer timer;
		Node *root;
		Node *last;

		explicit Data();
		~Data();
	};
	static Data &d();

	static void print(QDebug &out, Node *node, int depth, bool clear);
#else
	template<class T>
	explicit Sample(const T &)
	{
	}

	void start() {}
	void close() {}

	static void print(bool clear = true)
	{
		Q_UNUSED(clear);
	}
#endif
};
