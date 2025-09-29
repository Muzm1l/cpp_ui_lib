#ifndef GRAPHLAYOUT_H
#define GRAPHLAYOUT_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QString>
#include <vector>
#include <map>
#include "graphcontainer.h"
#include "waterfalldata.h"
#include "graphtype.h"

enum class LayoutType
{
    GPW1W = 0,  // 1 window only
    GPW4W = 1,  // 4 windows in 2x2 grid
    GPW2WV = 2, // 2 windows in vertical line
    GPW2WH = 3, // 2 windows in horizontal line
    GPW4WH = 4, // 4 windows in horizontal line
    HIDDEN = 5  // Hidden

};

class GraphLayout : public QWidget
{
    Q_OBJECT
public:
    explicit GraphLayout(QWidget *parent, LayoutType layoutType);
    ~GraphLayout();

    void setLayoutType(LayoutType layoutType);
    LayoutType getLayoutType() const;

    // Sizing methods
    void setGraphViewSize(int width, int height);
    void updateLayoutSizing();

    // Data point methods - operate on all visible containers
    void setData(const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setData(const WaterfallData &data);
    void clearData();
    void addDataPoint(qreal yValue, const QDateTime &timestamp);
    void addDataPoints(const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);

    // Data point methods - operate on specific container by label
    void setData(const QString &containerLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setData(const QString &containerLabel, const WaterfallData &data);
    void clearData(const QString &containerLabel);
    void addDataPoint(const QString &containerLabel, qreal yValue, const QDateTime &timestamp);
    void addDataPoints(const QString &containerLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);

    // Data options management - operate on specific container by label
    void addDataOption(const QString &containerLabel, const GraphType &graphType, WaterfallData &dataSource);
    void removeDataOption(const QString &containerLabel, const GraphType &graphType);
    void clearDataOptions(const QString &containerLabel);
    void setCurrentDataOption(const QString &containerLabel, const GraphType &graphType);
    GraphType getCurrentDataOption(const QString &containerLabel) const;
    std::vector<GraphType> getAvailableDataOptions(const QString &containerLabel) const;
    WaterfallData *getDataOption(const QString &containerLabel, const GraphType &graphType);
    bool hasDataOption(const QString &containerLabel, const GraphType &graphType) const;

    // Data options management - operate on all visible containers
    void addDataOption(const GraphType &graphType, WaterfallData &dataSource);
    void removeDataOption(const GraphType &graphType);
    void clearDataOptions();
    void setCurrentDataOption(const GraphType &graphType);
    
    // Data point methods for specific data sources
    void addDataPointToDataSource(const QString& dataSourceLabel, qreal yValue, const QDateTime& timestamp);
    void addDataPointsToDataSource(const QString& dataSourceLabel, const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps);
    void setDataToDataSource(const QString& dataSourceLabel, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps);
    void setDataToDataSource(const QString& dataSourceLabel, const WaterfallData& data);
    void clearDataSource(const QString& dataSourceLabel);
    
    // Data source management
    WaterfallData* getDataSource(const QString& dataSourceLabel);
    bool hasDataSource(const QString& dataSourceLabel) const;
    std::vector<QString> getDataSourceLabels() const;
    
    // Container management
    std::vector<QString> getContainerLabels() const;
    bool hasContainer(const QString& containerLabel) const;

    // Set the current time 
    void setCurrentTime(const QTime& time);

private:
    LayoutType m_layoutType;

    std::vector<GraphContainer *> m_graphContainers;
    std::vector<QString> m_containerLabels;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_graphContainersRow1Layout;
    QHBoxLayout *m_graphContainersRow2Layout;

    std::map<QString, WaterfallData *> m_dataSources;

    std::vector<QString> dataSourceLabels;

    void attachContainerDataSources();
    void initializeContainers();
    int getContainerIndex(const QString& containerLabel) const;
    void disconnectAllContainerConnections();

signals:
};

#endif // GRAPHLAYOUT_H
