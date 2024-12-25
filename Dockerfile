FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    tini \
	supervisor \
    --no-install-recommends

RUN mkdir -p /var/log/irondome/ /etc/supervisor/conf.d

RUN pip3 install psutil inotify_simple watchdog

RUN apt-get clean && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/usr/bin/tini", "--"]

CMD ["supervisord", "-c", "/etc/supervisor/supervisord.conf"]