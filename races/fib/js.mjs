// Race: fib (js)
// Extracted from suite/js/algo.mjs — build and run via suite/run.py

const fib = (n) => (n < 2 ? n : fib(n - 1) + fib(n - 2));
