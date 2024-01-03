declare class Brplot {
  constructor(canvasElementId: string);
  /**
    * Call this after calling the constructor to initialize async stuff
  */
  async initializeAsync(): Promise<void>;
  resize(width: number, height: number): void;

  pushPoint(y: number): void;
  pushPoint(x: number, y: number): void;
  pushPoint(x: number, y: number, group: number): void;

  /**
    * Called before every frame
  */
  setOnNewFrame(onNewFrame: (br: Brplot) => void): void;
  startDrawing(): void;
  stopDrawing(): void;
  redrawOnce(): void;
}
export { Brplot };
