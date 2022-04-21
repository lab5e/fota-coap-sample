#!/usr/bin/bash

# Launch the report-and-possibly-download every 30 seconds
while /bin/true; do
    ./fota-sample
    # If there is a new image it will be downloaded to image.new. Rename to fota-sample
    # to use the new version.
    if [ -f image.new ]; then
        echo "**** A new version is available!"
        mv fota-sample fota-sample.old
        mv image.new fota-sample
    fi
    sleep 30
done
