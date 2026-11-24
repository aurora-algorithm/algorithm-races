using System.Text.Json.Serialization;

namespace Kernels;

/// <summary>Source-generated serializers, so the JIT and NativeAOT builds run the same JSON code.</summary>
[JsonSourceGenerationOptions(PropertyNamingPolicy = JsonKnownNamingPolicy.SnakeCaseLower)]
[JsonSerializable(typeof(OrderDocument))]
[JsonSerializable(typeof(KernelResult))]
internal sealed partial class KernelsJson : JsonSerializerContext;
