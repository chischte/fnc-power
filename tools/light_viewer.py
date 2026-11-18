"""Local KinCony viewer; network work never blocks Tk's event loop."""
import concurrent.futures
import json
import queue
import socket
import struct
import ipaddress
import re
from pathlib import Path
import tkinter as tk
from tkinter import ttk, messagebox
import urllib.request
from zeroconf import Zeroconf, ServiceBrowser, ServiceListener

ROOT = Path(__file__).resolve().parents[1]
SERVICE = "_fnc-light._tcp.local."
DEVICE_FILE = ROOT / "viewer.devices.local.json"
DEFAULT_DEVICES = [("fnc-licht-1.local", "192.168.31.182"), ("fnc-licht-2.local", None)]


# Keep the established fnc-fogger dark palette consistent across both viewers.
THEME = {
    "page": "#171e1b", "card": "#242e29", "plot": "#1d2621",
    "grid": "#3c4b43", "text": "#e6eee9", "muted": "#adbbb3",
    "selected": "#78c79b", "button_on": "#638372", "button_off": "#3c4b43",
}


def configure_theme(window):
    window.configure(bg=THEME["page"])
    style = ttk.Style(window)
    style.theme_use("clam")
    style.configure("TFrame", background=THEME["card"])
    style.configure("Page.TFrame", background=THEME["page"])
    style.configure("TLabel", background=THEME["card"], foreground=THEME["text"], font=("Segoe UI", 11))
    style.configure("Muted.TLabel", foreground=THEME["muted"], font=("Segoe UI", 10))
    style.configure("Accent.TLabel", foreground=THEME["selected"])
    style.configure("Page.TLabel", background=THEME["page"])
    style.configure("TButton", background=THEME["button_off"], foreground=THEME["text"],
                    borderwidth=0, padding=(14, 10), font=("Segoe UI", 10))
    style.map("TButton", background=[("active", THEME["button_on"])])
    style.configure("Primary.TButton", background=THEME["button_on"], foreground="#ffffff")
    for widget in ("TEntry", "TSpinbox"):
        style.configure(widget, fieldbackground=THEME["plot"], background=THEME["grid"],
                        foreground=THEME["text"], insertcolor=THEME["text"],
                        bordercolor=THEME["grid"], lightcolor=THEME["grid"],
                        darkcolor=THEME["grid"], padding=9, font=("Segoe UI", 11))
    style.configure("TSeparator", background=THEME["grid"])
    style.configure("Light.Horizontal.TProgressbar", background=THEME["selected"],
                    troughcolor=THEME["plot"], borderwidth=0, lightcolor=THEME["selected"],
                    darkcolor=THEME["selected"], thickness=6)


def section(frame, title):
    ttk.Separator(frame).pack(fill="x", pady=(18, 16))
    ttk.Label(frame, text=title, style="Muted.TLabel").pack(anchor="w", pady=(0, 8))
    row = ttk.Frame(frame)
    row.pack(fill="x")
    return row


def request(host, command=None, value=""):
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    headers = {}
    if command:
        config = json.loads((ROOT / "network.local.json").read_text(encoding="utf-8"))
        headers = {"X-OTA-Password": config["ota_password"], "X-Command": command, "X-Value": value}
    url = "http://" + host + ("/api/control" if command else "/api/status")
    req = urllib.request.Request(url, data=b"" if command else None, headers=headers)
    with opener.open(req, timeout=3) as response:
        data = response.read(8192)
    if command:
        return data.decode()
    status = json.loads(data)
    status["_address"] = socket.gethostbyname(host)
    return status


def discover_local_devices(seed, events):
    """Legacy unicast mDNS also works when Windows blocks multicast replies."""
    try:
        address = socket.gethostbyname(seed)
        network = ipaddress.ip_network(address + "/24", strict=False)
        if not network.is_private:
            return
        question = b"".join(bytes([len(part)]) + part.encode() for part in SERVICE.rstrip(".").split("."))
        packet = struct.pack("!6H", 7, 0, 1, 0, 0, 0) + question + b"\0" + struct.pack("!HH", 12, 1)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as connection:
            connection.settimeout(2)
            for host in network.hosts():
                connection.sendto(packet, (str(host), 5353))
            found = set()
            while True:
                try:
                    data, source = connection.recvfrom(4096)
                except socket.timeout:
                    return
                host = source[0]
                if host in found or b"_fnc-light" not in data or source[1] != 5353:
                    continue
                found.add(host)
                try:
                    status = request(host)
                    events.put(("device", host, status["hostname"] + ".local"))
                except (OSError, ValueError, KeyError):
                    continue
    except OSError:
        return


class Discovery(ServiceListener):
    def __init__(self, events):
        self.events = events

    def add_service(self, zeroconf, service_type, name):
        info = zeroconf.get_service_info(service_type, name)
        if info:
            addresses = info.parsed_scoped_addresses()
            if addresses:
                ipv4 = next((address for address in addresses if ":" not in address), None)
                if ipv4:
                    self.events.put(("device", ipv4, info.server.rstrip(".")))

    update_service = add_service

    def remove_service(self, zeroconf, service_type, name):
        pass


class DevicePanel:
    def __init__(self, window, frame, hostname, address, pool, remember):
        self.window = window
        self.events = queue.Queue()
        self.pool = pool
        self.remember = remember
        self.pending = False
        self.devices = {hostname: address} if address else {}
        self.service = False
        self.sync_brightness = True
        self.host = tk.StringVar(value=hostname)
        self.on_time = tk.StringVar()
        self.off_time = tk.StringVar()
        self.name = tk.StringVar()
        self.renaming = False
        self.brightness = tk.IntVar(value=35)
        self.summary = tk.StringVar(value="Connecting ...")
        self.notice = tk.StringVar(value="")
        self.level_text = tk.StringVar(value="-- %")
        self.mode_text = tk.StringVar(value="connecting")
        self.level = tk.DoubleVar(value=0)
        self.build_hostname_editor(frame)
        ttk.Label(frame, textvariable=self.mode_text, style="Accent.TLabel").pack(anchor="w", pady=(6, 18))
        ttk.Label(frame, text="CURRENT BRIGHTNESS", style="Muted.TLabel").pack(anchor="w")
        ttk.Label(frame, textvariable=self.level_text, font=("Segoe UI", 34, "bold")).pack(anchor="w")
        ttk.Progressbar(frame, variable=self.level, maximum=100, style="Light.Horizontal.TProgressbar").pack(fill="x", pady=(4, 12))
        ttk.Label(frame, textvariable=self.summary, style="Muted.TLabel", justify="left").pack(anchor="w")
        self.build_controls(frame)
        ttk.Label(frame, textvariable=self.notice, style="Muted.TLabel", wraplength=400).pack(anchor="w", pady=(12, 0))
        window.after(100, self.drain)
        self.poll()

    def build_controls(self, frame):
        row = section(frame, "BRIGHTNESS")
        self.spin = ttk.Spinbox(row, from_=0, to=100, textvariable=self.brightness, width=5, justify="center")
        self.spin.pack(side="left")
        ttk.Label(row, text="%", style="Muted.TLabel").pack(side="left", padx=8)
        ttk.Button(row, text="Save", style="Primary.TButton", command=self.save_brightness).pack(side="left", padx=(4, 8))
        self.reset_button = ttk.Button(row, text="Default 35 %", command=lambda: self.command("reset"))
        self.reset_button.pack(side="left")
        row = section(frame, "SCHEDULE")
        for label, variable in (("On", self.on_time), ("Off", self.off_time)):
            ttk.Label(row, text=label).pack(side="left", padx=(0, 8))
            ttk.Entry(row, textvariable=variable, width=6, justify="center").pack(side="left", padx=(0, 16))
        ttk.Button(row, text="Save", style="Primary.TButton", command=self.save_schedule).pack(side="left")
        row = section(frame, "SERVICE LIGHT")
        self.service_button = ttk.Button(row, text="Start service light", command=self.toggle_service)
        self.service_button.pack(side="left")
        ttk.Label(row, text="Ends automatically after 10 minutes", style="Muted.TLabel").pack(side="left", padx=12)

    def build_hostname_editor(self, frame):
        header = ttk.Frame(frame)
        header.pack(fill="x", anchor="w")
        self.hostname_label = ttk.Label(header, textvariable=self.host,
                                        font=("Segoe UI", 20, "bold"), cursor="hand2")
        self.hostname_label.pack(side="left")
        self.hostname_label.bind("<Button-1>", self.edit_hostname)
        self.hostname_editor = ttk.Frame(header)
        self.hostname_entry = ttk.Entry(self.hostname_editor, textvariable=self.name, width=14, justify="center",
                                        font=("Segoe UI", 20, "bold"))
        self.hostname_entry.pack(side="left")
        ttk.Label(self.hostname_editor, text=".local", font=("Segoe UI", 20, "bold")).pack(side="left")
        self.hostname_entry.bind("<Return>", self.rename)
        self.hostname_entry.bind("<Escape>", self.cancel_hostname_edit)
        self.hostname_entry.bind("<FocusOut>", self.rename)

    def edit_hostname(self, event=None):
        if self.renaming:
            return
        self.renaming = True
        self.name.set(self.host.get().removesuffix(".local"))
        self.hostname_label.pack_forget()
        self.hostname_editor.pack(side="left")
        self.hostname_entry.focus_set()
        self.hostname_entry.selection_range(0, "end")

    def cancel_hostname_edit(self, event=None):
        self.renaming = False
        self.hostname_editor.pack_forget()
        self.hostname_label.pack(side="left")

    def submit(self, kind, host, command=None, value=""):
        future = self.pool.submit(request, self.devices.get(host, host), command, value)
        def completed(result):
            try:
                self.events.put((kind, host, result.result()))
            except Exception as error:
                self.events.put(("error", host, str(error)))
        future.add_done_callback(completed)

    def refresh(self):
        if not self.pending:
            self.pending = True
            self.submit("status", self.host.get().strip())

    def poll(self):
        self.refresh()
        self.window.after(3000, self.poll)

    def command(self, command, value=""):
        self.notice.set("Saving ...")
        self.submit("saved", self.host.get().strip(), command, value)

    def rename(self, event=None):
        if not self.renaming:
            return
        name = self.name.get().strip()
        if not re.fullmatch(r"[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?", name):
            # End editing before opening a modal to avoid recursive FocusOut events.
            self.cancel_hostname_edit()
            messagebox.showerror("Invalid name", "Use 1-63 lowercase letters, numbers or hyphens. The name must start and end with a letter or number.", parent=self.window)
            self.edit_hostname()
            self.name.set(name)
            return
        self.cancel_hostname_edit()
        if name + ".local" != self.host.get():
            self.command("name", name)

    def save_brightness(self):
        try:
            value = self.brightness.get()
        except tk.TclError:
            messagebox.showerror("Brightness", "Enter a whole percentage.")
            return
        self.command("brightness", str(value))

    def save_schedule(self):
        on, off = self.on_time.get().strip(), self.off_time.get().strip()
        if not all(re.fullmatch(r"(?:[01][0-9]|2[0-3]):[0-5][0-9]", value) for value in (on, off)):
            messagebox.showerror("Schedule", "Enter valid times in HH:MM format.")
            return
        self.command("schedule", on + "," + off)

    def register_device(self, hostname, address):
        previous = [name for name, ip in self.devices.items() if ip == address and name != hostname]
        selected = self.host.get() in previous
        for name in previous:
            del self.devices[name]
        self.devices[hostname] = address

        if selected or self.host.get() not in self.devices:
            self.host.set(hostname)
        self.remember(self.host.get(), self.devices.get(self.host.get()))

    def toggle_service(self):
        self.command("service", "off" if self.service else "on")

    def drain(self):
        while not self.events.empty():
            kind, host, data = self.events.get_nowait()
            if kind == "device":
                self.register_device(data, host)
                continue
            self.pending = False
            if host != self.host.get().strip():
                continue
            if kind == "error":
                self.notice.set("No connection. Check device power and network.")
                self.summary.set("")
                self.mode_text.set("offline")
                self.level_text.set("-- %")
                self.level.set(0)
            elif kind == "saved":
                self.sync_brightness = True
                self.notice.set("Saved on the device.")
                self.submit("status", host)
            else:
                self.show_status(data)
        self.window.after(100, self.drain)

    def show_status(self, status):
        address = status.get("_address") or self.devices.get(self.host.get(), "unknown")
        self.register_device(status["hostname"] + ".local", address)
        mode = "service light" if status["service"] else "always off" if status["disabled"] else "automatic mode"
        time = status["time"] if status["clockValid"] else "Clock invalid"
        self.mode_text.set("connected | " + mode)
        self.level_text.set(f"{status['output']:.0f} %")
        self.level.set(status["output"])
        self.summary.set(f"Device time {time}  |  MAC {status.get('mac', 'unavailable')}  |  IP {address}"
                         + ("\nHardware error" if not status['hardwareOk'] or not status['outputOk'] else ""))
        if not self.name.get():
            self.name.set(status["hostname"])
            self.brightness.set(status["brightness"])
        if self.sync_brightness:
            self.brightness.set(status["brightness"])
            self.on_time.set(status["on"])
            self.off_time.set(status["off"])
            self.sync_brightness = False
        self.reset_button.configure(text=f"Default {status['default']} %")
        self.spin.configure(to=status["maximum"])
        self.service = status["service"]
        self.service_button.configure(text="Stop service light" if self.service else "Start service light")


class Viewer:
    def __init__(self, window):
        self.window = window
        self.events = queue.Queue()
        self.pool = concurrent.futures.ThreadPoolExecutor(max_workers=5)
        window.title("FNC Light")
        window.geometry("980x660")
        window.minsize(980, 660)
        window.iconbitmap(str(ROOT / "assets" / "light-aligned.ico"))
        configure_theme(window)
        content = ttk.Frame(window, style="Page.TFrame", padding=(20, 16, 20, 20))
        content.pack(fill="both", expand=True)
        self.panels = []
        self.records = list(DEFAULT_DEVICES)
        try:
            records = json.loads(DEVICE_FILE.read_text(encoding="utf-8"))
            if len(records) == 2 and all(isinstance(name, str) and name.endswith(".local")
                                        and (address is None or isinstance(address, str)) for name, address in records):
                self.records = records
        except (OSError, ValueError, TypeError):
            pass
        for index, (hostname, address) in enumerate(self.records):
            frame = ttk.Frame(content, padding=24)
            frame.grid(row=0, column=index, sticky="nsew", padx=8)
            content.columnconfigure(index, weight=1, uniform="devices")
            self.panels.append(DevicePanel(window, frame, hostname, address, self.pool,
                                           lambda name, ip, index=index: self.remember(index, name, ip)))
        content.rowconfigure(0, weight=1)
        self.zeroconf = Zeroconf()
        self.browser = ServiceBrowser(self.zeroconf, SERVICE, Discovery(self.events))
        window.protocol("WM_DELETE_WINDOW", self.close)
        window.after(100, self.drain)
        self.discover()

    def remember(self, index, hostname, address):
        record = (hostname, address)
        if tuple(self.records[index]) == record:
            return
        self.records[index] = record
        try:
            temporary = DEVICE_FILE.with_suffix(".tmp")
            temporary.write_text(json.dumps(self.records, indent=2), encoding="utf-8")
            temporary.replace(DEVICE_FILE)
        except OSError:
            self.panels[index].notice.set("Could not save the device assignment locally.")

    def discover(self):
        self.pool.submit(discover_local_devices, self.panels[0].host.get(), self.events)
        self.window.after(30000, self.discover)

    def drain(self):
        while not self.events.empty():
            _, address, hostname = self.events.get_nowait()
            panel = next((panel for panel in self.panels
                          if hostname == panel.host.get() or address in panel.devices.values()), None)
            if panel is None:
                panel = self.panels[1]
            panel.register_device(hostname, address)
            if not panel.devices.get(panel.host.get()):
                panel.host.set(hostname)
            panel.refresh()
        self.window.after(100, self.drain)

    def close(self):
        self.browser.cancel()
        self.zeroconf.close()
        self.pool.shutdown(wait=False, cancel_futures=True)
        self.window.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    Viewer(root)
    root.mainloop()
