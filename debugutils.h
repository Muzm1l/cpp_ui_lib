#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#include <QDebug>
#include <QLoggingCategory>

/**
 * @brief Debug utility for conditional debug output
 * 
 * Usage:
 *   DEBUG_OUT() << "Debug message";
 *   DEBUG_OUT() << "Value:" << value;
 * 
 * To enable/disable debug output:
 *   DebugUtils::setDebugEnabled(true);  // Enable
 *   DebugUtils::setDebugEnabled(false); // Disable
 */
class DebugUtils
{
public:
    /**
     * @brief Enable or disable debug output globally
     * @param enabled true to enable debug output, false to disable
     */
    static void setDebugEnabled(bool enabled) {
        s_debugEnabled = enabled;
    }
    
    /**
     * @brief Check if debug output is currently enabled
     * @return true if debug is enabled, false otherwise
     */
    static bool isDebugEnabled() {
        return s_debugEnabled;
    }

private:
    static bool s_debugEnabled;
};

// Convenience macro for conditional debug output
// If debug is disabled, this expands to a no-op that discards the expression
#ifdef QT_NO_DEBUG_OUTPUT
    #define DEBUG_OUT() if (false) QNoDebug()
#else
    #define DEBUG_OUT() if (DebugUtils::isDebugEnabled()) qDebug()
#endif

#endif // DEBUGUTILS_H

