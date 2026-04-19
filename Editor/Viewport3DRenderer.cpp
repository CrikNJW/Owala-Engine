#include "pch.h"
#include "Viewport3DRenderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
	GLuint CompileShader(GLenum shaderType, const char* source) {
		const GLuint shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);
		GLint isCompiled = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE) {
			char infoLog[1024]{};
			glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
			std::fprintf(stderr, "3D viewport shader compile error:\n%s\n", infoLog);
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

	GLuint LinkProgram(const char* vertexSource, const char* fragmentSource) {
		const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
		const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
		if (vertexShader == 0 || fragmentShader == 0) {
			if (vertexShader != 0) glDeleteShader(vertexShader);
			if (fragmentShader != 0) glDeleteShader(fragmentShader);
			return 0;
		}

		const GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		GLint linked = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (linked == GL_FALSE) {
			char infoLog[1024]{};
			glGetProgramInfoLog(program, 1024, nullptr, infoLog);
			std::fprintf(stderr, "3D viewport program link error:\n%s\n", infoLog);
			glDeleteProgram(program);
			return 0;
		}
		return program;
	}

	glm::mat4 BuildModelMatrix(const Editor::Viewport3DRenderer::PrimitiveRenderData& primitive) {
		glm::mat4 model(1.0f);
		model = glm::translate(model, primitive.position);
		model = glm::rotate(model, glm::radians(primitive.rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(primitive.rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(primitive.rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, primitive.scale);
		return model;
	}

	glm::vec3 CameraForward(float yawDeg, float pitchDeg) {
		const float yaw = glm::radians(yawDeg);
		const float pitch = glm::radians(pitchDeg);
		glm::vec3 forward{};
		forward.x = std::cos(yaw) * std::cos(pitch);
		forward.y = std::sin(pitch);
		forward.z = std::sin(yaw) * std::cos(pitch);
		if (glm::length(forward) <= 0.0001f) {
			return glm::vec3(0.0f, 0.0f, -1.0f);
		}
		return glm::normalize(forward);
	}
}

namespace Editor {
	void Viewport3DRenderer::Initialize() {
		if (initialized_) return;
		BuildShaders();
		BuildMeshes();
		BuildDefaultTexture();
		initialized_ = (litProgram_ != 0 && shadowProgram_ != 0);
	}

	void Viewport3DRenderer::Shutdown() {
		if (cubeMesh_.vbo != 0) glDeleteBuffers(1, &cubeMesh_.vbo);
		if (cubeMesh_.vao != 0) glDeleteVertexArrays(1, &cubeMesh_.vao);
		if (circleMesh_.vbo != 0) glDeleteBuffers(1, &circleMesh_.vbo);
		if (circleMesh_.vao != 0) glDeleteVertexArrays(1, &circleMesh_.vao);
		if (planeMesh_.vbo != 0) glDeleteBuffers(1, &planeMesh_.vbo);
		if (planeMesh_.vao != 0) glDeleteVertexArrays(1, &planeMesh_.vao);
		cubeMesh_ = {};
		circleMesh_ = {};
		planeMesh_ = {};

		if (defaultTexture_ != 0) {
			glDeleteTextures(1, &defaultTexture_);
			defaultTexture_ = 0;
		}
		if (litProgram_ != 0) {
			glDeleteProgram(litProgram_);
			litProgram_ = 0;
		}
		if (shadowProgram_ != 0) {
			glDeleteProgram(shadowProgram_);
			shadowProgram_ = 0;
		}

		queuedPrimitives_.clear();
		initialized_ = false;
	}

	void Viewport3DRenderer::Begin(const CameraState& camera, float aspectRatio, const LightState& light) {
		currentCamera_ = camera;
		currentAspectRatio_ = (aspectRatio > 0.0f) ? aspectRatio : (16.0f / 9.0f);
		currentLight_ = light;
		queuedPrimitives_.clear();
	}

	void Viewport3DRenderer::Submit(const PrimitiveRenderData& primitive) {
		queuedPrimitives_.push_back(primitive);
	}

	void Viewport3DRenderer::Render() {
		if (!initialized_) return;

		const glm::vec3 cameraForward = CameraForward(currentCamera_.yaw, currentCamera_.pitch);
		const glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::mat4 view = glm::lookAt(currentCamera_.position, currentCamera_.position + cameraForward, cameraUp);
		const glm::mat4 projection = glm::perspective(glm::radians(currentCamera_.fov), std::max(currentAspectRatio_, 0.01f), 0.1f, 3000.0f);

		glm::vec3 lightDirection = currentLight_.direction;
		if (glm::length(lightDirection) <= 0.0001f) {
			lightDirection = glm::vec3(-0.45f, -1.0f, -0.35f);
		}
		lightDirection = glm::normalize(lightDirection);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);

		glUseProgram(litProgram_);
		glUniformMatrix4fv(glGetUniformLocation(litProgram_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(litProgram_, "uProj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(glGetUniformLocation(litProgram_, "uLightDir"), 1, glm::value_ptr(lightDirection));
		glUniform3f(glGetUniformLocation(litProgram_, "uLightColor"), currentLight_.color.r, currentLight_.color.g, currentLight_.color.b);
		glUniform1f(glGetUniformLocation(litProgram_, "uLightIntensity"), std::max(currentLight_.intensity, 0.0f));
		glUniform1f(glGetUniformLocation(litProgram_, "uAmbient"), std::clamp(currentLight_.ambient, 0.0f, 1.0f));

		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(litProgram_, "uTexture"), 0);

		PrimitiveRenderData ground{};
		ground.type = Framework::Primitive3DType::Cube;
		ground.scale = glm::vec3(140.0f, 1.0f, 140.0f);
		ground.tint = { 0.55f, 0.57f, 0.62f, 1.0f };
		ground.texture = defaultTexture_;
		const glm::mat4 groundModel = BuildModelMatrix(ground);
		glUniformMatrix4fv(glGetUniformLocation(litProgram_, "uModel"), 1, GL_FALSE, glm::value_ptr(groundModel));
		glUniform4f(glGetUniformLocation(litProgram_, "uTint"), ground.tint.r, ground.tint.g, ground.tint.b, ground.tint.a);
		glUniform1i(glGetUniformLocation(litProgram_, "uUseTexture"), 1);
		glBindTexture(GL_TEXTURE_2D, defaultTexture_);
		DrawMesh(planeMesh_);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glUseProgram(shadowProgram_);
		glUniformMatrix4fv(glGetUniformLocation(shadowProgram_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shadowProgram_, "uProj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(glGetUniformLocation(shadowProgram_, "uLightDir"), 1, glm::value_ptr(lightDirection));
		glUniform1f(glGetUniformLocation(shadowProgram_, "uGroundY"), 0.0f);
		glUniform1f(glGetUniformLocation(shadowProgram_, "uShadowAlpha"), 0.35f);

		for (const PrimitiveRenderData& primitive : queuedPrimitives_) {
			if (!primitive.castShadow) continue;
			const glm::mat4 model = BuildModelMatrix(primitive);
			glUniformMatrix4fv(glGetUniformLocation(shadowProgram_, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
			if (primitive.type == Framework::Primitive3DType::Circle) DrawMesh(circleMesh_);
			else DrawMesh(cubeMesh_);
		}

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		// Draw lit primitives after the shadow projection pass so object pixels
		// always overdraw any projected shadow overlap.
		glUseProgram(litProgram_);
		glUniformMatrix4fv(glGetUniformLocation(litProgram_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(litProgram_, "uProj"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(glGetUniformLocation(litProgram_, "uLightDir"), 1, glm::value_ptr(lightDirection));
		glUniform3f(glGetUniformLocation(litProgram_, "uLightColor"), currentLight_.color.r, currentLight_.color.g, currentLight_.color.b);
		glUniform1f(glGetUniformLocation(litProgram_, "uLightIntensity"), std::max(currentLight_.intensity, 0.0f));
		glUniform1f(glGetUniformLocation(litProgram_, "uAmbient"), std::clamp(currentLight_.ambient, 0.0f, 1.0f));
		glUniform1i(glGetUniformLocation(litProgram_, "uTexture"), 0);

		for (const PrimitiveRenderData& primitive : queuedPrimitives_) {
			const glm::mat4 model = BuildModelMatrix(primitive);
			const Framework::Texture boundTexture = (primitive.texture != 0) ? primitive.texture : defaultTexture_;
			const bool hasTexture = (boundTexture != 0);

			glUniformMatrix4fv(glGetUniformLocation(litProgram_, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
			glUniform4f(glGetUniformLocation(litProgram_, "uTint"), primitive.tint.r, primitive.tint.g, primitive.tint.b, primitive.tint.a);
			glUniform1i(glGetUniformLocation(litProgram_, "uUseTexture"), hasTexture ? 1 : 0);
			glBindTexture(GL_TEXTURE_2D, boundTexture);

			if (primitive.type == Framework::Primitive3DType::Circle) DrawMesh(circleMesh_);
			else DrawMesh(cubeMesh_);
		}

		glDisable(GL_DEPTH_TEST);
	}

	void Viewport3DRenderer::BuildShaders() {
		static constexpr const char* LIT_VS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vWorldPos;
out vec3 vWorldNormal;
out vec2 vUV;
void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vWorldNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vUV = aUV;
    gl_Position = uProj * uView * worldPos;
})";

		static constexpr const char* LIT_FS = R"(#version 330 core
in vec3 vWorldPos;
in vec3 vWorldNormal;
in vec2 vUV;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uAmbient;
uniform vec4 uTint;
uniform sampler2D uTexture;
uniform int uUseTexture;
out vec4 FragColor;
void main() {
    vec3 baseColor = uTint.rgb;
    if (uUseTexture == 1) {
        baseColor *= texture(uTexture, vUV).rgb;
    }
    vec3 N = normalize(vWorldNormal);
    vec3 L = normalize(-uLightDir);
    float diffuse = max(dot(N, L), 0.0);
    float lighting = clamp(uAmbient + diffuse * uLightIntensity, 0.0, 2.0);
    vec3 finalColor = baseColor * (uLightColor * lighting);
    FragColor = vec4(finalColor, uTint.a);
})";

		static constexpr const char* SHADOW_VS = R"(#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uLightDir;
uniform float uGroundY;
void main() {
    vec3 dir = normalize(uLightDir);
    float denom = max(-dir.y, 0.0001);
    vec3 worldPos = vec3(uModel * vec4(aPos, 1.0));
    float t = (worldPos.y - uGroundY) / denom;
    vec3 projected = worldPos + dir * t;
    projected.y = uGroundY + 0.015;
    gl_Position = uProj * uView * vec4(projected, 1.0);
})";

		static constexpr const char* SHADOW_FS = R"(#version 330 core
uniform float uShadowAlpha;
out vec4 FragColor;
void main() {
    FragColor = vec4(0.0, 0.0, 0.0, uShadowAlpha);
})";

		litProgram_ = LinkProgram(LIT_VS, LIT_FS);
		shadowProgram_ = LinkProgram(SHADOW_VS, SHADOW_FS);
	}

	void Viewport3DRenderer::BuildMeshes() {
		auto uploadMesh = [](Mesh& mesh, const std::vector<float>& vertices) {
			mesh.vertexCount = static_cast<int>(vertices.size() / 8);
			glGenVertexArrays(1, &mesh.vao);
			glGenBuffers(1, &mesh.vbo);
			glBindVertexArray(mesh.vao);
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
			glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 6));
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		};

		const std::vector<float> cubeVertices = {
			// Front
			-0.5f,-0.5f, 0.5f, 0.f,0.f,1.f, 0.f,0.f,  0.5f,-0.5f, 0.5f, 0.f,0.f,1.f, 1.f,0.f,  0.5f, 0.5f, 0.5f, 0.f,0.f,1.f, 1.f,1.f,
			-0.5f,-0.5f, 0.5f, 0.f,0.f,1.f, 0.f,0.f,  0.5f, 0.5f, 0.5f, 0.f,0.f,1.f, 1.f,1.f, -0.5f, 0.5f, 0.5f, 0.f,0.f,1.f, 0.f,1.f,
			// Back
			 0.5f,-0.5f,-0.5f, 0.f,0.f,-1.f, 0.f,0.f, -0.5f,-0.5f,-0.5f, 0.f,0.f,-1.f, 1.f,0.f, -0.5f, 0.5f,-0.5f, 0.f,0.f,-1.f, 1.f,1.f,
			 0.5f,-0.5f,-0.5f, 0.f,0.f,-1.f, 0.f,0.f, -0.5f, 0.5f,-0.5f, 0.f,0.f,-1.f, 1.f,1.f,  0.5f, 0.5f,-0.5f, 0.f,0.f,-1.f, 0.f,1.f,
			// Left
			-0.5f,-0.5f,-0.5f, -1.f,0.f,0.f, 0.f,0.f, -0.5f,-0.5f, 0.5f, -1.f,0.f,0.f, 1.f,0.f, -0.5f, 0.5f, 0.5f, -1.f,0.f,0.f, 1.f,1.f,
			-0.5f,-0.5f,-0.5f, -1.f,0.f,0.f, 0.f,0.f, -0.5f, 0.5f, 0.5f, -1.f,0.f,0.f, 1.f,1.f, -0.5f, 0.5f,-0.5f, -1.f,0.f,0.f, 0.f,1.f,
			// Right
			 0.5f,-0.5f, 0.5f, 1.f,0.f,0.f, 0.f,0.f,  0.5f,-0.5f,-0.5f, 1.f,0.f,0.f, 1.f,0.f,  0.5f, 0.5f,-0.5f, 1.f,0.f,0.f, 1.f,1.f,
			 0.5f,-0.5f, 0.5f, 1.f,0.f,0.f, 0.f,0.f,  0.5f, 0.5f,-0.5f, 1.f,0.f,0.f, 1.f,1.f,  0.5f, 0.5f, 0.5f, 1.f,0.f,0.f, 0.f,1.f,
			// Top
			-0.5f, 0.5f, 0.5f, 0.f,1.f,0.f, 0.f,0.f,  0.5f, 0.5f, 0.5f, 0.f,1.f,0.f, 1.f,0.f,  0.5f, 0.5f,-0.5f, 0.f,1.f,0.f, 1.f,1.f,
			-0.5f, 0.5f, 0.5f, 0.f,1.f,0.f, 0.f,0.f,  0.5f, 0.5f,-0.5f, 0.f,1.f,0.f, 1.f,1.f, -0.5f, 0.5f,-0.5f, 0.f,1.f,0.f, 0.f,1.f,
			// Bottom
			-0.5f,-0.5f,-0.5f, 0.f,-1.f,0.f, 0.f,0.f,  0.5f,-0.5f,-0.5f, 0.f,-1.f,0.f, 1.f,0.f,  0.5f,-0.5f, 0.5f, 0.f,-1.f,0.f, 1.f,1.f,
			-0.5f,-0.5f,-0.5f, 0.f,-1.f,0.f, 0.f,0.f,  0.5f,-0.5f, 0.5f, 0.f,-1.f,0.f, 1.f,1.f, -0.5f,-0.5f, 0.5f, 0.f,-1.f,0.f, 0.f,1.f
		};
		uploadMesh(cubeMesh_, cubeVertices);

		std::vector<float> circleVertices;
		constexpr int slices = 24;
		constexpr int stacks = 16;
		circleVertices.reserve(static_cast<size_t>(slices) * static_cast<size_t>(stacks) * 6 * 8);

		auto addVertex = [&circleVertices](const glm::vec3& p, const glm::vec3& n, float u, float v) {
			circleVertices.insert(circleVertices.end(), { p.x, p.y, p.z, n.x, n.y, n.z, u, v });
			};

		for (int y = 0; y < stacks; ++y) {
			const float v0 = static_cast<float>(y) / static_cast<float>(stacks);
			const float v1 = static_cast<float>(y + 1) / static_cast<float>(stacks);
			const float phi0 = (v0 - 0.5f) * static_cast<float>(M_PI);
			const float phi1 = (v1 - 0.5f) * static_cast<float>(M_PI);
			const float y0 = std::sin(phi0) * 0.5f;
			const float y1 = std::sin(phi1) * 0.5f;
			const float r0 = std::cos(phi0) * 0.5f;
			const float r1 = std::cos(phi1) * 0.5f;

			for (int x = 0; x < slices; ++x) {
				const float u0 = static_cast<float>(x) / static_cast<float>(slices);
				const float u1 = static_cast<float>(x + 1) / static_cast<float>(slices);
				const float theta0 = u0 * 2.0f * static_cast<float>(M_PI);
				const float theta1 = u1 * 2.0f * static_cast<float>(M_PI);

				const glm::vec3 p00{ std::cos(theta0) * r0, y0, std::sin(theta0) * r0 };
				const glm::vec3 p10{ std::cos(theta1) * r0, y0, std::sin(theta1) * r0 };
				const glm::vec3 p01{ std::cos(theta0) * r1, y1, std::sin(theta0) * r1 };
				const glm::vec3 p11{ std::cos(theta1) * r1, y1, std::sin(theta1) * r1 };

				const glm::vec3 n00 = glm::normalize(p00);
				const glm::vec3 n10 = glm::normalize(p10);
				const glm::vec3 n01 = glm::normalize(p01);
				const glm::vec3 n11 = glm::normalize(p11);

				addVertex(p00, n00, u0, v0);
				addVertex(p10, n10, u1, v0);
				addVertex(p11, n11, u1, v1);

				addVertex(p00, n00, u0, v0);
				addVertex(p11, n11, u1, v1);
				addVertex(p01, n01, u0, v1);
			}
		}
		uploadMesh(circleMesh_, circleVertices);

		const std::vector<float> planeVertices = {
			-0.5f, 0.0f,-0.5f, 0.f,1.f,0.f, 0.f,0.f,
			 0.5f, 0.0f,-0.5f, 0.f,1.f,0.f, 1.f,0.f,
			 0.5f, 0.0f, 0.5f, 0.f,1.f,0.f, 1.f,1.f,
			-0.5f, 0.0f,-0.5f, 0.f,1.f,0.f, 0.f,0.f,
			 0.5f, 0.0f, 0.5f, 0.f,1.f,0.f, 1.f,1.f,
			-0.5f, 0.0f, 0.5f, 0.f,1.f,0.f, 0.f,1.f
		};
		uploadMesh(planeMesh_, planeVertices);
	}

	void Viewport3DRenderer::BuildDefaultTexture() {
		if (defaultTexture_ != 0) return;
		const unsigned char pixels[] = {
			210, 210, 210, 255,  90,  90,  90, 255,
			 90,  90,  90, 255, 210, 210, 210, 255
		};
		glGenTextures(1, &defaultTexture_);
		glBindTexture(GL_TEXTURE_2D, defaultTexture_);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Viewport3DRenderer::DrawMesh(const Mesh& mesh) const {
		if (mesh.vao == 0 || mesh.vertexCount <= 0) return;
		glBindVertexArray(mesh.vao);
		glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
		glBindVertexArray(0);
	}
}
