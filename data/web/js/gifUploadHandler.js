function gifUploadHandler() {
  return {
    async playGifFullscreen(gifName) {
      try {
        await fetch(`/api/v1/gif/play`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ name: gifName }),
        });
      } catch (e) {
        alert("Error when playing gif: " + e);
      }
    },

    async stopGif() {
      try {
        const res = await fetch(`/api/v1/gif/stop`, { method: "POST" });
        if (!res.ok) {
          const txt = await res.text().catch(() => "");
          alert("Failed to stop GIF: " + txt);
        }
      } catch (e) {
        alert("Error when stopping gif: " + e);
      }
    },

    humanFileSize,
    uploading: false,
    uploadMessage: "",
    gifs: [],
    usedBytes: 0,
    totalBytes: 0,
    freeBytes: 0,

    get usedBytesHR() {
      return humanFileSize(this.usedBytes);
    },

    get totalBytesHR() {
      return humanFileSize(this.totalBytes);
    },

    get freeBytesHR() {
      return humanFileSize(this.freeBytes);
    },

    gifListLoaded: false,

    async uploadGif() {
      this.uploading = true;
      this.uploadMessage = "";
      const file = this.$refs.fileInput.files[0];

      if (!file || file.type !== "image/gif") {
        this.uploadMessage = "Please select a GIF file";
        this.uploading = false;

        return;
      }

      const formData = new FormData();
      formData.append("upload", file, file.name);

      try {
        const response = await fetch("/api/v1/gif", {
          method: "POST",
          body: formData,
        });

        const result = await response.json();

        if (result.status === "success") {
          this.uploadMessage = "GIF uploaded: " + result.filename;
          await this.fetchGifList();
        } else {
          this.uploadMessage = result.message || "Upload failed";
        }
      } catch (e) {
        this.uploadMessage = "Error uploading GIF: " + e;
      }

      this.uploading = false;
    },

    async fetchGifList() {
      this.gifListLoaded = false;

      try {
        const response = await fetch("/api/v1/gif");
        const data = await response.json();
        this.gifs = data.files || [];
        this.usedBytes = data.usedBytes || 0;
        this.totalBytes = data.totalBytes || 0;
        this.freeBytes = data.freeBytes || 0;
      } catch (e) {
        this.gifs = [];
        this.usedBytes = this.totalBytes = this.freeBytes = 0;
      }

      this.gifListLoaded = true;
    },

    async deleteGif(gifName) {
      if (!confirm(`Delete ${gifName}? This cannot be undone.`)) return;

      try {
        const res = await fetch("/api/v1/gif", {
          method: "DELETE",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ name: gifName }),
        });

        if (!res.ok) {
          let msg = await res.text().catch(() => "");
          try {
            const j = JSON.parse(msg || "{}");
            msg = j.message || msg;
          } catch (e) {
            // ignore
          }
          alert("Delete failed: " + (msg || res.status));
          return;
        }

        const j = await res.json().catch(() => ({}));
        if (j.status && j.status === "success") {
          await this.fetchGifList();
          alert("Deleted: " + gifName);
        } else {
          alert("Delete failed: " + (j.message || "unknown"));
        }
      } catch (e) {
        alert("Error deleting GIF: " + e);
      }
    },

    async init() {
      await this.fetchGifList();
    },
  };
}
