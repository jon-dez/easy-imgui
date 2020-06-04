#pragma once
#include <string>
#include "imgui.h"
#include "dir_explorer.hpp"

enum ChildAction {
    ChildAction_None,
    ChildAction_OpenFile
};

namespace ImGui {
    namespace DirectoryExplorer {
        struct DirectoryCtx {
            std::shared_ptr<DirExplorer> explorer;
        };
        DirectoryCtx NewDirExplorer(const std::string& context_name, const std::string& start_path);
        bool Begin(DirectoryCtx dir_ctx);
        void ShowHistoryButtons(DirectoryCtx dir_ctx);
        void ListDirectory(DirectoryCtx dir_ctx, const char * filter_list = "");
        void ShowPathBar(DirectoryCtx dir_ctx, float width = -1.0f);
        ChildAction SelectedChildShow(DirectoryCtx dir_ctx, std::string& fill_in);
        void End();
        bool OpenFileDialog(DirectoryCtx dir_ctx, std::string& selected_file_name, bool& show, const char* filter_list = "", const ImVec2& init_size = {300.0f,200.0f});
    }
}
