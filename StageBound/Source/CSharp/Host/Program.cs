using System.Reflection;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Text;
using System.Text.Json;

namespace Owala.ScriptHost;

public static class Bridge
{
    private sealed class FieldTransfer
    {
        public string name { get; set; } = string.Empty;
        public string type { get; set; } = string.Empty;
        public string value { get; set; } = string.Empty;
    }

    private enum ScriptMethodId
    {
        OnCreate = 0,
        OnUpdate = 1,
        OnDestroy = 2
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct NativeApi
    {
        public IntPtr logUtf8;
        public IntPtr getTransformPosition;
        public IntPtr setTransformPosition;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void NativeLogUtf8Delegate(IntPtr messageUtf8);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int NativeGetTransformPositionDelegate(ulong entity, out float x, out float y);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int NativeSetTransformPositionDelegate(ulong entity, float x, float y);

    private sealed class ScriptInstance
    {
        public AssemblyLoadContext Context { get; init; } = default!;
        public Type Type { get; init; } = default!;
        public object? Instance { get; init; }
        public MethodInfo? OnCreate { get; init; }
        public MethodInfo? OnUpdate { get; init; }
        public MethodInfo? OnDestroy { get; init; }
        public Dictionary<string, object?>? SavedState { get; set; }
    }

    private static readonly object Sync = new();
    private static readonly Dictionary<string, ScriptInstance> Instances = new();

    private static NativeLogUtf8Delegate? s_log;
    private static NativeGetTransformPositionDelegate? s_getTransformPos;
    private static NativeSetTransformPositionDelegate? s_setTransformPos;

    [UnmanagedCallersOnly(EntryPoint = "SetNativeApi")]
    public static int SetNativeApi(nint nativeApiPtr, nint errorBufferPtr, int errorBufferCapacity)
    {
        try
        {
            if (nativeApiPtr == 0)
            {
                return Fail("Native API pointer is null.", errorBufferPtr, errorBufferCapacity);
            }

            NativeApi api = Marshal.PtrToStructure<NativeApi>(nativeApiPtr);
            if (api.logUtf8 != IntPtr.Zero) s_log = Marshal.GetDelegateForFunctionPointer<NativeLogUtf8Delegate>(api.logUtf8);
            if (api.getTransformPosition != IntPtr.Zero) s_getTransformPos = Marshal.GetDelegateForFunctionPointer<NativeGetTransformPositionDelegate>(api.getTransformPosition);
            if (api.setTransformPosition != IntPtr.Zero) s_setTransformPos = Marshal.GetDelegateForFunctionPointer<NativeSetTransformPositionDelegate>(api.setTransformPosition);
            return 0;
        }
        catch (Exception ex)
        {
            return Fail(ex.ToString(), errorBufferPtr, errorBufferCapacity);
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "LoadScript")]
    public static int LoadScript(nint idPtr, nint assemblyPathPtr, nint typeNamePtr, nint errorBufferPtr, int errorBufferCapacity)
    {
        try
        {
            string id = PtrToUtf8(idPtr);
            string assemblyPath = PtrToUtf8(assemblyPathPtr);
            string typeName = PtrToUtf8(typeNamePtr);

            if (string.IsNullOrWhiteSpace(id) || string.IsNullOrWhiteSpace(assemblyPath) || string.IsNullOrWhiteSpace(typeName))
            {
                return Fail("LoadScript requires id, assembly path, and type name.", errorBufferPtr, errorBufferCapacity);
            }
            if (!File.Exists(assemblyPath))
            {
                return Fail($"Assembly not found: {assemblyPath}", errorBufferPtr, errorBufferCapacity);
            }

            lock (Sync)
            {
                Dictionary<string, object?>? savedState = null;
                if (Instances.TryGetValue(id, out ScriptInstance? oldInstance))
                {
                    savedState = CaptureState(oldInstance.Instance);
                    DestroyInternal(id, oldInstance, 0, invokeDestroy: false);
                }

                var context = new AssemblyLoadContext($"OwalaScript_{id}", isCollectible: true);
                context.Resolving += (loadContext, requestedAssemblyName) =>
                {
                    string? requestedName = requestedAssemblyName.Name;
                    if (string.Equals(requestedName, typeof(Bridge).Assembly.GetName().Name, StringComparison.OrdinalIgnoreCase))
                    {
                        return typeof(Bridge).Assembly;
                    }

                    string? scriptDir = Path.GetDirectoryName(Path.GetFullPath(assemblyPath));
                    if (!string.IsNullOrWhiteSpace(scriptDir) && !string.IsNullOrWhiteSpace(requestedName))
                    {
                        string candidatePath = Path.Combine(scriptDir, requestedName + ".dll");
                        if (File.Exists(candidatePath))
                        {
                            return loadContext.LoadFromAssemblyPath(candidatePath);
                        }
                    }

                    return null;
                };
                Assembly assembly;
                using (FileStream stream = File.OpenRead(Path.GetFullPath(assemblyPath)))
                {
                    assembly = context.LoadFromStream(stream);
                }

                Type? type = ResolveScriptType(assembly, typeName);
                if (type is null)
                {
                    context.Unload();
                    return Fail($"Type not found: {typeName}", errorBufferPtr, errorBufferCapacity);
                }

                object? instance = null;
                if (!type.IsAbstract && !type.IsSealed)
                {
                    instance = Activator.CreateInstance(type);
                    if (instance is null)
                    {
                        context.Unload();
                        return Fail($"Could not create instance of type: {typeName}", errorBufferPtr, errorBufferCapacity);
                    }
                    RestoreState(instance, savedState);
                }

                MethodInfo? onCreate = type.GetMethod("OnCreate", BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
                MethodInfo? onUpdate = type.GetMethod("OnUpdate", BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
                MethodInfo? onDestroy = type.GetMethod("OnDestroy", BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);

                Instances[id] = new ScriptInstance
                {
                    Context = context,
                    Type = type,
                    Instance = instance,
                    OnCreate = onCreate,
                    OnUpdate = onUpdate,
                    OnDestroy = onDestroy
                };
            }

            return 0;
        }
        catch (Exception ex)
        {
            return Fail(ex.ToString(), errorBufferPtr, errorBufferCapacity);
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "InvokeScript")]
    public static int InvokeScript(nint idPtr, int methodId, ulong entity, nint errorBufferPtr, int errorBufferCapacity)
    {
        try
        {
            string id = PtrToUtf8(idPtr);
            if (string.IsNullOrWhiteSpace(id))
            {
                return Fail("InvokeScript requires id.", errorBufferPtr, errorBufferCapacity);
            }

            lock (Sync)
            {
                if (!Instances.TryGetValue(id, out ScriptInstance? script))
                {
                    return Fail($"Unknown script id: {id}", errorBufferPtr, errorBufferCapacity);
                }

                MethodInfo? method = methodId switch
                {
                    (int)ScriptMethodId.OnCreate => script.OnCreate,
                    (int)ScriptMethodId.OnUpdate => script.OnUpdate,
                    (int)ScriptMethodId.OnDestroy => script.OnDestroy,
                    _ => null
                };

                if (script.Instance is OwalaBehaviour behaviour)
                {
                    behaviour.__SetEntity(entity);
                }

                InvokeMethod(method, script.Instance, script.Type, entity, allowMissing: methodId == (int)ScriptMethodId.OnDestroy);
            }
            return 0;
        }
        catch (Exception ex)
        {
            return Fail(ex.ToString(), errorBufferPtr, errorBufferCapacity);
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "DestroyScript")]
    public static int DestroyScript(nint idPtr, ulong entity, nint errorBufferPtr, int errorBufferCapacity)
    {
        try
        {
            string id = PtrToUtf8(idPtr);
            if (string.IsNullOrWhiteSpace(id))
            {
                return Fail("DestroyScript requires id.", errorBufferPtr, errorBufferCapacity);
            }

            lock (Sync)
            {
                if (Instances.TryGetValue(id, out ScriptInstance? script))
                {
                    DestroyInternal(id, script, entity, invokeDestroy: true);
                }
            }
            return 0;
        }
        catch (Exception ex)
        {
            return Fail(ex.ToString(), errorBufferPtr, errorBufferCapacity);
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "ResetScripts")]
    public static int ResetScripts(nint errorBufferPtr, int errorBufferCapacity)
    {
        try
        {
            lock (Sync)
            {
                foreach (string id in Instances.Keys.ToList())
                {
                    DestroyInternal(id, Instances[id], 0, invokeDestroy: true);
                }
                Instances.Clear();
            }
            return 0;
        }
        catch (Exception ex)
        {
            return Fail(ex.ToString(), errorBufferPtr, errorBufferCapacity);
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "DescribeScriptFields")]
    public static int DescribeScriptFields(nint assemblyPathPtr, nint typeNamePtr, nint outJsonBufferPtr, int outJsonBufferCapacity, nint errorBufferPtr, int errorBufferCapacity)
    {
        try
        {
            string assemblyPath = PtrToUtf8(assemblyPathPtr);
            string typeName = PtrToUtf8(typeNamePtr);
            if (string.IsNullOrWhiteSpace(assemblyPath) || string.IsNullOrWhiteSpace(typeName))
            {
                return Fail("DescribeScriptFields requires assembly path and type name.", errorBufferPtr, errorBufferCapacity);
            }
            if (!File.Exists(assemblyPath))
            {
                return Fail($"Assembly not found: {assemblyPath}", errorBufferPtr, errorBufferCapacity);
            }

            var context = new AssemblyLoadContext($"OwalaFieldDescribe_{Guid.NewGuid()}", isCollectible: true);
            try
            {
                context.Resolving += (loadContext, requestedAssemblyName) =>
                {
                    string? requestedName = requestedAssemblyName.Name;
                    if (string.Equals(requestedName, typeof(Bridge).Assembly.GetName().Name, StringComparison.OrdinalIgnoreCase))
                    {
                        return typeof(Bridge).Assembly;
                    }

                    string? scriptDir = Path.GetDirectoryName(Path.GetFullPath(assemblyPath));
                    if (!string.IsNullOrWhiteSpace(scriptDir) && !string.IsNullOrWhiteSpace(requestedName))
                    {
                        string candidatePath = Path.Combine(scriptDir, requestedName + ".dll");
                        if (File.Exists(candidatePath))
                        {
                            return loadContext.LoadFromAssemblyPath(candidatePath);
                        }
                    }
                    return null;
                };

                Assembly assembly;
                using (FileStream stream = File.OpenRead(Path.GetFullPath(assemblyPath)))
                {
                    assembly = context.LoadFromStream(stream);
                }

                Type? type = ResolveScriptType(assembly, typeName);
                if (type is null)
                {
                    return Fail($"Type not found: {typeName}", errorBufferPtr, errorBufferCapacity);
                }

                object? instance = null;
                if (!type.IsAbstract && !type.IsSealed)
                {
                    instance = Activator.CreateInstance(type);
                }

                List<FieldTransfer> fields = EnumerateSerializableFields(type, instance);
                string json = JsonSerializer.Serialize(fields);
                WriteUtf8(json, outJsonBufferPtr, outJsonBufferCapacity);
            }
            finally
            {
                context.Unload();
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }

            return 0;
        }
        catch (Exception ex)
        {
            return Fail(ex.ToString(), errorBufferPtr, errorBufferCapacity);
        }
    }

    [UnmanagedCallersOnly(EntryPoint = "ApplyScriptFields")]
    public static int ApplyScriptFields(nint idPtr, nint fieldsJsonPtr, nint errorBufferPtr, int errorBufferCapacity)
    {
        try
        {
            string id = PtrToUtf8(idPtr);
            string fieldsJson = PtrToUtf8(fieldsJsonPtr);
            if (string.IsNullOrWhiteSpace(id))
            {
                return Fail("ApplyScriptFields requires id.", errorBufferPtr, errorBufferCapacity);
            }

            lock (Sync)
            {
                if (!Instances.TryGetValue(id, out ScriptInstance? script))
                {
                    return Fail($"Unknown script id: {id}", errorBufferPtr, errorBufferCapacity);
                }
                if (script.Instance is null)
                {
                    return 0;
                }

                List<FieldTransfer>? fields = JsonSerializer.Deserialize<List<FieldTransfer>>(fieldsJson);
                if (fields is null) return 0;
                foreach (FieldTransfer field in fields)
                {
                    ApplyFieldValue(script.Instance, field);
                }
            }
            return 0;
        }
        catch (Exception ex)
        {
            return Fail(ex.ToString(), errorBufferPtr, errorBufferCapacity);
        }
    }

    private static void DestroyInternal(string id, ScriptInstance script, ulong entity, bool invokeDestroy)
    {
        if (invokeDestroy)
        {
            if (script.Instance is OwalaBehaviour behaviour)
            {
                behaviour.__SetEntity(entity);
            }
            InvokeMethod(script.OnDestroy, script.Instance, script.Type, entity, allowMissing: true);
        }

        Instances.Remove(id);
        script.Context.Unload();
        GC.Collect();
        GC.WaitForPendingFinalizers();
    }

    private static void InvokeMethod(MethodInfo? method, object? instance, Type type, ulong entity, bool allowMissing)
    {
        if (method is null)
        {
            if (allowMissing) return;
            throw new MissingMethodException($"{type.FullName} method not found.");
        }

        ParameterInfo[] parameters = method.GetParameters();
        if (parameters.Length == 0)
        {
            method.Invoke(instance, null);
            return;
        }
        if (parameters.Length == 1 && parameters[0].ParameterType == typeof(ulong))
        {
            method.Invoke(instance, new object[] { entity });
            return;
        }
        throw new InvalidOperationException($"Unsupported signature for {type.FullName}.{method.Name}");
    }

    private static Dictionary<string, object?> CaptureState(object? instance)
    {
        if (instance is null) return new Dictionary<string, object?>();

        var state = new Dictionary<string, object?>();
        foreach (FieldInfo field in instance.GetType().GetFields(BindingFlags.Instance | BindingFlags.Public))
        {
            if (Attribute.IsDefined(field, typeof(NonSerializedAttribute))) continue;
            object? value = field.GetValue(instance);
            if (IsSerializableSimpleType(field.FieldType))
            {
                state[field.Name] = value;
            }
        }
        return state;
    }

    private static void RestoreState(object? instance, Dictionary<string, object?>? state)
    {
        if (instance is null || state is null) return;
        Type type = instance.GetType();
        foreach (var pair in state)
        {
            FieldInfo? field = type.GetField(pair.Key, BindingFlags.Instance | BindingFlags.Public);
            if (field is null || Attribute.IsDefined(field, typeof(NonSerializedAttribute))) continue;
            if (!IsSerializableSimpleType(field.FieldType)) continue;
            if (pair.Value is null)
            {
                field.SetValue(instance, null);
                continue;
            }

            object converted = field.FieldType.IsEnum
                ? Enum.ToObject(field.FieldType, pair.Value)
                : Convert.ChangeType(pair.Value, field.FieldType);
            field.SetValue(instance, converted);
        }
    }

    private static bool IsSerializableSimpleType(Type type)
    {
        type = Nullable.GetUnderlyingType(type) ?? type;
        return type.IsPrimitive || type == typeof(string) || type == typeof(decimal) || type.IsEnum;
    }

    private static List<FieldTransfer> EnumerateSerializableFields(Type type, object? instance)
    {
        var fields = new List<FieldTransfer>();
        foreach (FieldInfo field in type.GetFields(BindingFlags.Instance | BindingFlags.Public))
        {
            if (Attribute.IsDefined(field, typeof(NonSerializedAttribute))) continue;
            if (!IsSerializableSimpleType(field.FieldType)) continue;

            object? rawValue = instance is null ? null : field.GetValue(instance);
            fields.Add(new FieldTransfer
            {
                name = field.Name,
                type = field.FieldType.FullName ?? field.FieldType.Name,
                value = SerializeFieldValue(rawValue, field.FieldType)
            });
        }
        return fields;
    }

    private static string SerializeFieldValue(object? value, Type fieldType)
    {
        if (value is null) return string.Empty;
        Type normalized = Nullable.GetUnderlyingType(fieldType) ?? fieldType;

        if (normalized == typeof(bool))
        {
            return ((bool)value) ? "true" : "false";
        }
        if (normalized.IsEnum)
        {
            return Convert.ToInt64(value, CultureInfo.InvariantCulture).ToString(CultureInfo.InvariantCulture);
        }

        return Convert.ToString(value, CultureInfo.InvariantCulture) ?? string.Empty;
    }

    private static void ApplyFieldValue(object instance, FieldTransfer transfer)
    {
        if (string.IsNullOrWhiteSpace(transfer.name))
        {
            return;
        }

        Type type = instance.GetType();
        FieldInfo? field = type.GetField(transfer.name, BindingFlags.Instance | BindingFlags.Public);
        if (field is null || Attribute.IsDefined(field, typeof(NonSerializedAttribute))) return;
        if (!IsSerializableSimpleType(field.FieldType)) return;

        object? converted = ParseFieldValue(transfer.value, field.FieldType);
        if (converted is null && Nullable.GetUnderlyingType(field.FieldType) is null && field.FieldType != typeof(string))
        {
            return;
        }
        field.SetValue(instance, converted);
    }

    private static object? ParseFieldValue(string value, Type fieldType)
    {
        Type targetType = Nullable.GetUnderlyingType(fieldType) ?? fieldType;
        if (targetType == typeof(string))
        {
            return value;
        }
        if (string.IsNullOrWhiteSpace(value))
        {
            return Nullable.GetUnderlyingType(fieldType) is null ? Activator.CreateInstance(targetType) : null;
        }
        if (targetType == typeof(bool))
        {
            if (bool.TryParse(value, out bool parsedBool))
            {
                return parsedBool;
            }
            return false;
        }
        if (targetType.IsEnum)
        {
            if (long.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out long parsedEnum))
            {
                return Enum.ToObject(targetType, parsedEnum);
            }
            return Activator.CreateInstance(targetType);
        }
        if (targetType == typeof(float))
        {
            if (float.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out float parsedFloat))
            {
                return parsedFloat;
            }
            return 0f;
        }
        if (targetType == typeof(double))
        {
            if (double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out double parsedDouble))
            {
                return parsedDouble;
            }
            return 0d;
        }
        if (targetType == typeof(decimal))
        {
            if (decimal.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out decimal parsedDecimal))
            {
                return parsedDecimal;
            }
            return 0m;
        }
        if (targetType == typeof(long))
        {
            if (long.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out long parsedLong))
            {
                return parsedLong;
            }
            return 0L;
        }
        if (targetType == typeof(int))
        {
            if (int.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsedInt))
            {
                return parsedInt;
            }
            return 0;
        }
        if (targetType == typeof(short))
        {
            if (short.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out short parsedShort))
            {
                return parsedShort;
            }
            return (short)0;
        }
        if (targetType == typeof(byte))
        {
            if (byte.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out byte parsedByte))
            {
                return parsedByte;
            }
            return (byte)0;
        }
        if (targetType == typeof(ulong))
        {
            if (ulong.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out ulong parsedULong))
            {
                return parsedULong;
            }
            return 0UL;
        }
        if (targetType == typeof(uint))
        {
            if (uint.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out uint parsedUInt))
            {
                return parsedUInt;
            }
            return 0U;
        }
        if (targetType == typeof(ushort))
        {
            if (ushort.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out ushort parsedUShort))
            {
                return parsedUShort;
            }
            return (ushort)0;
        }
        if (targetType == typeof(sbyte))
        {
            if (sbyte.TryParse(value, NumberStyles.Integer, CultureInfo.InvariantCulture, out sbyte parsedSByte))
            {
                return parsedSByte;
            }
            return (sbyte)0;
        }
        if (targetType == typeof(char))
        {
            return value[0];
        }

        return Convert.ChangeType(value, targetType, CultureInfo.InvariantCulture);
    }

    private static Type? ResolveScriptType(Assembly assembly, string typeName)
    {
        Type? exact = assembly.GetType(typeName, throwOnError: false, ignoreCase: false);
        if (exact is not null) return exact;

        Type[] allTypes;
        try
        {
            allTypes = assembly.GetTypes();
        }
        catch (ReflectionTypeLoadException ex)
        {
            allTypes = ex.Types.Where(t => t is not null).Cast<Type>().ToArray();
        }

        Type? fullNameMatch = allTypes.FirstOrDefault(t => string.Equals(t.FullName, typeName, StringComparison.Ordinal));
        if (fullNameMatch is not null) return fullNameMatch;

        Type? simpleNameMatch = allTypes.FirstOrDefault(t => string.Equals(t.Name, typeName, StringComparison.Ordinal));
        return simpleNameMatch;
    }

    private static string PtrToUtf8(nint ptr)
    {
        if (ptr == 0) return string.Empty;
        return Marshal.PtrToStringUTF8(ptr) ?? string.Empty;
    }

    private static int Fail(string message, nint errorBufferPtr, int errorBufferCapacity)
    {
        WriteError(message, errorBufferPtr, errorBufferCapacity);
        return -1;
    }

    private static void WriteError(string message, nint errorBufferPtr, int errorBufferCapacity)
    {
        WriteUtf8(message, errorBufferPtr, errorBufferCapacity);
    }

    private static void WriteUtf8(string message, nint bufferPtr, int capacity)
    {
        if (bufferPtr == 0 || capacity <= 0) return;
        byte[] bytes = Encoding.UTF8.GetBytes(message ?? "Unknown error");
        int writeCount = Math.Min(bytes.Length, capacity - 1);
        Marshal.Copy(bytes, 0, bufferPtr, writeCount);
        Marshal.WriteByte(bufferPtr, writeCount, 0);
    }

    internal static void NativeLog(string text)
    {
        if (s_log is null) return;
        nint utf8 = Marshal.StringToCoTaskMemUTF8(text ?? string.Empty);
        try { s_log(utf8); }
        finally { Marshal.FreeCoTaskMem(utf8); }
    }

    internal static bool NativeGetPosition(ulong entity, out float x, out float y)
    {
        x = y = 0f;
        if (s_getTransformPos is null) return false;
        return s_getTransformPos(entity, out x, out y) != 0;
    }

    internal static bool NativeSetPosition(ulong entity, float x, float y)
    {
        if (s_setTransformPos is null) return false;
        return s_setTransformPos(entity, x, y) != 0;
    }
}

public abstract class OwalaBehaviour
{
    public ulong Entity { get; private set; }

    internal void __SetEntity(ulong entity) => Entity = entity;

    protected void Log(string message) => Bridge.NativeLog(message);

    protected bool GetPosition(out float x, out float y) => Bridge.NativeGetPosition(Entity, out x, out y);

    protected bool SetPosition(float x, float y) => Bridge.NativeSetPosition(Entity, x, y);
}
