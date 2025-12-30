import Module from "/bin/rpn_calculator.js"

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
  }

  setOnNewFrame(onNewFrame) {
    if (typeof onNewFrame !== "function") {
      throw `Expected function as paramter for on new frame, got ${onNewFrame}`;
    }
    this.onNewFrame = onNewFrame;
  }

  resize(width, height) {
    const dpi = window.devicePixelRatio;
    this.canvas.width  =  width;
    this.canvas.height = height;
    this.module._rpn_wasm_resize(width, height);
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
      console.error("Not initatilized. Run initializeAsync() first");
      return;
    }
    if (this.onNewFrame !== undefined) this.onNewFrame(this);
    this.resize(document.body.clientWidth, document.body.clientHeight);
    this.module._rpn_one_frame();
  }

  async initializeAsync() {
    this.module = await this.module_task;
    this.module._rpn_init(window.devicePixelRatio);
    this.module._rpn_one_frame();
    this.canvas.addEventListener("pointerdown", (ev) => {
      if (ev.pointerType == "touch") {
        ev.preventDefault();
        let local_x = ev.pageX - this.canvas.offsetLeft;
        let local_y = ev.pageY - this.canvas.offsetTop;
        this.module._rpn_wasm_touch_event(0, local_x, local_y, ev.pointerId);
      }
    });
    this.canvas.addEventListener("pointermove", (ev) => {
      if (ev.pointerType == "touch") {
        ev.preventDefault();
        let local_x = ev.pageX - this.canvas.offsetLeft;
        let local_y = ev.pageY - this.canvas.offsetTop;
        this.module._rpn_wasm_touch_event(1, local_x, local_y, ev.pointerId);
      }
    });
    this.canvas.addEventListener("pointerup", (ev) => {
      if (ev.pointerType == "touch") {
        ev.preventDefault();
        let local_x = ev.pageX - this.canvas.offsetLeft;
        let local_y = ev.pageY - this.canvas.offsetTop;
        this.module._rpn_wasm_touch_event(2, local_x, local_y, ev.pointerId);
      }
    });
  }

  _loopRedraw = () => {
    if (!this.startedDrawing) return;
    let _this = this;
    this.redrawOnce();
    function foo() {
      _this.redrawOnce();
      requestAnimationFrame(foo);
    }
    requestAnimationFrame(foo);
  }
}
