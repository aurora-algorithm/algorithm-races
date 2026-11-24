// Race: image (java)
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

import java.awt.RenderingHints;

import java.awt.image.BufferedImage;

import java.io.ByteArrayInputStream;

import java.io.ByteArrayOutputStream;

import java.io.IOException;

import java.io.UncheckedIOException;

import java.nio.file.Files;

import java.util.ArrayList;

import java.util.List;

import java.util.Map;

import java.util.function.Function;

import java.util.function.UnaryOperator;

import javax.imageio.IIOImage;

import javax.imageio.ImageIO;

import javax.imageio.ImageReader;

import javax.imageio.ImageWriteParam;

import javax.imageio.ImageWriter;

import javax.imageio.stream.MemoryCacheImageOutputStream;

final class Work {

  private static final int PHOTOS = 8;

  private static final int TARGET_WIDTH = 1080;

  private static final float JPEG_QUALITY = 0.8f;

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

  private static int targetHeight(int width, int height) {
    return (height * TARGET_WIDTH + width / 2) / width;
  }

  @FunctionalInterface
  interface Converter {
    byte[] convert(byte[] jpeg) throws IOException;
  }

  private static Main.Kernel<List<byte[]>, List<byte[]>> imageKernel(Converter converter) {
    Main.Prepare<List<byte[]>> load =
        (n, data) -> {
          var photos = new ArrayList<byte[]>();
          for (int i = 0; i < n; i++) {
            photos.add(Files.readAllBytes(data.resolve("photos/photo-" + (i % PHOTOS) + ".jpg")));
          }
          return photos;
        };
    Function<List<byte[]>, List<byte[]>> run =
        photos -> {
          var out = new ArrayList<byte[]>();
          for (var photo : photos) {
            try {
              out.add(converter.convert(photo));
            } catch (IOException e) {
              throw new UncheckedIOException(e);
            }
          }
          return out;
        };
    return kernel(
        load,
        run,
        out -> out.size() + ":" + jpegSize(out.getLast()),
        out -> Map.of("out_bytes", out.stream().mapToLong(b -> b.length).sum()));
  }

  /** "WxH" from the JPEG header, read after timing. */
  private static String jpegSize(byte[] jpeg) {
    try (var in = ImageIO.createImageInputStream(new ByteArrayInputStream(jpeg))) {
      ImageReader reader = ImageIO.getImageReaders(in).next();
      try {
        reader.setInput(in);
        return reader.getWidth(0) + "x" + reader.getHeight(0);
      } finally {
        reader.dispose();
      }
    } catch (IOException e) {
      throw new UncheckedIOException(e);
    }
  }

  private static byte[] convertImageIo(byte[] jpeg) throws IOException {
    BufferedImage src = ImageIO.read(new ByteArrayInputStream(jpeg));
    int height = targetHeight(src.getWidth(), src.getHeight());
    var small = new BufferedImage(TARGET_WIDTH, height, BufferedImage.TYPE_INT_RGB);
    var g = small.createGraphics();
    g.setRenderingHint(
        RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BICUBIC);
    g.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
    g.drawImage(src, 0, 0, TARGET_WIDTH, height, null);
    g.dispose();
    var out = new ByteArrayOutputStream();
    ImageWriter writer = ImageIO.getImageWritersByFormatName("jpeg").next();
    ImageWriteParam param = writer.getDefaultWriteParam();
    param.setCompressionMode(ImageWriteParam.MODE_EXPLICIT);
    param.setCompressionQuality(JPEG_QUALITY);
    try (var stream = new MemoryCacheImageOutputStream(out)) {
      writer.setOutput(stream);
      writer.write(null, new IIOImage(small, null, null), param);
    } finally {
      writer.dispose();
    }
    return out.toByteArray();
  }

}
