#ifndef RENDERCOMMANDS_H
#define RENDERCOMMANDS_H

#include <QString>
#include <QDateTime>
#include <utility>
#include <variant>

enum class RenderPath {
    None,
    Incremental,
    RangeOnly,
    Full
};

inline int renderPathRank(RenderPath p)
{
    switch (p) {
    case RenderPath::None:
        return 0;
    case RenderPath::RangeOnly:
        return 1;
    case RenderPath::Incremental:
        return 2;
    case RenderPath::Full:
        return 3;
    }
    return 0;
}

inline RenderPath maxRenderPath(RenderPath a, RenderPath b)
{
    return renderPathRank(a) >= renderPathRank(b) ? a : b;
}

struct DataAppend {
    QString seriesLabel;
};

struct ScopeChange {
    QDateTime min;
    QDateTime max;
};

struct StyleChange {};

struct ForceInvalidate {
    QString reason;
};

struct YRangeChange {};

/** Marks every series dirty with Incremental path (e.g. symbol layer refresh). */
struct IncrementalRedrawAllSeries {};

using RenderCommand = std::variant<DataAppend, ScopeChange, StyleChange, ForceInvalidate, YRangeChange,
                                   IncrementalRedrawAllSeries>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#endif
