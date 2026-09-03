#include <matplot/matplot.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace plt = matplot;
int c = 1;

// The circle is: 10 = (x - 2)^2 + y^2.
// Rewriting it gives: (x - 2)^2 + y^2 = 10.
constexpr double kCircleCenterX = 2.0;
constexpr double kCircleRadiusSquared = 10.0;
constexpr double kCircleRadius = 3.16227766016837933199;  // sqrt(10)

// Find the leftmost x-value where a horizontal line y = constant touches the
// circle. If there is no real point, return an empty optional value.
std::optional<double> leftmostCircleX(double y) {
    // Solve the circle equation for x:
    // x = 2 +/- sqrt(10 - y^2).
    const double insideSquareRoot = kCircleRadiusSquared - y * y;

    // A negative value means this horizontal line never reaches the circle.
    if (insideSquareRoot < 0.0) {
        return std::nullopt;
    }

    // The minus sign chooses the smaller (leftmost) x-value.
    return kCircleCenterX - std::sqrt(insideSquareRoot);
}

// Make x-coordinates for a circle centered at (2, 0).
std::vector<double> makeCircleX(const std::vector<double>& angles) {
    std::vector<double> xValues;
    xValues.reserve(angles.size());

    for (double angle : angles) {
        xValues.push_back(kCircleCenterX +
                          kCircleRadius * std::cos(angle));
    }

    return xValues;
}

// Make y-coordinates for a circle centered at (2, 0).
std::vector<double> makeCircleY(const std::vector<double>& angles) {
    std::vector<double> yValues;
    yValues.reserve(angles.size());

    for (double angle : angles) {
        yValues.push_back(kCircleRadius * std::sin(angle));
    }

    return yValues;
}

int main() {
    // Draw several horizontal reference lines so the full requested view is
    // visible. Lines at y >= 2 do not intersect this circle, but are still
    // useful as reference lines.
    std::vector<double> yLevels;
    for (double y = -15.0; y <= 15.0; y += 0.5) {
        yLevels.push_back(y);
    }
    // Create a smooth circle using many angles from 0 to 2*pi.
    constexpr double pi = 3.14159265358979323846;
    const std::vector<double> angles = plt::linspace(0.0, 2.0 * pi, 500);
    const std::vector<double> circleX = makeCircleX(angles);
    const std::vector<double> circleY = makeCircleY(angles);

    // These values define the visible part of the graph. A finite graph window
    // lets us draw the mathematical condition x < intersection clearly.
    auto minY = std::min_element(circleY.begin(), circleY.end());
    auto maxY = std::max_element(circleY.begin(), circleY.end());

    double yMin = *minY - 5.0;
    double yMax = *maxY + 5.0;

    auto minX = std::min_element(circleX.begin(), circleX.end());
    auto maxX = std::max_element(circleX.begin(), circleX.end());

    double xMin = *minX - 5.0;
    double xMax = *maxX + 5.0;



    // Quiet mode waits until the end before drawing. This prevents Gnuplot
    // from receiving hundreds of partial redraws while we style the graph.
    auto figure = plt::figure(true);
    auto axes = figure->current_axes();

    // Keep every plotted object on the same axes. Without hold enabled,
    // Matplot++ replaces the previous plot each time axes->plot is called.
    axes->hold(true);



    // Draw the circle first so the horizontal lines and intercept markers are
    // easy to see on top of it.
    auto circle = axes->plot(circleX, circleY);
    circle->line_width(2.5);
    circle->color("black");
    circle->display_name("10 = (x - 2)^2 + y^2");

    // Examine each requested y-level and draw its horizontal line.
    for (double y : yLevels) {
        const std::optional<double> intersectionX = leftmostCircleX(y);

        std::vector<double> lineX;

        if (intersectionX.has_value()) {
            // Draw only the part where x is less than the intercept. The tiny
            // gap keeps the endpoint mathematically separate from the marker.
            constexpr double endpointGap = 1.0e-6;
            lineX = {xMin, intersectionX.value() - endpointGap};
        } else {
            // y >= 2 is outside the circle (the circle only reaches y = +/-1),
            // so it has no intercept to stop at. Keep it as a reference line.
            lineX = {xMin, xMax};
        }

        // A vector filled with y draws a horizontal line at that y-level.
        const std::vector<double> lineY(lineX.size(), y);
        auto horizontalLine = axes->plot(lineX, lineY);
        horizontalLine->line_width(1.8);

        // Use the same color for every horizontal line. The circle remains
        // black so it is visually distinct.
        horizontalLine->color("blue");

        if (intersectionX.has_value()) {
            horizontalLine->display_name("y = " + std::to_string(y) +
                                         " (x < intercept)");

            // Draw a red circle marker at the leftmost intercept point.
            auto marker = axes->plot(
                std::vector<double>{intersectionX.value()},
                std::vector<double>{y}, "or");
            marker->marker_size(8);
            marker->marker_face_color("red");
            marker->color("red");
            marker->display_name("leftmost intercept");

            // Print the exact point so it is also available in the terminal.
            std::cout << std::fixed << std::setprecision(3)
                      << "y = " << y << ": leftmost intercept = ("
                      << intersectionX.value() << ", " << y << ")\n";
        } else {
            // A dotted style distinguishes a line with no circle intercept.
            horizontalLine->line_style(":");
            horizontalLine->display_name("y = " + std::to_string(y) +
                                         " (no intercept)");

            std::cout << std::fixed << std::setprecision(3)
                      << "y = " << y << ": no real circle intercept\n";
        }
    }

    // Give the graph useful labels and a readable viewing window.
    plt::title("Leftmost Intersections with a Circle");
    plt::xlabel("x");
    plt::ylabel("y");
    plt::xlim({xMin, xMax});
    plt::ylim({yMin, yMax});
    plt::grid(plt::on);
    // Open the native Matplot++ graph window and keep it open.
    figure->show();
    return 0;
}
