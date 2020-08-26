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
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.nio.ShortBuffer;
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
    public enum OSType {
        UNIX, MACOS, IOS, WINDOWS;
    }
    public enum CPUFamily {
        /** AMD/Intel */
        X86,
        /** ARM 32bit */
        ARM32,
        /** ARM 64bit */
        ARM64,
        /** Power PC */
        PPC,
        /** SPARC */
        SPARC,
        /** Mips */
        MIPS,
        /** PA RISC */
        PA_RISC,
        /** Itanium */
        IA64,
        /** Hitachi SuperH */
        SuperH;
    }
    public enum CPUType {
        /** ARM 32bit default, usually little endian */
        ARM(       CPUFamily.ARM32,     true),
        /** ARM7EJ, ARM9E, ARM10E, XScale, usually little endian */
        ARMv5(     CPUFamily.ARM32,     true),
        /** ARM11, usually little endian */
        ARMv6(     CPUFamily.ARM32,     true),
        /** ARM Cortex, usually little endian */
        ARMv7(     CPUFamily.ARM32,     true),
        // 4

        /** X86 32bit, little endian */
        X86_32(    CPUFamily.X86,     true),
        /** PPC 32bit default, usually big endian */
        PPC(       CPUFamily.PPC,     true),
        /** MIPS 32bit, big endian (mips) or little endian (mipsel) */
        MIPS_32(   CPUFamily.MIPS,    true),
        /** Hitachi SuperH 32bit default, ??? endian */
        SuperH(    CPUFamily.SuperH,  true),
        /** SPARC 32bit, big endian */
        SPARC_32(  CPUFamily.SPARC,   true),
        // 9

        /** ARM64 default (64bit), usually little endian */
        ARM64(     CPUFamily.ARM64,     false),
        /** ARM AArch64 (64bit), usually little endian */
        ARMv8_A(   CPUFamily.ARM64,     false),
        /** X86 64bit, little endian */
        X86_64(    CPUFamily.X86,     false),
        /** PPC 64bit default, usually big endian */
        PPC64(     CPUFamily.PPC,     false),
        /** MIPS 64bit, big endian (mips64) or little endian (mipsel64) ? */
        MIPS_64(   CPUFamily.MIPS,    false),
        /** Itanium 64bit default, little endian */
        IA64(      CPUFamily.IA64,    false),
        /** SPARC 64bit, big endian */
        SPARCV9_64(CPUFamily.SPARC,   false),
        /** PA_RISC2_0 64bit, ??? endian */
        PA_RISC2_0(CPUFamily.PA_RISC, false);
        // 17

        public final CPUFamily family;
        public final boolean is32Bit;

        CPUType(final CPUFamily type, final boolean is32Bit){
            this.family = type;
            this.is32Bit = is32Bit;
        }

        /**
         * Returns {@code true} if the given {@link CPUType} is compatible
         * w/ this one, i.e. at least {@link #family} and {@link #is32Bit} is equal.
         */
        public final boolean isCompatible(final CPUType other) {
            if( null == other ) {
                return false;
            } else if( other == this ) {
                return true;
            } else {
                return this.family == other.family &&
                       this.is32Bit == other.is32Bit;
            }
        }

        public static final CPUType query(final String cpuABILower) {
            if( null == cpuABILower ) {
                throw new IllegalArgumentException("Null cpuABILower arg");
            }
            if(        cpuABILower.equals("x86")  ||
                       cpuABILower.equals("i386") ||
                       cpuABILower.equals("i486") ||
                       cpuABILower.equals("i586") ||
                       cpuABILower.equals("i686") ) {
                return X86_32;
            } else if( cpuABILower.equals("x86_64") ||
                       cpuABILower.equals("amd64")  ) {
                return X86_64;
            } else if( cpuABILower.equals("ia64") ) {
                return IA64;
            } else if( cpuABILower.equals("aarch64") ) {
                return ARM64;
            } else if( cpuABILower.startsWith("arm") ) {
                if(        cpuABILower.equals("armv8-a")   ||
                           cpuABILower.equals("arm-v8-a") ||
                           cpuABILower.equals("arm-8-a") ||
                           cpuABILower.equals("arm64-v8a") ) {
                    return ARMv8_A;
                } else if( cpuABILower.startsWith("arm64") ) {
                    return ARM64;
                } else if( cpuABILower.startsWith("armv7") ||
                           cpuABILower.startsWith("arm-v7") ||
                           cpuABILower.startsWith("arm-7") ||
                           cpuABILower.startsWith("armeabi-v7") ) {
                    return ARMv7;
                } else if( cpuABILower.startsWith("armv5") ||
                           cpuABILower.startsWith("arm-v5") ||
                           cpuABILower.startsWith("arm-5") ) {
                    return ARMv5;
                } else if( cpuABILower.startsWith("armv6") ||
                           cpuABILower.startsWith("arm-v6") ||
                           cpuABILower.startsWith("arm-6") ) {
                    return ARMv6;
                } else {
                    return ARM;
                }
            } else if( cpuABILower.equals("sparcv9") ) {
                return SPARCV9_64;
            } else if( cpuABILower.equals("sparc") ) {
                return SPARC_32;
            } else if( cpuABILower.equals("pa_risc2.0") ) {
                return PA_RISC2_0;
            } else if( cpuABILower.startsWith("ppc64") ) {
                return PPC64;
            } else if( cpuABILower.startsWith("ppc") ) {
                return PPC;
            } else if( cpuABILower.startsWith("mips64") ) {
                return MIPS_64;
            } else if( cpuABILower.startsWith("mips") ) {
                return MIPS_32;
            } else if( cpuABILower.startsWith("superh") ) {
                return SuperH;
            } else {
                throw new RuntimeException("Please port CPUType detection to your platform (CPU_ABI string '" + cpuABILower + "')");
            }
        }
    }
    public enum ABIType {
        GENERIC_ABI       ( 0x00 ),
        /** ARM GNU-EABI ARMEL -mfloat-abi=softfp */
        EABI_GNU_ARMEL    ( 0x01 ),
        /** ARM GNU-EABI ARMHF -mfloat-abi=hard */
        EABI_GNU_ARMHF    ( 0x02 ),
        /** ARM EABI AARCH64 (64bit) */
        EABI_AARCH64      ( 0x03 );

        public final int id;

        ABIType(final int id){
            this.id = id;
        }

        /**
         * Returns {@code true} if the given {@link ABIType} is compatible
         * w/ this one, i.e. they are equal.
         */
        public final boolean isCompatible(final ABIType other) {
            if( null == other ) {
                return false;
            } else {
                return other == this;
            }
        }

        public static final ABIType query(final CPUType cpuType, final String cpuABILower) {
            if( null == cpuType ) {
                throw new IllegalArgumentException("Null cpuType");
            } else if( null == cpuABILower ) {
                throw new IllegalArgumentException("Null cpuABILower");
            } else if( CPUFamily.ARM64 == cpuType.family ) {
                return EABI_AARCH64;
            } else if( CPUFamily.ARM32 == cpuType.family ) {
                // FIXME: We only support EABI_GNU_ARMHF on ARM 32bit for now!
                return EABI_GNU_ARMHF;
            } else {
                return GENERIC_ABI;
            }
        }
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
            final String _os_arch1 = props[i++];
            user_dir = props[i++];
            java_user_lib_path = props[i++];
            java_boot_lib_path = props[i++];

            final boolean LITTLE_ENDIAN = queryIsLittleEndianImpl();
            final CPUType CPU_TYPE = CPUType.query(_os_arch1);
            final ABIType ABI_TYPE = ABIType.query(CPU_TYPE, _os_arch1);
            final String _os_arch2 = getArchName(CPU_TYPE, ABI_TYPE, LITTLE_ENDIAN);
            os_arch = null != _os_arch2 ? _os_arch2 : _os_arch1;
            os_and_arch = os_name+"-"+os_arch;
            if( BluetoothFactory.DEBUG ) {
                System.err.println("PlatformToolkit: os_name "+os_name+", os_arch ("+_os_arch1+" -> "+_os_arch2+" ->) "+os_arch+" (final), "+
                                   "CPU_TYPE "+CPU_TYPE+", ABI_TYPE "+ABI_TYPE+", LITTLE_ENDIAN "+LITTLE_ENDIAN);
            }
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

        case UNIX:
        default:
            prefix = "lib";
            suffix = ".so";
            break;
        }
    }

    private static final boolean queryIsLittleEndianImpl() {
        final ByteBuffer tst_b = ByteBuffer.allocateDirect(4 /* SIZEOF_INT */).order(ByteOrder.nativeOrder()); // 32bit in native order
        final IntBuffer tst_i = tst_b.asIntBuffer();
        final ShortBuffer tst_s = tst_b.asShortBuffer();
        tst_i.put(0, 0x0A0B0C0D);
        return 0x0C0D == tst_s.get(0);
    }
    private static final String getArchName(final CPUType cpuType, final ABIType abiType, final boolean littleEndian) {
        switch( abiType ) {
            case EABI_GNU_ARMEL:
                return "arm"; // actually not supported!
            case EABI_GNU_ARMHF:
                return "armhf";
            case EABI_AARCH64:
                return "arm64";
            default:
                break;
        }

        switch( cpuType ) {
            case X86_32:
                return "i386";
            case PPC:
                return "ppc";
            case MIPS_32:
                return littleEndian ? "mipsel" : "mips";
            case SuperH:
                return "superh";
            case SPARC_32:
                return "sparc";

            case X86_64:
                return "amd64";
            case PPC64:
                return littleEndian ? "ppc64le" : "ppc64";
            case MIPS_64:
                return "mips64";
            case IA64:
                return "ia64";
            case SPARCV9_64:
                return "sparcv9";
            case PA_RISC2_0:
                return "risc2.0";
            default:
                return null;
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
    private static final String getCanonicalPath(final String path) {
        return AccessController.doPrivileged(new PrivilegedAction<String>() {
            @Override
            public String run() {
                try {
                    final File f = new File(path);
                    // f.getCanonicalPath() also resolved '.', '..' and symbolic links in contrast to f.getAbsolutePath()
                    return f.getCanonicalPath();
                } catch (final Throwable t) {
                    if( BluetoothFactory.DEBUG ) {
                        System.err.println("getAbsolutePath("+path+") failed: "+t.getMessage());
                    }
                    return null;
                }
            } } );
    }
    private static final void addPath(final String msg, final String path, final String platformName, final List<String> paths) {
        if( null != path && path.length() > 0 ) {
            final String fullpath = path + File.separator + platformName;
            final String abspath = getCanonicalPath(fullpath);
            if( null != abspath ) {
                final boolean isDup = paths.contains(abspath);
                if( BluetoothFactory.DEBUG ) {
                    System.err.println("  "+abspath+" (addPath "+msg+", dropped duplicate "+isDup+")");
                }
                if( !isDup ) {
                    paths.add(abspath);
                }
            }
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
        if( BluetoothFactory.DEBUG ) {
            System.err.println();
            System.err.println("PlatformToolkit.loadLibrary: libBaseName "+libBaseName+":");
        }
        final List<String> possiblePaths = enumerateLibraryPaths(libBaseName, true /* searchSystemPath */, false /* searchSystemPathFirst */, cl);
        if( BluetoothFactory.DEBUG ) {
            System.err.println();
        }

        // Iterate down these and see which one if any we can actually find.
        for (final Iterator<String> iter = possiblePaths.iterator(); iter.hasNext(); ) {
            final String path = iter.next();
            try {
                System.load(path);
                if( BluetoothFactory.DEBUG ) {
                    System.err.println("  "+path+" success");
                }
                return true;
            } catch (final Throwable t0) {
                if( BluetoothFactory.DEBUG ) {
                    System.err.println("  "+path+" failed: "+t0.getMessage());
                }
                t[0] = t0;
            }
        }

        // Fall back to loadLibrary
        try {
            System.loadLibrary(libBaseName);
            if( BluetoothFactory.DEBUG ) {
                System.err.println("  "+libBaseName+" success");
            }
            return true;
        } catch (final Throwable t0) {
            if( BluetoothFactory.DEBUG ) {
                System.err.println("  "+libBaseName+" failed: "+t0.getMessage());
            }
            t[0] = t0;
        }
        return false;
    }
}
