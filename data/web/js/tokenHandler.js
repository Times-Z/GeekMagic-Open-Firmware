function tokenHandler() {
  const storageKey = "Authorization";

  return {
    token: "",
    statusMsg: "",
    showToken: false,
    loading: false,

    init() {
      const stored = localStorage.getItem(storageKey) || "";
      this.token = stored;
      this.statusMsg = stored
        ? "Token loaded from browser storage."
        : "No token stored yet.";
    },

    async saveToken() {
      const trimmed = this.token.trim();
      if (!trimmed) {
        this.statusMsg = "Please enter a token.";
        return;
      }
      this.loading = true;
      this.statusMsg = "Checking token...";
      try {
        const res = await fetch("/api/v1/token/check", {
          method: "GET",
          headers: { Authorization: "Bearer " + trimmed },
        });
        if (!res.ok) {
          let msg = "Invalid token.";
          try {
            const data = await res.json();
            msg = data.error || data.message || msg;
          } catch (e) {
            // ignore
          }
          this.statusMsg = msg;
          return;
        }
        localStorage.setItem(storageKey, trimmed);
        this.statusMsg = "Token valid and saved.";
      } catch (e) {
        this.statusMsg = "Token check failed.";
      } finally {
        this.loading = false;
      }
    },

    clearToken() {
      localStorage.removeItem(storageKey);
      this.token = "";
      this.statusMsg = "Token removed.";
    },
  };
}
