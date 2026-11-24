// Race: sort-str (rust)
// Extracted from suite/rust/src/sorts.rs — build and run via suite/run.py

pub fn std_sort_strings(mut a: Vec<String>) -> Vec<String> {
    a.sort();
    a
}
