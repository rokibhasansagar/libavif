#!/bin/bash

# Escape codes
PLR="\033[38;5;161m"
GRY="\033[38;5;248m"
BLD="\033[1m"
YLW="\033[33m"
RST="\033[0m"

# Function to display usage information
show_usage() {
    echo -e "${BLD}oavif.sh${RST} | Optimized AVIF encoding based on your input\n"
    echo -e "${GRY}Usage${RST}:\n\t$0 -i <${YLW}input${RST}> -o <${YLW}output${RST}> [${GRY}-q <crf>${RST}] [${GRY}-s <speed>${RST}]\n"
    echo -e "${GRY}Options${RST}:"
    echo -e "\t-i <input>\tInput video file"
    echo -e "\t-o <output>\tOutput video file"
    echo -e "\t-q <crf>\tEncoding CRF (0-63; default: 32)"
    echo -e "\t-s <speed>\tCompression effort (0-8; default: 2)"
    exit 1
}

# SVT-AV1 encode (for even width & height)
encode_avifenc_svt() {
    local input=$1
    local output=$2
    local speed=$3
    local crf=$4
    # echo -e "Encoding with SVT-AV1..."
    avifenc -s "$speed" -c svt -d 10 -y 420 \
        -a "crf=$crf" -a tune=4 \
        "$input" -o "$output"
}

# AOM encode (for odd width & height)
encode_avifenc_aom() {
    local input=$1
    local output=$2
    local speed=$3
    local crf=$4
    # echo -e "Encoding with aomenc..."
    avifenc -s "$speed" -j all -d 10 -y "$aomyuv" \
        --min 0 --max 63 \
        --minalpha 0 --maxalpha 63 \
        -a end-usage=q \
        -a "cq-level=$crf" \
        -a tune=ssim -a tune-content=default \
        -a deltaq-mode=3 -a enable-qm=1 \
        -a sb-size=dynamic -a aq-mode=0 \
        "$input" -o "$output"
}

# Function to encode video
encode_image() {
    local input=$1
    local output=$2
    local speed=$3
    local crf=$4

    image_info=$(identify -format "%w %h %[channels] %k" "$input")
    width=$(echo "$image_info" | cut -d' ' -f1)
    height=$(echo "$image_info" | cut -d' ' -f2)
    channels=$(echo "$image_info" | cut -d' ' -f3)
    colors=$(echo "$image_info" | cut -d' ' -f4)

    # If there's gray channel or unique colors is less/equal to 256, use YUV420 is aom instead of YUV444
    if [[ $channels == *"gray"* ]] || [[ $colors -le 256 ]]; then
        local aomyuv=420
    else
        local aomyuv=444
    fi

    if [[ $channels == *"bgra"* ]] || [[ $channels == *"rgba"* ]]; then
        # aomenc for images with alpha channel
        echo -e "Alpha channel detected, encoding with aomenc..."
        encode_avifenc_aom "$input" "$output" "$speed" "$crf" "$aomyuv"
    elif [ $((width % 2)) -eq 0 ] && [ $((height % 2)) -eq 0 ]; then
        # SVT-AV1 for even width & height
        echo -e "Encoding with SVT-AV1..."
        encode_avifenc_svt "$input" "$output" "$speed" "$crf"
    else
        # aomenc for odd width & height
        echo -e "Odd width or height detected, encoding with aomenc..."
        encode_avifenc_aom "$input" "$output" "$speed" "$crf" "$aomyuv"
    fi
}

# Set defaults
crf=32
speed=2

# Parse command line arguments
while getopts ":i:o:q:s:h" opt; do
    case ${opt} in
        i ) input=$OPTARG ;;
        o ) output=$OPTARG ;;
        q ) crf=$OPTARG ;;
        s ) speed=$OPTARG ;;
        h ) show_usage ;;
        \? ) show_usage ;;
    esac
done

# Check for required arguments
if [ -z "$input" ] || [ -z "$output" ]; then
    show_usage
fi

# Validate input file
if [ ! -f "$input" ]; then
    echo -e "${GRY}Error: Input file not found${RST}"
    exit 1
fi

# Encode image
if encode_image "$input" "$output" "$speed" "$crf"; then
    input_size=$(du -h "$input" | awk '{print $1}')
    outpt_size=$(du -h "$output" | awk '{print $1}')
    echo -e "${YLW}$input${RST} (${GRY}$input_size${RST}) -> ${YLW}$output${RST} (${GRY}$outpt_size${RST}) | ${PLR}CRF ${crf} Speed ${speed}${RST}"
else
    echo -e "${GRY}Error: Encoding failed${RST}"
    exit 1
fi

