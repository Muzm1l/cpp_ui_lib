#ifndef RULERSTATE_H
#define RULERSTATE_H

#include <QDateTime>

/**
 * @brief State of a single ruler indicator (RTW or BTW).
 *
 * Drawn as a numbered circle: yellow when selected, white when active
 * but unselected. Inactive rulers are not drawn.
 */
struct RulerState
{
    bool active = false;        ///< Whether the ruler is shown on the graph
    QDateTime timestamp;        ///< Time-axis position of the ruler
    qreal range = 0.0;          ///< Range/bearing-axis position of the ruler
};

#endif // RULERSTATE_H
