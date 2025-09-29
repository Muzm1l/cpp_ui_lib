#ifndef GRAPHTYPE_H
#define GRAPHTYPE_H

#include <QString>

//"BDW", "BRW",  "BTW", "FDW", "FTW" ,"LTW", "RTW"
enum class GraphType
{
    BDW = 0,        // BDWGraph
    BRW = 1,        // BRWGraph  
    BTW = 2,        // BTWGraph
    FDW = 3,        // FDWGraph
    FTW = 4,        // FTWGraph
    LTW = 5,        // LTWGraph
    RTW = 6         // RTWGraph
};

// Helper function to get string representation of GraphType
QString graphTypeToString(GraphType type);

// Helper function to get GraphType from string
GraphType stringToGraphType(const QString& title);

// Helper function to get all available graph types
std::vector<GraphType> getAllGraphTypes();

// Helper function to get all available graph type strings
std::vector<QString> getAllGraphTypeStrings();

#endif // GRAPHTYPE_H
