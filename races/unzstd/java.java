// Race: unzstd (java)
// Extracted from suite/java/src/main/java/kernels/Work.java — build and run via suite/run.py

package kernels;
import com.github.luben.zstd.Zstd;
import de.siegmar.fastcsv.reader.CsvReader;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HexFormat;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.function.Function;
import java.util.function.UnaryOperator;
import java.util.stream.Collectors;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;
import javax.imageio.IIOImage;
import javax.imageio.ImageIO;
import javax.imageio.ImageReader;
import javax.imageio.ImageWriteParam;
import javax.imageio.ImageWriter;
import javax.imageio.stream.MemoryCacheImageOutputStream;
import net.coobird.thumbnailator.Thumbnails;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVRecord;
import org.springframework.security.crypto.argon2.Argon2PasswordEncoder;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;

import com.github.luben.zstd.Zstd;

import java.io.IOException;

import java.nio.file.Files;

import java.nio.file.Path;

import java.security.MessageDigest;

import java.security.NoSuchAlgorithmException;

import java.util.HexFormat;

import java.util.Map;

import java.util.function.Function;

import java.util.function.UnaryOperator;

final class Work {

  private Work() {}

  private static <I, R> Main.Kernel<I, R> kernel(
      Main.Prepare<I> prepare,
      Function<I, R> run,
      Function<R, String> checksum,
      Function<R, Map<String, Long>> extra) {
    return new Main.Kernel<>(
        prepare, UnaryOperator.identity(), (in, ctx) -> run.apply(in), checksum, extra);
  }

  private static <I, R> Main.Kernel<I, R> kernel(
      Main.Prepare<I> prepare, Function<I, R> run, Function<R, String> checksum) {
    return kernel(prepare, run, checksum, r -> Map.of());
  }

  private static byte[] corpus(int n, Path data) throws IOException {
    return Files.readAllBytes(data.resolve("corpus-" + n + ".bin"));
  }

  static String plainChecksum(byte[] raw) {
    try {
      var digest = MessageDigest.getInstance("SHA-256").digest(raw);
      return raw.length + ":" + HexFormat.of().formatHex(digest);
    } catch (NoSuchAlgorithmException e) {
      throw new IllegalStateException(e);
    }
  }

  private static Main.Kernel<byte[], byte[]> decompressKernel(
      UnaryOperator<byte[]> compress, UnaryOperator<byte[]> decompress) {
    return kernel((n, data) -> compress.apply(corpus(n, data)), decompress, Work::plainChecksum);
  }

  private static byte[] zstd(byte[] raw) {
    return Zstd.compress(raw, 3);
  }

  private static byte[] unzstd(byte[] packed) {
    return Zstd.decompress(packed, Math.toIntExact(Zstd.getFrameContentSize(packed)));
  }

}
