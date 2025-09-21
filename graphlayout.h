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
    explicit GraphLayout(QWidget *parent, LayoutType layoutType, const std::vector<QString>& dataSourceLabels);
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

    // Data point methods - operate on specific container by index
    void setData(int containerIndex, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setData(int containerIndex, const WaterfallData &data);
    void clearData(int containerIndex);
    void addDataPoint(int containerIndex, qreal yValue, const QDateTime &timestamp);
    void addDataPoints(int containerIndex, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);

    // Data options management - operate on specific container by index
    void addDataOption(int containerIndex, const QString &title, WaterfallData &dataSource);
    void removeDataOption(int containerIndex, const QString &title);
    void clearDataOptions(int containerIndex);
    void setCurrentDataOption(int containerIndex, const QString &title);
    QString getCurrentDataOption(int containerIndex) const;
    std::vector<QString> getAvailableDataOptions(int containerIndex) const;
    WaterfallData *getDataOption(int containerIndex, const QString &title);
    bool hasDataOption(int containerIndex, const QString &title) const;

    // Data options management - operate on all visible containers
    void addDataOption(const QString &title, WaterfallData &dataSource);
    void removeDataOption(const QString &title);
    void clearDataOptions();
    void setCurrentDataOption(const QString &title);
    
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

private:
    LayoutType m_layoutType;

    std::vector<GraphContainer *> m_graphContainers;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_graphContainersRow1Layout;
    QHBoxLayout *m_graphContainersRow2Layout;

    std::map<QString, WaterfallData *> m_dataSources;

    void attachContainerDataSources();
    void initializeContainers();

signals:
};

#endif // GRAPHLAYOUT_H
