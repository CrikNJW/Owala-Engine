#pragma once

#include "Entity.h"
#include "Components.h"
#include <filesystem>
#include <future>
#include <string>
#include <unordered_map>

namespace Framework {
	class CSharpScriptEngine {
	public:
		static CSharpScriptEngine& Get();
		void UpdateScript(Entity entity, NativeScriptsComponent::NativeScript& script);
		void DestroyScriptInstance(Entity entity, NativeScriptsComponent::NativeScript& script);
		void ResetRuntime();
		~CSharpScriptEngine();

	private:
		struct ScriptState {
			bool hasWriteTime = false;
			std::filesystem::file_time_type lastWriteTime{};
			bool loaded = false;
			bool created = false;
			bool sourceMissingLogged = false;
			bool invokeFailedLogged = false;
			std::filesystem::path assemblyPath;
			std::string typeName;
		};

		struct NativeApi {
			void(__cdecl* logUtf8)(const char* message) = nullptr;
			int(__cdecl* getTransformPosition)(uint64_t entity, float* outX, float* outY) = nullptr;
			int(__cdecl* setTransformPosition)(uint64_t entity, float x, float y) = nullptr;
		};

		using HostFxrInitializeForRuntimeConfigFn = int(__cdecl*)(const wchar_t*, const void*, void**);
		using HostFxrGetRuntimeDelegateFn = int(__cdecl*)(void*, int, void**);
		using HostFxrCloseFn = int(__cdecl*)(void*);
		using LoadAssemblyAndGetFunctionPointerFn = int(__stdcall*)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*, void*, void**);

		using ManagedSetNativeApiFn = int(__stdcall*)(const NativeApi*, char*, int);
		using ManagedLoadScriptFn = int(__stdcall*)(const char*, const char*, const char*, char*, int);
		using ManagedInvokeScriptFn = int(__stdcall*)(const char*, int, uint64_t, char*, int);
		using ManagedDestroyScriptFn = int(__stdcall*)(const char*, uint64_t, char*, int);
		using ManagedResetFn = int(__stdcall*)(char*, int);

		bool hostBuilt_ = false;
		bool hostBuildFailedLogged_ = false;
		bool runtimeInitialized_ = false;
		bool compileRequested_ = true;
		bool compileInFlight_ = false;
		bool compileFailedLogged_ = false;
		std::future<int> compileFuture_;

		std::unordered_map<std::string, ScriptState> states_;

		void* hostfxrLibrary_ = nullptr;
		HostFxrInitializeForRuntimeConfigFn hostfxrInitializeForRuntimeConfig_ = nullptr;
		HostFxrGetRuntimeDelegateFn hostfxrGetRuntimeDelegate_ = nullptr;
		HostFxrCloseFn hostfxrClose_ = nullptr;
		LoadAssemblyAndGetFunctionPointerFn loadAssemblyAndGetFunctionPointer_ = nullptr;

		ManagedSetNativeApiFn managedSetNativeApi_ = nullptr;
		ManagedLoadScriptFn managedLoadScript_ = nullptr;
		ManagedInvokeScriptFn managedInvokeScript_ = nullptr;
		ManagedDestroyScriptFn managedDestroyScript_ = nullptr;
		ManagedResetFn managedReset_ = nullptr;
		NativeApi nativeApi_{};

		CSharpScriptEngine() = default;

		std::string ScriptInstanceKey(const NativeScriptsComponent::NativeScript& script) const;
		std::filesystem::path ResolveSourcePath(const NativeScriptsComponent::NativeScript& script) const;
		std::filesystem::path GetScriptsRootPath() const;
		std::filesystem::path GetScriptsProjectPath() const;
		std::filesystem::path GetHostProjectPath() const;
		std::filesystem::path GetHostAssemblyPath() const;
		std::filesystem::path GetHostRuntimeConfigPath() const;
		std::filesystem::path GetScriptsAssemblyPath() const;
		std::filesystem::path LocateHostFxrPath() const;

		void EnsureScriptsProjectExists() const;
		bool EnsureHostBuilt();
		bool EnsureScriptsAssemblyUpToDate();
		void MarkAllScriptsForReload();

		bool EnsureRuntimeInitialized();
		bool LoadHostFxr();
		bool ResolveManagedExports();
		bool RegisterNativeApi();
		void ShutdownRuntime();
		void ResetDelegatePointers();

		bool CallManagedLoad(const std::string& id, const std::filesystem::path& assemblyPath, const std::string& typeName, ScriptState& state);
		bool CallManagedInvoke(const std::string& id, int methodId, Entity entity, ScriptState& state);
		bool CallManagedDestroy(const std::string& id, Entity entity, ScriptState& state);

		bool RunCommand(const std::string& command) const;
		static std::wstring ToWide(const std::filesystem::path& path);
		static std::string Quote(const std::filesystem::path& path);
	};
}
