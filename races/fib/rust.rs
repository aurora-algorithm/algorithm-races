// Race: fib (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

fn fib(n: u32) -> u64 {
    if n < 2 {
        u64::from(n)
    } else {
        fib(n - 1) + fib(n - 2)
    }
}
