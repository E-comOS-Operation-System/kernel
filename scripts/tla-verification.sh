#!/bin/bash
# TLA+ Verification Script (Pure Shell)

set -e

echo "🔬 TLA+ Formal Verification for E-comOS Microkernel"
echo "===================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Timestamp for report
TIMESTAMP=$(date +"%Y-%m-%d %H:%M:%S")

# Results file
REPORT_FILE="TLA_VERIFICATION_REPORT.md"

# Initialize report
cat > "$REPORT_FILE" << EOF
# E-comOS TLA+ Verification Report
**Generated**: $TIMESTAMP

## Verification Results

| Model | Status | Distinct States | Total States | Duration |
|-------|--------|----------------|--------------|----------|
EOF

# Function to verify a TLA+ model
verify_model() {
    local model_name=$1
    local config_file="specs/tla/$model_name.cfg"
    local tla_file="specs/tla/$model_name.tla"
    local output_file="specs/tla/$model_name.tla.out"
    
    echo -e "\n${YELLOW}Verifying $model_name model...${NC}"
    
    if [[ ! -f "$tla_file" ]]; then
        echo -e "${RED}❌ TLA file not found: $tla_file${NC}"
        return 1
    fi
    
    if [[ ! -f "$config_file" ]]; then
        echo -e "${RED}❌ Config file not found: $config_file${NC}"
        return 1
    fi
    
    # Start time
    local start_time=$(date +%s)
    
    # Run TLC model checker
    java -cp /opt/tla-tools/tla2tools.jar \
        tlc2.TLC -workers auto -config "$config_file" "$tla_file" > "$output_file" 2>&1 || true
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Analyze results
    if grep -q "Model checking completed" "$output_file"; then
        local distinct_states=$(grep "distinct states" "$output_file" | head -1 | sed 's/.*: //')
        local total_states=$(grep "distinct states" "$output_file" | head -1 | sed 's/.*: //')
        
        echo -e "${GREEN}✅ $model_name verification PASSED${NC}"
        echo -e "   States: $distinct_states, Duration: ${duration}s"
        
        # Add to report
        echo "| $model_name | ✅ PASS | $distinct_states | $total_states | ${duration}s |" >> "$REPORT_FILE"
        
        return 0
    else
        local error_line=$(grep -i "error:" "$output_file" | head -1 || echo "Unknown error")
        echo -e "${RED}❌ $model_name verification FAILED${NC}"
        echo -e "   Error: $error_line"
        
        # Add to report
        echo "| $model_name | ❌ FAIL | N/A | N/A | ${duration}s |" >> "$REPORT_FILE"
        
        return 1
    fi
}

# Function to generate summary
generate_summary() {
    local passed_count=0
    local failed_count=0
    local total_count=0
    
    # Count results
    if grep -q "✅ PASS" "$REPORT_FILE"; then
        passed_count=$(grep -c "✅ PASS" "$REPORT_FILE")
    fi
    
    if grep -q "❌ FAIL" "$REPORT_FILE"; then
        failed_count=$(grep -c "❌ FAIL" "$REPORT_FILE")
    fi
    
    total_count=$((passed_count + failed_count))
    
    # Add summary to report
    cat >> "$REPORT_FILE" << EOF

## Summary

- **Total Models**: $total_count
- **✅ Passed**: $passed_count
- **❌ Failed**: $failed_count
- **Success Rate**: $((total_count > 0 ? passed_count * 100 / total_count : 0))%

EOF

    echo -e "\n${GREEN}📊 Verification Summary:${NC}"
    echo -e "   Total: $total_count, Passed: $passed_count, Failed: $failed_count"
    
    if [[ $failed_count -gt 0 ]]; then
        echo -e "${RED}❌ Some verifications failed. Check the report for details.${NC}"
        return 1
    else
        echo -e "${GREEN}✅ All verifications passed!${NC}"
        return 0
    fi
}

# Main verification process
main() {
    echo "Starting TLA+ model verification..."
    
    # Verify each model
    verify_model "Scheduler"
    verify_model "IPC"
    
    # Generate final summary
    generate_summary
    
    echo -e "\n${GREEN}📄 Detailed report saved to: $REPORT_FILE${NC}"
    
    # Display report location
    if [[ -f "$REPORT_FILE" ]]; then
        echo -e "\nQuick Report:"
        grep "|" "$REPORT_FILE" | grep -v "|---"
    fi
}

# Run main function
main "$@"
