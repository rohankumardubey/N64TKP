#include "frontend.hxx"
#include "imgui/imgui.h"

Frontend::Frontend() : n64_() {
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, n64_.GetWidth(), n64_.GetHeight(), 0, GL_RGBA, n64_.GetBitdepth(), nullptr);
}

void Frontend::Draw() {
    ImGui::Begin("Display");
    ImGui::Image((ImTextureID)texture_, ImVec2(n64_.GetWidth(), n64_.GetHeight()));
    if (ImGui::Button("Frame")) {
        step_frame();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, n64_.GetWidth(), n64_.GetHeight(), 0,  GL_RGBA, n64_.GetBitdepth(), n64_.GetColorData());
    }
    // if (ImGui::Button("Test2")) {
    //     debugger_.TickGdbStub();
    // }
    ImGui::End();
}

void Frontend::Load(const std::string& ipl, const std::string& rom) {
    n64_.LoadIPL(ipl);
    n64_.LoadCartridge(rom);
    n64_.Reset();
}

void Frontend::step_frame() {
    for (int i = 0; i < INSTRS_PER_FRAME; i++) {
        n64_.Update();
    }
}