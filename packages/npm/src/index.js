import Module from "./brplot.js"

setTimeout(async () => {
  const m = await Module({ canvas: document.getElementById("canvas") });
  console.log(m);
  window["brplot"] = m;
  const graph = m._graph_malloc();
  m._graph_init(graph, 500, 200);
  m._graph_draw(graph);
  window["graph"] = graph;
  window.requestAnimationFrame(loop);
}, 10);


function loop() {
  window["brplot"]._graph_draw(window["graph"]);
  window.requestAnimationFrame(loop);
}
