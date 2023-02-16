#ifndef FRONTEND_HXX
#define FRONTEND_HXX

#include <string>
#include <GL/glew.h>
#include "debugger.hxx"
#include "core/n64_impl.hxx"

class Frontend {
public:
    Frontend();
    void Draw();
    void Load(const std::string&, const std::string&);
private:
    GLuint texture_;
    TKPEmu::N64::N64 n64_;
    void step_frame();
    Debugger debugger_;
};
#endif