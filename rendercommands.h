#ifndef RENDERCOMMANDS_H
#define RENDERCOMMANDS_H

#include <QString>
#include <QDateTime>
#include <utility>

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

struct RenderCommand {
    enum class Kind {
        DataAppend,
        ScopeChange,
        StyleChange,
        ForceInvalidate,
        YRangeChange,
        IncrementalRedrawAllSeries
    };

    Kind kind;

    DataAppend dataAppend;
    ScopeChange scopeChange;
    ForceInvalidate forceInvalidate;

    RenderCommand()
        : kind(Kind::StyleChange)
    {}

    RenderCommand(DataAppend v)
        : kind(Kind::DataAppend), dataAppend(std::move(v))
    {}

    RenderCommand(ScopeChange v)
        : kind(Kind::ScopeChange), scopeChange(std::move(v))
    {}

    RenderCommand(StyleChange)
        : kind(Kind::StyleChange)
    {}

    RenderCommand(ForceInvalidate v)
        : kind(Kind::ForceInvalidate), forceInvalidate(std::move(v))
    {}

    RenderCommand(YRangeChange)
        : kind(Kind::YRangeChange)
    {}

    RenderCommand(IncrementalRedrawAllSeries)
        : kind(Kind::IncrementalRedrawAllSeries)
    {}
};

#endif
