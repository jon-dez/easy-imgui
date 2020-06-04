#include <iostream>
#include <map>
#include <memory>
#include <stack>

#ifdef WIN32
constexpr auto& strtok_r = strtok_s;
#endif

#include "ImGuiDirExplorer.hpp"
#include "dir_explorer.hpp"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../tools/ui_helpers.hpp"

namespace ImGui {
    namespace DirectoryExplorer {
        DirectoryCtx NewDirExplorer(const std::string& context_name, const std::string& start_path){
            return {
                .explorer{std::make_shared<DirExplorer>(context_name, start_path)}
            };
        }

        bool Begin(DirectoryCtx dir_ctx){
            if(!ImGui::BeginChild(dir_ctx.explorer->getExplorerName().c_str())){
                ImGui::EndChild();
                return false;
            }
            return true;
        }

        void ShowHistoryButtons(DirectoryCtx dir_ctx){
            static const ImVec4 button_disabled_color{0.0f,0.0f,0.0f,0.0f};
            //ImGui::Item
            bool had_forward_history = dir_ctx.explorer->hasForwardHistory();
            if(!had_forward_history){
                ImGui::PushStyleColor(ImGuiCol_Button, button_disabled_color);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_disabled_color);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_disabled_color);
            }

            if(ImGui::ArrowButton("back", ImGuiDir_Left))
                dir_ctx.explorer->goBackward();

            if(!had_forward_history){
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
            }

            ImGui::SameLine();
            
            bool had_back_history = dir_ctx.explorer->hasBackwardHistory();
            if(!had_back_history){
                ImGui::PushStyleColor(ImGuiCol_Button, button_disabled_color);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_disabled_color);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_disabled_color);
            }
            
            if(ImGui::ArrowButton("forward", ImGuiDir_Right))
                dir_ctx.explorer->returnForward();

            if(!had_back_history){
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
            }
        }

        void ShowPathBar(DirectoryCtx dir_ctx, float width){
            std::string path{dir_ctx.explorer->getCurrentDir()};
            ImGui::PushItemWidth(width);
            if(ImGui::InputText("", &path,ImGuiInputTextFlags_EnterReturnsTrue)){
                dir_ctx.explorer->swapDir(path);
            }
            ImGui::PopItemWidth();
        }

        /**
         * A filter list denotes the file extensions to list (seperated by commas.)
         */
        void ListDirectory(DirectoryCtx dir_ctx, const char* filter_list){
            static const char * filter_seperator = ",";

            int size_tok_buf = strlen(filter_list) + 1;
            auto filter_tok{std::make_unique<char[]>(size_tok_buf)};
            memset(filter_tok.get(), 0, size_tok_buf);
            strncpy(filter_tok.get(), filter_list, size_tok_buf - 1);
            int i;
            for(char* i_strtok = filter_tok.get(), i=0;
                filter_tok[i] != '\0';
                i += strlen(i_strtok), i_strtok = nullptr
            ){
                i_strtok = strtok_r(i_strtok, filter_seperator, &i_strtok);
            }

            for(auto dir_entry : *dir_ctx.explorer){
                auto _excludeDirEntry = [&](){
                    bool filter_match = false;
                    char* it = filter_tok.get();
                    while(*it){
                        if(dir_entry.path().extension().string().compare(it) == 0){
                            filter_match = true;
                            break;
                        }
                        it = it + strlen(it) + 1; // Get the next filter token.
                    }
                    // If there exists some filter && a filter was not matched.
                    return !(size_tok_buf < 2) && !filter_match;
                };

                if(!dir_entry.is_directory() && _excludeDirEntry()){
                    continue;
                }

                if(ImGui::Selectable(dir_entry.path().filename().string().c_str())){
                    if (dir_entry.is_directory()){
                        dir_ctx.explorer->swapDir(dir_entry.path().string());
                    }
                    else if(dir_entry.is_regular_file()) {
                            dir_ctx.explorer->selectChild(dir_entry.path().string());
                    }
                }
                ImGui::DragDrop::FileSource x{"Test", "test"};
                ImGui::DragDrop::BeginSource(x);
            }
        }

        inline bool hasSelectedChild(DirExplorer dir_explorer){
            return dir_explorer.getSelected().has_filename();
        }

        ChildAction SelectedChildShow(DirectoryCtx dir_ctx, std::string& fill_in){
            ChildAction c_action = ChildAction_None;
            if(hasSelectedChild(*dir_ctx.explorer)){
                auto path = dir_ctx.explorer->getSelected();
                ImGui::Text("Selected: %s", path.filename().string().c_str());
                ImGui::SameLine();
                if(ImGui::Button("Open file")) {
                    c_action = ChildAction_OpenFile;
                    fill_in = path.string();
                }
            }
            return c_action;
        }

        void End(){
            ImGui::EndChild();
        }

        bool OpenFileDialog(DirectoryCtx dir_ctx, std::string& selected_file_name, bool& show, const char* filter_list, const ImVec2& init_size){
            if(!show)
                return false;

            auto explorer_name{dir_ctx.explorer->getExplorerName()};
            
            if(!ImGui::IsPopupOpen(explorer_name.c_str())) {
                ImGui::OpenPopup(explorer_name.c_str());
                ImGui::SetNextWindowSize(init_size);
            }
            
            bool open = false;
            if(ImGui::BeginPopupModal(explorer_name.c_str(), &show)){
                ImGui::DirectoryExplorer::ShowHistoryButtons(dir_ctx);
                ImGui::SameLine();
                float width_path_bar = ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(filter_list).x - ImGui::GetStyle().ItemSpacing.x;
                ImGui::DirectoryExplorer::ShowPathBar(dir_ctx, width_path_bar);
                ImGui::SameLine();
                ImGui::Text("%s", filter_list);
                if(ImGui::DirectoryExplorer::SelectedChildShow(dir_ctx, selected_file_name) == ChildAction_OpenFile){
                    open = true;
                    show = false;
                }
                if(!ImGui::BeginChild("Directory List", {}, true)){
                    ImGui::EndChild();
                } else {
                    ImGui::DirectoryExplorer::ListDirectory(dir_ctx, filter_list);
                    ImGui::EndChild();
                }
                ImGui::EndPopup();
            }
            return open;
        }
    }
}

