#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);

    // Initialize some sample data for the graph
    std::vector<double> x_data = {0.0, 1.0, 2.0, 3.0, 4.0};
    std::vector<double> y1_data = {0.0, 2.0, 4.0, 6.0, 8.0};  // Linear growth
    std::vector<double> y2_data = {0.0, 1.0, 4.0, 9.0, 16.0}; // Quadratic growth

    // Set the data to the graph widget
    ui->widget->setData(x_data, y1_data, y2_data);
    ui->widget->setAxesLabels("Time (s)", "Speed (m/s)", "Distance (m)");

    // Our ship
    qreal ownShipSpeed = 30;
    qreal ownShipBearing = 90; //  Nautical degrees

    qreal sensorBearing = 250;

    // selected track
    qreal selectedTrackSpeed = 30;
    qreal selectedTrackRange = 50;
    qreal selectedTrackBearing = 200;

    // adopted track
    qreal adoptedTrackSpeed = 30;
    qreal adoptedTrackRange = 100;
    qreal adoptedTrackBearing = 300;

    ui->widget_2->setData(
        ownShipSpeed,
        ownShipBearing,
        sensorBearing,
        adoptedTrackRange,
        adoptedTrackSpeed,
        adoptedTrackBearing,
        selectedTrackRange,
        selectedTrackSpeed,
        selectedTrackBearing
    );
}

Widget::~Widget()
{
    delete ui;
}
