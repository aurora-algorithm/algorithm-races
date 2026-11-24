// Race: lru (rust)
// Extracted from suite/rust/src/algo.rs — build and run via suite/run.py

use std::cmp::{Ordering, Reverse};
use std::collections::{BTreeMap, BinaryHeap, HashMap, VecDeque};
use crate::inputs::{XorShift32, h31, hashmap_input, ints};
use crate::work::Outcome;
use crate::{Args, measure};

fn lru_keys(n: usize) -> (usize, Vec<u32>) {
    let capacity = (n / 100).max(1);
    let mut rng = XorShift32::new();
    let keys = (0..n)
        .map(|_| rng.next_u32() % (2 * capacity as u32))
        .collect();
    (capacity, keys)
}

const NIL: usize = usize::MAX;

/// One cache entry in a doubly linked list kept in a Vec (indices instead of pointers).
struct Entry {
    key: u32,
    prev: usize,
    next: usize,
}

/// std has no linked hash map: a HashMap from key to slot plus the list of slots, most recent at `head`.
struct Lru {
    slots: HashMap<u32, usize>,
    entries: Vec<Entry>,
    head: usize,
    tail: usize,
}

impl Lru {
    fn unlink(&mut self, i: usize) {
        let (prev, next) = (self.entries[i].prev, self.entries[i].next);
        match prev {
            NIL => self.head = next,
            p => self.entries[p].next = next,
        }
        match next {
            NIL => self.tail = prev,
            n => self.entries[n].prev = prev,
        }
    }

    fn push_front(&mut self, i: usize) {
        self.entries[i].prev = NIL;
        self.entries[i].next = self.head;
        match self.head {
            NIL => self.tail = i,
            h => self.entries[h].prev = i,
        }
        self.head = i;
    }

    fn keys_oldest_first(&self) -> Vec<i64> {
        let mut keys = Vec::with_capacity(self.entries.len());
        let mut i = self.tail;
        while i != NIL {
            keys.push(i64::from(self.entries[i].key));
            i = self.entries[i].prev;
        }
        keys
    }
}

fn lru(capacity: usize, keys: &[u32]) -> String {
    let mut cache = Lru {
        slots: HashMap::with_capacity(capacity),
        entries: Vec::with_capacity(capacity),
        head: NIL,
        tail: NIL,
    };
    let mut hits = 0;
    for &k in keys {
        if let Some(&i) = cache.slots.get(&k) {
            cache.unlink(i);
            cache.push_front(i);
            hits += 1;
            continue;
        }
        let i = if cache.entries.len() == capacity {
            let oldest = cache.tail;
            cache.unlink(oldest);
            cache.slots.remove(&cache.entries[oldest].key);
            cache.entries[oldest].key = k;
            oldest
        } else {
            cache.entries.push(Entry {
                key: k,
                prev: NIL,
                next: NIL,
            });
            cache.entries.len() - 1
        };
        cache.slots.insert(k, i);
        cache.push_front(i);
    }
    let misses = keys.len() - hits;
    format!("{hits}:{misses}:{}", h31(cache.keys_oldest_first()))
}
