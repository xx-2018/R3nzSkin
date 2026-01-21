#pragma once

#include "imgui/imgui.h"

// Logging control - only enabled in Debug mode for stealth
#ifdef _DEBUG
    #define ENABLE_LOGGING 1
#else
    #define ENABLE_LOGGING 0
#endif

class R3nzSkinLogger {
public:
    R3nzSkinLogger() { 
        #if ENABLE_LOGGING
        this->clear(); 
        #endif
    }

    void clear() noexcept
    {
        #if ENABLE_LOGGING
        this->buffer.clear();
        this->lineOffsets.clear();
        this->lineOffsets.push_back(0);
        #endif
    }

    void addLog(const char* fmt, ...) noexcept
    {
        #if ENABLE_LOGGING
        auto old_size{ this->buffer.size() };
        va_list args;
        va_start(args, fmt);
        buffer.appendfv(fmt, args);
        va_end(args);
        for (const auto new_size{ this->buffer.size() }; old_size < new_size; ++old_size) {
            if (this->buffer[old_size] == '\n')
                this->lineOffsets.push_back(old_size + 1);
        }
        #else
        // In Release mode, logging is disabled for stealth
        (void)fmt; // Suppress unused parameter warning
        #endif
    }

    void draw() noexcept
    {
        #if ENABLE_LOGGING
        if (ImGui::BeginPopup("Options")) {
            ImGui::Checkbox("Auto-scroll", &this->autoScroll);
            ImGui::EndPopup();
        }

        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");

        ImGui::SameLine();
        if (ImGui::Button("Clear"))
            this->clear();

        ImGui::SameLine();
        if (ImGui::Button("Copy"))
            ImGui::LogToClipboard();

        ImGui::SameLine();
        filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        
        const auto buf{ this->buffer.begin() };
        const auto bufEnd{ this->buffer.end() };
        
        if (this->filter.IsActive()) {
            for (auto line_no{ 0 }; line_no < this->lineOffsets.Size; ++line_no) {
                const auto line_start{ buf + this->lineOffsets[line_no] };
                const auto line_end{ (line_no + 1 < this->lineOffsets.Size) ? (buf + this->lineOffsets[line_no + 1] - 1) : bufEnd };
                if (this->filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        } else {
            ImGuiListClipper clipper;
            clipper.Begin(this->lineOffsets.Size);
            while (clipper.Step()) {
                for (auto line_no{ clipper.DisplayStart }; line_no < clipper.DisplayEnd; ++line_no) {
                    const auto line_start{ buf + this->lineOffsets[line_no] };
                    const auto line_end{ (line_no + 1 < this->lineOffsets.Size) ? (buf + this->lineOffsets[line_no + 1] - 1) : bufEnd };
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        
        ImGui::PopStyleVar();

        if (this->autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        #else
        ImGui::Text("Logging disabled in Release mode");
        #endif
    }
private:
    #if ENABLE_LOGGING
    ImGuiTextBuffer buffer;
    ImGuiTextFilter filter;
    ImVector<int> lineOffsets;
    bool autoScroll{ true };
    #endif
};
