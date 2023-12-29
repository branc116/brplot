import Module from "./brplot.js"

setTimeout(async () => {
  const m = await Module({ canvas: document.getElementById("canvas") });
  console.log(m);
  const graph = m._graph_malloc();
  m._graph_init(graph, 2*500, 2*200);
  m._graph_draw(graph);
  window["graph"] = graph;
  window["brplot"] = m;
  window.requestAnimationFrame(loop);
}, 10);

let i = 0;

function loop() {
  const brplot = window["brplot"];
  const graph = window["graph"];
  const pgs = brplot._graph_get_points_groups(graph);
  const t = i / 1000;
  brplot._points_group_push_xy(pgs, t, Math.sin(t), 0);
  ++i;
  brplot._graph_draw(graph);
  window.requestAnimationFrame(loop);
}
