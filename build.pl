#!/usr/bin/env perl
# =============================================================================
# E-comOS Kernel Build System (Perl Version)
# Copyright (C) 2025,2026 Saladin5101
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# =============================================================================

use strict;
use warnings;
use Time::HiRes qw(gettimeofday tv_interval);
use File::Path qw(make_path remove_tree);
use File::Copy;
use File::Basename;
use Cwd qw(abs_path);
use IPC::Open3;
use Symbol 'gensym';
use POSIX qw(:sys_wait_h);

# ===================== Configuration =====================
my $config = {
    # Toolchain configuration
    tools => {
        cc      => 'gcc',
        as      => 'as',
        ld      => 'ld',
        nasm    => 'nasm',
        objcopy => 'objcopy',
        qemu    => 'qemu-system-x86_64',
    },
    
    # Cross-compilation toolchain
    cross_tools => {
        cc      => 'x86_64-elf-gcc',
        as      => 'x86_64-elf-as',
        ld      => 'x86_64-elf-ld',
        objcopy => 'x86_64-elf-objcopy',
    },
    
    # Compiler flags
    cflags => [
        '-m64',
        '-ffreestanding',
        '-fno-builtin',
        '-fno-stack-protector',
        '-nostdlib',
        '-Wall',
        '-Wextra',
        '-c',
        '-mcmodel=large',
        '-Werror',
        '-pedantic',
        '-Iinclude',
    ],
    
    # Assembler flags
    asflags => ['--64'],
    
    # NASM flags
    nasm_elf_flags => ['-f', 'elf64'],
    nasm_bin_flags => ['-f', 'bin'],
    
    # Linker flags
    ldflags => [
        '-m', 'elf_x86_64',
        '-nostdlib',
        '-T', 'uefi.ld',
    ],
    
    # EFI configuration
    efi => {
        inc      => '/usr/include/efi',
        inc_x64  => '/usr/include/efi/x86_64',
        lib      => '/usr/lib',
        lds      => '/usr/lib/elf_x86_64_efi.lds',
        crt0     => '/usr/lib/crt0-efi-x86_64.o',
    },
    
    # Output files
    output => {
        kernel_bin  => 'kernel.bin',
        kernel_elf  => 'kernel.elf',
        kernel_efi  => 'kernel.efi',
        image_file  => 'canuse.img',
        stage1_bin  => 'dos25-release.bin',
        stage2_bin  => 'dos25-stage2.bin',
    },
    
    # Source files
    sources => {
        kernel => [
            'src/kernel/main.c',
            'src/kernel/syscall.c',
            'src/kernel/init.c',
            'src/kernel/debug.c',
            'src/ipc/ipc.c',
            'src/mm/mm.c',
            'src/sched/sched.c',
            'src/printkit/print.c',
            'src/time/time.c',
            'arch/x86_64/cpu/gdt.c',
            'arch/x86_64/interrupts/idt.c',
            'arch/x86_64/interrupts/isr.c',
            'arch/x86_64/interrupts/irq.c',
        ],
        
        asm => [
            'arch/x86_64/cpu/context_switch.s',
            'boot/uefi_start.s',
        ],
        
        asm_extra => [
            'arch/x86_64/interrupts/isr_asm.o',
            'arch/x86_64/interrupts/irq_asm.o',
        ],
        
        stage1 => 'src/boot/bootsect.s',
        stage2 => 'src/boot/bootsect-2nd.s',
        
        efi_asm => [
            'src/efi/efi_start.s',
        ],
    },
    
    # Build directories
    build_dir => 'build',
    obj_dir   => 'build/obj',
};

# ===================== Helper Functions =====================

sub print_table_header {
    print "\n" . "=" x 120 . "\n";
    printf "%-40s | %-50s | %-12s | %-15s\n", 
           "OBJECT", "COMMAND", "TIME (s)", "STATUS";
    print "-" x 120 . "\n";
}

sub print_table_row {
    my ($object, $command, $time, $status) = @_;
    
    # Truncate long strings
    $object  = substr($object, 0, 37) . "..." if length($object) > 40;
    $command = substr($command, 0, 47) . "..." if length($command) > 50;
    
    printf "%-40s | %-50s | %12.3f | %-15s\n", 
           $object, $command, $time, $status;
}

sub execute_command {
    my ($cmd, $desc) = @_;
    my $start_time = [gettimeofday];
    
    print "[EXEC] $desc\n" if $desc;
    print "      Command: $cmd\n" if $cmd;
    
    my ($output, $exit_code) = run_command($cmd);
    my $elapsed = tv_interval($start_time);
    
    return {
        success   => ($exit_code == 0),
        output    => $output,
        time      => $elapsed,
        exit_code => $exit_code,
    };
}

sub run_command {
    my ($cmd) = @_;
    
    my $pid = open(my $read, "$cmd 2>&1 |") or return ("Failed to execute: $!", 1);
    my $output = do { local $/; <$read> };
    close $read;
    
    my $exit_code = $? >> 8;
    return ($output, $exit_code);
}

sub find_tool {
    my ($tool, $cross) = @_;
    
    if ($cross) {
        my $cross_tool = $config->{cross_tools}{$tool};
        if (system("which $cross_tool >/dev/null 2>&1") == 0) {
            return $cross_tool;
        }
    }
    
    my $standard_tool = $config->{tools}{$tool};
    if (system("which $standard_tool >/dev/null 2>&1") == 0) {
        return $standard_tool;
    }
    
    die "ERROR: Cannot find tool '$tool'. Please install it.\n";
}

sub ensure_dir {
    my ($dir) = @_;
    make_path($dir) unless -d $dir;
}

# ===================== Build Functions =====================

sub compile_c_file {
    my ($source_file) = @_;
    my $obj_file = $source_file =~ s/\.c$/.o/r =~ s/^src/$config->{obj_dir}/r;
    
    ensure_dir(dirname($obj_file));
    
    my $cc = find_tool('cc', 1);
    my @cflags = @{$config->{cflags}};
    
    my $cmd = join(' ', $cc, @cflags, '-o', $obj_file, $source_file);
    
    my $result = execute_command($cmd, "Compiling C file: $source_file");
    
    print_table_row(
        $source_file,
        $cmd,
        $result->{time},
        $result->{success} ? "✓ SUCCESS" : "✗ FAILED"
    );
    
    if (!$result->{success}) {
        print "ERROR OUTPUT:\n" . $result->{output} . "\n";
    }
    
    return {
        success  => $result->{success},
        obj_file => $obj_file,
        output   => $result->{output},
    };
}

sub compile_asm_file {
    my ($source_file) = @_;
    my $obj_file = $source_file =~ s/\.s$/.o/r =~ s/^arch/$config->{obj_dir}/r;
    
    ensure_dir(dirname($obj_file));
    
    my $as = find_tool('as', 1);
    my @asflags = @{$config->{asflags}};
    
    my $cmd = join(' ', $as, @asflags, '-o', $obj_file, $source_file);
    
    my $result = execute_command($cmd, "Assembling: $source_file");
    
    print_table_row(
        $source_file,
        $cmd,
        $result->{time},
        $result->{success} ? "✓ SUCCESS" : "✗ FAILED"
    );
    
    if (!$result->{success}) {
        print "ERROR OUTPUT:\n" . $result->{output} . "\n";
    }
    
    return {
        success  => $result->{success},
        obj_file => $obj_file,
        output   => $result->{output},
    };
}

sub compile_nasm_file {
    my ($source_file, $is_binary) = @_;
    my $output_file = $source_file =~ s/\.s$/.bin/r;
    
    ensure_dir(dirname($output_file));
    
    my $nasm = $config->{tools}{nasm};
    my @flags = $is_binary ? @{$config->{nasm_bin_flags}} : @{$config->{nasm_elf_flags}};
    
    my $cmd = join(' ', $nasm, @flags, $source_file, '-o', $output_file);
    
    my $result = execute_command($cmd, "NASM assembling: $source_file");
    
    print_table_row(
        $source_file,
        $cmd,
        $result->{time},
        $result->{success} ? "✓ SUCCESS" : "✗ FAILED"
    );
    
    if (!$result->{success}) {
        print "ERROR OUTPUT:\n" . $result->{output} . "\n";
    }
    
    return {
        success      => $result->{success},
        output_file  => $output_file,
        output       => $result->{output},
    };
}

sub link_kernel {
    my (@obj_files) = @_;
    my $output_file = $config->{output}{kernel_elf};
    
    my $ld = find_tool('ld', 1);
    my @ldflags = @{$config->{ldflags}};
    
    my $cmd = join(' ', $ld, @ldflags, '-o', $output_file, @obj_files);
    
    my $result = execute_command($cmd, "Linking kernel");
    
    print_table_row(
        "LINK KERNEL",
        $cmd,
        $result->{time},
        $result->{success} ? "✓ LINKED" : "✗ FAILED"
    );
    
    if ($result->{success}) {
        my $size = -s $output_file;
        print "✓ Kernel ELF size: $size bytes\n";
    } else {
        print "ERROR OUTPUT:\n" . $result->{output} . "\n";
    }
    
    return {
        success      => $result->{success},
        output_file  => $output_file,
        output       => $result->{output},
    };
}

sub create_boot_image {
    my ($stage1, $stage2, $kernel) = @_;
    my $image_file = $config->{output}{image_file};
    
    # Create 1.44MB floppy image
    my $cmd1 = "dd if=/dev/zero of=$image_file bs=512 count=2880 2>/dev/null";
    my $cmd2 = "dd if=$stage1 of=$image_file bs=512 count=1 conv=notrunc 2>/dev/null";
    my $cmd3 = "dd if=$stage2 of=$image_file bs=512 seek=1 conv=notrunc 2>/dev/null";
    my $cmd4 = "dd if=$kernel of=$image_file bs=512 seek=14 conv=notrunc 2>/dev/null";
    
    print "[INFO] Creating bootable image: $image_file\n";
    
    my $result1 = execute_command($cmd1, "Creating blank image");
    my $result2 = execute_command($cmd2, "Writing stage1 bootloader");
    my $result3 = execute_command($cmd3, "Writing stage2 bootloader");
    my $result4 = execute_command($cmd4, "Writing kernel");
    
    if ($result1->{success} && $result2->{success} && 
        $result3->{success} && $result4->{success}) {
        
        my $size = -s $image_file;
        print "✓ Bootable image created: $image_file ($size bytes)\n";
        return 1;
    }
    
    return 0;
}

sub run_qemu {
    my ($image_file) = @_;
    my $qemu = $config->{tools}{qemu};
    
    if (!-f $image_file) {
        print "ERROR: Image file not found: $image_file\n";
        return 0;
    }
    
    print "[INFO] Starting QEMU with image: $image_file\n";
    print "       Press Ctrl+A, X to exit QEMU\n\n";
    
    my $cmd = "$qemu -fda $image_file -monitor stdio";
    system($cmd);
    
    return 1;
}

sub clean_build {
    my @patterns = (
        '*.o', '*.bin', '*.elf', '*.efi', '*.img',
        '*.log', '*.txt',
        "$config->{obj_dir}/*",
        "$config->{build_dir}/*",
    );
    
    print "[INFO] Cleaning build artifacts...\n";
    
    foreach my $pattern (@patterns) {
        my @files = glob($pattern);
        foreach my $file (@files) {
            unlink $file if -f $file;
            print "  Deleted: $file\n";
        }
    }
    
    # Clean directories
    remove_tree($config->{obj_dir}) if -d $config->{obj_dir};
    remove_tree($config->{build_dir}) if -d $config->{build_dir};
    
    print "✓ Clean complete\n";
    return 1;
}

# ===================== Main Build Process =====================

sub build_all {
    my $start_time = [gettimeofday];
    
    print "=" x 60 . "\n";
    print "E-comOS Kernel Build System (Perl Version)\n";
    print "=" x 60 . "\n\n";
    
    print_table_header();
    
    my @obj_files;
    my $all_success = 1;
    
    # Compile C source files
    foreach my $c_file (@{$config->{sources}{kernel}}) {
        if (-f $c_file) {
            my $result = compile_c_file($c_file);
            push @obj_files, $result->{obj_file} if $result->{success};
            $all_success &&= $result->{success};
        } else {
            print "WARNING: Source file not found: $c_file\n";
        }
    }
    
    # Compile assembly files
    foreach my $asm_file (@{$config->{sources}{asm}}) {
        if (-f $asm_file) {
            my $result = compile_asm_file($asm_file);
            push @obj_files, $result->{obj_file} if $result->{success};
            $all_success &&= $result->{success};
        } else {
            print "WARNING: Assembly file not found: $asm_file\n";
        }
    }
    
    # Compile extra assembly files (isr_asm.o, irq_asm.o)
    foreach my $extra_asm (@{$config->{sources}{asm_extra}}) {
        if ($extra_asm =~ /isr_asm\.o$/) {
            my $source = 'arch/x86_64/interrupts/isr.s';
            if (-f $source) {
                my $result = compile_asm_file($source);
                push @obj_files, $result->{obj_file} if $result->{success};
                $all_success &&= $result->{success};
            }
        } elsif ($extra_asm =~ /irq_asm\.o$/) {
            my $source = 'arch/x86_64/interrupts/irq.s';
            if (-f $source) {
                my $result = compile_asm_file($source);
                push @obj_files, $result->{obj_file} if $result->{success};
                $all_success &&= $result->{success};
            }
        }
    }
    
    # Link kernel
    if ($all_success && @obj_files) {
        my $result = link_kernel(@obj_files);
        $all_success &&= $result->{success};
        
        if ($result->{success}) {
            # Copy kernel.elf to kernel.bin
            copy($result->{output_file}, $config->{output}{kernel_bin});
            print "✓ Kernel binary created: $config->{output}{kernel_bin}\n";
        }
    } else {
        print "ERROR: Cannot link kernel - no object files or compilation failed\n";
        $all_success = 0;
    }
    
    my $total_time = tv_interval($start_time);
    print "\n" . "=" x 60 . "\n";
    print "BUILD COMPLETE\n";
    print "Total time: $total_time seconds\n";
    print "Status: " . ($all_success ? "✓ SUCCESS" : "✗ FAILED") . "\n";
    print "=" x 60 . "\n";
    
    return $all_success;
}

sub build_image {
    my $start_time = [gettimeofday];
    
    print "=" x 60 . "\n";
    print "Creating Bootable Image\n";
    print "=" x 60 . "\n\n";
    
    # First ensure kernel is built
    if (!-f $config->{output}{kernel_bin}) {
        print "Kernel not found. Building kernel first...\n";
        if (!build_all()) {
            print "ERROR: Cannot build kernel. Aborting image creation.\n";
            return 0;
        }
    }
    
    # Check for bootloader files
    my $stage1 = $config->{sources}{stage1};
    my $stage2 = $config->{sources}{stage2};
    
    if (!-f $stage1 || !-f $stage2) {
        print "WARNING: Bootloader source files not found:\n";
        print "  Stage1: $stage1\n" if !-f $stage1;
        print "  Stage2: $stage2\n" if !-f $stage2;
        print "\nCreating minimal image with kernel only...\n";
        
        # Create simple image with just kernel
        my $image_file = $config->{output}{image_file};
        my $kernel = $config->{output}{kernel_bin};
        
        my $cmd = "dd if=/dev/zero of=$image_file bs=512 count=2880 2>/dev/null && " .
                  "dd if=$kernel of=$image_file bs=512 conv=notrunc 2>/dev/null";
        
        my $result = execute_command($cmd, "Creating minimal kernel image");
        
        if ($result->{success}) {
            my $size = -s $image_file;
            print "✓ Minimal image created: $image_file ($size bytes)\n";
            print "  Note: This image may not be bootable without a bootloader\n";
            
            my $total_time = tv_interval($start_time);
            print "\nImage creation time: $total_time seconds\n";
            return 1;
        } else {
            print "ERROR: Failed to create image\n";
            return 0;
        }
    } else {
        # Build bootloader and create full image
        print_table_header();
        
        my $stage1_result = compile_nasm_file($stage1, 1);
        my $stage2_result = compile_nasm_file($stage2, 1);
        
        if ($stage1_result->{success} && $stage2_result->{success}) {
            my $success = create_boot_image(
                $stage1_result->{output_file},
                $stage2_result->{output_file},
                $config->{output}{kernel_bin}
            );
            
            my $total_time = tv_interval($start_time);
            print "\nImage creation time: $total_time seconds\n";
            return $success;
        } else {
            print "ERROR: Failed to build bootloader\n";
            return 0;
        }
    }
}

# ===================== Main Entry Point =====================

sub main {
    my $action = shift @ARGV || 'all';
    
    if ($action eq 'all' || $action eq 'build') {
        return build_all();
    }
    elsif ($action eq 'clean') {
        return clean_build();
    }
    elsif ($action eq 'image') {
        return build_image();
    }
    elsif ($action eq 'run') {
        my $image_file = $config->{output}{image_file};
        if (!-f $image_file) {
            print "Image not found. Building image first...\n";
            if (!build_image()) {
                print "ERROR: Cannot build image. Aborting run.\n";
                return 0;
            }
        }
        return run_qemu($image_file);
    }
    elsif ($action eq 'help' || $action eq '--help' || $action eq '-h') {
        print_help();
        return 1;
    }
    else {
        print "ERROR: Unknown action: $action\n\n";
        print_help();
        return 0;
    }
}

sub print_help {
    print <<"HELP";
E-comOS Build System (Perl Version)

Usage: $0 [action]

Available actions:
  all/build    Build everything (default)
  clean        Clean all build artifacts
  image        Create bootable disk image
  run          Run in QEMU
  help         Show this help message

Examples:
  $0              # Build everything
  $0 clean        # Clean build files
  $0 image        # Create bootable image
  $0 run          # Run in QEMU

Configuration:
  Toolchain:    Uses cross-compiler if available, falls back to system tools
  C Flags:      @{$config->{cflags}}
  LD Flags:     @{$config->{ldflags}}

HELP
}

# Run the build system
exit(main() ? 0 : 1);