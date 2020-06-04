#include <map>
#include "imgui.h"
#include "imgui_internal.h"
#include "ui_helpers.hpp"

namespace ImGui {
    /**
     * Credit: Ocornut
     * https://github.com/ocornut/imgui/issues/211#issuecomment-339241929
     */
    void DisableItems() {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    /**
     * Credit: Ocornut
     * https://github.com/ocornut/imgui/issues/211#issuecomment-339241929
     */
    void EnableItems() {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }

    void MakeSection(Display display_section, const ImVec2& size, bool* collapsable, int collapse_flags){
        if(!ImGui::BeginChild(display_section.first.c_str(), size)){
            ImGui::EndChild();
        } else {
            if((!collapsable) |
                (collapsable && ImGui::CollapsingHeader(display_section.first.c_str(), collapsable, collapse_flags))
            ){
                if(!collapsable) {
                    ImGui::Text("%s", display_section.first.c_str());
                    ImGui::Separator();
                }
                if(display_section.second)
                    std::invoke(display_section.second);
            }
            ImGui::EndChild();
        }
    }

    void ImageAutoFit(ImTextureID user_texture_id, const ImVec2& size_to_fit, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col){
        auto avail_size = ImGui::GetContentRegionAvail();
        auto resize = resizeRectAToFitInRectB(size_to_fit, avail_size);
        ImGui::Image(user_texture_id, resize, uv0, uv1, tint_col, border_col);
    }

    namespace DragDrop {
        static Source current_source;

        void ReceiveSource(std::function<void(Source&)> received_cb){
            if(ImGui::BeginDragDropTarget()){
                if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Source::type_name)){
                    received_cb(GetSource());
                }
                ImGui::EndDragDropTarget();
            }
        }

        void SetSource(Source source){
            current_source = std::move(source);
        }

        Source& GetSource() {
            return current_source;
        }
    }
}
