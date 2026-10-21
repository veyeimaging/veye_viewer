#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_JSON="$SCRIPT_DIR/auto_camera_index.json"
TMP_ENTRIES="$OUTPUT_JSON.entries.$$"
TMP_OUTPUT="$OUTPUT_JSON.tmp.$$"

trap 'rm -f "$TMP_ENTRIES" "$TMP_OUTPUT"' EXIT
: > "$TMP_ENTRIES"

shopt -s nullglob
media_nodes=(/dev/media*)

for media in "${media_nodes[@]}"; do
    if ! media_output=$(media-ctl -d "$media" -p); then
        echo "Error: failed to read media topology from $media" >&2
        exit 1
    fi

    if ! printf '%s\n' "$media_output" | awk -v media_node="$media" '
        function extract(text, pattern) {
            if (match(text, pattern)) {
                return substr(text, RSTART, RLENGTH)
            }
            return ""
        }

        function entity_name(block, lines, count, i, name) {
            count = split(block, lines, "\n")
            for (i = 1; i <= count; i++) {
                if (lines[i] ~ /entity [0-9]+:/) {
                    name = extract(lines[i], "entity [0-9]+: [^(]+ [(]")
                    sub(/^entity [0-9]+: /, "", name)
                    sub(/ [(]$/, "", name)
                    return name
                }
            }
            return ""
        }

        BEGIN {
            RS = ""
            sensor_count = 0
            video_count = 0
        }

        /subtype Sensor/ {
            sensor_name = entity_name($0)

            i2c_bus = extract(sensor_name, "[0-9]+-[0-9a-fA-F]+")
            sub(/-.*/, "", i2c_bus)
            subdev_node = extract($0, "/dev/v4l-subdev[0-9]+")

            if (sensor_name != "" && i2c_bus != "") {
                sensors[sensor_count] = sensor_name
                sensor_buses[sensor_count] = i2c_bus
                sensor_subdevs[sensor_count] = subdev_node
                sensor_count++
            }
        }

        /type Node subtype V4L/ {
            video_entity = entity_name($0)
            video_node = extract($0, "/dev/video[0-9]+")

            if (video_entity != "" && video_node != "") {
                videos[video_count] = video_node
                video_entities[video_count] = video_entity
                video_count++
            }
        }

        END {
            for (v = 0; v < video_count; v++) {
                for (s = 0; s < sensor_count; s++) {
                    if (index(video_entities[v], sensors[s]) > 0) {
                        printf("{\"media_node\":\"%s\",\"video_node\":\"%s\",\"video_subnode\":\"%s\",\"media_entity_name\":\"%s\",\"i2c_bus\":\"%s\"}\n",
                            media_node, videos[v], sensor_subdevs[s], sensors[s], sensor_buses[s])
                    }
                }
            }
        }
    ' >> "$TMP_ENTRIES"; then
        echo "Error: failed to parse media topology from $media" >&2
        exit 1
    fi
done

{
    echo "["
    if [[ -s "$TMP_ENTRIES" ]]; then
        sed '$!s/$/,/' "$TMP_ENTRIES"
    fi
    echo "]"
} > "$TMP_OUTPUT"

mv "$TMP_OUTPUT" "$OUTPUT_JSON"
echo "Done --> $OUTPUT_JSON"
