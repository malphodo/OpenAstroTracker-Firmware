#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OpenAstroTracker — harness de test série (protocole Meade / LX200 étendu OAT).

Utilise le port COM à la demande, envoie des commandes documentées dans
src/MeadeCommandProcessor.cpp et affiche les réponses.

Important pour SKR / OATControl : DTR désactivé par défaut (comme le firmware attend).
"""

from __future__ import annotations

import sys
import time
import threading
import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox

try:
    import serial
    import serial.tools.list_ports
except ImportError as e:
    print("Installez pyserial : pip install pyserial", file=sys.stderr)
    raise SystemExit(1) from e


DEFAULT_BAUD = 19200


def list_com_ports() -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    for p in serial.tools.list_ports.comports():
        desc = p.description or ""
        out.append((p.device, f"{p.device} — {desc}"))
    return out


class SerialSession:
    """Session série minimaliste (bloquante, appelée depuis un thread worker)."""

    def __init__(self) -> None:
        self.ser: serial.Serial | None = None

    def open(
        self,
        port: str,
        baud: int = DEFAULT_BAUD,
        dtr: bool = False,
        rts: bool = False,
    ) -> None:
        self.close()
        self.ser = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.05,
            write_timeout=2,
            dsrdtr=False,
        )
        self.ser.dtr = dtr
        self.ser.rts = rts

    def close(self) -> None:
        if self.ser and self.ser.is_open:
            try:
                self.ser.close()
            except Exception:
                pass
        self.ser = None

    @property
    def is_open(self) -> bool:
        return self.ser is not None and self.ser.is_open

    def send_raw(self, data: bytes) -> None:
        if not self.ser or not self.ser.is_open:
            raise RuntimeError("Port fermé")
        self.ser.write(data)

    def read_response(self, timeout_sec: float) -> str:
        """Lit des octets jusqu'à '#' ou expiration du délai global."""
        if not self.ser or not self.ser.is_open:
            raise RuntimeError("Port fermé")
        deadline = time.monotonic() + timeout_sec
        buf = bytearray()
        while time.monotonic() < deadline:
            try:
                n = self.ser.in_waiting
                if n:
                    chunk = self.ser.read(n)
                    buf.extend(chunk)
                    if b"#" in buf:
                        break
            except Exception:
                break
            time.sleep(0.01)
        return buf.decode("ascii", errors="replace")


def normalize_cmd(cmd: str) -> str:
    c = cmd.strip()
    if not c.startswith(":"):
        c = ":" + c
    if not c.endswith("#"):
        c = c + "#"
    return c


class OatHarnessApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("OAT Test Harness — OpenAstroTracker (Meade/LX200)")
        self.geometry("980x720")
        self.minsize(860, 600)

        self.session = SerialSession()
        self._worker_lock = threading.Lock()

        self._build_ui()

    def _build_ui(self) -> None:
        top = ttk.Frame(self, padding=8)
        top.pack(fill=tk.X)

        ttk.Label(top, text="Port").grid(row=0, column=0, sticky=tk.W)
        self.combo_port = ttk.Combobox(top, width=28, state="readonly")
        self.combo_port.grid(row=0, column=1, padx=4)
        ttk.Button(top, text="Rafraîchir", command=self.refresh_ports).grid(row=0, column=2)

        ttk.Label(top, text="Vitesse").grid(row=0, column=3, padx=(16, 0))
        self.entry_baud = ttk.Entry(top, width=8)
        self.entry_baud.insert(0, str(DEFAULT_BAUD))
        self.entry_baud.grid(row=0, column=4, padx=4)

        self.var_dtr = tk.BooleanVar(value=False)
        ttk.Checkbutton(top, text="DTR", variable=self.var_dtr).grid(row=0, column=5, padx=4)
        self.var_rts = tk.BooleanVar(value=False)
        ttk.Checkbutton(top, text="RTS", variable=self.var_rts).grid(row=0, column=6, padx=4)

        self.var_init_on_connect = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            top, text="Envoyer :I# à la connexion", variable=self.var_init_on_connect
        ).grid(row=0, column=7, padx=8)

        self.btn_connect = ttk.Button(top, text="Connecter", command=self.toggle_connect)
        self.btn_connect.grid(row=0, column=8, padx=8)

        self.lbl_status = ttk.Label(top, text="Déconnecté", foreground="red")
        self.lbl_status.grid(row=1, column=0, columnspan=9, sticky=tk.W, pady=(4, 0))

        nb = ttk.Notebook(self)
        nb.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        self._tab_connection(nb)
        self._tab_status(nb)
        self._tab_coords(nb)
        self._tab_tracking(nb)
        self._tab_az_alt(nb)
        self._tab_focus(nb)
        self._tab_gyro(nb)
        self._tab_gps(nb)
        self._tab_home(nb)
        self._tab_hardware(nb)
        self._tab_expert(nb)

        log_fr = ttk.LabelFrame(self, text="Journal", padding=4)
        log_fr.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))
        self.txt_log = scrolledtext.ScrolledText(log_fr, height=10, state=tk.DISABLED, font=("Consolas", 9))
        self.txt_log.pack(fill=tk.BOTH, expand=True)

        self.refresh_ports()

    def log(self, msg: str) -> None:
        self.txt_log.configure(state=tk.NORMAL)
        self.txt_log.insert(tk.END, msg.rstrip() + "\n")
        self.txt_log.see(tk.END)
        self.txt_log.configure(state=tk.DISABLED)

    def refresh_ports(self) -> None:
        ports = list_com_ports()
        values = [p[1] for p in ports]
        self.combo_port["values"] = values
        if ports:
            self.combo_port.current(0)
        else:
            self.combo_port.set("")

    def _selected_port_device(self) -> str | None:
        sel = self.combo_port.get()
        for dev, label in list_com_ports():
            if label == sel:
                return dev
        return None

    def toggle_connect(self) -> None:
        if self.session.is_open:
            self.session.close()
            self.lbl_status.config(text="Déconnecté", foreground="red")
            self.btn_connect.config(text="Connecter")
            self.log("--- Port fermé ---")
            return
        dev = self._selected_port_device()
        if not dev:
            messagebox.showwarning("Port", "Choisissez un port COM.")
            return
        try:
            baud = int(self.entry_baud.get().strip())
        except ValueError:
            messagebox.showerror("Vitesse", "Nombre invalide pour le baud.")
            return
        try:
            self.session.open(dev, baud, dtr=self.var_dtr.get(), rts=self.var_rts.get())
            self.lbl_status.config(text=f"Connecté {dev} @ {baud} baud", foreground="green")
            self.btn_connect.config(text="Déconnecter")
            self.log(f"--- Connecté {dev} @ {baud} (DTR={self.var_dtr.get()}, RTS={self.var_rts.get()}) ---")
            if self.var_init_on_connect.get():
                self.run_cmd_async(":I#", 1.5, note="init contrôle série")
        except Exception as e:
            messagebox.showerror("Série", str(e))

    def run_cmd_async(self, cmd: str, timeout: float, note: str = "") -> None:
        def work() -> None:
            with self._worker_lock:
                try:
                    c = normalize_cmd(cmd)
                    self.after(0, lambda: self.log(f">> {c}" + (f"  ({note})" if note else "")))
                    if not self.session.is_open:
                        self.after(0, lambda: self.log("!! Port non connecté"))
                        return
                    self.session.ser.reset_input_buffer()
                    self.session.send_raw(c.encode("ascii"))
                    resp = self.session.read_response(timeout)
                    self.after(0, lambda r=resp: self.log(f"<< {r!r}" if r else "<< (vide)"))
                except Exception as ex:
                    self.after(0, lambda: self.log(f"!! Erreur: {ex}"))

        threading.Thread(target=work, daemon=True).start()

    def run_cmd_sync_worker(self, cmd: str, timeout: float) -> str:
        with self._worker_lock:
            c = normalize_cmd(cmd)
            if not self.session.is_open:
                raise RuntimeError("Port fermé")
            self.session.ser.reset_input_buffer()
            self.session.send_raw(c.encode("ascii"))
            return self.session.read_response(timeout)

    # --- tabs ---

    def _row_buttons(self, parent, title: str, items: list[tuple[str, str, float]]) -> ttk.Frame:
        fr = ttk.LabelFrame(parent, text=title, padding=6)
        r, c = 0, 0
        for cmd, label, tmo in items:
            b = ttk.Button(
                fr,
                text=label,
                width=28,
                command=lambda cc=cmd, tt=tmo, lb=label: self.run_cmd_async(cc, tt, lb),
            )
            b.grid(row=r, column=c, padx=4, pady=2, sticky=tk.W)
            c += 1
            if c > 2:
                c = 0
                r += 1
        return fr

    def _tab_connection(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="Connexion / ID")
        self._row_buttons(
            tab,
            "Identification & mode",
            [
                (":I#", "Mode contrôle série :I#", 1.5),
                (":GVN#", "Version :GVN#", 2.0),
                (":GVP#", "Produit :GVP#", 2.0),
                (":GX#", "État monture :GX#", 2.0),
                (":D#", "Slew actif? :D#", 2.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

    def _tab_status(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="État détaillé")
        self._row_buttons(
            tab,
            "Indicateurs",
            [
                (":GIS#", "Slew RA/DEC :GIS#", 2.0),
                (":GIT#", "Suivi actif :GIT#", 2.0),
                (":GIG#", "Guidage :GIG#", 2.0),
                (":Gr#", "Cible RA :Gr#", 2.0),
                (":Gd#", "Cible DEC :Gd#", 2.0),
                (":GR#", "RA courant :GR#", 2.0),
                (":GD#", "DEC courant :GD#", 2.0),
                (":GC#", "Format date :Gc#", 2.0),
                (":GL#", "Heure locale :GL#", 2.0),
                (":Ga#", "Heure 12h :Ga#", 2.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

    def _tab_coords(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="Coordonnées")
        f1 = ttk.LabelFrame(tab, text="Lecture", padding=6)
        f1.pack(fill=tk.X)
        for cmd, label in [
            (":Gr#", "Cible RA"),
            (":Gd#", "Cible DEC"),
            (":GR#", "RA courant"),
            (":GD#", "DEC courant"),
        ]:
            ttk.Button(f1, text=label, command=lambda c=cmd: self.run_cmd_async(c, 2.0)).pack(side=tk.LEFT, padx=4)

        f2 = ttk.LabelFrame(tab, text="Définir cible (exemples — ajuster)", padding=6)
        f2.pack(fill=tk.X, pady=8)
        ttk.Label(f2, text="RA :SrHH:MM:SS#").grid(row=0, column=0, sticky=tk.W)
        self.entry_sr = ttk.Entry(f2, width=18)
        self.entry_sr.insert(0, "Sr12:30:00")
        self.entry_sr.grid(row=0, column=1, padx=4)
        ttk.Button(f2, text="Envoyer :S", command=self._send_sr).grid(row=0, column=2)

        ttk.Label(f2, text="DEC :Sd±dd*mm:ss#").grid(row=1, column=0, sticky=tk.W, pady=4)
        self.entry_sd = ttk.Entry(f2, width=18)
        self.entry_sd.insert(0, "Sd+45*00:00")
        self.entry_sd.grid(row=1, column=1, padx=4)
        ttk.Button(f2, text="Envoyer :S", command=self._send_sd).grid(row=1, column=2)

        f3 = ttk.LabelFrame(tab, text="Sync", padding=6)
        f3.pack(fill=tk.X)
        ttk.Button(f3, text="Sync sur cible :CM#", command=lambda: self.run_cmd_async(":CM#", 2.0)).pack(
            side=tk.LEFT, padx=4
        )

    def _send_sr(self) -> None:
        s = self.entry_sr.get().strip()
        if not s.startswith("S"):
            s = "Sr" + s
        self.run_cmd_async(s, 2.0, "set RA")

    def _send_sd(self) -> None:
        s = self.entry_sd.get().strip()
        if not s.startswith("S"):
            s = "Sd" + s
        self.run_cmd_async(s, 2.0, "set DEC")

    def _tab_tracking(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="Suivi / slew")
        self._row_buttons(
            tab,
            "Vitesses de slew (:R*)",
            [
                (":RS#", "Slew rapide :RS#", 1.5),
                (":RM#", "Find :RM#", 1.5),
                (":RC#", "Center :RC#", 1.5),
                (":RG#", "Guide :RG#", 1.5),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

        self._row_buttons(
            tab,
            "Suivi & slew vers cible",
            [
                (":MT1#", "Démarrer suivi :MT1#", 2.0),
                (":MT0#", "Arrêter suivi :MT0#", 2.0),
                (":MS#", "Slew vers cible :MS#", 3.0),
                (":Q#", "Stop tous axes :Q#", 2.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X, pady=8)

        self._row_buttons(
            tab,
            "Mouvement manuel (appuyer puis Stop)",
            [
                (":Me#", "Est :Me#", 1.5),
                (":Mw#", "Ouest :Mw#", 1.5),
                (":Mn#", "Nord :Mn#", 1.5),
                (":Ms#", "Sud :Ms#", 1.5),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

        gf = ttk.LabelFrame(tab, text="Impulsion de guidage :MGdNNNN (1–9999 ms)", padding=6)
        gf.pack(fill=tk.X, pady=8)
        ttk.Label(gf, text="Dir").grid(row=0, column=0)
        self.combo_gdir = ttk.Combobox(gf, width=4, values=["e", "w", "n", "s"], state="readonly")
        self.combo_gdir.set("e")
        self.combo_gdir.grid(row=0, column=1, padx=4)
        ttk.Label(gf, text="ms (4 chiffres)").grid(row=0, column=2)
        self.entry_gms = ttk.Entry(gf, width=8)
        self.entry_gms.insert(0, "0100")
        self.entry_gms.grid(row=0, column=3, padx=4)
        ttk.Button(gf, text="Envoyer :MG", command=self._send_guide).grid(row=0, column=4, padx=8)

        xf = ttk.LabelFrame(tab, text="Pas moteurs :MX[axe][pas]#  (r=RA d=DEC z=AZ l=ALT f=FOCUS)", padding=6)
        xf.pack(fill=tk.X, pady=8)
        self.combo_mx = ttk.Combobox(xf, width=3, values=["r", "d", "z", "l", "f"], state="readonly")
        self.combo_mx.set("r")
        self.combo_mx.pack(side=tk.LEFT, padx=4)
        self.entry_mx = ttk.Entry(xf, width=12)
        self.entry_mx.insert(0, "100")
        self.entry_mx.pack(side=tk.LEFT, padx=4)
        ttk.Button(xf, text="Envoyer :MX", command=self._send_mx).pack(side=tk.LEFT, padx=8)

    def _send_mx(self) -> None:
        ax = self.combo_mx.get().lower()
        try:
            steps = int(self.entry_mx.get().strip())
        except ValueError:
            messagebox.showerror("MX", "Nombre de pas invalide")
            return
        self.run_cmd_async(f":MX{ax}{steps}#", 8.0, "move stepper")

    def _send_guide(self) -> None:
        d = self.combo_gdir.get().lower()
        ms = self.entry_gms.get().strip().zfill(4)[:4]
        if len(ms) != 4 or not ms.isdigit():
            messagebox.showerror("Guidage", "Durée: 4 chiffres 0000–9999")
            return
        cmd = f":MG{d}{ms}#"
        self.run_cmd_async(cmd, 2.0, "guide pulse")

    def _tab_az_alt(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="AZ / ALT (AutoPA)")
        self._row_buttons(
            tab,
            "Lecture",
            [
                (":XGAA#", "Positions pas AZ|ALT :XGAA#", 2.0),
                (":XGAH#", "États autohoming :XGAH#", 2.0),
                (":XGA#", "Pas/° altitude :XGA#", 2.0),
                (":XGZ#", "Pas/° azimut :XGZ#", 2.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

        mf = ttk.LabelFrame(tab, text="Déplacement (arcmin)", padding=6)
        mf.pack(fill=tk.X, pady=8)
        ttk.Label(mf, text="Δ AZ").grid(row=0, column=0)
        self.entry_maz = ttk.Entry(mf, width=10)
        self.entry_maz.insert(0, "5")
        self.entry_maz.grid(row=0, column=1, padx=4)
        ttk.Button(mf, text=":MAZ", command=lambda: self._send_maz_alt("Z", self.entry_maz)).grid(row=0, column=2)
        ttk.Label(mf, text="Δ ALT").grid(row=0, column=4, padx=(16, 0))
        self.entry_mal = ttk.Entry(mf, width=10)
        self.entry_mal.insert(0, "5")
        self.entry_mal.grid(row=0, column=5, padx=4)
        ttk.Button(mf, text=":MAL", command=lambda: self._send_maz_alt("L", self.entry_mal)).grid(row=0, column=6)

        ttk.Button(tab, text="Rentrer AZ/ALT home :MAA#", command=lambda: self.run_cmd_async(":MAA#", 4.0)).pack(
            anchor=tk.W, pady=4
        )

    def _send_maz_alt(self, axis: str, entry: ttk.Entry) -> None:
        try:
            v = float(entry.get().strip())
        except ValueError:
            messagebox.showerror("Valeur", "Nombre invalide")
            return
        sign = "+" if v >= 0 else ""
        self.run_cmd_async(f":MA{axis}{sign}{v}#", 5.0, f"MA{axis}")

    def _tab_focus(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="Focuser")
        self._row_buttons(
            tab,
            "État & vitesse",
            [
                (":Fp#", "Position :Fp#", 2.0),
                (":FB#", "En mouvement? :FB#", 2.0),
                (":F1#", "Vitesse 1", 1.5),
                (":F2#", "Vitesse 2", 1.5),
                (":F3#", "Vitesse 3", 1.5),
                (":F4#", "Vitesse 4", 1.5),
                (":FS#", "Lent :FS#", 1.5),
                (":FF#", "Rapide :FF#", 1.5),
                (":FQ#", "Stop :FQ#", 2.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

        ff = ttk.LabelFrame(tab, text="Déplacement relatif :FMnnnn", padding=6)
        ff.pack(fill=tk.X, pady=8)
        self.entry_fm = ttk.Entry(ff, width=12)
        self.entry_fm.insert(0, "100")
        self.entry_fm.pack(side=tk.LEFT, padx=4)
        ttk.Button(ff, text="Envoyer FM", command=self._send_fm).pack(side=tk.LEFT)

        warn = ttk.Label(
            tab,
            text=":F+ et :F- déplacent en continu jusqu'à :FQ# — utiliser avec précaution.",
            foreground="darkred",
        )
        warn.pack(anchor=tk.W, pady=4)
        bf = ttk.Frame(tab)
        bf.pack(anchor=tk.W)
        ttk.Button(bf, text="F+ continu", command=lambda: self.run_cmd_async(":F+#", 1.0)).pack(side=tk.LEFT, padx=4)
        ttk.Button(bf, text="F- continu", command=lambda: self.run_cmd_async(":F-#", 1.0)).pack(side=tk.LEFT, padx=4)

    def _send_fm(self) -> None:
        try:
            steps = int(self.entry_fm.get().strip())
        except ValueError:
            messagebox.showerror("FM", "Entier invalide")
            return
        self.run_cmd_async(f":FM{steps}#", 5.0, "focus move")

    def _tab_gyro(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="Niveau (gyro)")
        self._row_buttons(
            tab,
            "MPU / niveau digital",
            [
                (":XL1#", "Allumer gyro :XL1#", 2.0),
                (":XL0#", "Éteindre gyro :XL0#", 2.0),
                (":XLGC#", "Pitch, roll courants :XLGC#", 2.0),
                (":XLGR#", "Réf. pitch, roll :XLGR#", 2.0),
                (":XLGT#", "Température °C :XLGT#", 2.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

    def _tab_gps(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="GPS")
        self._row_buttons(
            tab,
            "Diagnostic (firmware avec USE_GPS)",
            [
                (":XGP#", "Stats parseur :XGP#", 2.0),
                (":XGQ#", "Octets bruts récents :XGQ#", 2.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

        gf = ttk.LabelFrame(tab, text="Acquisition GPS :gT[timeout_ms]# (bloquant côté firmware)", padding=6)
        gf.pack(fill=tk.X, pady=8)
        ttk.Label(gf, text="Timeout (ms), ex. 120000").grid(row=0, column=0)
        self.entry_gps_t = ttk.Entry(gf, width=10)
        self.entry_gps_t.insert(0, "120000")
        self.entry_gps_t.grid(row=0, column=1, padx=4)
        ttk.Button(gf, text="Lancer :gT", command=self._send_gt).grid(row=0, column=2, padx=8)

    def _send_gt(self) -> None:
        try:
            ms = int(self.entry_gps_t.get().strip())
        except ValueError:
            messagebox.showerror("GPS", "Timeout ms invalide")
            return
        # Firmware: :gT optional ms in substring — handleMeadeGPS uses inCmd.substring(1) for timeout
        cmd = f":gT{ms}#" if ms > 0 else ":gT#"
        timeout_sec = max(ms / 1000.0 + 5.0, 10.0)
        self.run_cmd_async(cmd, timeout_sec, "GPS acquire")

    def _tab_home(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="Home / Park")
        self._row_buttons(
            tab,
            "Park & home",
            [
                (":hF#", "Aller home :hF#", 30.0),
                (":hP#", "Park :hP#", 30.0),
                (":hU#", "Unpark + suivi :hU#", 5.0),
                (":hZ#", "Définir home AZ/ALT :hZ#", 3.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

        self._row_buttons(
            tab,
            "Autohome capteurs Hall (si compilé)",
            [
                (":MHRR#", "RA home sens - :MHRR#", 60.0),
                (":MHRL#", "RA home sens + :MHRL#", 60.0),
                (":MHDU#", "DEC home up :MHDU#", 60.0),
                (":MHDD#", "DEC home down :MHDD#", 60.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X, pady=8)

    def _tab_hardware(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="Hardware / debug")
        self._row_buttons(
            tab,
            "Infos monture",
            [
                (":XGM#", "Infos HW :XGM#", 2.0),
                (":XGMS#", "Infos steppers :XGMS#", 2.0),
                (":XGR#", "Pas/° RA :XGR#", 2.0),
                (":XGD#", "Pas/° DEC :XGD#", 2.0),
                (":XGS#", "Calibration vitesse :XGS#", 2.0),
                (":XGT#", "Vitesse suivi :XGT#", 2.0),
                (":XGB#", "Backlash :XGB#", 2.0),
                (":XGH#", "HA LST format :XGH#", 2.0),
                (":XGL#", "LST :XGL#", 2.0),
            ],
        ).pack(anchor=tk.W, fill=tk.X)

    def _tab_expert(self, nb: ttk.Notebook) -> None:
        tab = ttk.Frame(nb, padding=8)
        nb.add(tab, text="Expert")
        ttk.Label(
            tab,
            text="Commande brute (max ~18 car. côté firmware — voir buffer série). Préfixe « : » et suffixe « # » ajoutés si absents.",
        ).pack(anchor=tk.W)
        self.entry_raw = ttk.Entry(tab, width=80)
        self.entry_raw.insert(0, ":GVN#")
        self.entry_raw.pack(fill=tk.X, pady=6)
        tf = ttk.Frame(tab)
        tf.pack(anchor=tk.W)
        ttk.Label(tf, text="Timeout (s)").pack(side=tk.LEFT)
        self.entry_raw_t = ttk.Entry(tf, width=8)
        self.entry_raw_t.insert(0, "3")
        self.entry_raw_t.pack(side=tk.LEFT, padx=6)
        ttk.Button(tf, text="Envoyer", command=self._send_raw).pack(side=tk.LEFT, padx=8)

    def _send_raw(self) -> None:
        try:
            tmo = float(self.entry_raw_t.get().strip())
        except ValueError:
            messagebox.showerror("Timeout", "Nombre invalide")
            return
        cmd = self.entry_raw.get().strip()
        self.run_cmd_async(cmd, tmo, "raw")


def main() -> None:
    app = OatHarnessApp()
    app.mainloop()


if __name__ == "__main__":
    main()
