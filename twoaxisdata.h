#ifndef TWOAXISDATA_H
#define TWOAXISDATA_H

#include <vector>
#include <algorithm>

class TwoAxisData {
private:
    std::vector<double> x_data;
    std::vector<double> y1_data;
    std::vector<double> y2_data;

    // Data ranges
    struct Range {
        double min = 0;
        double max = 0;
    };

    Range x_range;
    Range y1_range;
    Range y2_range;

    // Update ranges when data changes
    void updateRanges();

    static double interpolate(double x, const std::vector<double>& x_data, const std::vector<double>& y_data);
public:
    // Getters for data
    const std::vector<double>& getXData() const { return x_data; }
    const std::vector<double>& getY1Data() const { return y1_data; }
    const std::vector<double>& getY2Data() const { return y2_data; }

    // Getters for ranges
    double getXMin() const { return x_range.min; }
    double getXMax() const { return x_range.max; }
    double getY1Min() const { return y1_range.min; }
    double getY1Max() const { return y1_range.max; }
    double getY2Min() const { return y2_range.min; }
    double getY2Max() const { return y2_range.max; }

    // Setter
    bool setData(const std::vector<double>& x, const std::vector<double>& y1, const std::vector<double>& y2);

    // Getter for interpolation / direct
    double getY1AtX(double x) const;
    double getY2AtX(double x) const;
};
#endif // TWOAXISDATA_H

