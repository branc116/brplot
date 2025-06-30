import Module from "./brplot.js"

export class Brplot {
  constructor(canvasElementId) {
    this.canvas = document.getElementById(canvasElementId);
    if (!this.canvas) {
      throw `Canvas with id = '${canvasElementId}' don't exist`;
    }
    this.module_task = Module({canvas: this.canvas});
    this.canvas.oncontextmenu = (e) => e.preventDefault(true);
    this.startedDrawing = false;
    this.onNewFrame = undefined;
    this.width = this.canvas.width;
    this.height = this.canvas.height;
  }

  pushPoint(x, y, group) {
    if (group === undefined) group = 0;
    if (y === undefined && x !== undefined && typeof x === "number") {
      this.module._br_data_add_v1(this.plotter, x, group); 
    } else if (y !== undefined && x !== undefined && typeof x === "number" && typeof y === "number") {
      this.module._br_data_add_v2(this.plotter, x, y, group); 
    }
  }

  setOnNewFrame(onNewFrame) {
    if (typeof onNewFrame !== "function") {
      throw `Expected function as paramter for on new frame, got ${onNewFrame}`;
    }
    this.onNewFrame = onNewFrame;
  }

  resize(width, height) {
    this.canvas.width = this.width = width;
    this.canvas.height = this.height = height;
    this.module._br_wasm_resize(this.plotter, width, height);
  }

  startDrawing() {
    this.startedDrawing = true;
    requestAnimationFrame(this._loopRedraw);
  }
  
  stopDrawing() {
    this.startedDrawing = false;
  }

  redrawOnce() {
    if (this.module === undefined) {
      console.error("Brplot is not initatilized. Run initializeAsync() first");
      return;
    }
    if (this.onNewFrame !== undefined) this.onNewFrame(this);
    if (this.width !== this.canvas.width || this.height !== this.canvas.height) this.resize(this.canvas.width, this.canvas.height);
    this.module._br_wasm_loop(this.plotter);
  }

  async initializeAsync() {
    this.module = await this.module_task;
    const plotter = this.module._br_plotter_new(this.module._br_plotter_default_ctor());
    this.module._br_wasm_init(plotter);
    this.module._br_wasm_loop(plotter);
    this.plotter = plotter;
  }

  _loopRedraw = () => {
    if (!this.startedDrawing) return;
    this.redrawOnce();
    requestAnimationFrame(this._loopRedraw);
  }
}

