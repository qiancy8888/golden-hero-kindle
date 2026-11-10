"""Exercise the KUAL guard with mock Kindle services using a POSIX shell."""
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest


SHELL = os.environ.get("PLATINUM_TEST_SHELL", "sh")
GUARD = Path(__file__).resolve().parents[1] / "kual/extensions/GoldenHeroFullscreen/guarded-test.sh"


class PlatinumGuardTest(unittest.TestCase):
    def launch(self, scenario, saver=0):
        with tempfile.TemporaryDirectory(prefix="platinum-guard-") as directory:
            root = Path(directory)

            def write(name, content, executable=False):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding="utf-8", newline="\n")
                if executable:
                    path.chmod(0o755)

            guard = GUARD.read_text(encoding="utf-8")
            guard = (guard.replace("/mnt/us", "device")
                     .replace("/etc/prettyversion.txt", "prettyversion.txt")
                     .replace("/proc/fb", "fb")
                     .replace("/tmp/golden-hero-kpw1-session.lock", "session.lock")
                     .replace("for base in /bin /usr/bin /sbin /usr/sbin;", "for base in mocks;")
                     .replace("watchdog_sleep 5\n", "watchdog_sleep 0.1\n")
                     .replace("watchdog_sleep 3\n", "watchdog_sleep 0.1\n")
                     .replace("\nsleep 2\n", "\n"))
            write("guard.sh", guard)
            write("prettyversion.txt", "Kindle 5.6.1.1\n")
            write("fb", "0 mxc_epdc_fb\n")
            write("mocks/killall", '#!/bin/sh\nprintf "wm %s\n" "$*" >> operations.log\n', True)
            write("mocks/lipc-get-prop", f"#!/bin/sh\necho {saver}\n", True)
            write("mocks/lipc-set-prop", '#!/bin/sh\nprintf "lipc %s\n" "$*" >> operations.log\n', True)
            pulses = ('for i in 1 2 3 4 5 6 7 8 9 10 11 12; do\n'
                      '  printf "%s\\n" "$i" > "$4"\n  sleep 0.09\ndone\n')
            bodies = {
                "healthy": pulses + 'echo "Mock game finished normally."\n',
                "stalled": 'trap "" 15\nprintf "1\\n" > "$4"\nwhile :; do sleep 0.05; done\n',
                "orphan": 'kill -KILL "$(cat session.lock/pid)" || exit 2\n' + pulses,
            }
            write("device/platinum_hero/platinum_hero",
                  '#!/bin/sh\n[ "$1" != --verify-rom ] || exit 0\n[ "$#" = 4 ] || exit 2\n' + bodies[scenario], True)

            result = subprocess.run([SHELL, "guard.sh", "platinum"], cwd=root, capture_output=True,
                                    text=True, encoding="utf-8", timeout=20)
            if scenario == "orphan":
                deadline = time.monotonic() + 10
                while (root / "session.lock").exists() and time.monotonic() < deadline:
                    time.sleep(0.05)
            log = (root / "device/platinum_hero/logs/platinum.log").read_text(encoding="utf-8")
            operations = (root / "operations.log").read_text(encoding="utf-8").splitlines()
            self.assertFalse((root / "session.lock").exists(), log)
            self.assertLess(operations.index("wm -STOP awesome"), operations.index("wm -CONT awesome"))
            power = "lipc com.lab126.powerd preventScreenSaver"
            self.assertEqual([line for line in operations if line.startswith(power)],
                             [f"{power} 1", f"{power} 0"] if saver == 0 else [])
            return result.returncode, log

    def test_healthy_game_survives_watchdog_window(self):
        status, log = self.launch("healthy", saver=1)
        self.assertEqual(status, 0, log)
        self.assertIn("Mock game finished normally.", log)
        self.assertNotIn("heartbeat stopped", log)

    def test_stalled_game_recovers_platform(self):
        status, log = self.launch("stalled")
        self.assertNotEqual(status, 0, log)
        self.assertIn("Watchdog: game heartbeat stopped.", log)
        self.assertIn("UI restoration attempted.", log)

    def test_watchdog_recovers_if_launcher_dies(self):
        status, log = self.launch("orphan")
        self.assertNotEqual(status, 0, log)
        self.assertIn("Watchdog: KUAL launcher exited unexpectedly.", log)


if __name__ == "__main__":
    unittest.main()
