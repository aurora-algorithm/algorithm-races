// Race: sort-int (rust)
// Extracted from suite/rust/src/sorts.rs — build and run via suite/run.py

pub fn std_sort(mut a: Vec<i64>) -> Vec<i64> {
    a.sort();
    a
}
