#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_q.h"
#include "src/br_str.h"
#include "src/br_resampling2.h"
#include "src/imgui/imgui_extensions.h"

#include "imgui.h"

void br::ui_info(br_plotter_t* br) {
  ImGui::SetNextWindowBgAlpha(0.7f);
  char* scrach = br_scrach_get(128);
  if (ImGui::Begin("Info") && false == ImGui::IsWindowHidden()) {
    if (ImGui::CollapsingHeader("Allocations")) {
      int s = sprintf(scrach, "Allocations: %zu (%zu KB)", context.alloc_count, context.alloc_size >> 10); ImGui::TextUnformatted(scrach, scrach + s);
      s = sprintf(scrach, "Total Allocations: %zu (%zu KB)", context.alloc_total_count, context.alloc_total_size >> 10); ImGui::TextUnformatted(scrach, scrach + s);
      s = sprintf(scrach, "Max Allocations: %zu (%zu KB)", context.alloc_max_count, context.alloc_max_size >> 10); ImGui::TextUnformatted(scrach, scrach + s);
      s = sprintf(scrach, "Unaccounted Allocations: >%zu", context.free_of_unknown_memory); ImGui::TextUnformatted(scrach, scrach + s);
    }
    if (ImGui::CollapsingHeader("Draw Debug")) {
      ImGui::Text("FPS: %d", GetFPS());
      //int s = sprintf(scrach, "Draw Calls: %lu (%lu lines)", br->lines_mesh->draw_calls, br->lines_mesh->points_drawn); ImGui::TextUnformatted(scrach, scrach + s);
      for (size_t i = 0; i < br->groups.len; ++i) {
        br_data_t data = br->groups.arr[i];
        double time = br_resampling2_get_draw_time(data.resampling) * 1000;
        int n = sprintf(scrach, "Line #%d Draw time: %f", data.group_id, time);
        ImGui::TextUnformatted(scrach, scrach + n);
        n = sprintf(scrach, "Line #%d S: %f", data.group_id, br_resampling2_get_something(data.resampling));
        ImGui::TextUnformatted(scrach, scrach + n);
        n = sprintf(scrach, "Line #%d S2: %f", data.group_id, br_resampling2_get_something2(data.resampling));
        ImGui::TextUnformatted(scrach, scrach + n);

        //auto& a = br->groups.arr[i];
//        s = sprintf(scrach, "Line #%d intervals(%lu):", a.group_id, a.resampling->intervals_count); ImGui::TextUnformatted(scrach, scrach + s);
//        for (size_t j = 0; j < a.resampling->intervals_count; ++j) {
//          auto& inter = a.resampling->intervals[j];
//          s = sprintf(scrach, "   %lu [%lu - %lu] ", inter.count, inter.from, inter.from + inter.count);
//          help_resampling_dir_to_str(&scrach[s], inter.dir);
//          s = (int)strlen(scrach);
//          ImGui::TextUnformatted(scrach, scrach + s);
//        }
      }
    }
    if (ImGui::CollapsingHeader("Queue")) {
      q_commands* qc = br->commands;
      int s = sprintf(scrach, "Read  index: %zu", qc->read_index); ImGui::TextUnformatted(scrach, scrach + s);
      s = sprintf(scrach, "Write index: %zu", qc->write_index); ImGui::TextUnformatted(scrach, scrach + s);
      s = sprintf(scrach, "Length:      %zu", (qc->write_index - qc->read_index + qc->capacity) % qc->capacity); ImGui::TextUnformatted(scrach, scrach + s);
    }
  }
  ImGui::End();
  br_scrach_free();
}
