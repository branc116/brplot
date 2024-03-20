#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_q.h"
#include "src/imgui/imgui_extensions.h"

#include "imgui.h"
#include "raylib.h"

void br::ui_info(br_plotter_t* br) {
  ImGui::SetNextWindowBgAlpha(0.7f);
  if (ImGui::Begin("Info") && false == ImGui::IsWindowHidden()) {
    if (ImGui::CollapsingHeader("Allocations")) {
      int s = sprintf(context.buff, "Allocations: %lu (%lu KB)", context.alloc_count, context.alloc_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
      s = sprintf(context.buff, "Total Allocations: %lu (%lu KB)", context.alloc_total_count, context.alloc_total_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
      s = sprintf(context.buff, "Max Allocations: %lu (%lu KB)", context.alloc_max_count, context.alloc_max_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
      s = sprintf(context.buff, "Unaccounted Allocations: >%lu", context.free_of_unknown_memory); ImGui::TextUnformatted(context.buff, context.buff + s);
    }
    if (ImGui::CollapsingHeader("Draw Debug")) {
      ImGui::Text("FPS: %d", GetFPS());
      //int s = sprintf(context.buff, "Draw Calls: %lu (%lu lines)", br->lines_mesh->draw_calls, br->lines_mesh->points_drawn); ImGui::TextUnformatted(context.buff, context.buff + s);
      for (size_t i = 0; i < br->groups.len; ++i) {
        //auto& a = br->groups.arr[i];
//        s = sprintf(context.buff, "Line #%d intervals(%lu):", a.group_id, a.resampling->intervals_count); ImGui::TextUnformatted(context.buff, context.buff + s);
//        for (size_t j = 0; j < a.resampling->intervals_count; ++j) {
//          auto& inter = a.resampling->intervals[j];
//          s = sprintf(context.buff, "   %lu [%lu - %lu] ", inter.count, inter.from, inter.from + inter.count);
//          help_resampling_dir_to_str(&context.buff[s], inter.dir);
//          s = (int)strlen(context.buff);
//          ImGui::TextUnformatted(context.buff, context.buff + s);
//        }
      }
    }
    if (ImGui::CollapsingHeader("Queue")) {
      q_commands* qc = br->commands;
      int s = sprintf(context.buff, "Read  index: %lu", qc->read_index); ImGui::TextUnformatted(context.buff, context.buff + s);
      s = sprintf(context.buff, "Write index: %lu", qc->write_index); ImGui::TextUnformatted(context.buff, context.buff + s);
      s = sprintf(context.buff, "Length:      %lu", (qc->write_index - qc->read_index + qc->capacity) % qc->capacity); ImGui::TextUnformatted(context.buff, context.buff + s);
    }
  }
  ImGui::End();
}
