// Race: json (goose)
// Extracted from suite/goose/json_kernel.goose — build and run via suite/run.py

// The `json` kernel (kernels/SPEC.md section 5): parse the orders document into typed records with the standard
// library's json reader (serde style: each field read as the type the record expects), sum two fields, and write
// the whole document back as compact JSON. Strings stay slices of the input text (the reader copies only strings
// that contain escapes, and these have none), so the records hold no text of their own.

import std;
import json;

struct Order {
    id:          i64,
    customer_id: i64,
    status:      const u8[:],
    total_cents: i64,
    items:       i64,
    tags:        const u8[:][..8],
}

struct Totals { orders: i64 = 0, total_cents: i64 = 0, items: i64 = 0 }

fn read_orders(r: json::Reader&, orders: Order[>..]&, scratch: u8[>..]&) -> bool {
    json::each_member(r) { key =>
        guard key == "orders" else { json::skip(r); }
        json::each_element(r) {
            let o .= orders.push(Order { status: "", .. });
            json::each_member(r) { field =>
                if field == "id" {
                    o.id = json::read_int(r);
                } else if field == "customer_id" {
                    o.customer_id = json::read_int(r);
                } else if field == "status" {
                    o.status = json::read_string(r, scratch);
                } else if field == "total_cents" {
                    o.total_cents = json::read_int(r);
                } else if field == "items" {
                    o.items = json::read_int(r);
                } else if field == "tags" {
                    json::each_element(r) { o.tags.push(json::read_string(r, scratch)); };
                } else {
                    json::skip(r);
                }
            };
        };
    };
    return json::finish(r);
}

fn write_orders(orders: Order[:], out: u8[>..]&) {
    out.append("{\"orders\":[");
    for o, i in orders {
        if i > 0 { out.push(','); }
        out.push('{');
        json::format_key(out, "id");
        json::format_int(out, o.id);
        out.push(',');
        json::format_key(out, "customer_id");
        json::format_int(out, o.customer_id);
        out.push(',');
        json::format_key(out, "status");
        json::format_string(out, o.status);
        out.push(',');
        json::format_key(out, "total_cents");
        json::format_int(out, o.total_cents);
        out.push(',');
        json::format_key(out, "items");
        json::format_int(out, o.items);
        out.push(',');
        json::format_key(out, "tags");
        out.push('[');
        for t, j in o.tags {
            if j > 0 { out.push(','); }
            json::format_string(out, t);
        }
        out.append("]}");
    }
    out.append("]}");
}

fn json_roundtrip(text: const u8[:], out: u8[>..]&) -> Totals {
    var r = json::reader(text);
    var scratch: u8[>..]    = [];
    var orders:  Order[>..] = [];
    if !read_orders(r, orders, scratch) { abort("json: the orders file does not parse"); }
    var t = Totals { orders: orders.len };
    for o in orders {
        t.total_cents += o.total_cents;
        t.items += o.items;
    }
    write_orders(orders, out);
    return t;
}
