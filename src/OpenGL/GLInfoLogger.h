#ifndef DEF_GL_INFO_LOGGER_HPP
#define DEF_GL_INFO_LOGGER_HPP

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_opengl/opengl/juce_gl.h>

// helper from this kind sir:
// https://forum.juce.com/t/just-a-quick-gl-info-logger-func-for-any-of-you/32082/2
inline void logOpenGLInfoCallback(juce::OpenGLContext&) {
  int major = 0, minor = 0;
  juce::gl::glGetIntegerv(juce::gl::GL_MAJOR_VERSION, &major);
  juce::gl::glGetIntegerv(juce::gl::GL_MINOR_VERSION, &minor);

  juce::String stats;
  stats
      << "---------------------------" << juce::newLine
      << "=== OpenGL/GPU Information ===" << juce::newLine << "Vendor: "
      << juce::String((const char*)juce::gl::glGetString(juce::gl::GL_VENDOR))
      << juce::newLine << "Renderer: "
      << juce::String((const char*)juce::gl::glGetString(juce::gl::GL_RENDERER))
      << juce::newLine << "OpenGL Version: "
      << juce::String((const char*)juce::gl::glGetString(juce::gl::GL_VERSION))
      << juce::newLine << "OpenGL Major: " << juce::String(major)
      << juce::newLine << "OpenGL Minor: " << juce::String(minor)
      << juce::newLine << "OpenGL Shading Language Version: "
      << juce::String((const char*)juce::gl::glGetString(
             juce::gl::GL_SHADING_LANGUAGE_VERSION))
      << juce::newLine << "---------------------------" << juce::newLine;

  std::cerr << stats << std::endl;
}

#endif  // DEF_GL_INFO_LOGGER_HPP