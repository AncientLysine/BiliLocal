/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Async.h
*   Time:        2016/09/10
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

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QSharedPointer>
#include <QTimer>
#include <QVector>
#include <functional>
#include <type_traits>

template<typename F>
typename std::result_of<F()>::type makeFuture(F func)
{
    return func();
}

namespace
{
    template<typename F, typename T>
    struct ResultOf
    {
        typedef typename std::result_of<F(T)>::type Type;
    };

    template<typename F>
    struct ResultOf<F, void>
    {
        typedef typename std::result_of<F( )>::type Type;
    };
}

template<typename O, typename S>
class SignalResult
{
    typedef QSharedPointer<QVector<std::function<void(O, S)>>> V;

public:
    SignalResult()
        : sender(nullptr)
        , signal(nullptr)
        , ca(V::create())
    {
    }

    SignalResult(O sender, S signal)
        : sender(sender)
        , signal(signal)
        , ca(V::create())
    {
    }

    void setResult(const SignalResult<O, S> &result)
    {
        Q_ASSERT(sender == nullptr && signal == nullptr);
        sender = result.sender;
        signal = result.signal;
        for(auto iter : *ca) {
            iter(sender, signal);
        }
        ca->clear();
    }

    template<typename F>
    typename ResultOf<F, O>::Type onFinish(F cb)
    {
        return OnFinishImpl<F, typename ResultOf<F, O>::Type>::Do(sender, signal, ca, cb);
    }

private:
    O sender;
    S signal;
    V ca;

    template<typename F, typename R>
    struct OnFinishImpl
    {
        static R Do(O o, S s, V ca, F cb)
        {
            R r;
            auto reg = [cb, r](O o, S s){
                QObject::connect(o, s, [cb, o, r]() {
                    R(r).setResult(cb(o));
                });
            };
            if (o && s) {
                reg(o, s);
            }
            else{
                ca->append(reg);
            }
            return r;
        }
    };

    template<typename F>
    struct OnFinishImpl<F, void>
    {
        static void Do(O o, S s, V ca, F cb)
        {
            auto reg = [cb](O o, S s){
                QObject::connect(o, s, [cb, o]() {
                    cb(o);
                });
            };
            if (o && s) {
                reg(o, s);
            }
            else{
                ca->append(reg);
            }
        }
    };
};

template<typename O, typename S>
SignalResult<O, S> makeFuture(O sender, S signal)
{
    return SignalResult<O, S>(sender, signal);
}

template<typename T>
class FutureResult
{
    typedef QSharedPointer<QFutureWatcher<T>> W;

public:
    FutureResult()
        : wa(W::create())
    {
    }

    explicit FutureResult(const QFuture<T> &future)
        : wa(W::create())
	{
		//calling onFinish after setFuture is likely to produce race
		auto watcher = wa;
		QTimer::singleShot(0, [watcher, future]() mutable {
			watcher->setFuture(future);
		});
    }

    void setResult(const FutureResult<T> &result)
    {
        wa->setFuture(result.wa->future());
    }

    template<typename F>
    typename ResultOf<F, T>::Type onFinish(F cb)
    {
        return OnFinishImpl<F, typename ResultOf<F, T>::Type>::Do(wa, cb);
    }

private:
    W wa;

    template<typename F, typename R>
    struct OnFinishImpl
    {
        static R Do(W wa, F cb)
        {
            R r;
            QObject::connect(wa.data(), &QFutureWatcher<T>::finished, [=]() mutable {
                auto t = wa->future().result();
                wa.clear();
                r.setResult(cb(std::move(t)));
            });
            return r;
        }
    };

    template<typename F>
    struct OnFinishImpl<F, void>
    {
        static void Do(W wa, F cb)
        {
            QObject::connect(wa.data(), &QFutureWatcher<T>::finished, [=]() mutable {
                auto t = wa->future().result();
                wa.clear();
                cb(std::move(t));
            });
        }
    };
};

template<typename T>
FutureResult<T> makeFuture(const QFuture<T> &future)
{
    return FutureResult<T>(future);
}
