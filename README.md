# Brplot
brplot - [b]etter [r]l[plot]
Small application and library that plots data. [`Screenshots`](#Screenshots)

It works as a [`library`](#Brplot-as-an-library) or as an standalone application.

## Running brplot as an application
brplot reads data from standard input and draws the data. Expected format is described [`here`](#Input-format). But I think you'd be better of just reading the examples:

### Examples
I think that more or less all the examples listed on [ttyplot examples](https://github.com/tenox7/ttyplot#examples) should work with brplot ( just replace ttyplot with brplot. )
But here are some more examples:

#### Plot the first 8 Fibonacci numbers from user input
```bash
brplot
1 1 2 3 5 8 13 21
```

#### Nice Plot
```bash
# Plot numbers from 1 to 100
seq 100 | brplot;
```

#### Square(Nice) Plot
```bash
# Plot squeres of numbers from 1 to 100
python -c "[print(x*x) for x in range(100)]" | brplot;
```

#### Plot from data that is streamed to an UDP socket
```bash
nc -ulkp 8888 | brplot;
```

#### Plot from data that is streamed to a TCP socket
```bash
nc -lkp 8888 | brplot;
```

#### Plot random data
```bash
# This will most likely crash
cat /dev/random | brplot;
```

#### Plot the temperature of core 0 on your CPU.

```bash
#Plot temeratur of core 0 on your cpu.

# Step 0: Get sensors value
sensors;
# output:
#...
#Package id 0:  +52.0°C  (high = +105.0°C, crit = +105.0°C)
#Core 0:        +52.0°C  (high = +105.0°C, crit = +105.0°C)
#Core 1:        +52.0°C  (high = +105.0°C, crit = +105.0°C)
#...

# Step 1: Grep Core 0:
sensors | grep 'Core 0';
#output:
#Core 0:        +52.0°C  (high = +105.0°C, crit = +105.0°C)

# Step 2: Get only temperature:
sensors | grep 'Core 0' | awk -p '{print $3}';
# output:
# +52.0°C 

# Step 3: Create a loop and pipe the value of core temperature to brplot every 0.1 sec.
while :; do echo $(sensors | grep 'Core 0' | awk -p '{print $3}'); sleep 0.1; done | brplot
# brplot should not care about nonnumeric symbols So input `+52.0°C` should be fine.
```

#### Plot temperature of all CPU core 0.
```bash
while :; do echo $(sensors | grep 'Core' | awk -p '{print substr($3, 1, 4) ";" $2}'); sleep 0.1; done | brplot
# substr is needed because "+52.0C;1" would be recogined as:
# Add 52 to group 0
# Add 1 to group 1
# substr transforms "+52.0C" to "52.0", so one awk will output lines like "52.0;0"
```

#### Plot ram usage.
```bash
while :; do echo $(free | grep Mem | awk -p '{print $3/1024}'); sleep 0.01; done | brplot
```

#### UDP client in python

* Write an UDP client in python:
```python
import socket

client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
y_value = 69 # This can be any float32 value
group_id = 420 # This can be any int32 value
port_number = 42069 # This is a port number and it MUST be 42069
client_socket.sendto(f"{y_value};{group_id}".encode(), ("localhost", port_number))
```

Start brplot that listens to UDP port 8888:
```bash
nc -ulkp 8888 | brplot;
```

### Input format
```[[x-value],]y-value[,[z-value]][;[line-index]]```
```--command value```

#### Input examples
* 10      - insert point (new_x, 10) to line group 0
* 10,12   - insert point (10, 12) to line group 0
* 10;1    - insert point (new_x, 10) to line group 1
* 10,12;2 - insert point (10, 12) to line group 2

#### All commands 
* ```--zoom value```                  - zoom x & y axis to value
* ```--zoomx value```                 - zoom x axis to value
* ```--zoomy value```                 - zoom y axis to value
* ```--offsetx value```               - offset x axis to value
* ```--offsety value```               - offset y axis to value
* ```--show value```                  - show value-th group
* ```--hide value```                  - hide value-th group
* ```--extract group "Extract-str"``` - Define custom extraction rules

#### Extract-str
This is a string used to define how to transform input to get the data out.

Example Extract strings are:
|extractor string  |input string                          |out-x  |out-y|
|------------------|--------------------------------------|-------|-----|
| ```abc%x```      | ```abc12```                          |12.f   |NULL|
| ```abc%x```      | ```abc-12```                         |-12.f  |NULL|
| ```a%xbc```      | ```a12.2bc```                        |12.2f  |NULL|
| ```*%xabc```     | ```-------12e12abc```                |-12e12f|NULL|
| ```*-%xabc```    | ```-12e12abc```                      |12e12f |NULL|
| ```*\\\\%xabc``` | ```-------\\---\\\\12abc```          |12.f   |NULL|
| ```*\\%a%xabc``` | ```---abs\\%a12e12---\\%a10e10abc``` |10e10f |NULL|
| ```%y*aa%x```    | ```12a14aaaa13```                    |13.f   |12.f|
| ```%y.%x```      | ```12.13.14.15```                    |14.15f |12.13f|

Some more examples of valid and invalid extractors:
|is valid| extractor | desc |
|--------|-----------|------|
|true| "abc%x"||
|true| "a%xbc\\\\"||
|true| "*%xabc"||
|true| "*\\%a%xabc"||
|true| "*\\\\%xabc"||
|true| "%y*%x"||
|false| "abc%a%x" |  %a is not a valid capture|
|false| "abc%" |     % is unfinised capture|
|false| "abc\\" |    \ is unfinished escape|
|false| "a**bc" |    wild can't follow wild|
|false| "a%xbc*" |   wild can't be last character|
|false| "*\\%xabc" | Nothing is captured. %x is escaped|
|false| "%y%x" |     Capture can't follow capture Ex. : "1234" can be x=1,y=234, x=12,y=34, ...|
|false| "%y*%y" |    Can't have multiple captures in the expression|
|false| "%x*%x" |    Can't have multiple captures in the expression|

In the future they migh chage/be deleted.

## Brplot as an library
Brplot is designed to also be used as an library. 

### Working with C
Intended way to use brplot from C is to download the amalgamation version
of brplot which is a singe header library and just include it into your program.


```c
#define BRPLOT_IMPLEMENTATION
#include "brplot.h"

int main(void) {
 for (int i = -10; i < 10; ++i) brp_1(i, /* group_id */ 0);
 brp_wait(); // Wait until plot window is closed
}
```

```c
/* Animated plots */
#define BRPLOT_IMPLEMENTATION
#include "brplot.c"

int main(void) {
  br_data_id circle = 2, standing_wave = 3;
  float dr = 0.01f;
  brp_label("Circle", circle);
  brp_label("Standing wave", standing_wave);
  for (float radius = 0.0f; true; radius += dr) {
    for (float t = -10; t < 10; t += 0.1f) brp_2(radius*sinf(t),                 radius*cosf(t),        circle);
    for (float t = -10; t < 10; t += 0.1f) brp_2(         t/2.f, cosf(2*BR_PI*radius)*sinf(t)/t, standing_wave);
    brp_flush();
    brp_empty(circle);
    brp_empty(standing_wave);
    if (radius > 1 || radius < 0.0) dr *= -1.f;
  }
  brp_focus_all();
  brp_wait();
  return 0;
}
```

### Working with python
There are also python bindings for brplot.

```cmd
pip install brplot
```

```python
import brplot
brplot.plot(range(100))
```

```python
# Stock market simulation
import brplot
import random

for set in range(10):
  value = 1.1
  time = 0
  dt = 0.1
  for i in range(100000):
    value = value + value * random.uniform(-0.08, 0.0801) * dt
    time += dt
    brplot.plot(time, value, set)
```

### Working with javascript ( Wasm )
This bindings are a bit more messy and I don't like it that much..
```html
<html>
  <body>
    <canvas width="800" height="600" id="canvas"></canvas>
    <script type="module">
      // TODO: change 0.0.7 to the latest version...
      import { Brplot } from "https://cdn.jsdelivr.net/npm/brplot@0.0.7/index.js"

      const b = new Brplot("canvas");
      await b.initializeAsync();

      b.pushPoint(0, 1);
      b.setOnNewFrame(() => {
        b.pushPoint(Math.random());
      });

      b.startDrawing();
    </script>
  </body>
</html>
```

### Controls

#### Controls are only active when mouse is over the grap.

* Right mouse button + Move mouse        - Change offset
* Mouse wheel                            - Change zoom
* **LCTRL** + Mouse                      - Move or Resize plots ( and other windows )
* **X** + Mouse Wheel                    - Change zoom only in **X** axis
* **Y** + Mouse Wheel                    - Change zoom only in **Y** axis
* [**X**|**Y**] + [**LSHIFT**|**LCRTL**] - Change zoom [in|out] only in [**X**|**Y**] axis
* **F**                                  - Follow the visible lines. ( Camera will focus on the average of newest points added to each visible line. )
* **LCTRL** + **F**                      - Camera will focus on the average of newest points added to each visible line.
* **T**                                  - Add test points
* **C** + **LSHIFT**                     - Clear all points
* **C**                                  - Empty all points
* **R**                                  - Reset camera to default values
* **R** + **LSHIFT**                     - Reset camera zoom to (1, 1)
* **R** + **LCTRL**                      - Reset camera offest to (0, 0)
* **D**                                  - Toggle debug view.
* **S**                                  - Grab a screenshot.
* **3**                                  - Switch to 3D plot.
* **2**                                  - Switch to 2D plot.

#### Controls are only active if mouse is over element in list of graphs
* **C** + **LSHIFT** - Clear all points in line which the mouse is over.
* **C**              - Empty all points over which the mouse is over.
* Left mouse button  - Toggle visiblity of the line over which the mouse is over

## Compile
Brplot uses a custom build tools.

To use it you have to build it first:
```sh
cc -I. -o nob nob.c -lm
```
On windows use clang or cl or mingw or any other c compiler should do.

To just build the brplot run:
```sh
./nob
```

But now that you have compiled the build tool, you can do a bit more than just build.

To see what you can do, run:
```sh
./nob help
```

## Install
When built, brplot is only one file and you can install it using ```install``` command.
Here I'm installing it to ```/usr/bin``` directory, but this can be any other directory...

```bash
sudo install bin/brplot /usr/bin/brplot
```

## Uninstall
```bash
sudo rm /usr/bin/brplot
```

## Todo
* ~~Make drawing lines use buffers ( Don't use DrawLineStrip function by raylib. ) Maybe use DrawMesh? It's ok for plots with ~1'000'000 points, but I want more!~~
  * Implemented this now. For every line, 2 triangles are created. Old points are put in buffers and are drawn like that. Plotter can now handle 30'000'000 points, easy.
* When having many points ( 30'000'000 ), a few probles ocure:
  * Distant points start being rounded up/down to the closest float. It doesn't look right.
  * ~~When zoomed out a lot. It becomes quite slow. ( I guess there is a lot of drawing of the same pixel.. )~~
    * Maybe combine few lines that are close when zoomed out... ( how to detect this ? )
    * This is partly fixed for plots where x values are sorted.
    * ~~Problem still remains if x values aren't sorted.~~
      * This is now solved by finding intervals in which numbers are sorted one way or the other.
      * But still there is a worst case when every line is in different interval, and it will cause it to once aging be slow.
  * Maybe use geometry shader ( don't generate triangles on cpu. )
  * ~~Gpu memory usage will be lower. Current gpu memory usage:~~
    * ~~(N lines)*(2 triangles per line)*(3*vertices per triangle)*((3 floats for position) + (3 float for tangents))*(4 bytes per float)~~
    * ~~If N = 64'000'000, gpu usage will be ~9GB. This seems high...~~
    * This is partly fixed. If plot values are sequential gpu memory usage can be constant with regard to number of points.
    * ~~Problem still remains if x values aren't sorted.~~
        * ~~This is now solved by using quad tree structure for storing data points.~~
        * ~~Still there is work to be done to make quad tree structure closer to optimal.~~
        * This is now solved by finding intervals in which numbers are sorted one way or the other.
* ~~I'm not happy with the thickness of the line when zooming in and out.~~
  * ~~It's not that bad, but it's inconsistent.~~
  * Made is consistent. And now it's smooth af.
* ~~Quad tree rectangles are not inside one another, bounds of the outer quad are smaller than those of the inner quad. Fix this...~~
  * I deleted everything that had anything to do with quad trees so this is not a problem anymore.
* ~~Text looks like shit... I don't know how to fix it...~~
  * Text doesn't look like shit any more. I found a way to fix it.
* ~~Values on x,y axis should be on each horizontal and vertical line. ( Not in corners. )~~
  * Did this and it looks awesome.
* ~~Colors should be configurable. Black background is the best background, but maybe there will be a need for a white background.~~
  * ~~This will require having a configuration file ( Maybe )~~
  * Add setting of colors to the library api.
  * Or parse tty codes for changing colors... hmmm ( could be cool )
    * Does ttyplot do this ??
* Add something to plot points. ( scatter plot )
  * This will most likely require the use of quad tree, once again..
* Add something for testing the UI.
  * I want to record my actions and that play that back to see if something will segfault...
  * This will require, I guess some kind of rework of input handling.
    * A structure will have to be introduced that stores two function pointers. One predicate and one action. Each frame call that predicate and if true call action.
  * I saw pull request on raylib for something like that. But Ray answered that he has to look at the API more closely.
  * ~~Maybe create something that does not depend on glfw and can be tested on headless servers. This would enable me to run those tests on github ci.~~
    * Implemented this. On its own, this feature is useful, already found 1 double free.
    * This is more or less now fuzz testing. I like it.
* ~~Nicer UI for setting color of a line, if it's show, maybe to export data to file or stdout.~~
  * ~~Started to work with imgui~~
  * ~~First iteration of this is done~~
* ~~Stack panel improvements~~
  * ~~make it more general. So that it accepts any kind of element, not just button~~
  * ~~add like a scroll bar on the left size of a stack~~
  * ~~Fuck this shit. I moved to use imgui for this sort of stuff...~~
  * ~~Fuck ImGui I moved back to my own gui implementaion~~
* ~~Zig build doesn't build tools/font_export.c... Make zig build that also, else default_font.h can't be created.~~
  * ~~This is not needed anymore, because I no longer use zig. Zig is not ready yet.~~
* ~~Export image with numbers.~~
  * This now works more or less. Still needs a better UI and ability to change image resoultion. For now it's hardcoded
  * Make screenshots work on Web
  * Something to change screenshot resolution.
  * Something to change screenshot name. ( Or at least directory, and then set a name to a timestamp or something... )
* ~~Fix negative zero~~
* ~~Zoom in on the location where the mouse is located, not on center of the screen.~~
  * Fix with the help of my favorite brother Andrija.
* Export of data to a text file.
  * ~~Export to format readable by brplot.~~
  * ~~Export to csv.~~
  * ~~This will require some sort of file explorer to be implemented.~~
* ~~Export the whole graph ( That includes current offset and current zoom )~~
  * ~~This will require setting current offest and zoom from stdin ( Extend input format to handle this. )~~
    * ~~Maybe something like ``--zoomx 69.0``~~
      * Did this.
  * This is done.
* ~~Export what plots are visible and invisible.~~
  * ~~Need some file explorer.~~
* Support for touch input.
  * Support for draging with one finger.
  * Support for multitouch zoom.
* ~~There is something wrong with new tokenizer. Fix this!!~~
* ~~Support for exporting csv|brplot file by issuing command from stdin.~~
  * ~~New bug, bad numbers on x,y axis when exporting~~
* ~~For Imgui make a default layout~~
  * ~~This is only applyed to web version. ( Desktop versions have ability to save stuff on disk. )~~
* ~~For Imgui try to make the same shit with fonts as for raylib. Export only the subset of ttf font.~~
* ~~For Imgui disable default font and use the font that is used in the rest of the graph.~~
* ~~Number of draw call for RAYLIB,WEB is not corret - Fix this.~~
  * Fixed
* Add ability to lineary modify a line on the graph
  * Should this be done in shader
    * this will then requre to not batch different lines together...
    * or maybe somehow add this information to the color buffer?
  * Should be this be done during resampling
    * This will then require to offset points before resampling
    * This is most likely the way to go...
  * Offset for x-y value
  * Scale for x-y value
* ~~Give lines a names.~~
* Show the name of the line if you hover over it.
* Something like status bar stuff.
* Abitlity to make plots out of existing plots.
  * Afine transofrm
    * Plot3 = (Plot1.x, Plot2.y)
      * If Plot1 = [(0, 1), (1, 2), (2, 3)] and Plot2 = [(2, 2), (3, 4), (5, 9)] then Plot3 should be [(0, 2), (1, 4), (2, 9)] 
    * Plot2 = Plot1.x * 2
      * If Plot1 = [(0, 1), (1, 2), (2, 3)] and Plot2 = [(0, 1), (2, 2), (4, 3)]
    * ...
    * This should not require allocation of new memory for each point
    * Could be evalueted lazily
  * Non afine transforms
    * Plot2 = (sin(Plot1.x), cos(Plot1.y))
    * This should require allocation of new memory
    * This will require some sort of mechanism for calculating resulting points every frame such that high fps is not compromised. Say every frame calcuate 1'000'000 points.
* Ability to generate plots out of expressions
  * Say Plot1(x) = sin(x)
* Small regex stuf to definine how to get points from file.
  * ~~Something like `--extract 2 "F*oo%xB*ar%y"` - If input line is "Foasdasdo10Baasdasdar18" Then caputred point should be (10, 18)~~
  * Maybe add multipe points per line "Foo%1x %2y %2x %1y": "Foo10 11 12 13" -> [(10, 13), (12, 11)]
  * Current implementation is recursive, maybe think of a way to make it non recursive.
  * Current implementation runs each extractor one by one. Think of a way to run then in lock step.
* ~~Turn on or off datasets~~
    * ~~Icons for checkboxes~~
* ~~csht should genereate makefile dependency list~~
* I want touch to work.
  * ~~But for that I need a way of adding stuff on screen without stdin.~~
    * Done
    * ~~Say we need to implement read csv stuf..~~
      * Added csv reading and parsing and stuff, it's looking good
      * ~~For that we need file browser ( agin ) to locate that file.~~
        * Implemented file browser looking good
        * Impelement ALT-LEFT to go back.
        * ~~For that we need text input widget.~~
          * Implemented it but it looks bad and needs more features..
              * Impelement CTRL-BACKSPACE to delete a work.

* ~~Web resizing is again fucked, unfuck it.~~
* ~~Automaticaly generate functions to select icon that best fits wanted size..~~
* ~~Resizables should use br_free_list~~
* Implement snappings for resizables
  * ~~Regions should be drawn on screen when moving resizables where if I move the resizable, it snaps on that side of the screen.~~
  * ~~If mouse over regions snap the resizable to that region~~
  * If mouse over other resizable, draw regions over that other resizable and reparent the resizable that is being dragged
* ~~When dragging resizables, they should not be lerped~~
* Make all inputs to the app go trough the same interface
  * That interface should be some kind of queue structure
    * For that we need multithreaded queue implmenetation
* ~~Remove dependency on assert.h~~
* Make collapasables animated.

## Screenshots
Here is a history of how brplot looked over time:

##### 2025-05-21
![screenshot7](media/brplot_20250421_163315.png)
##### 2024-04-21
![screenshot6](media/brplot_20240421_172537.png)
##### 2023-10-29
![screenshot5](media/brplot_20231029_162206.png)
##### 2023-09-30
![screenshot4](media/rlplot_20230930_204431.png)
##### 2023-06-16
![screenshot3](media/rlplot_20230616_145925.png)
##### 2023-06-15
![screenshot2](media/rlplot_20230615_152303.png)
##### 2023-06-13
![screenshot1](media/rlplot.png)

### License

[`MIT`](./LICENSE)
[`Third party`](./external/LICENCES)

