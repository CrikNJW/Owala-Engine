#pragma once

#include "Graphics.h"
#include "Components.h"
#include <glm/vec3.hpp>
#include <vector>

namespace Editor {
	class Viewport3DRenderer {
	public:
		struct CameraState {
			glm::vec3 position{ 0.0f, 3.0f, 10.0f };
			float yaw = -90.0f;
			float pitch = -18.0f;
			float fov = 60.0f;
		};

		struct PrimitiveRenderData {
			Framework::Primitive3DType type = Framework::Primitive3DType::Cube;
			glm::vec3 position{ 0.0f, 0.0f, 0.0f };
			glm::vec3 rotationDegrees{ 0.0f, 0.0f, 0.0f };
			glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
			Framework::Texture texture = 0;
			Framework::Color tint{ 1.0f, 1.0f, 1.0f, 1.0f };
			bool castShadow = true;
		};

		struct LightState {
			glm::vec3 direction{ -0.45f, -1.0f, -0.35f };
			Framework::Color color{ 1.0f, 1.0f, 1.0f, 1.0f };
			float intensity = 1.0f;
			float ambient = 0.22f;
		};

		void Initialize();
		void Shutdown();
		bool IsInitialized() const { return initialized_; }

		void Begin(const CameraState& camera, float aspectRatio, const LightState& light);
		void Submit(const PrimitiveRenderData& primitive);
		void Render();

	private:
		struct Mesh {
			unsigned vao = 0;
			unsigned vbo = 0;
			int vertexCount = 0;
		};

		void BuildMeshes();
		void BuildShaders();
		void BuildDefaultTexture();
		void DrawMesh(const Mesh& mesh) const;

		bool initialized_ = false;
		unsigned litProgram_ = 0;
		unsigned shadowProgram_ = 0;
		unsigned defaultTexture_ = 0;

		Mesh cubeMesh_{};
		Mesh circleMesh_{};
		Mesh planeMesh_{};

		CameraState currentCamera_{};
		LightState currentLight_{};
		float currentAspectRatio_ = 16.0f / 9.0f;
		std::vector<PrimitiveRenderData> queuedPrimitives_{};
	};
}
