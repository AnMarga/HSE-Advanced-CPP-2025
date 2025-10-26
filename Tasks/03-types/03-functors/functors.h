#pragma once

template <class Functor>
class ReverseBinaryFunctor {
public:
    ReverseBinaryFunctor(Functor functor) : f_(functor) {
    }

    auto operator()(auto a, auto b) const {
        return f_(b, a);
    }

private:
    Functor f_;
};

template <class Functor>
class ReverseUnaryFunctor {
public:
    ReverseUnaryFunctor(Functor functor) : f_(functor) {
    }

    bool operator()(auto arg) const {
        return !f_(arg);
    }

private:
    Functor f_;
};

template <class Functor>
ReverseUnaryFunctor<Functor> MakeReverseUnaryFunctor(Functor functor) {
    return ReverseUnaryFunctor<Functor>(functor);
}

template <class Functor>
ReverseBinaryFunctor<Functor> MakeReverseBinaryFunctor(Functor functor) {
    return ReverseBinaryFunctor<Functor>(functor);
}
