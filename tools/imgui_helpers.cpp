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
        namespace detail {
            void ReceiveSource(const char* type_name, std::function<void(Source&)> received_cb){
                if(ImGui::BeginDragDropTarget()){
                    if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Source::payload_type)){
                        if(current_source.isSourceType(type_name))
                            received_cb(current_source);
                    }
                    ImGui::EndDragDropTarget();
                }
            }
        }

        void SetSource(Source&& source){
            current_source = std::move(source);
        }

        Source& GetSource() {
            return current_source;
        }

        Source::Source(Source&& move) noexcept
        {
            *this = std::move(move);
        }
        Source::Source(const Source& copy){
            *this = copy;
        }

        Source& Source::operator=(const Source& copy){
            if(copy.p_)
                p_ = copy.p_->copy();
            else
                p_ = nullptr;
            return *this;
        }

        Source& Source::operator=(Source&& move) noexcept {
            p_ = std::move(move.p_);
            return *this;
        }

        void Source::show() {
            if(p_)
                p_->show();
            else
                ImGui::Text("There is no source data.");
        }

        bool Source::isSourceType(const char* type_name){
            if(strcmp(detail::TypeName<Any>::get(), type_name) == 0)
                return true;
            if(p_)
                return strcmp(type_name, p_->typeName()) == 0;
            return false;
        }
    }
}

/**
 * For ADL Lookup.
 */
namespace std {
    void swap(ImGui::DragDrop::Source& a, ImGui::DragDrop::Source& b){
        swap(a.p_, b.p_);
    }
}
