#include "Common.h"
#include "Sample.h"

#ifdef GRAPHIC_DEBUG
Sample::Data::Data()
	: root(nullptr)
	, last(nullptr)
{
	root = last = new Node{ 0, nullptr, {} };
	timer.start();
}

Sample::Data::~Data()
{
	print(true);
	delete root;
}

Sample::Data &Sample::d()
{
	static QThreadStorage<Data> data;
	return data.localData();
}

Sample::Sample(QString name)
	: open(false)
	, name(name)
{
	start();
}

Sample::~Sample()
{
	close();
}

void Sample::start()
{
	if (open) {
		return;
	}
	Node *&last = d().last->children[name];
	if (last == nullptr) {
		last = new Node{ 0, d().last, {} };
	}
	d().last = last;
	time = d().timer.nsecsElapsed();
	open = true;
}

void Sample::close()
{
	if (open == false) {
		return;
	}
	d().last->time += d().timer.nsecsElapsed() - time;
	d().last = d().last->parent;
	open = false;
}

void Sample::print(bool clear)
{
	Q_ASSERT(d().root == d().last);
	d().last->time = d().timer.nsecsElapsed();
	QDebug debug(QtDebugMsg);
	debug << "----BiliLocal Profile----" << endl;
	print(debug, d().root, 0, clear);
	if (clear) {
		d().root->time = 0;
		d().root->children.clear();
		d().timer.start();
	}
}

void Sample::print(QDebug &out, Node *node, int depth, bool clear)
{
	for (auto iter = node->children.begin(); iter != node->children.end(); ++iter) {
		for (int i = 0; i < depth; ++i) {
			out.space().space();
		}
		out.noquote().nospace() << iter.key();
		int align = 40 - depth * 2 - iter.key().length();
		for (int i = 0; i < align; ++i) {
			out.space();
		}
		Node *n = *iter;
		int pct = n->parent ? (n->time * 100 / n->parent->time) : 100;
		out.noquote().nospace() << "pct:" << pct << "\ttime:" << n->time / 1000 << endl;
		print(out, n, depth + 1, clear);
		if (clear) {
			delete n;
		}
	}
}
#endif
