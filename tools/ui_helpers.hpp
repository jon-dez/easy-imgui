/**
 * 2020 Jonathan Mendez
 */
#pragma once
#include <functional>
#include <utility>
#include <string>
#include <map>
#include <memory>
#include <stack>

#include "imgui.h"

/**
 * Return a maximum rect size for a rect_a that would fit in rect_b.
 */
template<typename RectA, typename RectB>
inline RectA resizeRectAToFitInRectB(const RectA& rect_a, const RectB& rect_b){
    RectB ratios{ rect_b.x / rect_a.x, rect_b.y / rect_a.y};

    float scale = -1.0;

    bool x_fit = true;
    if(ratios.y * rect_a.x > rect_b.x)
        x_fit = false;
    
    bool y_fit = true;
    if(ratios.x * rect_a.y > rect_b.y)
        y_fit = false;
    
    if(x_fit && y_fit)
        scale = 1.0;
    else if(x_fit && !y_fit)
        scale = ratios.y;
    else if(!x_fit && y_fit)
        scale = ratios.x;
    else
        ;//std::cerr << "No x_fit and no y_fit" << std::endl;

    return {
        static_cast<decltype(rect_a.x)>(scale*rect_a.x),
        static_cast<decltype(rect_a.y)>(scale*rect_a.y)
    };
}

/**
 * Return the position of the top-left corner of where rect_a should be.
 */
template<typename RectA, typename RectB>
inline RectB centerRectAInRectB(const RectA& rect_a, const RectB& rect_b){
    RectB center{
        rect_b.x / 2.0f,
        rect_b.y /2.0f
    };
    return {
        static_cast<decltype(rect_b.x)>(center.x - rect_a.x / 2.0f),static_cast<decltype(rect_b.y)>(center.y - rect_a.y / 2.0f)
    };
}

namespace ImGui {
    using Display = std::pair<std::string, std::function<void()>>;

    using NavStack = std::stack<Display>;

    void DisableItems();
    void EnableItems();

    void MakeSection(Display display_section, const ImVec2& size = {0,0}, bool* collapsable = nullptr, int collapse_flags = 0);

    class ScopeDisableItems {
        bool disabled;
    public:
        inline ScopeDisableItems(bool disable_if_true) {
            if(disable_if_true) {
                DisableItems();
                disabled = true;
            } else 
                disabled = false;
        }
        inline ~ScopeDisableItems() {
            if(disabled)
                EnableItems();
        }
        inline void allowEnabled() {
            if(disabled) {
                EnableItems();
                disabled = false;
            }
        }
        /**
         * Make sure the items are disabled if enabled.
         */
        inline void ensureDisabled() {
            if(!disabled) {
                DisableItems();
                disabled = true;
            }
        }
    };

    void ImageAutoFit(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0,0), const ImVec2& uv1 = ImVec2(1,1), const ImVec4& tint_col = ImVec4(1,1,1,1), const ImVec4& border_col = ImVec4(0,0,0,0));

    template<typename T>
    void Show(T& t);

    template<typename T>
    inline void ShowP(void* p) {
        T& t = *(T*)p;
        Show(t);
    }

    namespace DragDrop {
        namespace detail {
            struct SourceBase {
                virtual void show() = 0;
                virtual std::unique_ptr<SourceBase> copy() const = 0;
                virtual void move(void* into) = 0;
                virtual void swap(void* with) = 0;
                virtual const char* typeName() = 0;

                virtual ~SourceBase() = default;
            };

            template<typename SourceType>
            struct SourceDerived : public SourceBase {
                SourceType source_data_;
                static constexpr const char * const str_name = SourceType::str_name;

                SourceDerived(SourceType source_data)
                    : source_data_{std::move(source_data)}
                {}

                void show() override {
                    ImGui::Show(source_data_);
                }

                std::unique_ptr<SourceBase> copy() const override {
                    return std::make_unique<SourceDerived>(source_data_);
                }

                void move(void* into) override {
                    SourceType& into_ref{*(SourceType*)into};
                    into_ref = std::move(source_data_);
                }

                void swap(void* with) override {
                    SourceType& with_ref{*(SourceType*)with};
                    std::swap(source_data_, with_ref);
                }

                inline const char * typeName() override {
                    return str_name;
                }
            };
        }

        struct Source {
            static constexpr const char* type_name = "easy_drag_and_drop_source";
            std::unique_ptr<detail::SourceBase> p_{nullptr};

            Source() = default;

            template<typename SourceType>
            Source(SourceType source_data)
                : p_{std::make_unique<detail::SourceDerived<SourceType>>(std::move(source_data))}
            {}

            inline Source(Source&& move) noexcept
                : p_{std::move(move.p_)}
            {}

            inline Source(const Source& copy)
            {
                *this = copy;
            }

            friend void swap(Source& a, Source& b);

            Source& operator=(const Source& copy){
                if(copy.p_)
                    p_ = copy.p_->copy();
                else
                    p_ = nullptr;
                return *this;
            }

            inline void show() {
                if(p_)
                    p_->show();
                else
                    ImGui::Text("There is no source data.");
            }

            inline const char* typeName(){
                if(p_)
                    return p_->typeName();
                return "NULL";
            }
        };


        void SetSource(Source source);
        Source& GetSource();

        template<typename T>
        void BeginSource(T& src_type, ImGuiDragDropFlags flags = ImGuiDragDropFlags_SourceAllowNullID, ImGuiCond cond = ImGuiCond_Once){
            //ImGui::PushID(&src_type);
            if(ImGui::BeginDragDropSource(flags)){
                if(auto payload = ImGui::GetDragDropPayload()){
                    if(cond == ImGuiCond_Always || payload->DataFrameCount == -1){
                        SetSource(src_type);
                    }
                    ImGui::MakeSection({GetSource().typeName(),
                        [](){
                            GetSource().show();
                        }
                    }, {200,100});
                }

                if(ImGui::SetDragDropPayload(
                    Source::type_name,
                    NULL,
                    0,
                    cond
                )){
                    ;
                }
                ImGui::EndDragDropSource();
            }
            //ImGui::PopID();
        }
        
        void ReceiveSource(std::function<void(Source&)> received_cb);

        struct FileSource {
            static constexpr const char* str_name = "File source";
            const char * filename;
            const char* ext;
        };
    }
    template<>
    inline void Show(DragDrop::FileSource& source) {
        ImGui::Text("File[%s]: %s", source.ext, source.filename);
    }
}

/**
 * For ADL Lookup.
 */
namespace std {
    inline void swap(ImGui::DragDrop::Source& a, ImGui::DragDrop::Source& b){
        swap(a.p_, b.p_);
    }
}
