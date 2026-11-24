// Race: mergesort-mt (java)
// Extracted from suite/java/src/main/java/kernels/ParallelKernels.java — build and run via suite/run.py

package kernels;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;

import java.util.ArrayList;

import java.util.List;

import java.util.concurrent.Callable;

import java.util.concurrent.ExecutionException;

import java.util.concurrent.ExecutorService;

import java.util.concurrent.Future;

final class ParallelKernels {

  /** Runs every task on the pool and waits for all of them. */
  static <T> List<T> runAll(ExecutorService pool, List<Callable<T>> tasks) {
    try {
      var results = new ArrayList<T>(tasks.size());
      for (Future<T> future : pool.invokeAll(tasks)) {
        results.add(future.get());
      }
      return results;
    } catch (InterruptedException e) {
      Thread.currentThread().interrupt();
      throw new IllegalStateException("interrupted while waiting for workers", e);
    } catch (ExecutionException e) {
      throw new IllegalStateException("a worker failed", e.getCause());
    }
  }

  static long[] mergesort(long[] a, int threads, ExecutorService pool) {
    var aux = new long[a.length];
    var bounds = new int[threads + 1];
    for (int t = 0; t <= threads; t++) {
      bounds[t] = (int) ((long) t * a.length / threads);
    }
    var sorts = new ArrayList<Callable<Void>>();
    for (int t = 0; t < threads; t++) {
      int lo = bounds[t];
      int hi = bounds[t + 1];
      sorts.add(() -> {
        Sorts.mergesort(a, aux, lo, hi);
        return null;
      });
    }
    runAll(pool, sorts);
    while (bounds.length > 2) {
      var merges = new ArrayList<Callable<Void>>();
      for (int p = 0; p + 2 < bounds.length; p += 2) {
        int lo = bounds[p];
        int mid = bounds[p + 1];
        int hi = bounds[p + 2];
        merges.add(() -> {
          Sorts.merge(a, aux, lo, mid, hi);
          return null;
        });
      }
      runAll(pool, merges);
      bounds = everyOther(bounds);
    }
    return a;
  }

  /** The runs left after a round of pairwise merges: bounds 0, 2, 4, ... and the last one. */
  private static int[] everyOther(int[] bounds) {
    var out = new int[bounds.length / 2 + 1];
    for (int i = 0; i < out.length; i++) {
      out[i] = bounds[Math.min(2 * i, bounds.length - 1)];
    }
    return out;
  }

}
