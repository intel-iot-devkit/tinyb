/**
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
package org.tinyb;

import java.io.File;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;


/**
 * Miscellaneous platform utilities, allowed to be used within same Java package.
 */
final class PlatformToolkit {
  private static boolean DEBUG = true;
  private static boolean DEBUG2 = true;

  private enum OSType {
      UNIX, MACOS, IOS, WINDOWS;
  }

  /** Lower case system property '{@code os.name}'. */
  static final String os_name;
  /** Lower case system property '{@code os.arch}' */
  static final String os_arch;
  private static final String user_dir;
  private static final String java_user_lib_path;
  private static final String java_boot_lib_path;

  /**
   * Unique platform denominator composed as '{@link #os_name}' + '-' + '{@link #os_arch}'.
   */
  static final String os_and_arch;
  static final OSType OS_TYPE;
  private static final boolean isOSX;

  private static final String prefix;
  private static final String suffix;

  static {
    {
        final String[] props =
          AccessController.doPrivileged(new PrivilegedAction<String[]>() {
              @Override
              public String[] run() {
                final String[] props = new String[5];
                int i=0;
                props[i++] = System.getProperty("os.name").toLowerCase(); // 0
                props[i++] = System.getProperty("os.arch").toLowerCase(); // 1
                props[i++] = System.getProperty("user.dir"); // 2
                props[i++] = System.getProperty("java.library.path"); // 3
                props[i++] = System.getProperty("sun.boot.library.path"); // 4
                return props;
              }
            });
        int i=0;
        os_name = props[i++];
        os_arch = props[i++];
        user_dir = props[i++];
        java_user_lib_path = props[i++];
        java_boot_lib_path = props[i++];
    }

    if( null != os_name && os_name.length() > 0 && null != os_arch && os_arch.length() > 0 ) {
        os_and_arch = os_name+"-"+os_arch;
    } else {
        os_and_arch = null;
    }

    if ( os_name.startsWith("mac os x") ||
         os_name.startsWith("darwin") ) {
        OS_TYPE = OSType.MACOS;
        isOSX = true;
    } else if ( os_name.startsWith("ios") ) {
        OS_TYPE = OSType.IOS;
        isOSX = true;
    } else if ( os_name.startsWith("windows") ) {
        OS_TYPE = OSType.WINDOWS;
        isOSX = false;
    } else {
        OS_TYPE = OSType.UNIX;
        isOSX = false;
    }

    switch (OS_TYPE) {
      case WINDOWS:
        prefix = "";
        suffix = ".dll";
        break;

      case MACOS:
      case IOS:
        prefix = "lib";
        suffix = ".dylib";
        break;

      /*
      case UNIX: */
      default:
        prefix = "lib";
        suffix = ".so";
        break;
    }
  }

  /**
   * Produces a list of potential full native library pathnames, denoted by its {@code libBaseName}.
   *
   * <p>
   * Basic order of library locations
   * <pre>
   * User locations:
   *   - current working directory + {@link #os_and_arch}
   *   - iterate through paths within 'java.library.path', adding {@link #os_and_arch} to each
   *   - current working directory
   *   - iterate through paths within 'java.library.path'
   *
   * System locations:
   *   - optional OSX path
   *   - iterate through paths within 'sun.boot.library.path'
   * </pre>
   * </p>
   *
   * <p>
   * Example:
   * <pre>
     /usr/local/projects/direct_bt/dist-amd64/linux-amd64/libdirect_bt.so (addPath cwd.os_and_arch)
     /usr/local/projects/direct_bt/dist-amd64/lib/linux-amd64/libdirect_bt.so (addPath java-user-libpath.os_and_arch:0)
     /usr/local/projects/direct_bt/dist-amd64/libdirect_bt.so (addPath cwd)
     /usr/local/projects/direct_bt/dist-amd64/lib/libdirect_bt.so (addPath java-user-libpath:0)
     /usr/lib/jvm/java-14-openjdk-amd64/lib/libdirect_bt.so (addPath java-boot-libpath:0)
   * </pre>
   *
   * @param libBaseName library basename without prefix (like 'lib') or suffix like '.so'.
   * @param searchSystemPath
   * @param searchSystemPathFirst
   * @param loader
   * @return
   */
  private static final List<String> enumerateLibraryPaths(final String libBaseName,
                                                   final boolean searchSystemPath,
                                                   final boolean searchSystemPathFirst,
                                                   final ClassLoader loader) {
    final List<String> paths = new ArrayList<String>();

    if ( libBaseName == null || libBaseName.length() == 0 ) {
      return paths;
    }

    final String libPlatformName = getPlatformName(libBaseName);

    if( searchSystemPath && searchSystemPathFirst ) {
        // Add probable Mac OS X-specific paths
        if ( isOSX ) {
            // Add historical location
            addPath("osx-1", "/Library/Frameworks/" + libBaseName + ".framework", libPlatformName, paths);
            // Add current location
            addPath("osx-2", "/System/Library/Frameworks/" + libBaseName + ".framework", libPlatformName, paths);
        }
        addMultiPaths("java-boot-libpath", java_boot_lib_path, libPlatformName, paths);
    }

    addPath("cwd.os_and_arch", user_dir+File.separator+os_and_arch, libPlatformName, paths);
    addMultiPaths2("java-user-libpath.os_and_arch", java_user_lib_path, os_and_arch, libPlatformName, paths);

    addPath("cwd", user_dir, libPlatformName, paths);
    addMultiPaths("java-user-libpath", java_user_lib_path, libPlatformName, paths);

    if( searchSystemPath && !searchSystemPathFirst ) {
        // Add probable Mac OS X-specific paths
        if ( isOSX ) {
            // Add historical location
            addPath("osx-1", "/Library/Frameworks/" + libBaseName + ".Framework", libPlatformName, paths);
            // Add current location
            addPath("osx-2", "/System/Library/Frameworks/" + libBaseName + ".Framework", libPlatformName, paths);
        }
        addMultiPaths("java-boot-libpath", java_boot_lib_path, libPlatformName, paths);
    }

    return paths;
  }


  private static final String getPlatformName(final String libBaseName) {
      return prefix + libBaseName + suffix;
  }
  private static final void addPath(final String msg, final String path, final String platformName, final List<String> paths) {
      if( null != path && path.length() > 0 ) {
          final String fullpath = path + File.separator + platformName;
          if( DEBUG2 ) {
              System.err.println("  "+fullpath+" (addPath "+msg+")");
          }
          paths.add(fullpath);
      }
  }
  private static final void addMultiPaths(final String msg, final String pathList, final String platformName, final List<String> paths) {
      if( null != pathList && pathList.length() > 0 ) {
          final StringTokenizer tokenizer = new StringTokenizer(pathList, File.pathSeparator);
          int i=0;
          while (tokenizer.hasMoreTokens()) {
              addPath(msg+":"+i, tokenizer.nextToken(), platformName, paths);
              i++;
          }
      }
  }
  private static final void addMultiPaths2(final String msg, final String pathList, final String subDir, final String platformName, final List<String> paths) {
      if( null != pathList && pathList.length() > 0 && null != subDir && subDir.length() > 0 ) {
          final StringTokenizer tokenizer = new StringTokenizer(pathList, File.pathSeparator);
          int i=0;
          while (tokenizer.hasMoreTokens()) {
              final String path = tokenizer.nextToken() + File.separator + subDir;
              addPath(msg+":"+i, path, platformName, paths);
              i++;
          }
      }
  }


  /**
   * Loads a native library, denoted by its {@code libBaseName}.
   *
   * <p>
   * Basic order of library locations
   * <pre>
   * User locations:
   *   - current working directory + {@link #os_and_arch}
   *   - iterate through paths within 'java.library.path', adding {@link #os_and_arch} to each
   *   - current working directory
   *   - iterate through paths within 'java.library.path'
   *
   * System locations:
   *   - optional OSX path
   *   - iterate through paths within 'sun.boot.library.path'
   * </pre>
   * </p>
   *
   * <p>
   * If the above fails, {@link System#loadLibrary(String)} is called using the plain {@code libBaseName},
   * exhausting all simple locations and methods.
   * </p>
   *
   * <p>
   * Example:
   * <pre>
     /usr/local/projects/direct_bt/dist-amd64/linux-amd64/libdirect_bt.so (addPath cwd.os_and_arch)
     /usr/local/projects/direct_bt/dist-amd64/lib/linux-amd64/libdirect_bt.so (addPath java-user-libpath.os_and_arch:0)
     /usr/local/projects/direct_bt/dist-amd64/libdirect_bt.so (addPath cwd)
     /usr/local/projects/direct_bt/dist-amd64/lib/libdirect_bt.so (addPath java-user-libpath:0)
     /usr/lib/jvm/java-14-openjdk-amd64/lib/libdirect_bt.so (addPath java-boot-libpath:0)
   * </pre>
   *
   * @param libBaseName library basename without prefix (like 'lib') or suffix like '.so'.
   * @param cl
   * @param t holder to store the last Throwable, if any
   * @return {@code true} if successful, otherwise {@code false}.
   */
  static boolean loadLibrary(final String libBaseName, final ClassLoader cl, final Throwable[] t) {
      if( DEBUG ) {
          System.err.println("PlatformToolkit.loadLibrary: libBaseName "+libBaseName+":");
      }
      final List<String> possiblePaths = enumerateLibraryPaths(libBaseName, true /* searchSystemPath */, false /* searchSystemPathFirst */, cl);
      if( DEBUG2 ) {
          System.err.println();
      }

      // Iterate down these and see which one if any we can actually find.
      for (final Iterator<String> iter = possiblePaths.iterator(); iter.hasNext(); ) {
          final String path = iter.next();
          try {
              System.load(path);
              if( DEBUG ) {
                  System.err.println("  "+path+" success");
              }
              return true;
          } catch (final Throwable t0) {
              if( DEBUG ) {
                  System.err.println("  "+path+" failed: "+t0.getMessage());
              }
              t[0] = t0;
          }
      }

      // Fall back to loadLibrary
      try {
          System.loadLibrary(libBaseName);
          if( DEBUG ) {
              System.err.println("  "+libBaseName+" success");
          }
          return true;
      } catch (final Throwable t0) {
          if( DEBUG ) {
              System.err.println("  "+libBaseName+" failed: "+t0.getMessage());
          }
          t[0] = t0;
      }
      return false;
  }
}
