// Race: argon2id (java)
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

import java.util.List;

import org.springframework.security.crypto.argon2.Argon2PasswordEncoder;

final class Work {

  private static final int ARGON2_MEMORY = 19456;

  private static final int ARGON2_TIME = 2;

  private static final int ARGON2_LANES = 1;

  private static final String ARGON2_PREFIX = "$argon2id$v=19$m=19456,t=2,p=1$";

  private static final String REF_PASSWORD = "correct horse battery staple";

  private static final String ARGON2_REF =
      "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

  private static final Argon2PasswordEncoder ARGON2 =
      new Argon2PasswordEncoder(16, 32, ARGON2_LANES, ARGON2_MEMORY, ARGON2_TIME);

  private static List<String> argon2Hash(List<String> pws) {
    return pws.stream().map(pw -> ARGON2.encode(pw)).toList();
  }

  private static String argon2Checksum(List<String> hashes) {
    int ok = 0;
    for (int i = 0; i < hashes.size(); i++) {
      var h = hashes.get(i);
      if (h.startsWith(ARGON2_PREFIX) && ARGON2.matches("user-" + i + "-password", h)) {
        ok++;
      }
    }
    int ref = ARGON2.matches(REF_PASSWORD, ARGON2_REF) ? 1 : 0;
    return hashes.size() + ":" + ok + ":" + ref;
  }

}
