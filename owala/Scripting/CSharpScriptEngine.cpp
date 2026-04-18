#include "pch.h"
#include "CSharpScriptEngine.h"
#include "Logger.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "FileUtility.h"

namespace Framework {
	namespace {
		constexpr int HOSTFXR_DELEGATE_LOAD_ASSEMBLY_AND_GET_FUNCTION_POINTER = 5;
		const wchar_t* UNMANAGEDCALLERSONLY_METHOD = reinterpret_cast<const wchar_t*>(-1);
		const wchar_t* MANAGED_BRIDGE_TYPE = L"Owala.ScriptHost.Bridge, OwalaScriptHost";

		constexpr int SCRIPT_METHOD_ON_CREATE = 0;
		constexpr int SCRIPT_METHOD_ON_UPDATE = 1;
		constexpr int SCRIPT_METHOD_ON_DESTROY = 2;

		void __cdecl NativeLogUtf8(const char* message) {
			if (!message) return;
			Logger::Get().Info(DebugType::GameManager) << "[C#] " << message << std::endl;
		}

		int __cdecl NativeGetTransformPosition(uint64_t entityValue, float* outX, float* outY) {
			if (!outX || !outY) return 0;
			if (!GameManager::Exists()) return 0;
			GameManager& game = GameManager::GetInstance();
			EntityStorage* ecs = game.GetSystems().ecs_;
			if (!ecs) return 0;

			EntityManager entityMgr(*ecs);
			const Entity entity = static_cast<Entity>(entityValue);
			if (!entityMgr.ValidateEntity(entity)) return 0;
			if (!entityMgr.HasComponent(entity, ComponentType::TransformType)) return 0;

			Transform& transform = entityMgr.GetComponent<Transform>(entity);
			*outX = transform.position.x;
			*outY = transform.position.y;
			return 1;
		}

		int __cdecl NativeSetTransformPosition(uint64_t entityValue, float x, float y) {
			if (!GameManager::Exists()) return 0;
			GameManager& game = GameManager::GetInstance();
			EntityStorage* ecs = game.GetSystems().ecs_;
			if (!ecs) return 0;

			EntityManager entityMgr(*ecs);
			const Entity entity = static_cast<Entity>(entityValue);
			if (!entityMgr.ValidateEntity(entity)) return 0;
			if (!entityMgr.HasComponent(entity, ComponentType::TransformType)) return 0;

			Transform& transform = entityMgr.GetComponent<Transform>(entity);
			transform.position.x = x;
			transform.position.y = y;
			return 1;
		}

		std::filesystem::path GetActiveGameRootPath() {
			const std::string& dataDir = FileIOSystem::GetSourceDataDir();
			if (!dataDir.empty()) {
				return std::filesystem::path(dataDir).parent_path().lexically_normal();
			}
			return std::filesystem::current_path().lexically_normal();
		}

		void MigrateLegacyHostProjectIfNeeded(const std::filesystem::path& sourceHostProjectPath) {
			std::error_code fsError;
			if (std::filesystem::exists(sourceHostProjectPath, fsError) && !fsError) {
				return;
			}

			const std::filesystem::path legacyHostDir = (GetActiveGameRootPath() / "Data" / "Scripts" / "CSharp" / "Host").lexically_normal();
			const std::filesystem::path legacyHostProject = legacyHostDir / "OwalaScriptHost.csproj";
			const std::filesystem::path legacyHostProgram = legacyHostDir / "Program.cs";
			fsError.clear();
			if (!std::filesystem::exists(legacyHostProject, fsError) || fsError) {
				return;
			}

			std::filesystem::create_directories(sourceHostProjectPath.parent_path(), fsError);
			if (fsError) {
				return;
			}

			fsError.clear();
			std::filesystem::copy_file(legacyHostProject, sourceHostProjectPath, std::filesystem::copy_options::overwrite_existing, fsError);
			if (fsError) {
				return;
			}

			fsError.clear();
			if (std::filesystem::exists(legacyHostProgram, fsError) && !fsError) {
				std::filesystem::copy_file(
					legacyHostProgram,
					sourceHostProjectPath.parent_path() / "Program.cs",
					std::filesystem::copy_options::overwrite_existing,
					fsError
				);
			}
		}
	}

	CSharpScriptEngine& CSharpScriptEngine::Get() {
		static CSharpScriptEngine instance;
		return instance;
	}

	CSharpScriptEngine::~CSharpScriptEngine() {
		ResetRuntime();
		ShutdownRuntime();
	}

	void CSharpScriptEngine::UpdateScript(Entity entity, NativeScriptsComponent::NativeScript& script) {
		if (script.backend != NativeScriptsComponent::ScriptBackend::CSharp) return;

		script.EnsureRuntimeScriptId();
		if (script.csharpTypeName.empty()) {
			script.csharpTypeName = script.serializedTypeName;
		}
		if (script.csharpSourcePath.empty() && !script.csharpTypeName.empty()) {
			const std::filesystem::path defaultPath = GetScriptsRootPath() / (script.csharpTypeName + ".cs");
			script.csharpSourcePath = defaultPath.string();
		}
		if (script.csharpTypeName.empty()) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Missing C# type name." << std::endl;
			return;
		}

		const std::string instanceKey = ScriptInstanceKey(script);
		ScriptState& state = states_[instanceKey];
		state.typeName = script.csharpTypeName;
		state.assemblyPath = GetScriptsAssemblyPath();

		const std::filesystem::path sourcePath = ResolveSourcePath(script);
		std::error_code fsError;
		const bool exists = std::filesystem::exists(sourcePath, fsError);
		if (fsError || !exists) {
			if (!state.sourceMissingLogged) {
				Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Source file not found: " << sourcePath.string() << std::endl;
				state.sourceMissingLogged = true;
			}
			return;
		}
		state.sourceMissingLogged = false;

		const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(sourcePath, fsError);
		const bool hasWriteTime = !fsError;
		const bool sourceChanged = !state.hasWriteTime || !hasWriteTime || (writeTime != state.lastWriteTime);
		if (sourceChanged) {
			state.loaded = false;
			state.created = false;
			state.invokeFailedLogged = false;
			compileRequested_ = true;
		}
		if (hasWriteTime) {
			state.lastWriteTime = writeTime;
			state.hasWriteTime = true;
		}

		if (!EnsureHostBuilt()) return;
		if (!EnsureRuntimeInitialized()) return;
		if (!EnsureScriptsAssemblyUpToDate()) return;

		if (!state.loaded) {
			if (!CallManagedLoad(instanceKey, state.assemblyPath, script.csharpTypeName, state)) return;
			state.loaded = true;
			state.created = false;
		}

		if (!state.created) {
			if (!CallManagedInvoke(instanceKey, SCRIPT_METHOD_ON_CREATE, entity, state)) return;
			state.created = true;
		}

		CallManagedInvoke(instanceKey, SCRIPT_METHOD_ON_UPDATE, entity, state);
	}

	void CSharpScriptEngine::DestroyScriptInstance(Entity entity, NativeScriptsComponent::NativeScript& script) {
		if (script.backend != NativeScriptsComponent::ScriptBackend::CSharp) return;
		if (script.runtimeScriptId == 0) return;

		const std::string instanceKey = ScriptInstanceKey(script);
		auto it = states_.find(instanceKey);
		if (it == states_.end()) return;

		ScriptState& state = it->second;
		if (state.loaded && runtimeInitialized_) {
			CallManagedDestroy(instanceKey, entity, state);
		}

		states_.erase(it);
	}

	void CSharpScriptEngine::ResetRuntime() {
		if (runtimeInitialized_ && managedReset_) {
			char errorBuffer[1024]{};
			const int rc = managedReset_(errorBuffer, static_cast<int>(sizeof(errorBuffer)));
			if (rc != 0 && errorBuffer[0] != '\0') {
				Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Managed reset failed: " << errorBuffer << std::endl;
			}
		}
		states_.clear();
	}

	std::string CSharpScriptEngine::ScriptInstanceKey(const NativeScriptsComponent::NativeScript& script) const {
		return std::to_string(script.runtimeScriptId);
	}

	std::filesystem::path CSharpScriptEngine::ResolveSourcePath(const NativeScriptsComponent::NativeScript& script) const {
		std::error_code fsError;
		std::filesystem::path path(script.csharpSourcePath);
		if (!path.empty()) {
			if (path.is_relative()) {
				path = std::filesystem::current_path() / path;
			}
			path = path.lexically_normal();
			if (std::filesystem::exists(path, fsError) && !fsError) {
				return path;
			}
		}

		if (!script.csharpTypeName.empty()) {
			const std::filesystem::path fallback = (GetScriptsRootPath() / (script.csharpTypeName + ".cs")).lexically_normal();
			fsError.clear();
			if (std::filesystem::exists(fallback, fsError) && !fsError) {
				return fallback;
			}
			return fallback;
		}
		return path;
	}

	std::filesystem::path CSharpScriptEngine::GetScriptsRootPath() const {
		return (GetActiveGameRootPath() / "Source" / "CSharp" / "UserScripts").lexically_normal();
	}

	std::filesystem::path CSharpScriptEngine::GetScriptsProjectPath() const {
		return (GetScriptsRootPath() / "OwalaScripts.csproj").lexically_normal();
	}

	std::filesystem::path CSharpScriptEngine::GetHostProjectPath() const {
		return (GetActiveGameRootPath() / "Source" / "CSharp" / "Host" / "OwalaScriptHost.csproj").lexically_normal();
	}

	std::filesystem::path CSharpScriptEngine::GetHostAssemblyPath() const {
		return (GetHostProjectPath().parent_path() / "bin" / "Debug" / "net8.0" / "OwalaScriptHost.dll").lexically_normal();
	}

	std::filesystem::path CSharpScriptEngine::GetHostRuntimeConfigPath() const {
		return (GetHostProjectPath().parent_path() / "bin" / "Debug" / "net8.0" / "OwalaScriptHost.runtimeconfig.json").lexically_normal();
	}

	std::filesystem::path CSharpScriptEngine::GetScriptsAssemblyPath() const {
		return (GetScriptsProjectPath().parent_path() / "bin" / "Debug" / "net8.0" / "OwalaScripts.dll").lexically_normal();
	}

	std::filesystem::path CSharpScriptEngine::LocateHostFxrPath() const {
		std::vector<std::filesystem::path> roots;
		if (const char* dotnetRoot = std::getenv("DOTNET_ROOT")) {
			roots.emplace_back(dotnetRoot);
		}
		if (const char* programFiles = std::getenv("ProgramFiles")) {
			roots.emplace_back(std::filesystem::path(programFiles) / "dotnet");
		}
		if (const char* programFilesX86 = std::getenv("ProgramFiles(x86)")) {
			roots.emplace_back(std::filesystem::path(programFilesX86) / "dotnet");
		}

		for (const std::filesystem::path& root : roots) {
			const std::filesystem::path fxrRoot = root / "host" / "fxr";
			std::error_code ec;
			if (!std::filesystem::exists(fxrRoot, ec) || ec) continue;

			std::filesystem::path bestVersionDir;
			std::vector<int> bestSemver{ -1, -1, -1 };

			for (const auto& entry : std::filesystem::directory_iterator(fxrRoot, ec)) {
				if (ec || !entry.is_directory()) continue;
				const std::string verStr = entry.path().filename().string();
				std::vector<int> semver{ 0, 0, 0 };
				std::stringstream ss(verStr);
				std::string token;
				int idx = 0;
				while (std::getline(ss, token, '.') && idx < 3) {
					try { semver[idx] = std::stoi(token); }
					catch (...) { semver[idx] = 0; }
					++idx;
				}
				if (semver > bestSemver) {
					bestSemver = semver;
					bestVersionDir = entry.path();
				}
			}

			if (!bestVersionDir.empty()) {
				const std::filesystem::path dllPath = bestVersionDir / "hostfxr.dll";
				if (std::filesystem::exists(dllPath, ec) && !ec) {
					return dllPath;
				}
			}
		}

		return {};
	}

	void CSharpScriptEngine::EnsureScriptsProjectExists() const {
		const std::filesystem::path projectPath = GetScriptsProjectPath();
		const std::filesystem::path hostAssemblyPath = (GetHostProjectPath().parent_path() / "bin" / "Debug" / "net8.0" / "OwalaScriptHost.dll").lexically_normal();
		std::error_code fsError;
		std::filesystem::create_directories(projectPath.parent_path(), fsError);

		std::ofstream out(projectPath, std::ios::binary);
		out <<
			"<Project Sdk=\"Microsoft.NET.Sdk\">\n"
			"  <PropertyGroup>\n"
			"    <TargetFramework>net8.0</TargetFramework>\n"
			"    <ImplicitUsings>enable</ImplicitUsings>\n"
			"    <Nullable>enable</Nullable>\n"
			"    <OutputType>Library</OutputType>\n"
			"  </PropertyGroup>\n"
			"  <ItemGroup>\n"
			"    <Reference Include=\"OwalaScriptHost\">\n"
			"      <HintPath>" << hostAssemblyPath.string() << "</HintPath>\n"
			"      <Private>false</Private>\n"
			"    </Reference>\n"
			"  </ItemGroup>\n"
			"</Project>\n";
	}

	bool CSharpScriptEngine::EnsureHostBuilt() {
		if (hostBuilt_) return true;

		const std::filesystem::path hostProject = GetHostProjectPath();
		MigrateLegacyHostProjectIfNeeded(hostProject);
		std::error_code fsError;
		if (!std::filesystem::exists(hostProject, fsError)) {
			if (!hostBuildFailedLogged_) {
				Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Host project missing: " << hostProject.string() << std::endl;
				hostBuildFailedLogged_ = true;
			}
			return false;
		}

		const std::string buildCommand = "dotnet build " + Quote(hostProject) + " --nologo --verbosity quiet";
		const bool ok = RunCommand(buildCommand);
		if (!ok) {
			if (!hostBuildFailedLogged_) {
				Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Failed to build host project." << std::endl;
				hostBuildFailedLogged_ = true;
			}
			return false;
		}

		hostBuilt_ = true;
		hostBuildFailedLogged_ = false;
		return true;
	}

	bool CSharpScriptEngine::EnsureScriptsAssemblyUpToDate() {
		EnsureScriptsProjectExists();
		const std::filesystem::path projectPath = GetScriptsProjectPath();

		if (compileInFlight_) {
			const auto status = compileFuture_.wait_for(std::chrono::seconds(0));
			if (status == std::future_status::ready) {
				const int result = compileFuture_.get();
				compileInFlight_ = false;
				if (result != 0) {
					if (!compileFailedLogged_) {
						Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Scripts compilation failed." << std::endl;
						compileFailedLogged_ = true;
					}
					return false;
				}
				compileFailedLogged_ = false;
				MarkAllScriptsForReload();
			}
			else {
				return false;
			}
		}

		if (compileRequested_) {
			const std::string buildCommand = "dotnet build " + Quote(projectPath) + " --nologo --verbosity quiet";
			compileFuture_ = std::async(std::launch::async, [buildCommand]() {
				return std::system(buildCommand.c_str());
			});
			compileInFlight_ = true;
			compileRequested_ = false;
			return false;
		}

		std::error_code fsError;
		if (!std::filesystem::exists(GetScriptsAssemblyPath(), fsError) || fsError) {
			compileRequested_ = true;
			return false;
		}
		return true;
	}

	void CSharpScriptEngine::MarkAllScriptsForReload() {
		for (auto& [_, state] : states_) {
			state.loaded = false;
			state.created = false;
			state.invokeFailedLogged = false;
		}
	}

	bool CSharpScriptEngine::EnsureRuntimeInitialized() {
		if (runtimeInitialized_) return true;
		if (!LoadHostFxr()) return false;

		const std::filesystem::path runtimeConfig = GetHostRuntimeConfigPath();
		std::error_code fsError;
		if (!std::filesystem::exists(runtimeConfig, fsError) || fsError) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Host runtimeconfig missing: " << runtimeConfig.string() << std::endl;
			return false;
		}

		void* hostContext = nullptr;
		const std::wstring runtimeConfigW = ToWide(runtimeConfig);
		const int initRc = hostfxrInitializeForRuntimeConfig_(runtimeConfigW.c_str(), nullptr, &hostContext);
		if (initRc != 0 || !hostContext) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: hostfxr init failed (" << initRc << ")." << std::endl;
			return false;
		}

		void* loadPtrRaw = nullptr;
		const int getDelRc = hostfxrGetRuntimeDelegate_(hostContext, HOSTFXR_DELEGATE_LOAD_ASSEMBLY_AND_GET_FUNCTION_POINTER, &loadPtrRaw);
		hostfxrClose_(hostContext);
		if (getDelRc != 0 || !loadPtrRaw) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: hostfxr get delegate failed (" << getDelRc << ")." << std::endl;
			return false;
		}

		loadAssemblyAndGetFunctionPointer_ = reinterpret_cast<LoadAssemblyAndGetFunctionPointerFn>(loadPtrRaw);
		if (!ResolveManagedExports()) return false;
		if (!RegisterNativeApi()) return false;

		runtimeInitialized_ = true;
		return true;
	}

	bool CSharpScriptEngine::LoadHostFxr() {
		if (hostfxrLibrary_) return true;

		const std::filesystem::path hostfxrPath = LocateHostFxrPath();
		if (hostfxrPath.empty()) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Could not locate hostfxr.dll." << std::endl;
			return false;
		}

#ifdef _WIN32
		HMODULE module = LoadLibraryW(ToWide(hostfxrPath).c_str());
		if (!module) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Failed to load hostfxr.dll." << std::endl;
			return false;
		}
		hostfxrLibrary_ = module;
		hostfxrInitializeForRuntimeConfig_ = reinterpret_cast<HostFxrInitializeForRuntimeConfigFn>(GetProcAddress(module, "hostfxr_initialize_for_runtime_config"));
		hostfxrGetRuntimeDelegate_ = reinterpret_cast<HostFxrGetRuntimeDelegateFn>(GetProcAddress(module, "hostfxr_get_runtime_delegate"));
		hostfxrClose_ = reinterpret_cast<HostFxrCloseFn>(GetProcAddress(module, "hostfxr_close"));
#else
		return false;
#endif

		if (!hostfxrInitializeForRuntimeConfig_ || !hostfxrGetRuntimeDelegate_ || !hostfxrClose_) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Failed to resolve hostfxr exports." << std::endl;
			ShutdownRuntime();
			return false;
		}
		return true;
	}

	bool CSharpScriptEngine::ResolveManagedExports() {
		const std::filesystem::path hostAssembly = GetHostAssemblyPath();
		std::error_code fsError;
		if (!std::filesystem::exists(hostAssembly, fsError) || fsError) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Host assembly missing: " << hostAssembly.string() << std::endl;
			return false;
		}
		const std::wstring hostAssemblyW = ToWide(hostAssembly);
		void* fn = nullptr;

		int rc = loadAssemblyAndGetFunctionPointer_(hostAssemblyW.c_str(), MANAGED_BRIDGE_TYPE, L"SetNativeApi", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn);
		if (rc != 0 || !fn) return false;
		managedSetNativeApi_ = reinterpret_cast<ManagedSetNativeApiFn>(fn);

		fn = nullptr;
		rc = loadAssemblyAndGetFunctionPointer_(hostAssemblyW.c_str(), MANAGED_BRIDGE_TYPE, L"LoadScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn);
		if (rc != 0 || !fn) return false;
		managedLoadScript_ = reinterpret_cast<ManagedLoadScriptFn>(fn);

		fn = nullptr;
		rc = loadAssemblyAndGetFunctionPointer_(hostAssemblyW.c_str(), MANAGED_BRIDGE_TYPE, L"InvokeScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn);
		if (rc != 0 || !fn) return false;
		managedInvokeScript_ = reinterpret_cast<ManagedInvokeScriptFn>(fn);

		fn = nullptr;
		rc = loadAssemblyAndGetFunctionPointer_(hostAssemblyW.c_str(), MANAGED_BRIDGE_TYPE, L"DestroyScript", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn);
		if (rc != 0 || !fn) return false;
		managedDestroyScript_ = reinterpret_cast<ManagedDestroyScriptFn>(fn);

		fn = nullptr;
		rc = loadAssemblyAndGetFunctionPointer_(hostAssemblyW.c_str(), MANAGED_BRIDGE_TYPE, L"ResetScripts", UNMANAGEDCALLERSONLY_METHOD, nullptr, &fn);
		if (rc != 0 || !fn) return false;
		managedReset_ = reinterpret_cast<ManagedResetFn>(fn);

		return true;
	}

	bool CSharpScriptEngine::RegisterNativeApi() {
		if (!managedSetNativeApi_) return false;
		nativeApi_.logUtf8 = &NativeLogUtf8;
		nativeApi_.getTransformPosition = &NativeGetTransformPosition;
		nativeApi_.setTransformPosition = &NativeSetTransformPosition;

		char errorBuffer[1024]{};
		const int rc = managedSetNativeApi_(&nativeApi_, errorBuffer, static_cast<int>(sizeof(errorBuffer)));
		if (rc != 0) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: SetNativeApi failed: " << errorBuffer << std::endl;
			return false;
		}
		return true;
	}

	void CSharpScriptEngine::ShutdownRuntime() {
		ResetDelegatePointers();
		runtimeInitialized_ = false;
#ifdef _WIN32
		if (hostfxrLibrary_) {
			FreeLibrary(reinterpret_cast<HMODULE>(hostfxrLibrary_));
			hostfxrLibrary_ = nullptr;
		}
#endif
	}

	void CSharpScriptEngine::ResetDelegatePointers() {
		hostfxrInitializeForRuntimeConfig_ = nullptr;
		hostfxrGetRuntimeDelegate_ = nullptr;
		hostfxrClose_ = nullptr;
		loadAssemblyAndGetFunctionPointer_ = nullptr;
		managedSetNativeApi_ = nullptr;
		managedLoadScript_ = nullptr;
		managedInvokeScript_ = nullptr;
		managedDestroyScript_ = nullptr;
		managedReset_ = nullptr;
	}

	bool CSharpScriptEngine::CallManagedLoad(const std::string& id, const std::filesystem::path& assemblyPath, const std::string& typeName, ScriptState& state) {
		if (!managedLoadScript_) return false;
		char errorBuffer[2048]{};
		const int rc = managedLoadScript_(id.c_str(), assemblyPath.string().c_str(), typeName.c_str(), errorBuffer, static_cast<int>(sizeof(errorBuffer)));
		if (rc != 0) {
			if (!state.invokeFailedLogged) {
				Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Load failed: " << errorBuffer << std::endl;
				state.invokeFailedLogged = true;
			}
			return false;
		}
		state.invokeFailedLogged = false;
		return true;
	}

	bool CSharpScriptEngine::CallManagedInvoke(const std::string& id, int methodId, Entity entity, ScriptState& state) {
		if (!managedInvokeScript_) return false;
		char errorBuffer[2048]{};
		const int rc = managedInvokeScript_(id.c_str(), methodId, static_cast<uint64_t>(entity), errorBuffer, static_cast<int>(sizeof(errorBuffer)));
		if (rc != 0) {
			if (!state.invokeFailedLogged) {
				Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Invoke failed: " << errorBuffer << std::endl;
				state.invokeFailedLogged = true;
			}
			return false;
		}
		state.invokeFailedLogged = false;
		return true;
	}

	bool CSharpScriptEngine::CallManagedDestroy(const std::string& id, Entity entity, ScriptState& state) {
		if (!managedDestroyScript_) return false;
		char errorBuffer[2048]{};
		const int rc = managedDestroyScript_(id.c_str(), static_cast<uint64_t>(entity), errorBuffer, static_cast<int>(sizeof(errorBuffer)));
		if (rc != 0) {
			Logger::Get().Error(DebugType::GameManager) << "CSharpScriptEngine: Destroy failed: " << errorBuffer << std::endl;
			return false;
		}
		state.loaded = false;
		state.created = false;
		return true;
	}

	bool CSharpScriptEngine::RunCommand(const std::string& command) const {
		const int exitCode = std::system(command.c_str());
		return exitCode == 0;
	}

	std::wstring CSharpScriptEngine::ToWide(const std::filesystem::path& path) {
		return path.wstring();
	}

	std::string CSharpScriptEngine::Quote(const std::filesystem::path& path) {
		return std::string("\"") + path.string() + "\"";
	}
}
