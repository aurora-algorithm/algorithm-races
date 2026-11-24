// Race: sort-int (goose)
// Extracted from suite/goose/main.goose — build and run via suite/run.py

import sort;

fn sort_int(a: i64[>..]&)
{
    sort.sort(a);
}
