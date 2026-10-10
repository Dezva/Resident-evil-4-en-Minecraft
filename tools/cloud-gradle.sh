#!/usr/bin/env bash
# Cloud helper: use the provided HTTPS proxy and the machine's trusted CA store.
# Windows users run gradlew.bat directly; no cloud proxy settings are committed to Gradle.
set -euo pipefail
cd "$(dirname "$0")/.."
export JAVA_HOME="${JAVA_HOME:-/workspace/tools/jdk-21.0.8+9}"
export GRADLE_USER_HOME="${GRADLE_USER_HOME:-/workspace/.cache/gradle}"
proxy_options=$(python3 - <<'PY'
import os, re, urllib.parse
proxy = os.environ.get('HTTPS_PROXY') or os.environ.get('https_proxy')
if proxy:
    p = urllib.parse.urlsplit(proxy)
    if p.username or p.password:
        raise SystemExit('An authenticated proxy needs a supported credential binding')
    if not p.hostname or not re.fullmatch(r'[A-Za-z0-9.-]+', p.hostname):
        raise SystemExit('Invalid proxy hostname')
    port = p.port or 80
    print(f'-Dhttps.proxyHost={p.hostname} -Dhttps.proxyPort={port} '
          f'-Dhttp.proxyHost={p.hostname} -Dhttp.proxyPort={port} '
          '-Dhttp.nonProxyHosts=localhost|127.*')
PY
)
if [ -n "$proxy_options" ]; then
    export GRADLE_OPTS="${GRADLE_OPTS:-} $proxy_options"
    if [ -f /etc/ssl/certs/java/cacerts ]; then
        export GRADLE_OPTS="$GRADLE_OPTS -Djavax.net.ssl.trustStore=/etc/ssl/certs/java/cacerts"
    fi
fi
exec ./gradlew "$@"
