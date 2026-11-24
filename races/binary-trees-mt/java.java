// Race: binary-trees-mt (java)
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

  static String binaryTrees(int n, int threads, ExecutorService pool) {
    return BenchmarksGame.binaryTreesLines(
        n,
        (depth, iterations) -> {
          int share = iterations / threads;
          var workers = new ArrayList<Callable<Integer>>();
          for (int t = 0; t < threads; t++) {
            workers.add(() -> BenchmarksGame.depthChecks(depth, share));
          }
          return runAll(pool, workers).stream().mapToInt(Integer::intValue).sum();
        });
  }

}
