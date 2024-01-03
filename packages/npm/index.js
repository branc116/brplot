import Module from "./brplot.js"

export class Brplot {
  constructor(canvasElementId) {
    this.canvas = document.getElementById(canvasElementId);
    if (!this.canvas) {
      throw `Canvas with id = '${canvasElementId}' don't exist`;
    }
    this.module_task = Module({ canvas: this.canvas });
    this.canvas.oncontextmenu = (e) => e.preventDefault(true);
    this.startedDrawing = false;
    this.onNewFrame = undefined;
    this.width = this.canvas.width;
    this.height = this.canvas.height;
    this.pgs = undefined;
  }

  pushPoint(x, y, group) {
    if (group === undefined) group = 0;
    if (y === undefined && x !== undefined && typeof x === "number") {
      this.module._points_group_push_y(this.pgs, x, group); 
    } else if (y !== undefined && x !== undefined && typeof x === "number" && typeof y === "number") {
      this.module._points_group_push_xy(this.pgs, x, y, group); 
    }
  }

  setOnNewFrame(onNewFrame) {
    if (typeof onNewFrame !== "function") {
      throw `Expected function as paramter for on new frame, got ${onNewFrame}`;
    }
    this.onNewFrame = onNewFrame;
  }

  resize(width, height) {
    this.module._graph_resize(this.graph, width, height);
    this.canvas.width = this.width = width;
    this.canvas.height = this.height = height;
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
      console.error("Brploter is not initatilized, yet");
      return;
    }
    if (this.onNewFrame !== undefined) {
      this.onNewFrame(this);
    }
    if (this.width !== this.canvas.width || this.height !== this.canvas.height) this.resize(this.canvas.width, this.canvas.height);
    this.module._graph_draw(this.graph);
  }

  async initializeAsync() {
    this.module = await this.module_task;
    const graph = this.module._graph_malloc();
    this.module._graph_init(graph, this.canvas.width, this.canvas.height);
    this.module._graph_draw(graph);
    this.graph = graph;
    this.pgs = this.module._graph_get_points_groups(this.graph);
  }

  _loopRedraw = () => {
    if (!this.startedDrawing) return;
    this.redrawOnce();
    requestAnimationFrame(this._loopRedraw);
  }
}

