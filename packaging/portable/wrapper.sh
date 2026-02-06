#!/bin/bash
# Launcher script for portable ATLabelMaster distribution
# This script ensures the application finds its assets directory

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set assets directory to the portable location
export LABELMASTER_ASSETS_DIR="${SCRIPT_DIR}/assets"

# Launch the application
exec "${SCRIPT_DIR}/LabelMaster" "$@"
