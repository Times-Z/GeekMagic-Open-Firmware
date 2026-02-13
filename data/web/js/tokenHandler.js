function tokenHandler() {
  const storageKey = "Authorization";

  return {
    token: "",
    newToken: "",
    statusMsg: "",
    changeStatusMsg: "",
    showToken: false,
    showNewToken: false,
    loading: false,
    hasStoredToken: false,

    init() {
      const stored = localStorage.getItem(storageKey) || "";
      this.token = stored;
      this.hasStoredToken = !!stored;
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
        const res = await fetch(\"/api/v1/token/check\", {
          method: "GET",
          headers: { Authorization: "Bearer " + trimmed },
        });
        if (!res.ok) {
          let msg = "Invalid token.";

          try {
            const data = await res.json();
            msg = data.error || data.message || msg;
          } catch (e) {}

          this.statusMsg = msg;

          return;
        }

        localStorage.setItem(storageKey, trimmed);
        this.hasStoredToken = true;
        this.statusMsg = "Token valid and saved.";
      } catch (e) {
        this.statusMsg = "Token check failed.";
      } finally {
        this.loading = false;
      }
    },

    async changeToken() {
      const current = this.token.trim();
      const next = this.newToken.trim();

      if (!current) {
        this.changeStatusMsg = "No current token available.";
        this.hasStoredToken = false;
        return;
      }

      if (!next) {
        this.changeStatusMsg = "Please enter a new token.";
        return;
      }

      this.loading = true;
      this.changeStatusMsg = "Saving new token...";

      try {
        const res = await fetch("/api/v1/token/save", {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            Authorization: "Bearer " + current,
          },
          body: JSON.stringify({ token: next }),
        });

        if (!res.ok) {
          let msg = "Token update failed.";
          try {
            const data = await res.json();
            msg = data.error || data.message || msg;
          } catch (e) {
            // ignore
          }
          this.changeStatusMsg = msg;

          return;
        }

        localStorage.setItem(storageKey, next);
        this.token = next;
        this.newToken = "";
        this.hasStoredToken = true;
        this.changeStatusMsg = "Token updated and saved.";
      } catch (e) {
        this.changeStatusMsg = "Token update request failed.";
      } finally {
        this.loading = false;
      }
    },

    clearToken() {
      localStorage.removeItem(storageKey);
      this.token = "";
      this.newToken = "";
      this.hasStoredToken = false;
      this.statusMsg = "Token removed.";
    },
  };
}
