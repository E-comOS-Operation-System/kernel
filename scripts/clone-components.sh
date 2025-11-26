#!/bin/bash

# E-comOS Component Cloner
# Reads config/components.conf and clones all userspace components

CONFIG_FILE="config/components.conf"
TEMP_DIR="temp-repos"

parse_config() {
    local section=""
    local repo=""
    local branch="main"
    local build_cmd="make"
    local binary=""
    local optional="false"
    
    while IFS= read -r line; do
        # Skip comments and empty lines
        [[ $line =~ ^#.*$ ]] && continue
        [[ -z "${line// }" ]] && continue
        
        # Parse section headers
        if [[ $line =~ ^\[(.+)\]$ ]]; then
            # Process previous section
            if [[ -n "$section" && -n "$repo" && -n "$binary" ]]; then
                clone_component "$section" "$repo" "$branch" "$build_cmd" "$binary" "$optional"
            fi
            
            # Start new section
            section="${BASH_REMATCH[1]}"
            repo=""
            branch="main"
            build_cmd="make"
            binary=""
            optional="false"
            continue
        fi
        
        # Parse key-value pairs
        if [[ $line =~ ^([^=]+)=(.+)$ ]]; then
            key="${BASH_REMATCH[1]// /}"
            value="${BASH_REMATCH[2]// /}"
            
            case "$key" in
                "repo") repo="$value" ;;
                "branch") branch="$value" ;;
                "build_cmd") build_cmd="$value" ;;
                "binary") binary="$value" ;;
                "optional") optional="$value" ;;
            esac
        fi
    done < "$CONFIG_FILE"
    
    # Process last section
    if [[ -n "$section" && -n "$repo" && -n "$binary" ]]; then
        clone_component "$section" "$repo" "$branch" "$build_cmd" "$binary" "$optional"
    fi
}

clone_component() {
    local name="$1"
    local repo="$2" 
    local branch="$3"
    local build_cmd="$4"
    local binary="$5"
    local optional="$6"
    
    echo "  → $name"
    
    if [[ ! -d "$TEMP_DIR/$name" ]]; then
        if git clone -b "$branch" "$repo" "$TEMP_DIR/$name" 2>/dev/null; then
            echo "    ✅ Cloned from $repo"
            
            # Try to build
            if (cd "$TEMP_DIR/$name" && $build_cmd 2>/dev/null); then
                echo "    🔨 Built successfully"
            else
                echo "    ⚠️  Build failed, using as-is"
            fi
        else
            if [[ "$optional" == "true" ]]; then
                echo "    ⏭️  Optional component, skipping"
                return
            fi
            
            echo "    ❌ Clone failed, creating placeholder"
            create_placeholder "$name" "$binary"
        fi
    else
        echo "    📁 Already exists"
    fi
}

create_placeholder() {
    local name="$1"
    local binary="$2"
    
    mkdir -p "$TEMP_DIR/$name"
    
    case "$name" in
        "vga-service")
            cat > "$TEMP_DIR/$name/${binary}.c" << 'EOF'
#include <stdint.h>
#define VGA_MEMORY 0xB8000
static uint16_t* vga = (uint16_t*)VGA_MEMORY;
void puts(const char* s) { 
    static size_t pos = 0;
    while (*s && pos < 2000) vga[pos++] = *s++ | 0x0F00;
}
int main() { puts("VGA Service Ready"); while(1); }
EOF
            ;;
        "shell")
            cat > "$TEMP_DIR/$name/${binary}.c" << 'EOF'
#include <stdint.h>
void shell_main() {
    // Basic command interpreter placeholder
    while (1) __asm__("hlt");
}
int main() { shell_main(); return 0; }
EOF
            ;;
        "ipc-helper")
            cat > "$TEMP_DIR/$name/${binary}.c" << 'EOF'
#include <stdint.h>
int ipc_send(int pid, void* msg, size_t len) { return 0; }
int ipc_recv(void* buf, size_t len) { return 0; }
int main() { return 0; }
EOF
            ;;
        *)
            cat > "$TEMP_DIR/$name/${binary}.c" << EOF
// Placeholder for $name
int main() { return 0; }
EOF
            ;;
    esac
    
    # Create simple Makefile
    cat > "$TEMP_DIR/$name/Makefile" << EOF
${binary}: ${binary}.c
	gcc -o ${binary} ${binary}.c
clean:
	rm -f ${binary}
EOF
}

main() {
    echo "📥 Cloning E-comOS userspace components..."
    
    if [[ ! -f "$CONFIG_FILE" ]]; then
        echo "❌ Config file not found: $CONFIG_FILE"
        exit 1
    fi
    
    mkdir -p "$TEMP_DIR"
    parse_config
    
    echo "✅ Component cloning complete"
}

main "$@"