# CplusIntercept

`CplusIntercept` is a small C++ program that uses the native C++ plotting
library [Matplot++](https://github.com/alandefreitas/matplotplusplus).

It draws:

- the circle `1 = (x + 4)^2 + y^2`;
- horizontal lines `y = 0`, `y = 1`, and `y = 2`;
- the leftmost intersection for each line that reaches the circle.

The circle has center `(-4, 0)` and radius `1`. Solving for the leftmost
intersection gives:

```text
x = -4 - sqrt(1 - y^2)
```

Therefore:

```text
y = 0  ->  (-5, 0)
y = 1  ->  (-4, 1)
y = 2  ->  no real intersection
```

Lines with an intersection are drawn only for `x <` that intersection. The
intersection itself is shown as a red marker. Since `y = 2` never reaches the
circle, it is shown as a dashed reference line across the graph.

## Build and run

On macOS, install the build tools once if needed:

```bash
brew install cmake gnuplot
```

From this project directory:

```bash
cmake -S . -B build
cmake --build build
./build/CplusIntercept
```

The first CMake command downloads Matplot++ automatically if it is not already
available. Matplot++ uses its Gnuplot backend by default, so install Gnuplot
once if it is not already installed:

```bash
brew install gnuplot       # macOS
# sudo apt install gnuplot # Ubuntu/Debian
```

A native plotting window will open when the program runs.
