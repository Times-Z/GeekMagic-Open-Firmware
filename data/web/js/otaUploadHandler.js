function otaUploadHandler() {
  return {
    uploading: false,
    uploadMessage: "",
    uploadType: "firmware",
    progress: 0,
    etaText: "",
    xhr: null,

    async uploadFile() {
      const fileInput = this.$refs.fileInput;
      if (!fileInput.files.length) {
        this.uploadMessage = "Please select a file";
        return;
      }

      const file = fileInput.files[0];
      if (!file.name.toLowerCase().endsWith(".bin")) {
        this.uploadMessage = "Only .bin files are allowed";
        return;
      }

      const endpoint =
        this.uploadType === "firmware" ? "/api/v1/ota/fw" : "/api/v1/ota/fs";

      this.uploading = true;
      this.uploadMessage = "";
      this.progress = 0;
      this.etaText = "";

      const formData = new FormData();
      formData.append("file", file);

      this.xhr = new XMLHttpRequest();
      const startTime = Date.now();

      this.xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          const percent = Math.round((e.loaded / e.total) * 100);
          this.progress = percent;
          const elapsed = (Date.now() - startTime) / 1000;
          const rate = e.loaded / Math.max(elapsed, 0.001);
          const remaining = e.total - e.loaded;
          if (rate > 0) {
            const eta = Math.round(remaining / rate);
            this.etaText = "ETA: " + eta + "s";
          } else {
            this.etaText = "";
          }
        } else {
          this.progress = Math.min(99, this.progress + 1);
        }
      };

      this.xhr.onload = async () => {
        this.uploading = false;
        try {
          if (this.xhr.status >= 200 && this.xhr.status < 300) {
            const data = JSON.parse(this.xhr.responseText || "{}");
            this.uploadMessage = data.message || "Upload finished";
            this.progress = 100;
            this.etaText = "";
          } else {
            this.uploadMessage =
              "Error: " + (this.xhr.responseText || this.xhr.status);
          }
        } catch (e) {
          this.uploadMessage = "Upload finished";
        }
        this.xhr = null;
      };

      this.xhr.onerror = () => {
        this.uploading = false;
        this.uploadMessage = "Upload error";
        this.xhr = null;
      };

      this.xhr.onabort = () => {
        this.uploading = false;
        this.uploadMessage = "Upload canceled";
        this.xhr = null;
      };

      this.xhr.open("POST", endpoint);
      this.xhr.send(formData);
    },

    async cancelUpload() {
      if (this.xhr) {
        try {
          this.xhr.abort();
        } catch (e) {
          // ignore
        }
      }

      try {
        await fetch("/api/v1/ota/cancel", { method: "POST" });
      } catch (e) {
        // ignore
      }
      this.uploading = false;
      this.progress = 0;
      this.etaText = "";
    },
  };
}
