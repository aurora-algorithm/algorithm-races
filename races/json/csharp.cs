// Race: json (csharp)
// Extracted from suite/csharp/Misc.cs — build and run via suite/run.py

using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
namespace Kernels;

using System.Text.Json;

public sealed record Order(long Id, long CustomerId, string Status, long TotalCents, long Items, List<string> Tags);

public sealed record OrderDocument(List<Order> Orders);

public sealed record JsonRoundTrip(int Orders, long TotalCents, long Items, byte[] Output);

public static class Misc
{

    /// <summary>Parses the file's UTF-8 bytes, as Rust and Go do (a .NET string would be UTF-16).</summary>
    public static JsonRoundTrip Json(byte[] utf8)
    {
        OrderDocument document = JsonSerializer.Deserialize(utf8, KernelsJson.Default.OrderDocument)
            ?? throw new InvalidDataException("orders file holds no document");
        long totalCents = 0, items = 0;
        foreach (Order order in document.Orders)
        {
            totalCents += order.TotalCents;
            items += order.Items;
        }
        byte[] output = JsonSerializer.SerializeToUtf8Bytes(document, KernelsJson.Default.OrderDocument);
        return new JsonRoundTrip(document.Orders.Count, totalCents, items, output);
    }

}
