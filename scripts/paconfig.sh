#!/bin/bash

newSourceName="echo_cancel_source"
newSinkName="echo_cancel_sink"

# Reload "module-echo-cancel"
echo Reload \"module-echo-cancel\"
pactl unload-module module-echo-cancel 2>/dev/null
if pactl load-module module-echo-cancel aec_method=webrtc source_name=$newSourceName sink_name=$newSinkName; then
	# Set a new default source and sink, if module-echo-cancel has loaded successfully.
	pacmd set-default-source $newSourceName
	pacmd set-default-sink $newSinkName
fi

