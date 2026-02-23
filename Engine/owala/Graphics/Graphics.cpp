/*********************************************************************//**
 * \file   Graphics.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (100%)
 *
 * \brief
 *      Declaration of the Graphics subsystem that wraps OpenGL calls
 *      to provide basic 2D rendering features such as colored shapes,
 *      textured sprites, text drawing, and framebuffer management.
 *
 * \details
 *      The Graphics system abstracts raw OpenGL rendering into higher-level
 *      APIs for 2D games and editors. It provides:
 *        - Primitive rendering (lines, rectangles, triangles)
 *        - Sprite rendering (single and batched)
 *        - Font and text layout utilities
 *        - Framebuffer management for offscreen rendering
 *
 *      Internally, the system manages its own OpenGL pipelines and buffers,
 *      handling batching for sprites and efficient texture usage.
 *
 *      Example Usage:
 *      \code
 *      Graphics gfx(windowSystem);
 *      gfx.beginFrame({0.1f, 0.1f, 0.12f, 1.0f});
 *      gfx.drawSprite(tex, {0,0}, {0.5f,0.5f});
 *      gfx.drawText(font, "Hello World", { -0.9f, 0.8f }, 0.5f, {1,1,1,1});
 *      gfx.endFrame();
 *      \endcode
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "Graphics.h"
#include "GameManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Logger.h"
#include "OwalaTime.h"

namespace Framework {

    // =========================
    // Shaders
    // =========================

    // Colored triangle (pos only)
    static const char* VS_COLOR = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    void main(){ gl_Position = vec4(aPos, 0.0, 1.0); }
    )";

    static const char* FS_COLOR = R"(#version 330 core
    uniform vec4 uColor;
    out vec4 FragColor;
    void main(){ FragColor = uColor; }
    )";

    // Textured quad (pos + uv)
    static const char* VS_TEX = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    layout(location=1) in vec2 aUV;
    out vec2 vUV;
    void main(){ gl_Position = vec4(aPos, 0.0, 1.0); vUV = aUV; }
    )";

    static const char* FS_TEX = R"(#version 330 core
    in vec2 vUV;
    uniform sampler2D uTex;
    out vec4 FragColor;
    void main(){ FragColor = texture(uTex, vUV); }
    )";

    // Textured glyphs (pos + uv), alpha from red channel
    static const char* VS_TEXT = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    layout(location=1) in vec2 aUV;
    out vec2 vUV;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        vUV = aUV;
    })";

    static const char* FS_TEXT = R"(#version 330 core
    in vec2 vUV;
    uniform sampler2D uTex;
    uniform vec4 uColor;
    out vec4 FragColor;
    void main() {
        float alpha = texture(uTex, vUV).r;
        FragColor = vec4(uColor.rgb, uColor.a * alpha);
    }
    )";

    // Occluder shader — renders shadow-casting geometry as solid white into the occluder FBO
    static const char* VS_OCCLUDER = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    void main(){ gl_Position = vec4(aPos, 0.0, 1.0); }
    )";

    static const char* FS_OCCLUDER = R"(#version 330 core
    out vec4 FragColor;
    void main(){ FragColor = vec4(1.0); }
    )";

    // Spotlight composite shader — reads the occluder mask and outputs the lighting overlay
    static const char* VS_SPOTLIGHT = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    layout(location=1) in vec2 aUV;
    out vec2 vUV;
    void main(){
        gl_Position = vec4(aPos, 0.0, 1.0);
        vUV = aUV;
    }
    )";

    static const char* FS_SPOTLIGHT = R"(#version 330 core
    in vec2 vUV;
    uniform sampler2D uOccluderMask;
    uniform vec2  uLightNDC;
    uniform float uRadius;
    uniform float uConeAngleRad;
    uniform float uDirectionRad;
    uniform vec4  uLightColor;
    uniform float uIntensity;
    uniform float uFalloff;
    uniform vec4  uAmbientColor;
    uniform int   uShadowRays;
    uniform int   uCastShadows;
    out vec4 FragColor;

    void main() {
        vec2 fragNDC = vUV * 2.0 - 1.0;
        vec2 toFrag  = fragNDC - uLightNDC;
        float dist   = length(toFrag);

        if (dist > uRadius) {
            FragColor = uAmbientColor;
            return;
        }

        float angle = atan(toFrag.y, toFrag.x);
        float angleDiff = abs(mod(angle - uDirectionRad + 3.14159265, 6.28318530) - 3.14159265);
        if (angleDiff > uConeAngleRad) {
            FragColor = uAmbientColor;
            return;
        }

        float lit = 1.0;
        if (uCastShadows != 0 && dist > 0.001) {
            vec2 dir = normalize(toFrag);
            float stepSize = dist / float(uShadowRays);
            for (int i = 1; i <= uShadowRays; i++) {
                vec2 sampleNDC = uLightNDC + dir * (stepSize * float(i));
                vec2 uv = sampleNDC * 0.5 + 0.5;
                if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) break;
                float occ = texture(uOccluderMask, uv).r;
                if (occ > 0.5) { lit = 0.0; break; }
            }
        }

        float attenuation  = pow(max(0.0, 1.0 - dist / uRadius), uFalloff);
        float coneEdge     = 1.0 - smoothstep(uConeAngleRad * 0.85, uConeAngleRad, angleDiff);
        float brightness   = lit * attenuation * coneEdge * uIntensity;
        FragColor = mix(uAmbientColor, uLightColor * brightness, brightness);
    }
    )";

    // Shader File Loading Helpers
    // =========================

    //static std::string LoadShaderFromFile(const char* path) {
    //    std::ifstream file(path);
    //    if (!file) {
    //        std::fprintf(stderr, "Failed to open shader file: %s\n", path);
    //        return "";
    //    }

    //    std::string content((std::istreambuf_iterator<char>(file)),
    //        std::istreambuf_iterator<char>());
    //    return content;
    //}

    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
            std::fprintf(stderr, "Shader compile error:\n%s\n", log);
        }
        return s;
    }

    static GLuint linkProgram(const char* vsSrc, const char* fsSrc) {
        GLuint v = compile(GL_VERTEX_SHADER, vsSrc);
        GLuint f = compile(GL_FRAGMENT_SHADER, fsSrc);
        GLuint p = glCreateProgram();
        glAttachShader(p, v);
        glAttachShader(p, f);
        glLinkProgram(p);
        glDeleteShader(v);
        glDeleteShader(f);
        GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetProgramInfoLog(p, 1024, nullptr, log);
            std::fprintf(stderr, "Program link error:\n%s\n", log);
        }
        return p;
    }

    //static GLuint linkProgramFromFiles(const char* vsPath, const char* fsPath) {
    //    std::string vsSrc = LoadShaderFromFile(vsPath);
    //    std::string fsSrc = LoadShaderFromFile(fsPath);

    //    if (vsSrc.empty() || fsSrc.empty()) {
    //        std::fprintf(stderr, "Failed to load shader files: %s, %s\n", vsPath, fsPath);
    //        return 0;
    //    }

    //    return linkProgram(vsSrc.c_str(), fsSrc.c_str());
    //}
    // =========================
    // Internal state / helpers
    // =========================

    // Cached uniform locations (avoid glGetUniformLocation per draw)
    static GLint g_uColorLoc = -1;
    static GLint g_uTexLoc_Tex = -1;
    static GLint g_uColorLoc_Text = -1;
    static GLint g_uTexLoc_Text = -1;

    // Spotlight uniform location cache
    static GLint g_sl_OccluderMask = -1;
    static GLint g_sl_LightNDC = -1;
    static GLint g_sl_Radius = -1;
    static GLint g_sl_ConeAngleRad = -1;
    static GLint g_sl_DirectionRad = -1;
    static GLint g_sl_LightColor = -1;
    static GLint g_sl_Intensity = -1;
    static GLint g_sl_Falloff = -1;
    static GLint g_sl_AmbientColor = -1;
    static GLint g_sl_ShadowRays = -1;
    static GLint g_sl_CastShadows = -1;

    // Batch buffer � reused each call (to avoid reallocations)
    static std::vector<float> g_batchVerts; // interleaved x,y,u,v for 6 verts per sprite

    // Pick a generous default; you can tune this
    static constexpr int MAX_SPRITES_DEFAULT = 10000;

    // =========================
    // ISystem
    // =========================

    void Graphics::Initialize() {
        // GLEW must be initialized after a valid GL context is current (WindowsSystem did that)
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::fprintf(stderr, "GLEW init failed\n");
        }

        // Back-face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Set initial viewport
        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(GameManager::GetInstance().GetSystems().win_->window(), &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);

        screenWidth = fbw;
        screenHeight = fbh;

        // ----- Colored pipeline -----
        progCol_ = linkProgram(VS_COLOR, FS_COLOR);
        //progCol_ = linkProgramFromFiles("../Shaders/color.vert", "../Shaders/color.frag");


        glGenVertexArrays(1, &vaoCol_);
        glBindVertexArray(vaoCol_);

        glGenBuffers(1, &vboCol_);
        glBindBuffer(GL_ARRAY_BUFFER, vboCol_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 3, nullptr, GL_STREAM_DRAW);

        glEnableVertexAttribArray(0); // aPos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

        g_uColorLoc = glGetUniformLocation(progCol_, "uColor");

        // ----- Textured sprite pipeline -----
        progTex_ = linkProgram(VS_TEX, FS_TEX);
        //progTex_ = linkProgramFromFiles("../Shaders/texture.vert", "../Shaders/texture.frag");


        glGenVertexArrays(1, &vaoTex_);
        glBindVertexArray(vaoTex_);

        glGenBuffers(1, &vboTex_);
        glBindBuffer(GL_ARRAY_BUFFER, vboTex_);

        // Reserve enough for many sprites in one go:
        // 6 verts * (x,y,u,v) * MAX_SPRITES
        const size_t bytes = sizeof(float) * 6 * 4 * MAX_SPRITES_DEFAULT;
        glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0); // aPos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
        glEnableVertexAttribArray(1); // aUV
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

        g_uTexLoc_Tex = glGetUniformLocation(progTex_, "uTex");

        // Pre-size batch CPU buffer to avoid realloc churn
        g_batchVerts.reserve(6 * 4 * 2048);

        // ----- Text rendering pipeline -----
        progText_ = linkProgram(VS_TEXT, FS_TEXT);
        //progText_ = linkProgramFromFiles("../Shaders/text.vert", "../Shaders/text.frag");

        glGenVertexArrays(1, &vaoText_);
        glBindVertexArray(vaoText_);

        glGenBuffers(1, &vboText_);
        glBindBuffer(GL_ARRAY_BUFFER, vboText_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4 * 256, nullptr, GL_STREAM_DRAW);

        glEnableVertexAttribArray(0); // aPos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
        glEnableVertexAttribArray(1); // aUV
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

        g_uColorLoc_Text = glGetUniformLocation(progText_, "uColor");
        g_uTexLoc_Text = glGetUniformLocation(progText_, "uTex");

        // Unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Create game framebuffer
        CreateGameFramebuffer(800, 600);

        // ----- Spotlight / shadow pipeline -----
        progOccluder_ = linkProgram(VS_OCCLUDER, FS_OCCLUDER);
        progSpotlight_ = linkProgram(VS_SPOTLIGHT, FS_SPOTLIGHT);

        // Cache spotlight uniform locations
        g_sl_OccluderMask = glGetUniformLocation(progSpotlight_, "uOccluderMask");
        g_sl_LightNDC = glGetUniformLocation(progSpotlight_, "uLightNDC");
        g_sl_Radius = glGetUniformLocation(progSpotlight_, "uRadius");
        g_sl_ConeAngleRad = glGetUniformLocation(progSpotlight_, "uConeAngleRad");
        g_sl_DirectionRad = glGetUniformLocation(progSpotlight_, "uDirectionRad");
        g_sl_LightColor = glGetUniformLocation(progSpotlight_, "uLightColor");
        g_sl_Intensity = glGetUniformLocation(progSpotlight_, "uIntensity");
        g_sl_Falloff = glGetUniformLocation(progSpotlight_, "uFalloff");
        g_sl_AmbientColor = glGetUniformLocation(progSpotlight_, "uAmbientColor");
        g_sl_ShadowRays = glGetUniformLocation(progSpotlight_, "uShadowRays");
        g_sl_CastShadows = glGetUniformLocation(progSpotlight_, "uCastShadows");

        // Fullscreen quad VAO/VBO for spotlight composite pass (NDC -1..1, UV 0..1)
        const float fsQuad[] = {
            -1.f,-1.f, 0.f,0.f,
             1.f,-1.f, 1.f,0.f,
             1.f, 1.f, 1.f,1.f,
            -1.f,-1.f, 0.f,0.f,
             1.f, 1.f, 1.f,1.f,
            -1.f, 1.f, 0.f,1.f
        };
        glGenVertexArrays(1, &vaoFullscreen_);
        glBindVertexArray(vaoFullscreen_);
        glGenBuffers(1, &vboFullscreen_);
        glBindBuffer(GL_ARRAY_BUFFER, vboFullscreen_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fsQuad), fsQuad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Create occluder FBO at screen resolution (can use half-res for performance)
        InitSpotlightFBO(fbw, fbh);
    }

    void Graphics::Update() {
        t_ += Framework::Time::DeltaTime();
        // Present the frame (anything drawn since beginFrame)
        glfwSwapBuffers(GameManager::GetInstance().GetSystems().win_->window());
    }

    void Graphics::Shutdown() {
        if (vboCol_) { glDeleteBuffers(1, &vboCol_);  vboCol_ = 0; }
        if (vaoCol_) { glDeleteVertexArrays(1, &vaoCol_); vaoCol_ = 0; }
        if (progCol_) { glDeleteProgram(progCol_); progCol_ = 0; }

        if (vboTex_) { glDeleteBuffers(1, &vboTex_);  vboTex_ = 0; }
        if (vaoTex_) { glDeleteVertexArrays(1, &vaoTex_); vaoTex_ = 0; }
        if (progTex_) { glDeleteProgram(progTex_); progTex_ = 0; }

        if (vboText_) { glDeleteBuffers(1, &vboText_);  vboText_ = 0; }
        if (vaoText_) { glDeleteVertexArrays(1, &vaoText_); vaoText_ = 0; }
        if (progText_) { glDeleteProgram(progText_); progText_ = 0; }

        // Spotlight / shadow resources
        if (vboFullscreen_) { glDeleteBuffers(1, &vboFullscreen_); vboFullscreen_ = 0; }
        if (vaoFullscreen_) { glDeleteVertexArrays(1, &vaoFullscreen_); vaoFullscreen_ = 0; }
        if (progOccluder_) { glDeleteProgram(progOccluder_);  progOccluder_ = 0; }
        if (progSpotlight_) { glDeleteProgram(progSpotlight_); progSpotlight_ = 0; }
        if (texOccluder_) { glDeleteTextures(1, &texOccluder_);      texOccluder_ = 0; }
        if (fboOccluder_) { glDeleteFramebuffers(1, &fboOccluder_);  fboOccluder_ = 0; }
    }

    // =========================
    // Colored triangle path
    // =========================

    void Graphics::beginFrame(const Color& clear) {
        glClearColor(clear.r, clear.g, clear.b, clear.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Graphics::drawTriangle(const Vector2D& a, const Vector2D& b, const Vector2D& c,
        const Color& color) {
        const float verts[6] = { a.x, a.y, b.x, b.y, c.x, c.y };

        glUseProgram(progCol_);
        glUniform4f(g_uColorLoc, color.r, color.g, color.b, color.a);

        glBindVertexArray(vaoCol_);
        glBindBuffer(GL_ARRAY_BUFFER, vboCol_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void Graphics::drawRectangle(const Vector2D& min, const Vector2D& max, const Color& color) {
        Vector2D p1 = { min.x, min.y };
        Vector2D p2 = { max.x, min.y };
        Vector2D p3 = { min.x, max.y };
        Vector2D p4 = { max.x, max.y };

        drawTriangle(p1, p2, p3, color);
        drawTriangle(p3, p2, p4, color);
    }

    // =========================
    // Texture path (.tex from your tutorial)
    // =========================

    Texture Graphics::loadTextureFromTex(const char* path) {
        const int width = 256, height = 256, bpp = 4;
        const size_t total = size_t(width) * height * bpp;

        std::ifstream f(path, std::ios::binary);
        if (!f) {
            std::fprintf(stderr, "Failed to open %s\n", path);
            return 0;
        }

        std::vector<unsigned char> buf(total);
        f.read(reinterpret_cast<char*>(buf.data()), total);
        if (!f) {
            std::fprintf(stderr, "Short read on %s\n", path);
            return 0;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    void Graphics::setWrap(Texture tex, int wrapS, int wrapT) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Graphics::setAlphaBlend(bool on) {
        if (on) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
        else { glDisable(GL_BLEND); }
    }

    // =========================
    // Single sprite (existing API)
    // =========================

    void Graphics::drawSprite(Texture& tex, const Vector2D& center,
        const Vector2D& halfSize, float rotation,
        const Vector2D& minUV, const Vector2D& maxUV) {
        const float cx = center.x, cy = center.y;
        const float hx = halfSize.x, hy = halfSize.y;

        const float c = std::cos(rotation);
        const float s = std::sin(rotation);

        float aspectCorrection = (float)screenHeight / (float)screenWidth;

        auto rot = [&](float x, float y) -> Vector2D {
            float rx = c * x - s * y;
            float ry = s * x + c * y;

            rx *= aspectCorrection; // Shrink X offset

            return { cx + rx, cy + ry };
            };

        Vector2D p0 = rot(-hx, -hy);
        Vector2D p1 = rot(hx, -hy);
        Vector2D p2 = rot(hx, hy);
        Vector2D p3 = rot(-hx, hy);

        const float verts[] = {
            p0.x, p0.y,  minUV.x, minUV.y,
            p1.x, p1.y,  maxUV.x, minUV.y,
            p2.x, p2.y,  maxUV.x, maxUV.y,

            p0.x, p0.y,  minUV.x, minUV.y,
            p2.x, p2.y,  maxUV.x, maxUV.y,
            p3.x, p3.y,  minUV.x, maxUV.y,
        };

        glEnable(GL_BLEND);
        glUseProgram(progTex_);
        glBindVertexArray(vaoTex_);
        glBindBuffer(GL_ARRAY_BUFFER, vboTex_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(g_uTexLoc_Tex, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void Graphics::drawSpriteEasy(Texture& tex, const Vector2D& topLeft,
        const Vector2D& size, float rotation,
        const Vector2D& minUV, const Vector2D& maxUV) {
        Vector2D center = { topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f };
        Vector2D halfSz = { size.x * 0.5f, size.y * 0.5f };
        drawSprite(tex, center, halfSz, rotation, minUV, maxUV);
    }

    // =========================
    // Batched sprites (NEW)
    // =========================

    void Graphics::drawSpriteBatch(Texture tex, const std::vector<BatchedSprite>& sprites) {

        if (sprites.empty()) {
            return;
        }

        // Safety: Handle large batches by splitting
        const size_t MAX_SPRITES_PER_BATCH = 1000;
        if (sprites.size() > MAX_SPRITES_PER_BATCH) {
            // Draw in chunks
            for (size_t i = 0; i < sprites.size(); i += MAX_SPRITES_PER_BATCH) {
                size_t end = std::min(i + MAX_SPRITES_PER_BATCH, sprites.size());
                std::vector<BatchedSprite> chunk(sprites.begin() + i, sprites.begin() + end);
                drawSpriteBatchChunk(tex, chunk);
            }
        }
        else {
            drawSpriteBatchChunk(tex, sprites);
        }

    }

    void Graphics::drawSpriteBatchChunk(Texture tex, const std::vector<BatchedSprite>& sprites) {

        g_batchVerts.clear();
        g_batchVerts.reserve(sprites.size() * 6 * 4);

        float aspectCorrection = (float)screenHeight / (float)screenWidth;

        for (const auto& s : sprites) {
            const float cx = s.topLeft.x + s.size.x * 0.5f;
            const float cy = s.topLeft.y + s.size.y * 0.5f;
            const float hx = s.size.x * 0.5f;
            const float hy = s.size.y * 0.5f;

            const float c = std::cos(s.rotation);
            const float q = std::sin(s.rotation);

            auto rot = [&](float x, float y) -> Vector2D {
                // 1. Rotate the point in "Square Local Space"
                float rx = c * x - q * y;
                float ry = q * x + c * y;

                // 2. Apply Aspect Ratio Correction to the X offset ONLY
                rx *= aspectCorrection;

                // 3. Add to center position
                return { cx + rx, cy + ry };
                };

            Vector2D p0 = rot(-hx, -hy);
            Vector2D p1 = rot(hx, -hy);
            Vector2D p2 = rot(hx, hy);
            Vector2D p3 = rot(-hx, hy);

            g_batchVerts.insert(g_batchVerts.end(), {
                p0.x, p0.y, s.minUV.x, s.minUV.y,
                p1.x, p1.y, s.maxUV.x, s.minUV.y,
                p2.x, p2.y, s.maxUV.x, s.maxUV.y,

                p0.x, p0.y, s.minUV.x, s.minUV.y,
                p2.x, p2.y, s.maxUV.x, s.maxUV.y,
                p3.x, p3.y, s.minUV.x, s.maxUV.y
                });
        }
        drawSpriteBatchRaw(tex, g_batchVerts.data(), sprites.size());

    }

    void Graphics::drawSpriteBatchRaw(Texture tex, const float* interleavedXYUV, size_t spriteCount) {

        if (!spriteCount || !interleavedXYUV) {
            return;
        }

        const size_t vertCount = spriteCount * 6;
        const size_t byteCount = vertCount * 4 * sizeof(float);

        // === CRITICAL SAFETY CHECK ===
        if (spriteCount > MAX_SPRITES_DEFAULT) {
            return;
        }


        // Save state
        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        GLint prevProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

        if (!blendWasEnabled) {
            glEnable(GL_BLEND);
        }

        glUseProgram(progTex_);
        glBindVertexArray(vaoTex_);
        glBindBuffer(GL_ARRAY_BUFFER, vboTex_);


        // Orphan buffer for better performance
        glBufferData(GL_ARRAY_BUFFER, byteCount, nullptr, GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, byteCount, interleavedXYUV);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(g_uTexLoc_Tex, 0);

        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertCount));

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
        }
        else {
        }

        // Restore state
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(prevProgram);

        if (!blendWasEnabled) {
            glDisable(GL_BLEND);
        }

    }

    // =========================
    // Texture loading
    // =========================

    Texture Graphics::loadTextureFromPNG(const char* path, bool flipY, bool srgb) {
        if (flipY) stbi_set_flip_vertically_on_load(1); else stbi_set_flip_vertically_on_load(0);

        int w = 0, h = 0, n = 0;
        unsigned char* data = stbi_load(path, &w, &h, &n, STBI_rgb_alpha); // force RGBA8
        if (!data) {
            std::fprintf(stderr, "stbi_load failed for '%s': %s\n", path, stbi_failure_reason());
            return 0;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        const GLint internal = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        // Better sampling (optional: generate mipmaps for zoomed-out scenes)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
        return tex;
    }

    Texture Graphics::loadTextureFromFile(const char* path, bool /*unused*/) {
        std::string p = path ? path : "";
        auto dot = p.find_last_of('.');
        if (dot != std::string::npos) {
            std::string ext = p.substr(dot + 1);
            for (char& c : ext) c = (char)std::tolower((unsigned char)c);

            if (ext == "png") return loadTextureFromPNG(path, /*flipY*/ true, /*srgb*/ false);
            if (ext == "tex") return loadTextureFromTex(path);
        }
        return loadTextureFromPNG(path, /*flipY*/ true, /*srgb*/ false);
    }

    // =========================
    // Lines & Text
    // =========================

    void Graphics::drawLine(const Vector2D& start, const Vector2D& end, const Color& color) {

        // Save the current blend state
        GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
        if (!blendWasEnabled) {
            // Enable blending if it's not already on
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glUseProgram(progCol_);
        glUniform4f(g_uColorLoc, color.r, color.g, color.b, color.a);

        // Thin quad as line
        Vector2D dir = { end.x - start.x, end.y - start.y };
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (length < 0.001f) return;

        dir.x /= length;
        dir.y /= length;
        Vector2D perp = { -dir.y, dir.x };

        const float thickness = 0.001f; // Line thickness
        Vector2D halfThickness = { perp.x * thickness, perp.y * thickness };

        Vector2D p1 = { start.x - halfThickness.x, start.y - halfThickness.y };
        Vector2D p2 = { start.x + halfThickness.x, start.y + halfThickness.y };
        Vector2D p3 = { end.x - halfThickness.x, end.y - halfThickness.y };
        Vector2D p4 = { end.x + halfThickness.x, end.y + halfThickness.y };

        drawTriangle(p1, p3, p2, color);
        drawTriangle(p2, p3, p4, color);

        if (!blendWasEnabled) {
            glDisable(GL_BLEND);
        }
    }

    void Graphics::drawText(Font& font, const std::string& textToDraw, const Vector2D& pos, float scale, const Color& color) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(progText_);

        glUniform4f(g_uColorLoc_Text, color.r, color.g, color.b, color.a);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, font.GetAtlasID());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
        glUniform1i(g_uTexLoc_Text, 0);

        glBindVertexArray(vaoText_);
        glBindBuffer(GL_ARRAY_BUFFER, vboText_);

        std::vector<float> vertices;
        vertices.reserve(textToDraw.length() * 6 * 4);

        float x = (pos.x + 1.0f) * 0.5f * screenWidth;
        float y = (pos.y + 1.0f) * 0.5f * screenHeight;
        y -= font.GetAscent(scale);

        for (char c_signed : textToDraw) {
            unsigned char c = static_cast<unsigned char>(c_signed);
            const auto& ch = font.GetCharacters().at(c);

            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;

            float ndc_xpos = (xpos / screenWidth) * 2.0f - 1.0f;
            float ndc_ypos = (ypos / screenHeight) * 2.0f - 1.0f;
            float ndc_w = (w / screenWidth) * 2.0f;
            float ndc_h = (h / screenHeight) * 2.0f;

            float u = ch.TexCoords.x;
            float v = ch.TexCoords.y;
            float uvw = ch.TexCoords.z;
            float uvh = ch.TexCoords.w;

            vertices.insert(vertices.end(), {
                ndc_xpos,         ndc_ypos + ndc_h,   u,       v,
                ndc_xpos,         ndc_ypos,           u,       v + uvh,
                ndc_xpos + ndc_w, ndc_ypos,           u + uvw, v + uvh,

                ndc_xpos,         ndc_ypos + ndc_h,   u,       v,
                ndc_xpos + ndc_w, ndc_ypos,           u + uvw, v + uvh,
                ndc_xpos + ndc_w, ndc_ypos + ndc_h,   u + uvw, v
                });

            x += (ch.Advance >> 6) * scale;
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
    }

    // =========================
    // Spotlight / Shadow methods
    // =========================

    void Graphics::InitSpotlightFBO(int width, int height) {
        // Clean up any existing FBO first (handles resize calls)
        if (texOccluder_) { glDeleteTextures(1, &texOccluder_);     texOccluder_ = 0; }
        if (fboOccluder_) { glDeleteFramebuffers(1, &fboOccluder_); fboOccluder_ = 0; }

        fboWidth_ = width;
        fboHeight_ = height;

        glGenFramebuffers(1, &fboOccluder_);
        glBindFramebuffer(GL_FRAMEBUFFER, fboOccluder_);

        glGenTextures(1, &texOccluder_);
        glBindTexture(GL_TEXTURE_2D, texOccluder_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texOccluder_, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::fprintf(stderr, "Spotlight occluder FBO incomplete!\n");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Graphics::RenderOccluderRect(float ndcX, float ndcY, float halfW, float halfH) {
        // Draws a solid white axis-aligned quad into the currently-bound FBO
        // ndcX/Y is the centre; halfW/halfH are in NDC units
        const float verts[12] = {
            ndcX - halfW, ndcY - halfH,
            ndcX + halfW, ndcY - halfH,
            ndcX + halfW, ndcY + halfH,
            ndcX - halfW, ndcY - halfH,
            ndcX + halfW, ndcY + halfH,
            ndcX - halfW, ndcY + halfH
        };

        glUseProgram(progOccluder_);
        glBindVertexArray(vaoCol_);          // reuse pos-only VAO layout
        glBindBuffer(GL_ARRAY_BUFFER, vboCol_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void Graphics::CompositeSpotlight(
        const Vector2D& lightNDC,
        float radiusNDC,
        float coneAngleRad,
        float directionRad,
        const Color& lightColor,
        float intensity,
        float falloff,
        const Color& ambientColor,
        int   shadowRays,
        bool  castShadows)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(progSpotlight_);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texOccluder_);
        glUniform1i(g_sl_OccluderMask, 0);
        glUniform2f(g_sl_LightNDC, lightNDC.x, lightNDC.y);
        glUniform1f(g_sl_Radius, radiusNDC);
        glUniform1f(g_sl_ConeAngleRad, coneAngleRad);
        glUniform1f(g_sl_DirectionRad, directionRad);
        glUniform4f(g_sl_LightColor, lightColor.r, lightColor.g, lightColor.b, lightColor.a);
        glUniform1f(g_sl_Intensity, intensity);
        glUniform1f(g_sl_Falloff, falloff);
        glUniform4f(g_sl_AmbientColor, ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);
        glUniform1i(g_sl_ShadowRays, shadowRays);
        glUniform1i(g_sl_CastShadows, castShadows ? 1 : 0);

        glBindVertexArray(vaoFullscreen_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
    }

    void Graphics::BindOccluderFBO() {
        glBindFramebuffer(GL_FRAMEBUFFER, fboOccluder_);
        glViewport(0, 0, fboWidth_, fboHeight_);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_BLEND);
    }

    void Graphics::UnbindOccluderFBO() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);
    }

    // =========================
    // Framebuffers
    // =========================
#pragma region FRAMEBUFFERS

    Graphics::Framebuffer::Framebuffer(const FramebufferSpecs& specifications) : specs(specifications) {
        glGenFramebuffers(1, &rendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, rendererID);

        // Create color texture
        glGenTextures(1, &colorAttachment);
        glBindTexture(GL_TEXTURE_2D, colorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, specs.width, specs.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment, 0);

        // Create depth renderbuffer
        glGenRenderbuffers(1, &depthAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, depthAttachment);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, specs.width, specs.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthAttachment);

        // Sanity check
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::Error(DebugType::GraphicsSystem) << "Framebuffer is incomplete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }






    // Delegate Constructor
    Graphics::Framebuffer::Framebuffer(unsigned width, unsigned height) : Graphics::Framebuffer(FramebufferSpecs{ width, height }) {}

    Graphics::Framebuffer::~Framebuffer() {
        if (depthAttachment) glDeleteRenderbuffers(1, &depthAttachment);
        if (colorAttachment) glDeleteTextures(1, &colorAttachment);
        if (rendererID) glDeleteFramebuffers(1, &rendererID);
    }

    void Graphics::Framebuffer::Bind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, rendererID);
        glViewport(0, 0, specs.width, specs.height);
    }

    void Graphics::Framebuffer::Unbind() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Graphics::Framebuffer::Resize(FramebufferSpecs specifications) {
        specs = specifications;

        // Delete old attachments
        if (depthAttachment) glDeleteRenderbuffers(1, &depthAttachment);
        if (colorAttachment) glDeleteTextures(1, &colorAttachment);

        // Rebuild framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, rendererID);

        glGenTextures(1, &colorAttachment);
        glBindTexture(GL_TEXTURE_2D, colorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, specs.width, specs.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment, 0);

        glGenRenderbuffers(1, &depthAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, depthAttachment);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, specs.width, specs.height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthAttachment);

        // Sanity check
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::Error(DebugType::GraphicsSystem) << "Framebuffer is incomplete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Graphics::Framebuffer::Resize(unsigned width, unsigned height) {
        Resize(FramebufferSpecs{ width, height });
    }

    void Graphics::CreateGameFramebuffer(unsigned width, unsigned height) {
        gameFramebuffer_ = std::make_unique<Framebuffer>(FramebufferSpecs{ width, height });
    }

    void Graphics::ResizeGameFramebuffer(unsigned width, unsigned height) {
        if (gameFramebuffer_) {
            gameFramebuffer_->Resize(width, height);
        }
        else {
            CreateGameFramebuffer(width, height);
        }
    }

    void Graphics::BindGameFramebuffer() const {
        if (gameFramebuffer_) {
            gameFramebuffer_->Bind();
        }
    }

    void Graphics::UnbindGameFramebuffer() const {
        if (gameFramebuffer_) {
            gameFramebuffer_->Unbind();
        }
    }

    // CPU Image Loading for background thread
    Graphics::ImageRawData Graphics::LoadImageToCPU(const char* path, bool flipY) {
        if (flipY) stbi_set_flip_vertically_on_load(1);
        else stbi_set_flip_vertically_on_load(0);

        ImageRawData data;
        // Force 4 channels (RGBA)
        data.pixels = stbi_load(path, &data.width, &data.height, &data.channels, STBI_rgb_alpha);

        if (!data.pixels) {
            std::fprintf(stderr, "stbi_load failed for '%s': %s\n", path, stbi_failure_reason());
        }
        return data;
    }

    // Upload CPU image data to GPU as texture on main thread
    Texture Graphics::CreateTextureFromRawData(const ImageRawData& data, bool srgb) {
        if (!data.pixels) return 0;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        const GLint internal = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;

        // Upload pixels to GPU
        glTexImage2D(GL_TEXTURE_2D, 0, internal, data.width, data.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.pixels);

        // Set parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    // Free CPU image data
    void Graphics::FreeImageRawData(ImageRawData& data) {
        if (data.pixels) {
            stbi_image_free(data.pixels);
            data.pixels = nullptr;
        }
    }

#pragma endregion


} // namespace Framework