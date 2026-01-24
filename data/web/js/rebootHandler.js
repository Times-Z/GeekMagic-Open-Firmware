function rebootHandler() {
  return {
    loading: false,
    message: "",
    async reboot() {
      this.loading = true;
      this.message = "Rebooting...";
      try {
        const res = await fetch("/api/v1/reboot", { method: "POST" });
        if (res.ok) {
          this.message = "Device is rebooting...";
        } else {
          this.message = "Reboot failed!";
        }
      } catch (e) {
        this.message = "Error: " + e;
      }
      setTimeout(() => {
        this.loading = false;
      }, 5000);
    },
  };
}
