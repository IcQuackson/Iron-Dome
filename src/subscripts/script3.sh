# script3.sh
#!/bin/bash

echo "script3.sh received paths: $@"
for path in "$@"; do
    echo "Will monitor: $path"
    # ...
done

# (some infinite loop or logic to monitor those paths)
while true; do
    sleep 5
done
