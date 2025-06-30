# Brplot
Small library that plots lines.

## About
Brplot is a project that target platforms outside of the web.
Brplot also target web using webassembly.
So to use Brplot, you have to have a browser that supports webassembly.

Only minimal set of functions is exported to javascript side. A lot more functions could be exported.
... If there will be a need for that.


### Example project
You can use all the fancy tooling around npm and such to use brplot.

Or, you can use cdn. It's up to you.

Both should work.

#### Example for web chads that only write HTML

```html
<html>
  <body>
    <canvas width="800" height="600" id="canvas1"></canvas>
    <script type="module">
      // TODO: change 0.0.7 to the latest version...
      import { Brplot } from "https://cdn.jsdelivr.net/npm/brplot@0.0.7/index.js"

      const b = new Brplot("canvas1");
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

#### Example for professional web developers that use npm, and typescript, and webpack and vite, and nextjs

```html
<html>
  <body>
    <canvas width="800" height="600" id="canvas1"></canvas>
    <script src="./dist/main.js" type="module"></script>
  </body>
</html>
```
NOTE: type of the script must be a module.

```javascript
import { Brplot } from "brplot"

// Note: canvas1 is the id of canvas element in html
// This will initialize brplot.
const b = new Brplot("canvas1");
await b.initializeAsync();

// You can push points outside of everything.
b.pushPoint(0, 1);
b.setOnNewFrame(() => {
  // You can push points before every frame.
  b.pushPoint(Math.random());
});

// Start rendering loop.
b.startDrawing();

// Call this if you want to stop rendering loop.
// b.stopDrawing()

// Call this if you only what to render 1 frame to the canvas.
// b.redrawOnce();
```

### Author
Branimir Ričko

### License
MIT
