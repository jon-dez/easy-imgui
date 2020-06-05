#pragma once

namespace ImGui {
    template<typename T>
    void Show(T& t);

    template<typename T>
    inline void ShowP(void* p) {
        T& t = *(T*)p;
        Show(t);
    }
}