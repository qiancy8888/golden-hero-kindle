#!/usr/bin/env python3
"""Exercise the production standalone guard with a fake Kindle filesystem."""
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
GUARD = ROOT / "kual/extensions/GoldenHeroFullscreen/guarded-test.sh"


def script(path, body):
    path.write_text("#!/bin/sh\n" + body, encoding="utf-8", newline="\n")
    path.chmod(0o755)


def main():
    with tempfile.TemporaryDirectory(prefix="golden-guard-") as directory:
        mock = Path(directory)
        disk = mock / "usb"
        app = disk / "golden_hero_standalone"
        app.mkdir(parents=True)
        tools = mock / "bin"
        tools.mkdir()
        lock = mock / "lock"
        events = mock / "events"
        script(tools / "killall", 'printf "%s\\n" "$1" >> "$GOLDEN_TEST_EVENTS"\n')
        script(tools / "lipc-set-prop", "exit 0\n")
        (mock / "version").write_text("Kindle 5.6.1.1\n", encoding="utf-8")
        (mock / "fb").write_text("0 mxc_epdc_fb\n", encoding="utf-8")
        guard = mock / "guard.sh"
        guard.write_text(
            GUARD.read_text(encoding="utf-8").replace("/mnt/us", str(disk))
            .replace("/etc/prettyversion.txt", str(mock / "version"))
            .replace("/proc/fb", str(mock / "fb"))
            .replace("/tmp/golden-hero-kpw1-session.lock", str(lock))
            .replace("/bin /usr/bin /sbin /usr/sbin", str(tools))
            .replace("sleep 2\n", "sleep .04\n").replace("sleep 4\n", "sleep .2\n")
            .replace("sleep 1\n", "sleep .1\n"),
            encoding="utf-8",
        )
        command = ["/bin/sh", str(guard), "standalone"]
        env = dict(os.environ, GOLDEN_TEST_EVENTS=str(events))
        exe = app / "golden_hero_standalone"
        log = app / "logs/standalone.log"

        script(exe, """set -eu
i=0
while [ "$i" -lt 35 ]; do
    printf '%s\\n' "$i" > "$GOLDEN_HERO_HEARTBEAT"
    i=$((i + 1))
    sleep .03
done
""")
        owner = subprocess.Popen(command, env=env)
        try:
            deadline = time.monotonic() + 3
            while not events.exists():
                assert time.monotonic() < deadline, "guard never paused the UI"
                time.sleep(.01)
            competitor = subprocess.run(command, env=env, timeout=4, check=False)
            assert competitor.returncode == 9
            assert owner.wait(timeout=5) == 0
        finally:
            if owner.poll() is None:
                owner.terminate()
                owner.wait(timeout=4)
        assert events.read_text(encoding="utf-8").splitlines() == ["-STOP", "-CONT"]
        assert not lock.exists()
        assert "watchdog: launcher gone" not in log.read_text(encoding="utf-8")

        events.write_text("", encoding="utf-8")
        script(exe, 'while sleep .1; do :; done\n')
        stuck = subprocess.run(command, env=env, timeout=5, check=False)
        assert stuck.returncode == 143
        assert events.read_text(encoding="utf-8").splitlines() == ["-STOP", "-CONT"]
        assert not lock.exists()
        assert "watchdog: launcher gone" in log.read_text(encoding="utf-8")
    print("Standalone guard checks passed.")


if __name__ == "__main__":
    main()
