function wifiHandler() {
  return {
    ssid: "",
    password: "",
    networks: [],
    scanning: false,
    statusMsg: "",
    showPassword: false,
    connecting: false,

    async scan() {
      this.scanning = true;
      this.statusMsg = "";
      try {
        const res = await fetch("/api/v1/wifi/scan");
        const nets = await res.json();
        // process: sort by rssi desc and enrich display fields
        this.networks = (nets || [])
          .map((n) => {
            const rssi =
              typeof n.rssi === "number" ? n.rssi : parseInt(n.rssi) || 0;
            const bars =
              rssi > -50
                ? "▮▮▮▮"
                : rssi > -60
                  ? "▮▮▮▯"
                  : rssi > -70
                    ? "▮▮▯▯"
                    : "▮▯▯▯";
            const secured = !!n.enc && n.enc !== 0;
            return {
              ssid: n.ssid || "",
              rssi,
              rssiDisplay: rssi + " dBm",
              bars,
              secured,
            };
          })
          .sort((a, b) => b.rssi - a.rssi);
      } catch (e) {
        this.statusMsg = "Scan failed";
        this.networks = [];
      }
      this.scanning = false;
    },

    selectNetwork(net) {
      this.ssid = net.ssid;
      // Require the user to provide the password explicitly
      this.password = "";
      this.statusMsg = "Enter password to connect";
      // focus password input if secured
      setTimeout(() => {
        const pw = document.getElementById("password");
        if (pw) pw.focus();
      }, 50);
    },

    async quickConnect(net) {
      this.selectNetwork(net);
      if (!net.secured) {
        // try connect without password
        await this.connect();
      } else {
        this.statusMsg = "Enter password and press Connect";
      }
    },

    async connect() {
      this.statusMsg = "Connecting...";
      this.connecting = true;
      try {
        const res = await fetch("/api/v1/wifi/connect", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ ssid: this.ssid, password: this.password }),
        });
        const j = await res.json();
        if (j.status === "connected") {
          this.statusMsg = "Connected: " + (j.ip || "");
          this.password = "";
        } else {
          this.statusMsg = "Error: " + (j.message || "failed");
        }
      } catch (e) {
        this.statusMsg = "Request failed";
      }
      this.connecting = false;
    },

    async forget() {
      this.ssid = "";
      this.password = "";
      this.statusMsg = "Cleared";
    },

    async init() {
      try {
        const res = await fetch("/api/v1/wifi/status");
        const j = await res.json();
        if (j.connected) {
          this.statusMsg = `Connected: ${j.ssid} ${j.ip}`;
          this.ssid = j.ssid || this.ssid;
        }
      } catch (e) {
        // ignore
      }
    },
  };
}
